#ifndef IBPICPDATACONTROL_H
#define IBPICPDATACONTROL_H

#include <QObject>
#include <QGuiApplication>
#include <QString>
#include <QFile>
#include <QMap>
#include <QVector>
#include <QVariant>

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

struct MergedUser
{
    QString id;
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
    void applyToPlot(int index, QObject *plotObj);
    QVariantMap calcStats(int index, double stime, double etime);
    double sampleValue(int index, double time, bool pressure);
    bool exportExcel(const QString &path);

private:
    bool ReadIbpicpData(QString filePath);
    bool ReadWaveData(QString filePath);
    bool ReadBsonData(QString filePath);
    void MegredUserData();
    const MergedUser *userAt(int index) const;

    QVariantList m_userData;
    QMap<QString, QVector<UserInfo>> m_userInfoMap;
    QVector<MergedUser> m_mergedUsers;
};
#endif // IBPICPDATACONTROL_H
