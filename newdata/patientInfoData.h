#ifndef PATIENT_INFO_DATA_H
#define PATIENT_INFO_DATA_H

#include "dataFileHead.h"


// --- 存储配置 ---
//#define SAVE_WAVE_TOTAL_SECONDS  (3600*100) // 100小时
#define SAVE_WAVE_TOTAL_SECONDS  (3600*24*14) // 14天
#define MAX_PATIENT_TOTAL 			7            // 7个病人槽位
#define PATIENT_ID_STR_LEN          100          // 病人ID/传感器ID字符串长度
#define WAVE_READ_CHUNK_SECONDS     86400        // 每次最大读取24小时


// --- 结构体定义 (严格对齐) ---

typedef struct
{
    uint32_t timestamp;  // Unix时间戳(秒)
    short icp;           // ICP 原始值
    short ict;           // ICT 原始值
} __APP_PACKED(1) ParamPointValue;

typedef struct
{
    char patientID[PATIENT_ID_STR_LEN];
    char sensorID[PATIENT_ID_STR_LEN];
    char name[85];
    uint32_t writeIndex;   // 内部滚动指针
    uint32_t count;        // 总采样点数
    uint32_t totalDuration; // 累计监护时长(秒)
    uint32_t endTime;       // 最后数据时间戳
    uint32_t startTime;     // 第1次传感器插入时存储的开始时间
    uint8_t  isValid;      // 是否已被占用
} __APP_PACKED(1) PatientInfoData;

typedef struct {
    int32_t  latestSlotIndex; // 最后一次操作的槽位 (0-6)
    uint32_t patientCount;    // 已存病人数量 (1-7)
    PatientInfoData slots[MAX_PATIENT_TOTAL];
} __APP_PACKED(1) PatientFileHead;

//文件头大小
#define PATIENT_FILE_HEAD_SIZE sizeof(PatientFileHead)
//一个数值的大小
#define PATIENT_ONE_POINT_SIZE sizeof(ParamPointValue)
//整个文件的大小
#define PATIENT_FILE_TOTAL_SIZE (PATIENT_FILE_HEAD_SIZE + (MAX_PATIENT_TOTAL * SAVE_WAVE_TOTAL_SECONDS * PATIENT_ONE_POINT_SIZE))
// --- 偏移量计算宏 ---
#define GET_REGION_OFFSET(index) (PATIENT_FILE_HEAD_SIZE + (index * SAVE_WAVE_TOTAL_SECONDS * PATIENT_ONE_POINT_SIZE))

void InitPatientStorage(void);
void RegisterPatientInfo(const char* id, const char* s_id, const char* name);
void RequestRegisterPatientInfo(const char* id, const char* s_id, const char* name);
bool TakeRegisterPatientInfo(char* idBuf, int idLen, char* sidBuf, int sidLen, char* nameBuf, int nameLen);
bool IsSensorRegistered(const char* sn);
void WritePatientBatchData(const char* id, ParamPointValue* points, int numPoints);
PatientInfoData* GetPatientByRecency(PatientFileHead* h, int n);
int GetPatientHistoryWave(const char* id, ParamPointValue * buf, int bufLen, long timeStart, long timeEnd, int *outActualOffset, int *outActualCount);
int DeletePatientFile(void);
void  testPatientData(void);
void  TestWriteLast10MinData(void);

// 病人文件头全局变量
extern PatientFileHead m_patientFileHead;

// 波形读取线程共享变量
extern volatile int g_waveReadRequest;
extern volatile int g_waveReadComplete;
extern int  g_waveRequestSeq;
extern volatile int g_waveReadCompletedSeq;
extern char g_waveReadPatientId[PATIENT_ID_STR_LEN];
extern long g_waveReadTimeStart;
extern volatile long g_waveReadCompletedTimeStart;
extern volatile int  g_waveReadCompletedDurationSec;
extern ParamPointValue g_waveReadBuf[WAVE_READ_CHUNK_SECONDS];
extern int g_waveReadCount;
extern long g_waveReadActualStart;  // 实际数据起始时间戳
extern long g_waveReadActualEnd;    // 实际数据结束时间戳
extern volatile bool g_recallDlgOpen;
extern volatile bool g_factoryResetOpen;

// --- 回顾菜单缓存机制 (6分钟循环缓存) ---
#define RECALL_CACHE_SECONDS    (6 * 60)    // 6分钟 = 360秒
#define RECALL_CACHE_BATCH_SIZE 30          // 每次批量写入30秒

typedef struct {
    ParamPointValue data[RECALL_CACHE_SECONDS];  // 6分钟数据缓存
    volatile int writeIdx;                        // 当前写入位置 (0 ~ RECALL_CACHE_SECONDS-1)
    volatile int count;                           // 当前缓存中的数据点数
    volatile bool active;                         // 缓存是否激活 (回顾菜单打开时为true)
    char sensorID[PATIENT_ID_STR_LEN];            // 当前缓存对应的传感器ID
} RecallCache;

extern RecallCache g_recallCache;

// 缓存操作函数
void RecallCache_Init(void);
void RecallCache_Clear(void);
bool RecallCache_Add(const char* sensorID, ParamPointValue* point);
bool RecallCache_IsFull(void);
void RecallCache_FlushToFile(void);  // 退出回顾菜单时批量写入
bool RecallCache_FlushOneBatch(void); // 每次写一批(30点), 返回true表示还有数据
extern int g_waveReadDurationSec;
extern uint32_t g_currentTotalDuration;

#endif
