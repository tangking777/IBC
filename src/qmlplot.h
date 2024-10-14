#ifndef QMLPLOT_H
#define QMLPLOT_H

#include <QtQuick>

class QCustomPlot;
class QCPGraph;
class QCPAbstractPlottable;
class QCPItemTracer;
class QCPItemText;
class QCPItemStraightLine;
class QCPRange;

class BasePlotItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    BasePlotItem( QQuickItem* parent = 0 );
    virtual ~BasePlotItem();
    void paint( QPainter* painter );
    virtual void initCustomPlotUI(){}

    QCustomPlot *getPlot();
    QCPItemTracer *m_tracer_temp = nullptr;
    QCPItemText *m_cur_Label_temp = nullptr;
    QCPItemTracer *m_tracer_power = nullptr;
    QCPItemText *m_cur_Label_power = nullptr;
    QCPItemTracer *m_tracer_time = nullptr;
    QCPItemText *m_cur_Label_time = nullptr;
    QCPItemStraightLine *m_refer_lineV = nullptr;
    QCPItemText* item_selected = nullptr;
protected:
    virtual void hoverMoveEvent(QHoverEvent *event);
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent *event );

    virtual void routeMouseEvents( QMouseEvent* event ) = 0;
    virtual void routeWheelEvents( QWheelEvent* event ) = 0;
    void itemRouteMouseEvents( QMouseEvent* event );
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
protected:
    virtual void routeMouseEvents( QMouseEvent* event );
    virtual void routeWheelEvents( QWheelEvent* event );

public slots:
    Q_INVOKABLE void setVoltageGraphData(const QVector<double> &keys, const QVector<double> &values);
    Q_INVOKABLE void setCurrentGraphData(const QVector<double> &keys, const QVector<double> &values);
    Q_INVOKABLE void setVoltageGraphVisible(bool value);
    Q_INVOKABLE void setCurrentGraphVisible(bool value);
    Q_INVOKABLE void setVoltageRange(double min, double max);
    Q_INVOKABLE void setCurrentRange(double min, double max);
    Q_INVOKABLE void setTimeRange(double min, double max);
    Q_INVOKABLE void replot();
    Q_INVOKABLE void rescaleAxes();
    Q_INVOKABLE void exportPdf(const QString path);
    Q_INVOKABLE void clearValue();

    Q_INVOKABLE void addLabel(const int type, const double xValue, const double yValue, const QString text);
private:
    QCPGraph* m_TempGraph = nullptr;
    QCPGraph* m_PresGraph = nullptr;
    double xAxisMaxValue = 0;
    double yAxisMaxValue = 0;
    double yAxis2MaxValue = 0;

    double xAxisMinValue = 0;
    double yAxisMinValue = 0;
    double yAxis2MinValue = 0;
};
#endif // QMLPLOT_H
