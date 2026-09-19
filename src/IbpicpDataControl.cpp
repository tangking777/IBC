#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QDateTime>
#include <QFont>
#include <QUrl>
#include <QRegularExpression>
#include <QStringList>
#include <QGuiApplication>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtEndian>
#include <algorithm>
#include "IbpicpDataControl.h"
#include "qmlplot.h"
#include "./Xlsx/xlsxdocument.h"
#include "./Xlsx/xlsxformat.h"

namespace {
    const int maxUserCnt = 7;
    const int infoByteLength = 44;
    const int nameAndIdByteLength = 13;
    const int ageAndSexByteLength = 1;
    const int infoTotleByteLength = 364;

    const int maxDataCnt = 1209600;
    const int dataByteLength = 10;
    const int checkAndTimeByteLength = 28;

    const qint64 waveFileSize = 67739750;
    const int waveHeaderSize = 2150;
    const int waveSlotCount = 7;
    const int waveSlotInfoSize = 306;
    const int wavePointSize = 8;
    const quint32 waveCapacity = 1209600;
    const qint64 waveRegionSize = 9676800;
    const int waveInvalidParam = -10000;
    const qint64 maxMergeGapSecs = 604800;
    const int excelMaxRows = 1048576;

    qint16 GetUIntData(QByteArray byteArr)
    {
        if (byteArr.isEmpty())
            return 0;
        return static_cast<qint16>(byteArr.at(0));
    }

    QString GetQStringDataByGBK(QByteArray binaryData, bool trimed = false)
    {
        QTextCodec *codec = QTextCodec::codecForName("GBK");
        QString string = codec ? codec->toUnicode(binaryData) : QString::fromUtf8(binaryData);
        if (trimed)
        {
            const int idx = string.indexOf(QChar(0x00));
            if (idx >= 0)
                string.truncate(idx);
        }
        return string;
    }

    bool UserInfoComparison(const UserInfo& a, const UserInfo& b) {
        if (a.timeVec.isEmpty())
            return false;
        if (b.timeVec.isEmpty())
            return true;
        return a.timeVec[0] < b.timeVec[0];
    }

    quint32 ReadU32LE(const char *p)
    {
        return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(p));
    }

    qint32 ReadI32LE(const char *p)
    {
        return qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(p));
    }

    qint16 ReadI16LE(const char *p)
    {
        return qFromLittleEndian<qint16>(reinterpret_cast<const uchar *>(p));
    }

    QString PackedString(const char *p, int len)
    {
        int n = 0;
        while (n < len && p[n] != '\0')
            ++n;
        if (n == 0)
            return QString();

        const QByteArray bytes(p, n);
        QTextCodec *codec = QTextCodec::codecForName("GBK");
        if (codec)
            return codec->toUnicode(bytes);
        return QString::fromUtf8(bytes);
    }

    bool IsPrintableSensorId(const char *s, int len)
    {
        if (!s || s[0] == '\0')
            return false;
        for (int i = 0; i < len; ++i)
        {
            const unsigned char ch = static_cast<unsigned char>(s[i]);
            if (ch == '\0')
                return i > 0;
            if (ch < 0x20 || ch > 0x7E)
                return false;
        }
        return true;
    }

    qint64 DeviceTsToLocalEpoch(quint32 rawTs)
    {
        const QDateTime utcWall = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(rawTs), Qt::UTC);
        const QDateTime localWall(utcWall.date(), utcWall.time(), Qt::LocalTime);
        return localWall.toSecsSinceEpoch();
    }

    double ScaleIcp(qint16 raw)
    {
        if (raw == waveInvalidParam)
            return 0.0;
        const double value = static_cast<double>(raw) / 10.0;
        if (value <= -50.0 || value > 150.0)
            return 0.0;
        return value;
    }

    double ScaleIct(qint16 raw)
    {
        if (raw == waveInvalidParam)
            return 0.0;
        const double value = static_cast<double>(raw) / 100.0;
        if (value < 1.0 || value > 50.0)
            return 0.0;
        return value;
    }

    QString toLocalPath(const QString &raw)
    {
        const QString s = raw.trimmed();
        if (s.startsWith(QLatin1String("file:"), Qt::CaseInsensitive))
            return QUrl(s).toLocalFile();
        return s;
    }

    int LowerBoundTime(const QVector<double> &times, double t)
    {
        auto it = std::lower_bound(times.constBegin(), times.constEnd(), t);
        return static_cast<int>(it - times.constBegin());
    }

    int NearestIndex(const QVector<double> &times, double t)
    {
        if (times.isEmpty())
            return -1;
        int i = LowerBoundTime(times, t);
        if (i <= 0)
            return 0;
        if (i >= times.size())
            return times.size() - 1;
        const double left = t - times[i - 1];
        const double right = times[i] - t;
        return (left <= right) ? (i - 1) : i;
    }

    QString sanitizeSheetName(QString name)
    {
        name.replace(QRegularExpression(QStringLiteral("[\\\\/:\\*\\?\\[\\]]")), QStringLiteral("_"));
        name = name.trimmed();
        if (name.isEmpty())
            name = QStringLiteral("Sheet");
        if (name.size() > 31)
            name = name.left(31);
        return name;
    }

    QString uniqueSheetName(QXlsx::Document &xlsx, QString base)
    {
        base = sanitizeSheetName(base);
        QString name = base;
        int n = 1;
        while (xlsx.sheetNames().contains(name))
        {
            const QString suffix = QString("_%1").arg(++n);
            name = sanitizeSheetName(base.left(31 - suffix.size()) + suffix);
        }
        return name;
    }
}

bool IbpicpDataControl::ReadIbpicpDatas(QVariantList filePaths, bool isMegred)
{
    m_userData.clear();
    m_userInfoMap.clear();
    m_mergedUsers.clear();
    for (QVariant value : filePaths)
    {
        const QString filePath = toLocalPath(value.toString());
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isFile())
            continue;

        const QString suffix = fileInfo.suffix().toLower();
        if (suffix == "bson")
        {
            ReadBsonData(filePath);
        }
        else if (suffix == "bin")
        {
            if (fileInfo.size() == waveFileSize)
                ReadWaveData(filePath);
            else
                ReadIbpicpData(filePath);
        }
    }

    if (isMegred)
        MegredUserData();

    emit userDataChanged();
    return true;
}

bool IbpicpDataControl::ReadIbpicpData(QString filePath)
{
    QFile inFile(filePath);
    if (!(inFile.open(QIODevice::ReadOnly)))
        return false;

    QByteArray infoByteArr = inFile.read(infoTotleByteLength);
    if (infoByteArr.size() < infoTotleByteLength)
    {
        inFile.close();
        return false;
    }

    const qint64 slotPayload = static_cast<qint64>(maxDataCnt) * dataByteLength
            + checkAndTimeByteLength * 2;

    for (int i = 0; i < maxUserCnt; i++)
    {
        UserInfo infoMap;
        const long infoIndex = i * infoByteLength;
        const QByteArray idByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2 + 2, 16);
        const QString id = PackedString(idByteArr.constData(), idByteArr.size());
        if (id.isEmpty())
        {
            inFile.skip(slotPayload);
            continue;
        }

        const QByteArray nameByteArr = infoByteArr.mid(infoIndex, nameAndIdByteLength);
        infoMap.name = GetQStringDataByGBK(nameByteArr, true);

        const QByteArray sexByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2,
                                                      ageAndSexByteLength);
        infoMap.isMan = GetUIntData(sexByteArr) == 0;

        const QByteArray ageByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2 + 1,
                                                      ageAndSexByteLength);
        infoMap.age = GetUIntData(ageByteArr);

        const QByteArray dataByteArr = inFile.read(maxDataCnt * dataByteLength);
        inFile.read(checkAndTimeByteLength);
        const QByteArray checkAndTimeByteArr = inFile.read(checkAndTimeByteLength);
        if (dataByteArr.size() < dataByteLength || checkAndTimeByteArr.size() < 16)
            continue;

        const char *timePtr = checkAndTimeByteArr.constData();
        const int second = ReadI16LE(timePtr + 0);
        const int minute = ReadI16LE(timePtr + 2);
        const int hour   = ReadI16LE(timePtr + 4);
        const int day    = ReadI16LE(timePtr + 6);
        const int month  = ReadI16LE(timePtr + 8);
        const int year   = ReadI16LE(timePtr + 10);
        QDateTime dateTime(QDate(year, month, day), QTime(hour, minute, second));
        const qint64 dateSecs = dateTime.toSecsSinceEpoch();

        int dataCnt = ReadI32LE(timePtr + 12);
        if (dataCnt < 0)
            dataCnt = 0;
        if (dataCnt > maxDataCnt)
            dataCnt = maxDataCnt;
        const int maxByBytes = dataByteArr.size() / dataByteLength;
        if (dataCnt > maxByBytes)
            dataCnt = maxByBytes;

        QVector<double> timeData;
        QVector<double> temperatureData;
        QVector<double> pressureData;
        const qint64 beginSecs = dateSecs - dataCnt;
        bool isStart = false;
        const char *samples = dataByteArr.constData();
        for (int j = 0; j < dataCnt; j++)
        {
            double preValue = static_cast<double>(ReadI16LE(samples + j * 10)) / 100.0;
            if (preValue <= -50 || preValue > 150)
                preValue = 0;
            double tempValue = static_cast<double>(ReadI16LE(samples + j * 10 + 4)) / 10.0;
            if (tempValue < 1 || tempValue > 50)
                tempValue = 0;
            if (preValue != 0 || tempValue != 0)
                isStart = true;
            if (!isStart)
                continue;
            timeData.push_back(static_cast<double>(beginSecs + j));
            pressureData.push_back(preValue);
            temperatureData.push_back(tempValue);
        }
        if (timeData.isEmpty())
            continue;
        infoMap.timeVec = timeData;
        infoMap.preVec = pressureData;
        infoMap.tempVec = temperatureData;
        m_userInfoMap[id].push_back(infoMap);
    }
    inFile.close();
    return true;
}

bool IbpicpDataControl::ReadWaveData(QString filePath)
{
    QFile inFile(filePath);
    if (!inFile.open(QIODevice::ReadOnly))
        return false;

    const QByteArray header = inFile.read(waveHeaderSize);
    if (header.size() != waveHeaderSize)
    {
        inFile.close();
        return false;
    }

    const char *head = header.constData();
    const qint32 latestSlotIndex = ReadI32LE(head);
    const quint32 patientCount = ReadU32LE(head + 4);
    if (latestSlotIndex < -1 || latestSlotIndex > 6 || patientCount > static_cast<quint32>(waveSlotCount))
    {
        inFile.close();
        return false;
    }

    for (int slot = 0; slot < waveSlotCount; ++slot)
    {
        const char *slotInfo = head + 8 + slot * waveSlotInfoSize;
        const quint8 isValid = static_cast<quint8>(slotInfo[305]);
        if (isValid != 1)
            continue;
        if (!IsPrintableSensorId(slotInfo + 100, 100))
            continue;

        const QString id = PackedString(slotInfo + 100, 100);
        if (id.isEmpty())
            continue;

        const quint32 writeIndex = ReadU32LE(slotInfo + 285);
        const quint32 count = ReadU32LE(slotInfo + 289);
        if (count == 0 || count > waveCapacity || writeIndex >= waveCapacity)
            continue;

        const qint64 regionOffset = static_cast<qint64>(waveHeaderSize) + static_cast<qint64>(slot) * waveRegionSize;
        if (!inFile.seek(regionOffset))
            continue;
        const QByteArray region = inFile.read(waveRegionSize);
        if (region.size() != waveRegionSize)
            continue;

        const char *points = region.constData();
        const quint32 validCount = qMin(count, waveCapacity);

        UserInfo infoMap;
        infoMap.name = PackedString(slotInfo + 200, 85);
        infoMap.age = 0;
        infoMap.isMan = true;

        QVector<double> timeData;
        QVector<double> pressureData;
        QVector<double> temperatureData;
        timeData.reserve(static_cast<int>(validCount));
        pressureData.reserve(static_cast<int>(validCount));
        temperatureData.reserve(static_cast<int>(validCount));

        qint64 lastTs = -1;
        for (quint32 logical = 0; logical < validCount; ++logical)
        {
            quint32 physicalIndex = logical;
            if (count >= waveCapacity)
                physicalIndex = (writeIndex + logical) % waveCapacity;

            const char *pt = points + static_cast<qint64>(physicalIndex) * wavePointSize;
            const quint32 rawTs = ReadU32LE(pt);
            if (rawTs == 0)
                continue;

            const qint64 ts = DeviceTsToLocalEpoch(rawTs);
            if (lastTs >= 0 && ts <= lastTs)
                continue;

            const qint16 icp = ReadI16LE(pt + 4);
            const qint16 ict = ReadI16LE(pt + 6);
            timeData.push_back(static_cast<double>(ts));
            pressureData.push_back(ScaleIcp(icp));
            temperatureData.push_back(ScaleIct(ict));
            lastTs = ts;
        }

        if (timeData.isEmpty())
            continue;

        infoMap.timeVec = timeData;
        infoMap.preVec = pressureData;
        infoMap.tempVec = temperatureData;
        m_userInfoMap[id].push_back(infoMap);
    }

    inFile.close();
    return true;
}

bool IbpicpDataControl::ReadBsonData(QString filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument document = QJsonDocument::fromJson(fileData);

    if (document.isArray()) {
        QJsonArray jsonArray = document.array();
        QVariantList dataList =  jsonArray.toVariantList();
        for (QVariant data : dataList)
        {
            UserInfo infoMap;
            QMap<QString, QVariant> dataMap = data.toMap();
            const QString id = dataMap["id"].toString();
            if (id.isEmpty())
                continue;
            infoMap.name = dataMap["name"].toString();
            infoMap.isMan = dataMap["isMan"].toBool();
            infoMap.age = dataMap["age"].toUInt();
            infoMap.timeVec = dataMap["timeData"].value<QVector<double>>();
            infoMap.preVec = dataMap["pressureData"].value<QVector<double>>();
            infoMap.tempVec = dataMap["temperatureData"].value<QVector<double>>();
            if (infoMap.timeVec.isEmpty())
            {
                const QVariantList tList = dataMap["timeData"].toList();
                const QVariantList pList = dataMap["pressureData"].toList();
                const QVariantList cList = dataMap["temperatureData"].toList();
                const int n = qMin(tList.size(), qMin(pList.size(), cList.size()));
                infoMap.timeVec.reserve(n);
                infoMap.preVec.reserve(n);
                infoMap.tempVec.reserve(n);
                for (int i = 0; i < n; ++i)
                {
                    infoMap.timeVec.push_back(tList[i].toDouble());
                    infoMap.preVec.push_back(pList[i].toDouble());
                    infoMap.tempVec.push_back(cList[i].toDouble());
                }
            }
            if (infoMap.timeVec.isEmpty())
                continue;
            m_userInfoMap[id].push_back(infoMap);
        }
    }

    return true;
}

void IbpicpDataControl::MegredUserData()
{
    m_mergedUsers.clear();
    m_userData.clear();
    for (const QString& id : m_userInfoMap.keys())
    {
        QVector<UserInfo> dataVec;
        for (const UserInfo &item : m_userInfoMap[id])
        {
            if (!item.timeVec.isEmpty())
                dataVec.push_back(item);
        }
        if (dataVec.isEmpty())
            continue;

        std::sort(dataVec.begin(), dataVec.end(), UserInfoComparison);

        QVector<double> timeData;
        QVector<double> temperatureData;
        QVector<double> pressureData;
        double lastTime = dataVec[0].timeVec[0];
        for (const UserInfo &userInfo : dataVec)
        {
            const double newStartTime = userInfo.timeVec[0];
            if (newStartTime > lastTime && newStartTime - lastTime > maxMergeGapSecs)
                continue;

            timeData.append(userInfo.timeVec);
            pressureData.append(userInfo.preVec);
            temperatureData.append(userInfo.tempVec);
            lastTime = timeData[timeData.length() - 1];
        }
        if (timeData.isEmpty())
            continue;

        MergedUser merged;
        merged.id = id;
        merged.name = dataVec[0].name;
        merged.age = dataVec[0].age;
        merged.isMan = dataVec[0].isMan;
        merged.timeVec = timeData;
        merged.preVec = pressureData;
        merged.tempVec = temperatureData;
        m_mergedUsers.push_back(merged);

        QVariantMap infoMap;
        infoMap.insert("id", id);
        infoMap.insert("name", merged.name);
        infoMap.insert("age", merged.age);
        infoMap.insert("isMan", merged.isMan);
        infoMap.insert("startTime", timeData.first());
        infoMap.insert("endTime", timeData.last());
        m_userData.push_back(infoMap);
    }
}

const MergedUser *IbpicpDataControl::userAt(int index) const
{
    if (index < 0 || index >= m_mergedUsers.size())
        return nullptr;
    return &m_mergedUsers.at(index);
}

void IbpicpDataControl::applyToPlot(int index, QObject *plotObj)
{
    CustomPlotItem *plot = qobject_cast<CustomPlotItem *>(plotObj);
    const MergedUser *user = userAt(index);
    if (!plot || !user)
        return;

    plot->initPlot();
    plot->setPresGraphData(user->timeVec, user->preVec);
    if (user->id.endsWith(QLatin1String("P")))
    {
        plot->setTempGraphVisible(false);
    }
    else
    {
        plot->setTempGraphVisible(true);
        plot->setTempGraphData(user->timeVec, user->tempVec);
    }
    plot->rescaleAxes();
    plot->replot();
}

QVariantMap IbpicpDataControl::calcStats(int index, double stime, double etime)
{
    QVariantMap out;
    out.insert("minPres", 0);
    out.insert("maxPres", 0);
    out.insert("avgPres", 0);
    out.insert("minTemp", 0);
    out.insert("maxTemp", 0);
    out.insert("avgTemp", 0);

    const MergedUser *user = userAt(index);
    if (!user || user->timeVec.isEmpty() || etime < stime)
        return out;

    const int n = user->timeVec.size();
    int begin = LowerBoundTime(user->timeVec, stime);
    int end = LowerBoundTime(user->timeVec, etime);
    if (end < n && user->timeVec[end] <= etime)
        ++end;
    if (begin < 0)
        begin = 0;
    if (end > n)
        end = n;
    if (begin >= end)
        return out;

    double minPres = 1e300, maxPres = -1e300, sumPres = 0;
    double minTemp = 1e300, maxTemp = -1e300, sumTemp = 0;
    int presCnt = 0, tempCnt = 0;
    const int preN = qMin(n, user->preVec.size());
    const int tempN = qMin(n, user->tempVec.size());
    for (int i = begin; i < end; ++i)
    {
        const double pres = (i < preN) ? user->preVec[i] : 0.0;
        const double temp = (i < tempN) ? user->tempVec[i] : 0.0;
        if (pres == 0.0 && temp == 0.0)
            continue;
        if (pres != 0.0)
        {
            minPres = qMin(minPres, pres);
            maxPres = qMax(maxPres, pres);
            sumPres += pres;
            ++presCnt;
        }
        if (temp != 0.0)
        {
            minTemp = qMin(minTemp, temp);
            maxTemp = qMax(maxTemp, temp);
            sumTemp += temp;
            ++tempCnt;
        }
    }

    if (presCnt > 0)
    {
        out["minPres"] = minPres;
        out["maxPres"] = maxPres;
        out["avgPres"] = sumPres / presCnt;
    }
    if (tempCnt > 0)
    {
        out["minTemp"] = minTemp;
        out["maxTemp"] = maxTemp;
        out["avgTemp"] = sumTemp / tempCnt;
    }
    return out;
}

double IbpicpDataControl::sampleValue(int index, double time, bool pressure)
{
    const MergedUser *user = userAt(index);
    if (!user)
        return 0.0;
    const int i = NearestIndex(user->timeVec, time);
    if (i < 0)
        return 0.0;
    if (pressure)
        return (i < user->preVec.size()) ? user->preVec[i] : 0.0;
    return (i < user->tempVec.size()) ? user->tempVec[i] : 0.0;
}

bool IbpicpDataControl::exportExcel(const QString &path)
{
    const QString filePath = toLocalPath(path);
    QXlsx::Document xlsx;
    QXlsx::Format format;
    format.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setFont(QFont(qApp->font().family(), 12));

    for (const MergedUser &user : m_mergedUsers)
    {
        const int n = qMin(user.timeVec.size(), qMin(user.preVec.size(), user.tempVec.size()));
        struct DayBucket {
            QString key;
            QVector<int> rows;
        };
        QVector<DayBucket> days;
        QMap<QString, int> dayIndex;

        for (int i = 0; i < n; ++i)
        {
            if (user.preVec[i] == 0.0 && user.tempVec[i] == 0.0)
                continue;
            const QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(user.timeVec[i]));
            const QString key = dt.toString("yyyyMMdd");
            if (!dayIndex.contains(key))
            {
                dayIndex.insert(key, days.size());
                days.push_back(DayBucket{key, QVector<int>()});
            }
            days[dayIndex[key]].rows.push_back(i);
        }

        int totalRows = 0;
        for (const DayBucket &d : days)
            totalRows += d.rows.size();
        const bool needSplit = (days.size() > 1) && (totalRows + 4 > excelMaxRows);

        auto writeSheet = [&](const QString &sheetName, const QVector<int> &rows)
        {
            xlsx.addSheet(sheetName);
            xlsx.selectSheet(sheetName);
            xlsx.setColumnWidth(1, 30);
            xlsx.setColumnWidth(2, 15);
            xlsx.setColumnWidth(3, 15);
            xlsx.setColumnWidth(4, 10);

            int rowIndex = 1;
            int cloIndex = 1;
            xlsx.write(rowIndex, cloIndex, user.id, format);
            xlsx.write(rowIndex, ++cloIndex, user.name, format);
            xlsx.write(rowIndex, ++cloIndex, user.isMan ? QStringLiteral("男") : QStringLiteral("女"), format);
            xlsx.write(rowIndex, ++cloIndex, QString::number(user.age), format);
            rowIndex += 2;
            cloIndex = 1;
            xlsx.write(rowIndex, cloIndex, QStringLiteral("时间"), format);
            xlsx.write(rowIndex, ++cloIndex, QStringLiteral("压力 mmHg"), format);
            xlsx.write(rowIndex, ++cloIndex, QStringLiteral("温度 ℃"), format);

            int written = 3;
            for (int i : rows)
            {
                if (written + 1 >= excelMaxRows)
                    break;
                ++rowIndex;
                ++written;
                cloIndex = 1;
                const QDateTime timeValue = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(user.timeVec[i]));
                xlsx.write(rowIndex, cloIndex, timeValue.toString("yyyy-MM-dd hh:mm:ss"), format);
                xlsx.write(rowIndex, ++cloIndex, user.preVec[i], format);
                xlsx.write(rowIndex, ++cloIndex, user.tempVec[i], format);
            }
        };

        if (!needSplit)
        {
            QVector<int> allRows;
            for (const DayBucket &d : days)
                allRows.append(d.rows);
            writeSheet(uniqueSheetName(xlsx, user.id), allRows);
        }
        else
        {
            for (const DayBucket &d : days)
                writeSheet(uniqueSheetName(xlsx, user.id + "_" + d.key), d.rows);
        }
    }

    return xlsx.saveAs(filePath);
}
