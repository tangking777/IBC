#ifndef IBPICPDATACONTROL_H
#define IBPICPDATACONTROL_H

#include <QObject>
#include <QGuiApplication>
#include <QString>
#include <QFile>
#include <QVector>

#include "CustomPlot/qcustomplot.h"


struct UserInfo
{
    QString name;
    qint16 age;
    bool isMan;
    QVector<double> timeVec;
    QVector<double> preVec;
    QVector<double> tempVec;
};

class IbpicpDataControl : public QObject
{

    Q_OBJECT
public:

    Q_PROPERTY(QVariantList userData READ getUserData NOTIFY userDataChanged USER true)
    explicit IbpicpDataControl(QObject *parent = nullptr):QObject(parent){}
    QVariantList getUserData() {return m_userData;};
signals:
    void userDataChanged();

public slots:
    bool ReadIbpicpDatas(QVariantList filePaths, bool isMegred = false);

private:
    bool ReadIbpicpData(QString filePath);
    bool ReadBsonData(QString filePath);
    void MegredUserData();

    QVariantList m_userData;
    QMap<QString, QVector<UserInfo>> m_userInfoMap;
};
#endif // IBPICPDATACONTROL_H
