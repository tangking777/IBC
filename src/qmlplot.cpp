#include "qmlplot.h"
#include "./CustomPlot/qcustomplot.h"

#include <QDebug>

BasePlotItem::BasePlotItem( QQuickItem* parent ) : QQuickPaintedItem( parent )
    , m_CustomPlot(new QCustomPlot() )
{
    setFlag( QQuickItem::ItemHasContents, true );
    setAcceptedMouseButtons( Qt::AllButtons );
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);

    connect( this, &QQuickPaintedItem::widthChanged, this, &BasePlotItem::updateCustomPlotSize );
    connect( this, &QQuickPaintedItem::heightChanged, this, &BasePlotItem::updateCustomPlotSize );
    //connect(this, &BasePlotItem::wheelEvent, this, &BasePlotItem::slot_wheelEvent);
    //connect(m_CustomPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(routeMouseEvents(QMouseEvent*)));
}

BasePlotItem::~BasePlotItem()
{
    delete getPlot();
    m_CustomPlot = nullptr;
}

void BasePlotItem::paint( QPainter* painter )
{
    if (getPlot())
    {
        QPixmap    picture( boundingRect().size().toSize() );
        QCPPainter qcpPainter( &picture );

        getPlot()->toPainter( &qcpPainter );

        painter->drawPixmap( QPoint(), picture );
    }
}

QCustomPlot *BasePlotItem::getPlot()
{
    if( !m_CustomPlot){
        m_CustomPlot = new QCustomPlot;
    }
    return m_CustomPlot;
}

void BasePlotItem::mousePressEvent( QMouseEvent* event )
{
    routeMouseEvents( event );
}

void BasePlotItem::mouseReleaseEvent( QMouseEvent* event )
{
    routeMouseEvents( event );
}

void BasePlotItem::mouseMoveEvent( QMouseEvent* event )
{
    routeMouseEvents( event );
}

void BasePlotItem::mouseDoubleClickEvent( QMouseEvent* event )
{
    routeMouseEvents( event );
}

void BasePlotItem::wheelEvent( QWheelEvent *event )
{
    routeWheelEvents( event );
}
void BasePlotItem::graphClicked( QCPAbstractPlottable* plottable )
{

}

void BasePlotItem::routeMouseEvents( QMouseEvent* event )
{
    if (getPlot())
    {
        // if(event->button() == Qt::MouseButton::LeftButton)
        if (event->buttons() & Qt::LeftButton)
        {
            QMouseEvent* newEvent = new QMouseEvent( event->type(), event->localPos(), event->button(), event->buttons(), event->modifiers() );
            QCoreApplication::postEvent( getPlot(), newEvent );

            QList<QCPAxis*> axes;
            if (getPlot()->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
            {
                axes << getPlot()->xAxis;
                getPlot()->axisRect()->setRangeDragAxes(axes);
                getPlot()->axisRect()->setRangeDrag(getPlot()->xAxis->orientation());
            }
            else if (getPlot()->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
            {
                axes << getPlot()->yAxis;
                getPlot()->axisRect()->setRangeDragAxes(axes);
                getPlot()->axisRect()->setRangeDrag(getPlot()->yAxis->orientation());
            }
            else if (getPlot()->yAxis2->selectedParts().testFlag(QCPAxis::spAxis))
            {
                axes << getPlot()->yAxis2;
                getPlot()->axisRect()->setRangeDragAxes(axes);
                getPlot()->axisRect()->setRangeDrag(getPlot()->yAxis2->orientation());
            }
            else
            {
                axes << getPlot()->yAxis << getPlot()->xAxis << getPlot()->yAxis2;
                getPlot()->axisRect()->setRangeDragAxes(axes);
                getPlot()->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
            }
        }
        else if (event->buttons() & Qt::RightButton)
        {
            m_refer_lineV->setVisible(true);
            m_tracer_temp->setVisible(true);
            m_cur_Label_temp->setVisible(true);

            if(getPlot()->yAxis2->visible())
            {
                m_tracer_power->setVisible(true);
                m_cur_Label_power->setVisible(true);
            }

            double cur_x = getPlot()->xAxis->pixelToCoord(event->pos().x());

            m_tracer_temp->setGraph(getPlot()->graph(0));
            m_tracer_temp->setGraphKey(cur_x);
            m_tracer_temp->setInterpolating(true);
            m_tracer_temp->updatePosition();

            m_tracer_power->setGraph(getPlot()->graph(1));
            m_tracer_power->setGraphKey(cur_x);
            m_tracer_power->setInterpolating(true);
            m_tracer_power->updatePosition();

            double yValue = m_tracer_temp->position->value();
            m_cur_Label_temp->position->setCoords(0, 10);
            m_cur_Label_temp->setText(QString::number(yValue, 'f', 1));

            double yValue2 = m_tracer_power->position->value();
            m_cur_Label_power->position->setCoords(0, 10);
            m_cur_Label_power->setText(QString::number(yValue2, 'f', 1));

            m_refer_lineV->point1->setCoords(cur_x, 0);
            m_refer_lineV->point2->setCoords(cur_x, 100);

            getPlot()->replot();
        }

    }
}

void BasePlotItem::routeWheelEvents( QWheelEvent* event )
{
    if (getPlot())
    {
        QList<QCPAxis*> axes;
        QWheelEvent* newEvent = new QWheelEvent( event->pos(), event->delta(), event->buttons(), event->modifiers(), event->orientation() );
        QCoreApplication::postEvent( getPlot(), newEvent );

        if (getPlot()->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        {
            axes << getPlot()->xAxis;
            getPlot()->axisRect()->setRangeZoomAxes(axes);
            getPlot()->axisRect()->setRangeZoom(getPlot()->xAxis->orientation());
        }
        else if (getPlot()->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        {
            axes << getPlot()->yAxis;
            getPlot()->axisRect()->setRangeZoomAxes(axes);
            getPlot()->axisRect()->setRangeZoom(getPlot()->yAxis->orientation());
        }
        else if (getPlot()->yAxis2->selectedParts().testFlag(QCPAxis::spAxis))
        {
            axes << getPlot()->yAxis2;
            getPlot()->axisRect()->setRangeZoomAxes(axes);
            getPlot()->axisRect()->setRangeZoom(getPlot()->yAxis2->orientation());
        }
        else
        {
            axes << getPlot()->yAxis << getPlot()->xAxis << getPlot()->yAxis2;
            getPlot()->axisRect()->setRangeZoomAxes(axes);
            getPlot()->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
        }
    }
}

void BasePlotItem::hoverMoveEvent(QHoverEvent *event)
{

}

void BasePlotItem::updateCustomPlotSize()
{
    if (getPlot())
    {
        getPlot()->setGeometry(0, 0, (int)width(), (int)height());
        getPlot()->setViewport(QRect(0, 0, (int)width(), (int)height()));
    }
}

void BasePlotItem::onCustomReplot()
{
    update();
}

////
/// \brief CustomPlotItem::CustomPlotItem
/// \param parent
///
CustomPlotItem::CustomPlotItem( QQuickItem* parent ) : BasePlotItem( parent )
{
}

CustomPlotItem::~CustomPlotItem()
{

}

void CustomPlotItem::initCustomPlot()
{
    updateCustomPlotSize();

    m_PresGraph = getPlot()->addGraph();
    m_TempGraph = getPlot()->addGraph(getPlot()->xAxis, getPlot()->yAxis2);
    m_TempGraph->setPen(QPen(Qt::red));
    m_TempGraph->setName("温度 °C");
    m_PresGraph->setPen(QPen(Qt::blue));
    m_PresGraph->setName("压力 mmHg");

    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("yy-MM-dd\nhh:mm:ss");
    dateTicker->setTickCount(5);
    dateTicker->setTickStepStrategy(QCPAxisTicker::tssMeetTickCount);
    getPlot()->xAxis->setTicker(dateTicker);

    getPlot()->yAxis->setNumberFormat("f");
    getPlot()->yAxis->setNumberPrecision(2);
    getPlot()->yAxis->setLabel("压力 mmHg");
    getPlot()->yAxis->setTickLength(10,5);
    getPlot()->yAxis->setRange(20, 30);

    getPlot()->yAxis2->setVisible(false);
    getPlot()->yAxis2->setNumberFormat("f");
    getPlot()->yAxis2->setNumberPrecision(2);
    getPlot()->yAxis2->setLabel("温度 °C");
    getPlot()->yAxis2->setTickLength(10,5);
    getPlot()->yAxis2->setRange(35, 42);

    getPlot()->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    getPlot()->legend->setVisible(true);
    getPlot()->legend->setBrush(QColor(255,255,255,0));
    getPlot()->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignLeft);


    m_tracer_temp = new QCPItemTracer(getPlot());
    m_tracer_temp->setStyle(QCPItemTracer::tsCircle);
    m_tracer_temp->setPen(QPen(Qt::red, 3, Qt::SolidLine));
    m_tracer_temp->setBrush(Qt::SolidPattern);
    m_tracer_temp->setVisible(false);

    m_tracer_power = new QCPItemTracer(getPlot());
    m_tracer_power->setStyle(QCPItemTracer::tsCircle);
    m_tracer_power->setPen(QPen(Qt::red, 3, Qt::SolidLine));
    m_tracer_power->setBrush(Qt::SolidPattern);
    m_tracer_power->setVisible(false);

    m_tracer_power->position->setType(QCPItemPosition::ptPlotCoords);
    m_tracer_power->setSize(5);

    m_tracer_temp->position->setType(QCPItemPosition::ptPlotCoords);
    m_tracer_temp->setSize(5);

    m_tracer_temp->setGraph(m_TempGraph);
    m_tracer_power->setGraph(m_PresGraph);

    m_cur_Label_temp = new QCPItemText(getPlot());
    m_cur_Label_temp->position->setParentAnchor(m_tracer_temp->position);
    m_cur_Label_temp->setFont(QFont(qApp->font().family(), 12));
    m_cur_Label_temp->setColor(Qt::white);
    m_cur_Label_temp->setVisible(false);
    m_cur_Label_temp->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_cur_Label_temp->setBrush(QBrush(Qt::red));


    m_cur_Label_power = new QCPItemText(getPlot());
    m_cur_Label_power->position->setParentAnchor(m_tracer_power->position);
    m_cur_Label_power->setFont(QFont(qApp->font().family(), 12));
    m_cur_Label_power->setColor(Qt::white);
    m_cur_Label_power->setVisible(false);
    m_cur_Label_power->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_cur_Label_power->setBrush(QBrush(Qt::blue));

    m_refer_lineV = new QCPItemStraightLine(getPlot());
    m_refer_lineV->setPen(QPen(Qt::red, 1, Qt::DashLine));
    m_refer_lineV->setVisible(false);

    connect( getPlot(), &QCustomPlot::afterReplot, this, &CustomPlotItem::onCustomReplot );

    //connect(getPlot(), SIGNAL(mouseRelease(QMouseEvent *)), this, SLOT(slot_mouseRelease(QMouseEvent *)));
}

void CustomPlotItem::setVoltageGraphData(const QVector<double> &keys, const QVector<double> &values)
{
    if(m_TempGraph){
        m_TempGraph->setData(keys, values);
    }
}

void CustomPlotItem::setCurrentGraphData(const QVector<double> &keys, const QVector<double> &values)
{
    if(m_PresGraph)
    {
        m_PresGraph->setData(keys, values);
    }
}

void CustomPlotItem::setVoltageGraphVisible(bool value)
{
    if(m_TempGraph){
        m_TempGraph->setVisible(value);
        getPlot()->yAxis2->setVisible(value);
    }
}

void CustomPlotItem::setCurrentGraphVisible(bool value)
{
    if(m_PresGraph)
    {
        m_PresGraph->setVisible(value);
    }
}

void CustomPlotItem::setVoltageRange(double min, double max)
{
    getPlot()->yAxis2->setRange(min, max);
}

void CustomPlotItem::setCurrentRange(double min, double max)
{
    getPlot()->yAxis->setRange(min, max);
}

void CustomPlotItem::setTimeRange(double min, double max)
{
    getPlot()->xAxis->setRange(min, max);
}

void CustomPlotItem::replot()
{
    getPlot()->replot();
}

void CustomPlotItem::rescaleAxes()
{
    getPlot()->rescaleAxes();
}

void CustomPlotItem::exportPdf(const QString path)
{
    getPlot()->savePdf(path);
}
