#ifndef IBPICPDATACONTROL_H
#define IBPICPDATACONTROL_H

#include <QObject>
#include <QGuiApplication>
#include <QString>
#include <QFile>
#include <QVector>

#include "CustomPlot/qcustomplot.h"


class IbpicpDataControl : public QObject
{

    Q_OBJECT
public:

    Q_PROPERTY(QVariantList userData READ getUserData NOTIFY userDataChanged USER true)
    explicit IbpicpDataControl(QObject *parent = nullptr):QObject(parent){}
    //bool PaintTable(int index, QCustomPlot* widget);
    QVariantList getUserData() {return m_userData;};
signals:
    void userDataChanged();

public slots:
    bool ReadIbpicpData(QString filePath, bool isMegred = false);
private:
    QVariantList m_userData;
    //QCustomPlot *m_Multichannel = nullptr;
};
#endif // IBPICPDATACONTROL_H
