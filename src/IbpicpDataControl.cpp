#include <QFile>
#include <QTextCodec>
#include <QDateTime>
#include <QFont>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "IbpicpDataControl.h"

namespace {
    // 常量定义
    const int maxUserCnt = 7;
    const int infoByteLength = 44;
    const int nameAndIdByteLength = 13;
    const int ageAndSexByteLength = 1;
    const int infoTotleByteLength = 364;

    //const int singleDataTotleByteLength = 120960056;
    const int maxDataCnt = 1209600;
    const int dataByteLength = 10;
    const int checkAndTimeByteLength = 28;
    const int timeUintByteLength = 2;
    const int checkByteLength = 4;

    qint16 GetUIntData(QByteArray byteArr)
    {
        return static_cast<qint16>(byteArr.at(0));
    }

    qint16 GetInt16Data(QByteArray byteArray)
    {
        if (byteArray.size() != 2) {
            return 0;
        }

        char firstByte = byteArray.at(1);
        char secondByte = byteArray.at(0);

        qint16 totalValue;

        if (firstByte & 0x80) {
            totalValue = -(((~firstByte & 0x7F) << 8) | static_cast<quint8>(secondByte));
        } else {
            totalValue = (static_cast<quint8>(firstByte) << 8) | static_cast<quint8>(secondByte);
        }
        return totalValue;
    }

    qint32 GetInt32Data(QByteArray byteArray)
    {
        if (byteArray.size() != 4) {
            return 0;
        }
        char firstByte = byteArray.at(3);
        char secondByte = byteArray.at(2);
        char thirdByte = byteArray.at(1);
        char fourthByte = byteArray.at(0);

        return (static_cast<quint8>(firstByte) << 24) |
                (static_cast<quint8>(secondByte) << 16) |
                (static_cast<quint8>(thirdByte) << 8) |
                (static_cast<quint8>(fourthByte));

    }

    QString GetQStringDataByGBK(QByteArray binaryData, bool trimed = false)
    {
        QTextCodec *codec = QTextCodec::codecForName("GBK");//指定QString的编码方式
        QString string = codec->toUnicode(binaryData);//nameArray可以是char*，可以是QByteArray
        if(trimed)
        {
            int idx = string.indexOf(QChar(0x00));
            string.remove(idx, string.length() - idx);
        }
        return string;
    }

    QString GetQStringData(QByteArray binaryData)
    {
        return QString::fromUtf8(binaryData);
    }
}

bool IbpicpDataControl::ReadIbpicpDatas(QVariantList filePaths, bool isMegred)
{
    m_userData.clear();
    for(QVariant value : filePaths)
    {
        QString filePath = value.toString();
        if (filePath.startsWith("file:///"))
            filePath = filePath.mid(8);

        QFileInfo fileInfo(filePath);
        if(!fileInfo.isFile())
        {
            continue;
        }
        if(fileInfo.suffix() == "bson")
        {
            ReadBsonData(filePath);
        }
        else if(fileInfo.suffix() == "bin")
        {
            ReadIbpicpData(filePath, isMegred);
        }
    }

    if(isMegred)
        MegredUserData();

    return true;
}

bool IbpicpDataControl::ReadIbpicpData(QString filePath, bool isMegred)
{
    QFile inFile(filePath);
    if(!(inFile.open(QIODevice::ReadOnly)))//打开二进制文件
    {
        return false;
    }

    QByteArray infoByteArr = inFile.read(infoTotleByteLength);

    for(int i = 0; i < maxUserCnt; i++)
    {
        QVariantMap infoMap;
        long infoIndex = i * infoByteLength;
        QByteArray idByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2 + 2, 16);//infoByteArr.mid(infoIndex + nameAndIdByteLength, nameAndIdByteLength);
        QString id = GetQStringData(idByteArr);
        if(id.isEmpty())
        {
            continue;
        }
        infoMap.insert("id",id);

        QByteArray nameByteArr = infoByteArr.mid(infoIndex, nameAndIdByteLength);
        infoMap.insert("name", GetQStringDataByGBK(nameByteArr, true));

        QByteArray sexByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2
                                                , ageAndSexByteLength);
        infoMap.insert("isMan", GetUIntData(sexByteArr) == 0);
        QByteArray ageByteArr = infoByteArr.mid(infoIndex + nameAndIdByteLength * 2 + 1
                                                , ageAndSexByteLength);
        infoMap.insert("age", GetUIntData(ageByteArr));


        QByteArray dataByteArr = inFile.read(maxDataCnt * dataByteLength);
        QByteArray oldCheckAndTimeByteArr = inFile.read(checkAndTimeByteLength);
        QByteArray checkAndTimeByteArr = inFile.read(checkAndTimeByteLength);

        int second = GetInt16Data(checkAndTimeByteArr.mid(0, timeUintByteLength));
        int minute = GetInt16Data(checkAndTimeByteArr.mid(2, timeUintByteLength));
        int hour  = GetInt16Data(checkAndTimeByteArr.mid(4, timeUintByteLength));
        int day   = GetInt16Data(checkAndTimeByteArr.mid(6, timeUintByteLength));
        int month = GetInt16Data(checkAndTimeByteArr.mid(8, timeUintByteLength));
        int year  = GetInt16Data(checkAndTimeByteArr.mid(10, timeUintByteLength));
        QDateTime dateTime(QDate(year, month, day), QTime(hour, minute, second));
        qint64 dateSecs = dateTime.toSecsSinceEpoch();

        int dataCnt = GetInt32Data(checkAndTimeByteArr.mid(12, checkByteLength));

        QVector<double> timeData;
        QVector<double> temperatureData;
        QVector<double> pressureData;
        qint64 beginSecs = dateSecs - dataCnt;
        for(int j = 0; j < dataCnt; j++)
        {
            QByteArray valueArr = dataByteArr.mid(j * 10, 2);
            double preValue = (double)GetInt16Data(valueArr) / 100.0;
            if(preValue <= -50 || preValue > 150)
            {
                continue;
            }
            QByteArray valueArr2 = dataByteArr.mid(j * 10 + 4, 2);
            double tempValue = (double)GetInt16Data(valueArr2) / 10.0;
            if(tempValue > 50)
            {
                continue;
            }
            timeData.push_back((double)beginSecs + j);
            pressureData.push_back(preValue);
            temperatureData.push_back(tempValue);
        }

        if(isMegred)
        {
            bool isFind = false;
            for(int i = 0; i < m_userData.count(); i++)
            {
                QVariantMap dataMap = m_userData[i].toMap();
                if(dataMap["id"].toString() == id)
                {
                    QVector<double> existsTimeData = dataMap["timeData"].value<QVector<double>>();
                    double oldEndTime = existsTimeData[existsTimeData.length() - 1];
                    double newStartTime = timeData[0];
                    QVector<double> blankTimeVec;
                    QVector<double> blankPreVec;
                    QVector<double> blankTemVec;
                    for(int p = (int)oldEndTime + 1; p < (int)newStartTime; p++)
                    {
                        blankTimeVec.push_back(p);
                        blankPreVec.push_back(0.0);
                        blankTemVec.push_back(0.0);
                    }
                    existsTimeData.append(blankTimeVec);
                    existsTimeData.append(timeData);
                    dataMap.insert("timeData", QVariant::fromValue(existsTimeData));

                    QVector<double> existsPressureData = dataMap["pressureData"].value<QVector<double>>();
                    existsPressureData.append(blankPreVec);
                    existsPressureData.append(pressureData);
                    dataMap.insert("pressureData", QVariant::fromValue(existsPressureData));

                    QVector<double> existsTemperatureData = dataMap["temperatureData"].value<QVector<double>>();
                    existsTemperatureData.append(blankTemVec);
                    existsTemperatureData.append(temperatureData);
                    dataMap.insert("temperatureData", QVariant::fromValue(existsTemperatureData));

                    m_userData[i] = dataMap;
                    isFind = true;
                }
            }

            if(!isFind)
            {
                infoMap.insert("timeData", QVariant::fromValue(timeData));
                infoMap.insert("pressureData", QVariant::fromValue(pressureData));
                infoMap.insert("temperatureData", QVariant::fromValue(temperatureData));
                m_userData.push_back(infoMap);
            }
        }
        else
        {
            infoMap.insert("timeData", QVariant::fromValue(timeData));
            infoMap.insert("pressureData", QVariant::fromValue(pressureData));
            infoMap.insert("temperatureData", QVariant::fromValue(temperatureData));
            m_userData.push_back(infoMap);
        }
    }
    emit userDataChanged();
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
        qDebug() << "JSON Array:" << jsonArray;
    }

    return true;
}


void IbpicpDataControl::MegredUserData()
{

}
