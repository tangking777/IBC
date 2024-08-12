#ifndef QMLPLOT_H
#define QMLPLOT_H

#include <QtQuick>

class QCustomPlot;
class QCPGraph;
class QCPAbstractPlottable;
class QCPItemTracer;
class QCPItemText;
class QCPItemStraightLine;

class BasePlotItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    BasePlotItem( QQuickItem* parent = 0 );
    virtual ~BasePlotItem();
    void paint( QPainter* painter );
    virtual void initCustomPlotUI(){}

    QCustomPlot *getPlot();
    QCPItemTracer *m_tracer_temp;
    QCPItemText *m_cur_Label_temp;
    QCPItemTracer *m_tracer_power;
    QCPItemText *m_cur_Label_power;
    QCPItemStraightLine *m_refer_lineV;
protected:
    virtual void hoverMoveEvent(QHoverEvent *event);
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent *event );

    void routeMouseEvents( QMouseEvent* event );
    void routeWheelEvents( QWheelEvent* event );
public slots:
    void graphClicked( QCPAbstractPlottable* plottable );
    void onCustomReplot();
    void updateCustomPlotSize();
private:
    QCustomPlot* m_CustomPlot;

};

class CustomPlotItem : public BasePlotItem
{
    Q_OBJECT
public:
    CustomPlotItem( QQuickItem* parent = 0 );
    virtual ~CustomPlotItem();
    Q_INVOKABLE void initCustomPlot();
    // void slot_mouseRelease(QMouseEvent *event);
// protected:
//     virtual void mousePressEvent( QMouseEvent* event );

public slots:
    Q_INVOKABLE void setVoltageGraphData(const QVector<double> &keys, const QVector<double> &values);
    Q_INVOKABLE void setCurrentGraphData(const QVector<double> &keys, const QVector<double> &values);
    Q_INVOKABLE void setVoltageRange(double min, double max);
    Q_INVOKABLE void setCurrentRange(double min, double max);
    Q_INVOKABLE void setTimeRange(double min, double max);
    Q_INVOKABLE void replot();
    Q_INVOKABLE void rescaleAxes();
    Q_INVOKABLE void exportPdf(const QString path);

private:
    QCPGraph* m_TempGraph = nullptr;
    QCPGraph* m_PresGraph = nullptr;

};
#endif // QMLPLOT_H
