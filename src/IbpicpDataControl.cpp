#include <QFile>
#include <QTextCodec>
#include <QDateTime>
#include <QFont>

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

bool IbpicpDataControl::ReadIbpicpData(QString filePath, bool isMegred)
{
    m_userData.clear();

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
        // if(pUserDataMap.contains(id))
        // {
        //     userData = pUserDataMap[id];
        // }
        // else
        // {
        //     pUserDataMap[id] = userData;
        // }
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
        // check
        // int index  = GetInt16Data(checkAndTimeByteArr.mid(12, checkByteLength));
        // int indexCheck  = GetInt16Data(checkAndTimeByteArr.mid(16, checkByteLength));
        // int totle  = GetInt16Data(checkAndTimeByteArr.mid(20, checkByteLength));
        // int totleCheck  = GetInt16Data(checkAndTimeByteArr.mid(24, checkByteLength));
        QVector<double> timeData;
        QVector<double> temperatureData;
        QVector<double> pressureData;
        qint64 beginSecs = dateSecs - dataCnt;
        for(int j = 0; j < dataCnt; j++)
        {
            QByteArray valueArr = dataByteArr.mid(j * 10, 2);
            double preValue = (double)GetInt16Data(valueArr) / 100.0;
            //qDebug() << "preValue  " << j <<" == "<< valueArr << " -> " << preValue;
            if(preValue <= -50 || preValue > 150)
            {
                continue;
            }
            QByteArray valueArr2 = dataByteArr.mid(j * 10 + 4, 2);
            double tempValue = (double)GetInt16Data(valueArr2) / 10.0;
            if(tempValue < 30 || tempValue > 50)
            {
                continue;
            }
            timeData.push_back((double)beginSecs + j);
            pressureData.push_back(preValue);
            temperatureData.push_back(tempValue);
        }

        qDebug() << " data count == " << timeData.count();
        if(isMegred)
        {
            bool isFind = false;
            for(int i = 0; i < m_userData.count(); i++)
            {
                QVariantMap dataMap = m_userData[i].toMap();
                if(dataMap["id"].toString() == id)
                {
                    QVector<double> existsTimeData = dataMap["timeData"].value<QVector<double>>();
                    existsTimeData.append(timeData);
                    dataMap.insert("timeData", QVariant::fromValue(existsTimeData));

                    QVector<double> existsPressureData = dataMap["pressureData"].value<QVector<double>>();
                    existsPressureData.append(pressureData);
                    dataMap.insert("pressureData", QVariant::fromValue(existsPressureData));

                    QVector<double> existsTemperatureData = dataMap["temperatureData"].value<QVector<double>>();
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

/*
bool IbpicpDataControl::PaintTable(int index, QCustomPlot *widget)
{
    if(pUserDataVec.length() <= index)
    {
        return false;
    }
    UserData curUserData = *pUserDataVec[index];
    m_Multichannel = new QCustomPlot();
    m_Multichannel = widget;
    m_Multichannel->axisRect()->setRangeZoomFactor(0.5, 1);

    m_Multichannel->addGraph();
    QSharedPointer<QCPAxisTickerDateTime> timer(new QCPAxisTickerDateTime);
    timer->setDateTimeFormat("yy-MM-dd hh:mm:ss");
    //设置时间轴 一共几格
    timer->setTickCount(24);
    //设置label 旋转35° 横着显示可能显示不全
    m_Multichannel->xAxis->setTickLabelRotation(35);
    timer->setTickStepStrategy(QCPAxisTicker::tssMeetTickCount);
    //设置坐标轴
    m_Multichannel->xAxis->setTicker(timer);

    // m_Multichannel->yAxis2->setVisible(true);
    // m_Multichannel->yAxis2->setRange(30,45);
    // m_Multichannel->addGraph(m_Multichannel->xAxis,m_Multichannel->yAxis2);
    m_Multichannel->graph(0)->setPen(QPen(QColor(255,0,0,155)));   //设置曲线颜色
    m_Multichannel->graph(0)->setName("压力");          //设置曲线名称
    m_Multichannel->graph(0)->setData(curUserData.timeData, curUserData.temperatureData);
    // m_Multichannel->graph(1)->setPen(QPen(Qt::red));
    // m_Multichannel->graph(1)->setName("温度");
    // m_Multichannel->graph(1)->setData(curUserData.timeData, curUserData.temperatureData);
    // m_Multichannel->xAxis->setLimits(QCPAxis::iRange, QCPAxis::iRange); // 限制轴的范围
    // m_Multichannel->yAxis->setLimits(QCPAxis::iRange, QCPAxis::iRange);
    //x轴设置
    QSharedPointer<QCPAxisTickerFixed> intTicker_M(new QCPAxisTickerFixed);
    intTicker_M->setTickStep(1);                                      //设置刻度之间的步长为1
    intTicker_M->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);   //设置缩放策略
    // m_Multichannel->xAxis->setTicker(intTicker_M);                    //应用自定义整形ticker,防止使用放大功能时出现相同的x刻度值
    // m_Multichannel->xAxis->ticker()->setTickCount(11);                //刻度数量
    // m_Multichannel->xAxis->setNumberFormat("f");                      //x轴刻度值格式
    // m_Multichannel->xAxis->setNumberPrecision(0);                     //刻度值精度
    m_Multichannel->xAxis->setLabel("数量(n)");                        //设置标签
    //m_Multichannel->xAxis->setLabelFont(QFont(font().family(),8));    //设置标签字体大小
    // m_Multichannel->xAxis->setRange(0, 2000);
    // m_Multichannel->yAxis->setRange(0, 40);
    // m_Multichannel->xAxis->setSubTickLength(0,0);                     //子刻度长度
    // m_Multichannel->xAxis->setTickLength(10,5);                       //主刻度长度
    //y轴设置
    m_Multichannel->yAxis->setNumberFormat("f");
    m_Multichannel->yAxis->setNumberPrecision(2);
    m_Multichannel->yAxis->setLabel("距离(m)");
    //m_Multichannel->yAxis->setLabelFont(QFont(font().family(),8));
    m_Multichannel->yAxis->setTickLength(10,5);

    m_Multichannel->setInteractions(QCP::iRangeDrag| QCP::iRangeZoom);   //设置鼠标交互,曲线及图例可点击,可拖动
    m_Multichannel->legend->setVisible(true);                  //设置图例可见
    m_Multichannel->legend->setBrush(QColor(255,255,255,0));   //设置背景透明
    m_Multichannel->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignRight);   //设置图例居右上


    // QLinearGradient axisRectGradient;
    // axisRectGradient.setStart(0, 0);
    // axisRectGradient.setFinalStop(0, m_Multichannel->height());
    // axisRectGradient.setColorAt(0, QColor(255,0,0,20));
    // axisRectGradient.setColorAt(1, QColor(255,0,0,0));
    // m_Multichannel->graph(0)->setBrush(QBrush(axisRectGradient));

    m_Multichannel->graph(0)->rescaleAxes();
    m_Multichannel->replot();
    //m_Multichannel->graph(1)->setBrush(QBrush(QColor(0,255,255,150))); //蓝色，透明度150

    // //游标
    // tracer = new QCPItemTracer(m_Multichannel);       //生成游标
    // m_Multichannel->setMouseTracking(true);           //让游标自动随鼠标移动，若不想游标随鼠标动，则禁止
    // //tracer->setPen(QPen(QBrush(QColor(Qt::red)),Qt::DashLine));   //虚线游标
    // tracer->setPen(QPen(Qt::red));                    //圆圈轮廓颜色
    // tracer->setBrush(QBrush(Qt::red));                //圆圈圈内颜色
    // tracer->setStyle(QCPItemTracer::tsCircle);        //圆圈
    // tracer->setSize(5);
    // //tracer->setVisible(false);                      //设置可见性

    // //游标说明
    // tracerLabel = new QCPItemText(m_Multichannel);                     //生成游标说明
    // //tracerLabel->setVisible(false);                                  //设置可见性
    // tracerLabel->setLayer("overlay");                                  //设置图层为overlay，因为需要频繁刷新
    // tracerLabel->setPen(QPen(Qt::black));                              //设置游标说明颜色
    // tracerLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);   //左上
    // tracerLabel->setPadding(QMargins(4,4,4,4));                        //文字距离边框几个像素
    // tracerLabel->position->setParentAnchor(tracer->position);          //设置标签自动随着游标移动

    // //选择不同的曲线
    // connect(m_Multichannel,SIGNAL(selectionChangedByUser()),this,SLOT(slot_SelectionChanged()));

    // //初始化QRubberBand   //矩形放大
    // rubberBand = new QRubberBand(QRubberBand::Rectangle,m_Multichannel);
    // //连接鼠标事件发出的信号，实现绑定
    // connect(m_Multichannel,SIGNAL(mousePress(QMouseEvent*)),this,SLOT(mousePress(QMouseEvent*)));
    // connect(m_Multichannel,SIGNAL(mouseMove(QMouseEvent*)),this,SLOT(mouseMove(QMouseEvent*)));
    // connect(m_Multichannel,SIGNAL(mouseRelease(QMouseEvent*)),this,SLOT(mouseRelease(QMouseEvent*)));

    // //lambda表达式 mouseMoveEvent
    // connect(m_Multichannel,&QCustomPlot::mouseMove,[=](QMouseEvent* event){
    //     if(tracer->graph() == nullptr)
    //     {
    //         return;
    //     }
    //     if(tracer->graph()->data()->isEmpty())
    //     {
    //         return;
    //     }
    //     if(tracer->visible())
    //     {
    //         if(tracerGraph)
    //         {
    //             double x = m_Multichannel->xAxis->pixelToCoord(event->pos().x());
    //             tracer->setGraphKey(x);             //将游标横坐标设置成刚获得的横坐标数据x
    //             //tracer->setInterpolating(true);   //自动计算y值,若只想看已有点,不需要这个
    //             tracer->updatePosition();           //使得刚设置游标的横纵坐标位置生效
    //             tracerLabel->setText(QString("x:%1\ny:%2").arg(tracer->position->key()).arg(tracer->position->value()));
    //             m_Multichannel->replot(QCustomPlot::rpQueuedReplot);
    //         }
    //     }
    // });
    m_Multichannel->savePdf("D:\\tianjinICPdata.pdf");
    return true;
}
*/
