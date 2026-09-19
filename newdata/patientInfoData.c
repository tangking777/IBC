#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h> // For malloc/free
#include <math.h>
#include "funTool.h"
#include "x_config.h"
// #include "x_string.h"
#include "patientInfoData.h"

#define PARAM_POINT_VALUE_COUNT (60*30)
#define BATCH_SIZE              256

PatientFileHead m_patientFileHead;
static ParamPointValue m_paramPointBuf[PARAM_POINT_VALUE_COUNT];
// 静态变量用于记住上次读取的位置
static int g_lastReadSlot = -1;              // 上次读取的槽位
static uint32_t g_lastReadIndex = 0;         // 上次读取的索引位置
static uint32_t g_lastTimestamp = 0;         // 上次读取的时间戳
static char g_currentPatientId[PATIENT_ID_STR_LEN] = ""; // 当前正在读取的病人ID

// 波形读取线程共享变量
volatile int g_waveReadRequest = 0;
volatile int g_waveReadComplete = 0;
int g_waveRequestSeq = 0;
volatile int g_waveReadCompletedSeq = 0;
char g_waveReadPatientId[PATIENT_ID_STR_LEN] = "";
long g_waveReadTimeStart = 0;
volatile long g_waveReadCompletedTimeStart = 0;
volatile int g_waveReadCompletedDurationSec = 1800;
ParamPointValue g_waveReadBuf[WAVE_READ_CHUNK_SECONDS];
int g_waveReadCount = 0;
volatile bool g_recallDlgOpen = false;
volatile bool g_factoryResetOpen = false;
int g_waveReadDurationSec = 1800;
uint32_t g_currentTotalDuration = 0;  // 当前传感器累计监护时长（秒）
long g_waveReadActualStart = 0;
long g_waveReadActualEnd = 0;

// --- 回顾菜单缓存实现 ---
RecallCache g_recallCache = {0};

/**
 * @brief 初始化回顾缓存
 */
void RecallCache_Init(void)
{
    memset(&g_recallCache, 0, sizeof(RecallCache));
    g_recallCache.active = false;
    g_recallCache.writeIdx = 0;
    g_recallCache.count = 0;
    APP_printf("\r\n[RecallCache] Initialized\r\n");
}

/**
 * @brief 清除回顾缓存
 */
void RecallCache_Clear(void)
{
    memset(g_recallCache.data, 0, sizeof(g_recallCache.data));
    g_recallCache.writeIdx = 0;
    g_recallCache.count = 0;
    g_recallCache.active = false;
    g_recallCache.sensorID[0] = '\0';
    APP_printf("\r\n[RecallCache] Cleared\r\n");
}

/**
 * @brief 添加数据点到缓存
 * @param sensorID 传感器ID
 * @param point 数据点
 * @return true=成功, false=缓存已满或失败
 */
bool RecallCache_Add(const char* sensorID, ParamPointValue* point)
{
    if (!g_recallCache.active) {
        // 缓存未激活，直接返回失败（让调用方写入文件）
        return false;
    }
    
    if (!sensorID || !sensorID[0] || !point) {
        return false;
    }
    
    // 检查传感器ID是否匹配（如果不匹配，清除缓存重新开始）
    if (g_recallCache.sensorID[0] == '\0') {
        // 首次写入，记录传感器ID
        strncpy(g_recallCache.sensorID, sensorID, sizeof(g_recallCache.sensorID) - 1);
    } else if (strcmp(g_recallCache.sensorID, sensorID) != 0) {
        // 传感器ID变化，先刷新旧数据，再重新开始
        APP_printf("\r\n[RecallCache] Sensor changed, flush old data\r\n");
        RecallCache_FlushToFile();
        RecallCache_Clear();
        strncpy(g_recallCache.sensorID, sensorID, sizeof(g_recallCache.sensorID) - 1);
    }
    
    // 检查缓存是否已满
    if (g_recallCache.count >= RECALL_CACHE_SECONDS) {
        APP_printf("\r\n[RecallCache] Cache full, stop writing\r\n");
        return false;  // 缓存已满，不再写入
    }
    
    // 写入数据到当前位置
    g_recallCache.data[g_recallCache.writeIdx] = *point;
    g_recallCache.writeIdx = (g_recallCache.writeIdx + 1) % RECALL_CACHE_SECONDS;
    g_recallCache.count++;
    
    return true;
}

/**
 * @brief 检查缓存是否已满
 */
bool RecallCache_IsFull(void)
{
    return g_recallCache.count >= RECALL_CACHE_SECONDS;
}

/**
 * @brief 将缓存数据批量写入文件
 * 退出回顾菜单时调用，每次写30秒数据
 */
void RecallCache_FlushToFile(void)
{
    if (!g_recallCache.active || g_recallCache.count == 0) {
        APP_printf("\r\n[RecallCache] Nothing to flush\r\n");
        return;
    }
    
    APP_printf("\r\n[RecallCache] Flushing %d points to file...\r\n", g_recallCache.count);
    
    // 按时间顺序整理数据（处理循环缓冲区的环绕）
    ParamPointValue sortedData[RECALL_CACHE_SECONDS];
    int startIdx;
    
    if (g_recallCache.count < RECALL_CACHE_SECONDS) {
        // 缓存未满，数据从0开始
        startIdx = 0;
    } else {
        // 缓存已满，数据从writeIdx开始（最旧的数据）
        startIdx = g_recallCache.writeIdx;
    }
    
    // 按时间顺序复制到sortedData
    for (int i = 0; i < g_recallCache.count; i++) {
        int srcIdx = (startIdx + i) % RECALL_CACHE_SECONDS;
        sortedData[i] = g_recallCache.data[srcIdx];
    }
    
    // 批量写入：每次写30秒数据
    int written = 0;
    while (written < g_recallCache.count) {
        int batchSize = RECALL_CACHE_BATCH_SIZE;
        if (written + batchSize > g_recallCache.count) {
            batchSize = g_recallCache.count - written;
        }
        
        WritePatientBatchData(g_recallCache.sensorID, &sortedData[written], batchSize);
        written += batchSize;
        
        APP_printf("\r\n[RecallCache] Written %d/%d points\r\n", written, g_recallCache.count);
        
        // 每次写入后延时，避免Flash过载
        APP_OSTimeDly(50);  // 50ms延时
    }
    
    APP_printf("\r\n[RecallCache] Flush complete: %d points written\r\n", written);
    
    // 清除缓存
    RecallCache_Clear();
}

// 每次写一批(30点)，返回true表示还有数据待刷
bool RecallCache_FlushOneBatch(void)
{
    if (!g_recallCache.active || g_recallCache.count == 0)
        return false;

    static int flushOffset = 0;
    static ParamPointValue sortedData[RECALL_CACHE_SECONDS];
    static int sortedCount = 0;

    // 首次调用：排序整理数据
    if (flushOffset == 0) {
        int startIdx;
        if (g_recallCache.count < RECALL_CACHE_SECONDS)
            startIdx = 0;
        else
            startIdx = g_recallCache.writeIdx;
        for (int i = 0; i < g_recallCache.count; i++) {
            int srcIdx = (startIdx + i) % RECALL_CACHE_SECONDS;
            sortedData[i] = g_recallCache.data[srcIdx];
        }
        sortedCount = g_recallCache.count;
    }

    int batchSize = RECALL_CACHE_BATCH_SIZE;
    if (flushOffset + batchSize > sortedCount)
        batchSize = sortedCount - flushOffset;

    WritePatientBatchData(g_recallCache.sensorID, &sortedData[flushOffset], batchSize);
    flushOffset += batchSize;

    if (flushOffset >= sortedCount) {
        flushOffset = 0;
        sortedCount = 0;
        RecallCache_Clear();
        return false;
    }
    return true;
}

/**
 * 重置读取上下文，用于开始新的读取序列
 */
void ResetPatientReadContext(void) 
{
    g_lastReadSlot = -1;
    g_lastReadIndex = 0;
    g_lastTimestamp = 0;
    g_currentPatientId[0] = '\0';
}

int DeletePatientFile(void)
{
    memset(&m_patientFileHead, 0, sizeof(PatientFileHead));
    m_patientFileHead.latestSlotIndex = -1;
    WriteFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    APP_printf("\r\n DeletePatientFile: waveData.bin reset\r\n");
    return 0;
}


void  WritePatientFileHead(void)
{
    WriteFileData(PATIENT_FILE_NAME , &(m_patientFileHead) ,PATIENT_FILE_HEAD_SIZE, 0);
}

//
void ReadPatientFileHead(void)
{
    ReadFileData(PATIENT_FILE_NAME , &(m_patientFileHead) ,PATIENT_FILE_HEAD_SIZE, 0);
}

/**
 * 初始化：确保文件存在并预分配空间
 */
void InitPatientStorage(void)
{
    memset(&m_patientFileHead, 0, sizeof(PatientFileHead));
    m_patientFileHead.latestSlotIndex = -1;
    WriteFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    APP_printf("\r\n InitPatientStorage: waveData.bin initialized\r\n");
}

/**
 * 注册/切换病人 (7人循环覆盖)
 */
void RegisterPatientInfo(const char* id, const char* s_id, const char* name)
{
    //读取文件头
    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    // 确定新位置：0->1->2->3->4->5->6->0
    int nextSlot = (m_patientFileHead.latestSlotIndex + 1) % MAX_PATIENT_TOTAL;
    if (m_patientFileHead.patientCount < MAX_PATIENT_TOTAL)
         m_patientFileHead.patientCount++;
    m_patientFileHead.latestSlotIndex = nextSlot;
    // 清洗旧槽位并写入新信息
    PatientInfoData *p = &m_patientFileHead.slots[nextSlot];
    memset(p, 0, sizeof(PatientInfoData));
    snprintf(p->patientID, sizeof(p->patientID), "%s", id);
    snprintf(p->sensorID, sizeof(p->sensorID), "%s", s_id);
    p->isValid = 1;
    snprintf(p->name, sizeof(p->name), "%s", name);
    //写文件头
    WriteFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
}

// 请求在存储线程中注册病人
static volatile bool g_registerPatientReq = false;
static char g_regPatientId[PATIENT_ID_STR_LEN];
static char g_regSensorId[PATIENT_ID_STR_LEN];
static char g_regPatientName[85];

void RequestRegisterPatientInfo(const char* id, const char* s_id, const char* name)
{
    snprintf(g_regPatientId, sizeof(g_regPatientId), "%s", id ? id : "");
    snprintf(g_regSensorId, sizeof(g_regSensorId), "%s", s_id ? s_id : "");
    snprintf(g_regPatientName, sizeof(g_regPatientName), "%s", name ? name : "");
    g_registerPatientReq = true;
}

bool TakeRegisterPatientInfo(char* idBuf, int idLen, char* sidBuf, int sidLen, char* nameBuf, int nameLen)
{
    if (!g_registerPatientReq) return false;
    g_registerPatientReq = false;
    snprintf(idBuf, idLen, "%s", g_regPatientId);
    snprintf(sidBuf, sidLen, "%s", g_regSensorId);
    snprintf(nameBuf, nameLen, "%s", g_regPatientName);
    return true;
}

bool IsSensorRegistered(const char* sn)
{
    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    for (int i = 0; i < MAX_PATIENT_TOTAL; i++)
    {
        if (m_patientFileHead.slots[i].isValid && strcmp(m_patientFileHead.slots[i].sensorID, sn) == 0) 
        {
            APP_printf("\r\n\r\n\r\n\r\n IsSensorRegistered: SN=%s already registered (slot %d) \r\n\r\n\r\n\r\n", sn, i);
            return true;
        }
    }
    APP_printf("\r\n\r\n\r\n\r\n IsSensorRegistered: SN=%s not registered, will  register \r\n\r\n\r\n\r\n\r\n", sn);
    return false;
}

/**
 * 核心：批量写入数据 (10秒一批)
 * @param patientID 目标病人ID
 * @param points 数据数组
 * @param numPoints 数组长度 (例如 10)
 */
void WritePatientBatchData(const char* id, ParamPointValue* points, int numPoints)
{
    int slot = -1, i,nextWriteIndex,baseOffset,startPos,dataPos;
    PatientInfoData *p;
    // 读取文件头
    ReadFileData(PATIENT_FILE_NAME, &m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    // 1. 定位病人槽位
    for (i = 0; i < MAX_PATIENT_TOTAL; i++)
    {
        if (m_patientFileHead.slots[i].isValid && strcmp(m_patientFileHead.slots[i].sensorID, id) == 0)
        {
            slot = i;
            break;
        }
    }
    if (slot != -1) 
    {
        p = &m_patientFileHead.slots[slot];
        baseOffset = GET_REGION_OFFSET(slot);
        // 检查是否会在写入过程中跨越循环缓冲区边界
        nextWriteIndex = (p->writeIndex + 1) % SAVE_WAVE_TOTAL_SECONDS;
        // if (nextWriteIndex + numPoints <= SAVE_WAVE_TOTAL_SECONDS) 
        // {
        //     // 数据不会跨越边界，可以一次性写入
        //     // 直接使用传入的points数组进行写入
        //     startPos = baseOffset + (nextWriteIndex * PATIENT_ONE_POINT_SIZE);
        //     // 一次性写入所有数据
        //     WriteFileData(PATIENT_FILE_NAME, points, numPoints * PATIENT_ONE_POINT_SIZE, startPos);
        //     // 更新写入索引和计数（模拟循环写入的效果）
        //     for (i = 0; i < numPoints; i++) 
        //     {
        //         p->writeIndex = (p->writeIndex + 1) % SAVE_WAVE_TOTAL_SECONDS;
        //         if (p->count < SAVE_WAVE_TOTAL_SECONDS) 
        //             p->count++;
        //     }
        // }
        // else 
        {
            // 首次写入时记录开始时间
        if (p->count == 0)
            p->startTime = points[0].timestamp;

        //逐点写入 (处理病人内部 100小时滚动)
            for ( i = 0; i < numPoints; i++)
            {
                dataPos = baseOffset + (p->writeIndex * PATIENT_ONE_POINT_SIZE);
                //APP_printf("\r\nWritePatientBatchData(): dataPos = %d,points[i].timestamp = %ld\r\n", dataPos,points[i].timestamp);
                WriteFileData(PATIENT_FILE_NAME, (unsigned char*)(&points[i]), PATIENT_ONE_POINT_SIZE, dataPos);
                APP_OSTimeDly(100);
                ParamPointValue tmp;
                ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&tmp, PATIENT_ONE_POINT_SIZE, dataPos);
                //APP_printf("\r\nWritePatientBatchData(): tmp.timestamp = %ld\r\n", tmp.timestamp);
                p->writeIndex = (p->writeIndex + 1) % SAVE_WAVE_TOTAL_SECONDS;
                if (p->count < SAVE_WAVE_TOTAL_SECONDS) 
                    p->count++;
            }
        }
        // 更新统计信息
        p->totalDuration = (p->count >= SAVE_WAVE_TOTAL_SECONDS) ? SAVE_WAVE_TOTAL_SECONDS : p->count;
        g_currentTotalDuration = p->totalDuration;
        p->endTime = points[numPoints - 1].timestamp;
        // 关键：同步更新后的索引到文件头
        WriteFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
    }
}


/**
 * 通用读取：根据新鲜度获取病人信息 (1=最新, 2=次新...)
 */
static bool IsPatientSlotValid(const PatientInfoData* slot)
{
    int i;

    if (slot == NULL || slot->isValid != 1)
        return false;

    /* Sensor SN is displayed as text.  Do not expose erased/corrupted flash
     * (for example 0xFF bytes after factory reset) to LVGL as a C string. */
    for (i = 0; i < PATIENT_ID_STR_LEN; i++)
    {
        unsigned char ch = (unsigned char)slot->sensorID[i];
        if (ch == '\0')
            return i > 0;
        if (ch < 0x20 || ch > 0x7E)
            return false;
    }
    return false;
}

PatientInfoData* GetPatientByRecency(PatientFileHead* h, int n)
{
    if (h == NULL || n <= 0 || n > MAX_PATIENT_TOTAL
        || h->latestSlotIndex < 0 || h->latestSlotIndex >= MAX_PATIENT_TOTAL
        || h->patientCount > MAX_PATIENT_TOTAL)
        return NULL;
    // 从最新槽位开始往前遍历, 跳过无效槽位, 返回第n个有效病人
    int found = 0;
    for (int i = 0; i < MAX_PATIENT_TOTAL; i++)
    {
        int idx = (h->latestSlotIndex - i + MAX_PATIENT_TOTAL) % MAX_PATIENT_TOTAL;
        if (IsPatientSlotValid(&h->slots[idx]))
        {
            found++;
            if (found == n)
                return &h->slots[idx];
        }
    }
    return NULL;
}





/**
 * 读取某段时间的数据 - 新版本
 * @param id 病人ID
 * @param buf 输出缓冲区
 * @param bufLen 缓冲区长度
 * @param timeStart 开始时间（秒）
 * @param timeEnd 结束时间（秒）
 * @return 实际读取的数据点数量
 */
// 将线性索引(0=最旧)映射到环形缓冲区实际位置
static uint32_t LinearToCircular(uint32_t linear, uint32_t oldestPos, uint32_t totalSlots) {
    return (oldestPos + linear) % totalSlots;
}

// 读取指定槽位的第 linear 个点(0=最旧)
static void ReadLinearPoint(long base, uint32_t linear, uint32_t oldestPos, uint32_t totalSlots, ParamPointValue *out) {
    uint32_t cir = LinearToCircular(linear, oldestPos, totalSlots);
    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)out, sizeof(ParamPointValue), base + cir * sizeof(ParamPointValue));
}

int GetPatientHistoryWave(const char* id, ParamPointValue * buf, int bufLen, long timeStart, long timeEnd, int *outActualOffset, int *outActualCount)
{
    if (outActualOffset) *outActualOffset = 0;
    if (outActualCount)  *outActualCount  = 0;
    if (!buf || bufLen <= 0 || timeStart > timeEnd)
        return -1;

    ParamPointValue invalidValue = {0, INVALID_PARAM, INVALID_PARAM};
    int i;
    for (i = 0; i < bufLen; i++)
        buf[i] = invalidValue;

    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);

    int slot = -1;
    for (i = 0; i < MAX_PATIENT_TOTAL; i++) {
        if (m_patientFileHead.slots[i].isValid && strcmp(m_patientFileHead.slots[i].sensorID, id) == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return -2;

    PatientInfoData *p = &m_patientFileHead.slots[slot];
    if (p->count == 0)
        return -3;

    long base = GET_REGION_OFFSET(slot);
    const uint32_t totalSlots = SAVE_WAVE_TOTAL_SECONDS;
    const uint32_t totalPoints = (p->count < totalSlots) ? p->count : totalSlots;

    // 环形缓冲中最旧数据的位置
    uint32_t oldestPos;
    if (p->count < totalSlots)
        oldestPos = 0;
    else
        oldestPos = p->writeIndex % totalSlots;

    // 二分查找: 找第一个 timestamp >= timeStart 的点
    uint32_t lo = 0, hi = totalPoints;
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        ParamPointValue pt;
        ReadLinearPoint(base, mid, oldestPos, totalSlots, &pt);
        if (pt.timestamp < timeStart)
            lo = mid + 1;
        else
            hi = mid;
    }

    // 从 lo 开始批量读取, 按时间戳对齐填入buf对应位置
    uint32_t startLinear = lo;
    static ParamPointValue batch[BATCH_SIZE];
    int actualCount = 0;  // 实际读到的时间点数量
    int firstIdx = -1, lastIdx = -1;  // 实际数据在buf中的索引范围
    
    while (startLinear < totalPoints) {
        uint32_t cirStart = LinearToCircular(startLinear, oldestPos, totalSlots);
        uint32_t contig = totalSlots - cirStart;
        uint32_t remaining = totalPoints - startLinear;
        uint32_t chunk = remaining;
        if (chunk > contig)    chunk = contig;
        if (chunk > BATCH_SIZE) chunk = BATCH_SIZE;

        long pos = base + cirStart * sizeof(ParamPointValue);
        ReadFileData(PATIENT_FILE_NAME, (unsigned char*)batch, chunk * sizeof(ParamPointValue), pos);

        for (uint32_t j = 0; j < chunk; j++)
        {
            if (batch[j].timestamp > timeEnd)
            {
                startLinear = totalPoints;
                break;
            }
            int idx = (int)(batch[j].timestamp - timeStart);
            if (idx >= 0 && idx < bufLen) {
                buf[idx] = batch[j];
                actualCount++;
                if (firstIdx < 0) firstIdx = idx;
                lastIdx = idx;
            }
        }
        startLinear += chunk;
    }

    // 如果没有读到任何数据, 返回0
    if (actualCount == 0)
        return 0;

    // 填充整个查询范围内的缺失点为0（包括firstIdx之前和lastIdx之后）
    // 这样波形可以从查询范围的开始时刻绘制，前面没有数据的部分显示为基线
    for (i = 0; i < bufLen; i++)
    {
        if (buf[i].icp == INVALID_PARAM)
            buf[i].icp = 0;
        if (buf[i].ict == INVALID_PARAM)
            buf[i].ict = 0;
    }

    // 返回实际数据的索引范围和跨度（包括中间缺失的时间点）
    int dataSpan = lastIdx - firstIdx + 1;  // 实际数据占据的buf长度
    
    if (outActualOffset) *outActualOffset = firstIdx;
    if (outActualCount) *outActualCount  = dataSpan;
    return actualCount;
}





/**
 * 读取某段时间的数据
 */
// void GetPatientHistoryWave(uint32_t id, uint32_t start, uint32_t end) 
// {
//     // PatientFileHead h;
//     // FILE* fp = fopen(PATIENT_FILE_NAME, "rb");
//     // if (!fp) return;
//     // fread(&h, sizeof(PatientFileHead), 1, fp);
//     int slot = -1,i=0,base;
//     PatientInfoData *p;
//     ParamPointValue pt;
//     //读头文件信息
//     ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&m_patientFileHead, PATIENT_FILE_HEAD_SIZE, 0);
//     for ( i = 0; i < MAX_PATIENT_TOTAL; i++) 
//     {
//         if (m_patientFileHead.slots[i].isValid && m_patientFileHead.slots[i].patientID == id) 
//         {
//             slot = i; 
//             break;
//         }
//     }
//     if (slot != -1) 
//     {
//         p = &m_patientFileHead.slots[slot];
//         base = GET_REGION_OFFSET(slot);
//         APP_printf("\r\n[Log] ID:%d (%s) Data:\r\n", id, p->name);
//         for (i = 0; i < p->count; i++) 
//         {
//             //fseek(fp, base + (i * sizeof(ParamPointValue)), SEEK_SET);
//             //fread(&pt, sizeof(ParamPointValue), 1, fp);
//             //ReadFileData(NIBP_DATA_FILE_NAME, buf, sizeof(ParamPointValue), base + (i * sizeof(ParamPointValue)));
//             if (pt.timestamp >= start && pt.timestamp <= end) 
//             {
//                 APP_printf("  T:%u V:%.2f\n", pt.timestamp, pt.ict);
//             }
//         }
//     }
//     //fclose(fp);
// }

// --- 演示主程序 ---

void  testPatientData(void) 
{
    //InitPatientStorage();
    DeletePatientFile();
    ResetPatientReadContext();
    // 模拟注册7个病人
    const char* pNames[] = {"Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace"};
    for (int i = 0; i < 7; i++) 
    {
        char tmpId[PATIENT_ID_STR_LEN];
        snprintf(tmpId, sizeof(tmpId), "%d", 100 + i);
        //RegisterPatientInfo(tmpId, "1", pNames[i]);
    }

    // 获取第3新的病人（Alice=100, Bob=101, ..., Grace=106最新，第3新是Eve=104）
    PatientInfoData* p = GetPatientByRecency(&m_patientFileHead, 1);
    if (p) 
    {
        const char* id = p->patientID;
        APP_printf("Selected 3rd latest: %s (ID:%s)\n", p->name, id);
        // 模拟10秒写一批数据 (共30个点，写3批)
        for (int batch = 0; batch < 3; batch++) 
        {
            ParamPointValue batchData[10];
            for (int j = 0; j < 10; j++) 
            {
                batchData[j].timestamp = 17127400 + (batch * 10) + j;
                batchData[j].ict = 36.5f + j * 0.1f;
            }
            WritePatientBatchData(id, batchData, 10);
            printf("Batch %d (10 points) saved and file closed.\n", batch + 1);
        }
        //读取
        int off, cnt;
        int pointNum = GetPatientHistoryWave(id,m_paramPointBuf,30,  17127400, 17127429, &off, &cnt);
        printf(" \r\n  pointNum %d \r\n ", pointNum);
    }

    return 0;
}

// 批量写入当前时间前10分钟的模拟ICP数据(600点,约需60秒)
void TestWriteLast10MinData(void)
{
    PatientFileHead head;
    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&head, PATIENT_FILE_HEAD_SIZE, 0);

    if (head.patientCount == 0) {
        //RegisterPatientInfo("1", "1", "TestPatient");
    }
    ReadFileData(PATIENT_FILE_NAME, (unsigned char*)&head, PATIENT_FILE_HEAD_SIZE, 0);
    PatientInfoData* p = GetPatientByRecency(&head, 1);
    if (!p) {
        APP_printf("No patient found\r\n");
        return;
    }

    const char* id = p->patientID;
    long baseTimestamp = 2000000;
    int i, j;

    APP_printf("Writing 600pts(10min) for %s (ID:%s)...\r\n", p->name, id);

    ParamPointValue batchData[10];
    for (i = 0; i < 60; i++) {
        for (j = 0; j < 10; j++) {
            int idx = i * 10 + j;
            batchData[j].timestamp = baseTimestamp + idx;
            float icpVal = 15.0f + 5.0f * sin(2.0f * 3.14f * idx / 100.0f)
                         + 3.0f * sin(2.0f * 3.14f * idx / 10.0f);
            batchData[j].ict = (short)(icpVal * 10);
        }
        WritePatientBatchData(id, batchData, 10);
        if (i % 10 == 0)
            APP_printf("  Batch %d/60\r\n", i + 1);
    }
    APP_printf("Done. 600 points written.\r\n");
}
