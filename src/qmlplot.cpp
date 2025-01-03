#include "qmlplot.h"
#include "./CustomPlot/qcustomplot.h"

#include <QDebug>
#include "./Xlsx/xlsxdocument.h"
#include "./Xlsx/xlsxformat.h"

BasePlotItem::BasePlotItem( QQuickItem* parent ) : QQuickPaintedItem( parent )
    , m_CustomPlot(new QCustomPlot() )
{
    setFlag( QQuickItem::ItemHasContents, true );
    setAcceptedMouseButtons( Qt::AllButtons );
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);

    connect( this, &QQuickPaintedItem::widthChanged, this, &BasePlotItem::updateCustomPlotSize );
    connect( this, &QQuickPaintedItem::heightChanged, this, &BasePlotItem::updateCustomPlotSize );
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
    getPlot()->mousePressEvent(event);
    if (getPlot()->selectedItems().isEmpty())
    {
        routeMouseEvents( event );
    }
    else
    {
        item_selected = dynamic_cast<QCPItemText*>(getPlot()->selectedItems().value(0));
        selectedItemChanged(true);
    }
}

void BasePlotItem::mouseReleaseEvent( QMouseEvent* event )
{

    getPlot()->mouseReleaseEvent(event);
    if(item_selected)
    {
        item_selected->setSelected(false);
        item_selected = nullptr;
        selectedItemChanged(false);
        getPlot()->replot();
        return;
    }

    if(!getPlot()->selectedItems().isEmpty())
    {
        item_selected = dynamic_cast<QCPItemText*>(getPlot()->selectedItems().value(0));
        selectedItemChanged(true);
    }
}

void BasePlotItem::mouseMoveEvent( QMouseEvent* event )
{
    if(item_selected != nullptr)
    {
        itemRouteMouseEvents( event );
    }
    else
    {
        routeMouseEvents( event );
    }
}

void BasePlotItem::mouseDoubleClickEvent( QMouseEvent* event )
{
    getPlot()->mouseDoubleClickEvent(event);
    routeMouseEvents( event );
}

void BasePlotItem::wheelEvent( QWheelEvent *event )
{
    routeWheelEvents( event );
}

void BasePlotItem::itemRouteMouseEvents(QMouseEvent *event)
{
    if (item_selected && item_selected->position) {
        double cur_x = item_selected->position->keyAxis()->pixelToCoord(event->pos().x());
        double cur_y = item_selected->position->valueAxis()->pixelToCoord(event->pos().y());
        item_selected->position->setCoords(cur_x, cur_y);
        getPlot()->replot();
    }
}
void BasePlotItem::graphClicked( QCPAbstractPlottable* plottable )
{

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
    m_TempGraph->setPen(QPen(QColor(255, 59, 48)));
    m_TempGraph->setName("温度 ℃");
    m_PresGraph->setPen(QPen(QColor(0, 122, 255)));
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

    getPlot()->yAxis2->setVisible(true);
    getPlot()->yAxis2->setNumberFormat("f");
    getPlot()->yAxis2->setNumberPrecision(2);
    getPlot()->yAxis2->setLabel("温度 ℃");
    getPlot()->yAxis2->setTickLength(10,5);
    getPlot()->yAxis2->setRange(35, 42);

    getPlot()->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectItems);
    getPlot()->legend->setVisible(true);
    getPlot()->legend->setBrush(QColor(255,255,255,0));
    getPlot()->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignTop|Qt::AlignLeft);


    m_tracer_temp = new QCPItemTracer(getPlot());
    m_tracer_temp->setStyle(QCPItemTracer::tsCircle);
    m_tracer_temp->setPen(QPen(QColor(0, 122, 255), 3, Qt::SolidLine));
    m_tracer_temp->setBrush(Qt::SolidPattern);
    m_tracer_temp->setVisible(false);
    m_tracer_temp->position->setType(QCPItemPosition::ptPlotCoords);
    m_tracer_temp->setSize(5);
    m_tracer_temp->setGraph(m_TempGraph);

    m_tracer_pres = new QCPItemTracer(getPlot());
    m_tracer_pres->setStyle(QCPItemTracer::tsCircle);
    m_tracer_pres->setPen(QPen(QColor(255, 59, 48), 3, Qt::SolidLine));
    m_tracer_pres->setBrush(Qt::SolidPattern);
    m_tracer_pres->setVisible(false);
    m_tracer_pres->position->setType(QCPItemPosition::ptPlotCoords);
    m_tracer_pres->setSize(5);
    m_tracer_pres->setGraph(m_PresGraph);

    m_cur_Label_temp = new QCPItemText(getPlot());
    m_cur_Label_temp->position->setParentAnchor(m_tracer_temp->position);
    m_cur_Label_temp->setFont(QFont(qApp->font().family(), 13));
    m_cur_Label_temp->setColor(Qt::white);
    m_cur_Label_temp->setVisible(false);
    m_cur_Label_temp->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_cur_Label_temp->setBrush(QBrush(QColor(0, 122, 255)));
    m_cur_Label_temp->setSelectable(false);
    m_cur_Label_temp->setPadding(QMargins(5,5,5,5));


    m_cur_Label_pres = new QCPItemText(getPlot());
    m_cur_Label_pres->position->setParentAnchor(m_tracer_pres->position);
    m_cur_Label_pres->setFont(QFont(qApp->font().family(), 13));
    m_cur_Label_pres->setColor(Qt::white);
    m_cur_Label_pres->setVisible(false);
    m_cur_Label_pres->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_cur_Label_pres->setBrush(QBrush(QColor(255, 59, 48)));
    m_cur_Label_pres->setSelectable(false);
    m_cur_Label_pres->setPadding(QMargins(5,5,5,5));


    m_cur_Label_time = new QCPItemText(getPlot());
    m_cur_Label_time->setFont(QFont(qApp->font().family(), 13));
    m_cur_Label_time->setColor(Qt::white);
    m_cur_Label_time->setVisible(false);
    m_cur_Label_time->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_cur_Label_time->setBrush(QBrush(QColor(76, 217, 100)));
    m_cur_Label_time->setSelectable(false);
    m_cur_Label_time->setPadding(QMargins(5,5,5,5));

    m_refer_lineV = new QCPItemStraightLine(getPlot());
    m_refer_lineV->setPen(QPen(QColor(255, 59, 48), 1, Qt::DashLine));
    m_refer_lineV->setVisible(false);
    connect( getPlot(), &QCustomPlot::afterReplot, this, &CustomPlotItem::onCustomReplot );
}

void CustomPlotItem::routeMouseEvents( QMouseEvent* event )
{
    if (getPlot())
    {
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
            m_cur_Label_time->setVisible(true);
            m_refer_lineV->setVisible(true);

            if(!m_TempGraph->visible())
            {
                m_tracer_temp->setVisible(false);
                m_cur_Label_temp->setVisible(false);
            }
            else
            {
                m_tracer_temp->setVisible(true);
                m_cur_Label_temp->setVisible(true);
            }

            if(!m_PresGraph->visible())
            {
                m_tracer_pres->setVisible(false);
                m_cur_Label_pres->setVisible(false);
            }
            else
            {
                m_tracer_pres->setVisible(true);
                m_cur_Label_pres->setVisible(true);
            }

            double cur_x = getPlot()->xAxis->pixelToCoord(event->pos().x());

            m_tracer_pres->setGraph(getPlot()->graph(0));
            m_tracer_pres->setGraphKey(cur_x);
            m_tracer_pres->setInterpolating(true);
            m_tracer_pres->updatePosition();

            m_tracer_temp->setGraph(getPlot()->graph(1));
            m_tracer_temp->setGraphKey(cur_x);
            m_tracer_temp->setInterpolating(true);
            m_tracer_temp->updatePosition();

            double yValue = m_tracer_temp->position->value();
            m_cur_Label_temp->position->setCoords(0, 10);
            m_cur_Label_temp->setText(QString::number(yValue, 'f', 1));

            double yValue2 = m_tracer_pres->position->value();
            m_cur_Label_pres->position->setCoords(0, 10);
            m_cur_Label_pres->setText(QString::number(yValue2, 'f', 1));

            double lowerPos = getPlot()->yAxis->range().lower;
            double upperPos = getPlot()->yAxis->range().upper;
            m_cur_Label_time->position->setCoords(cur_x, lowerPos);
            QDateTime timeValue = QDateTime::fromSecsSinceEpoch(cur_x);
            m_cur_Label_time->setText(timeValue.toString("yyyy-MM-dd hh:mm:ss"));

            m_refer_lineV->point1->setCoords(cur_x, lowerPos - 100);
            m_refer_lineV->point2->setCoords(cur_x, upperPos + 100);

            getPlot()->replot();
        }
    }
}

void CustomPlotItem::routeWheelEvents( QWheelEvent* event )
{
    if (getPlot())
    {
        double factor = (event->delta() > 0) ? 0.9 : 1.1;
        QPointF mousePos = event->position();

        QRect axisRect = getPlot()->axisRect()->rect();
        if(axisRect.contains(mousePos.toPoint()))
        {
            double yMouse = getPlot()->yAxis->pixelToCoord(mousePos.y());
            QCPRange yRange = getPlot()->yAxis->range();
            double yLower = yMouse - (yMouse - yRange.lower) * factor;
            double yUpper = yMouse + (yRange.upper - yMouse) * factor;
            getPlot()->yAxis->setRange(yLower, yUpper);

            double yMouse2 = getPlot()->yAxis2->pixelToCoord(mousePos.y());
            QCPRange yRange2 = getPlot()->yAxis2->range();
            double yLower2 = yMouse2 - (yMouse2 - yRange2.lower) * factor;
            double yUpper2 = yMouse2 + (yRange2.upper - yMouse2) * factor;
            getPlot()->yAxis2->setRange(yLower2, yUpper2);


            double xMouse = getPlot()->xAxis->pixelToCoord(mousePos.x());
            QCPRange xRange = getPlot()->xAxis->range();
            double xLower = xMouse - (xMouse - xRange.lower) * factor;
            double xUpper = xMouse + (xRange.upper - xMouse) * factor;
            getPlot()->xAxis->setRange(xLower, xUpper);
        }
        else if(mousePos.x() < axisRect.left())
        {
            double yMouse = getPlot()->yAxis->pixelToCoord(mousePos.y());
            QCPRange yRange = getPlot()->yAxis->range();

            double yLower = yMouse - (yMouse - yRange.lower) * factor;
            double yUpper = yMouse + (yRange.upper - yMouse) * factor;

            getPlot()->yAxis->setRange(yLower, yUpper);
        }
        else if(mousePos.x() > axisRect.right())
        {
            double yMouse2 = getPlot()->yAxis2->pixelToCoord(mousePos.y());
            QCPRange yRange2 = getPlot()->yAxis2->range();

            double yLower2 = yMouse2 - (yMouse2 - yRange2.lower) * factor;
            double yUpper2 = yMouse2 + (yRange2.upper - yMouse2) * factor;

            getPlot()->yAxis2->setRange(yLower2, yUpper2);

        }
        else
        {
            double xMouse = getPlot()->xAxis->pixelToCoord(mousePos.x());
            QCPRange xRange = getPlot()->xAxis->range();

            double xLower = xMouse - (xMouse - xRange.lower) * factor;
            double xUpper = xMouse + (xRange.upper - xMouse) * factor;

            getPlot()->xAxis->setRange(xLower, xUpper);

        }

        if(getPlot()->xAxis->range().lower < xAxisMinValue
            || getPlot()->xAxis->range().upper > xAxisMaxValue)
        {
            setTimeRange(xAxisMinValue, xAxisMaxValue);
        }

        if(getPlot()->yAxis->range().lower < yAxisMinValue
            || getPlot()->yAxis->range().upper > yAxisMaxValue)
        {
            setPresRange(yAxisMinValue, yAxisMaxValue);
        }

        if(getPlot()->yAxis2->range().lower < yAxis2MinValue
            || getPlot()->yAxis2->range().upper > yAxis2MaxValue)
        {
            setTempRange(yAxis2MinValue, yAxis2MaxValue);
        }
        getPlot()->replot();
    }
}

void CustomPlotItem::setLabelVisible(bool visible, const int index)
{
    if(getPlot())
    {
        QList<QCPAbstractItem*> itemList;
        int itemCount = getPlot()->itemCount();
        for(int i = 0; i < itemCount; i++)
        {
            QCPAbstractItem* item = getPlot()->item(i);
            if(item)
            {
                if(QCPItemText* textItem = dynamic_cast<QCPItemText*>(item))
                {
                    if(textItem->selectable())
                    {
                        QCPAxis *associatedAxis = textItem->position->valueAxis();
                        if(associatedAxis == getPlot()->yAxis && index == 0)
                        {
                            textItem->setVisible(visible);
                        }

                        if(associatedAxis == getPlot()->yAxis2 && index == 1)
                        {
                            textItem->setVisible(visible);
                        }
                    }
                }
                else if(QCPItemLine* lineItem = dynamic_cast<QCPItemLine*>(item))
                {
                    QCPAxis *associatedAxis = lineItem->end->valueAxis();
                    if(associatedAxis == getPlot()->yAxis && index == 0)
                    {
                        lineItem->setVisible(visible);
                    }

                    if(associatedAxis == getPlot()->yAxis2 && index == 1)
                    {
                        lineItem->setVisible(visible);
                    }
                }

            }
        }
    }
}

void CustomPlotItem::initPlot()
{
    m_tracer_temp->setVisible(false);
    m_cur_Label_temp->setVisible(false);
    m_tracer_pres->setVisible(false);
    m_cur_Label_pres->setVisible(false);
    m_cur_Label_time->setVisible(false);
    m_refer_lineV->setVisible(false);
    item_selected = nullptr;
}


void CustomPlotItem::setTempGraphData(const QVector<double> &keys, const QVector<double> &values)
{
    if(m_TempGraph){
        m_TempGraph->setData(keys, values);
    }
}

void CustomPlotItem::setPresGraphData(const QVector<double> &keys, const QVector<double> &values)
{
    if(m_PresGraph)
    {
        m_PresGraph->setData(keys, values);
    }
}

void CustomPlotItem::setTempGraphVisible(bool value)
{
    if(m_TempGraph){
        m_TempGraph->setVisible(value);
        getPlot()->yAxis2->setVisible(value);
        setLabelVisible(value, 1);
    }
}

void CustomPlotItem::setPresGraphVisible(bool value)
{
    if(m_PresGraph)
    {
        m_PresGraph->setVisible(value);
        setLabelVisible(value, 0);
    }
}

bool CustomPlotItem::getTempGraphVisible()
{
    if(m_TempGraph){
        return m_TempGraph->visible();
    }
    return false;
}

bool CustomPlotItem::getPresGraphVisible()
{
    if(m_PresGraph){
        return m_PresGraph->visible();
    }
    return false;
}

void CustomPlotItem::setTempRange(double min, double max)
{
    getPlot()->yAxis2->setRange(min, max);
}

void CustomPlotItem::setPresRange(double min, double max)
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
    xAxisMaxValue = getPlot()->xAxis->range().upper;
    yAxisMaxValue = getPlot()->yAxis->range().upper;
    yAxis2MaxValue = getPlot()->yAxis2->range().upper;

    xAxisMinValue = getPlot()->xAxis->range().lower;
    yAxisMinValue = getPlot()->yAxis->range().lower;
    yAxis2MinValue = getPlot()->yAxis2->range().lower;
}

void CustomPlotItem::exportPdf(const QString path)
{
    getPlot()->savePdf(path);
}
void CustomPlotItem::exportExcel(const QString path, const QString data)
{
    QXlsx::Document xlsx;
    QXlsx::Format format;
    format.setHorizontalAlignment(QXlsx::Format::AlignLeft);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setFont(QFont(qApp->font().family(), 12));

    QJsonDocument document = QJsonDocument::fromJson(data.toUtf8());
    if (document.isArray()) {
        QJsonArray jsonArray = document.array();
        QVariantList dataList =  jsonArray.toVariantList();
        //int sheetIndex = 0;
        for(QVariant data : dataList)
        {
            QMap<QString, QVariant> dataMap = data.toMap();
            const QString id = dataMap["id"].toString();
            xlsx.addSheet(id);
            xlsx.selectSheet(id);
            xlsx.setColumnWidth(1, 30);
            xlsx.setColumnWidth(2, 15);
            xlsx.setColumnWidth(3, 15);
            xlsx.setColumnWidth(4, 10);

            int rowIndex = 1;
            int cloIndex = 1;
            xlsx.write(rowIndex, cloIndex, id, format);
            xlsx.write(rowIndex, ++cloIndex, dataMap["name"].toString(), format);
            xlsx.write(rowIndex, ++cloIndex, dataMap["isMan"].toBool() ? "男" : "女", format);
            xlsx.write(rowIndex, ++cloIndex, dataMap["age"].toString(), format);
            QVariantList timeVec = dataMap["timeData"].toList();
            QVariantList preVec = dataMap["pressureData"].toList();
            QVariantList tempVec = dataMap["temperatureData"].toList();
            size_t vecSize = std::min(timeVec.size(), std::min(preVec.size(), tempVec.size()));
            rowIndex += 2;
            cloIndex = 1;
            xlsx.write(rowIndex, cloIndex, "时间", format);
            xlsx.write(rowIndex, ++cloIndex, "压力 mmHg", format);
            xlsx.write(rowIndex, ++cloIndex, "温度 ℃", format);
            for (size_t i = 0; i < vecSize; i++)
            {
                if(preVec[i] == 0 && tempVec[i] == 0)
                {
                    continue;
                }
                ++rowIndex;
                cloIndex = 1;
                QDateTime timeValue = QDateTime::fromSecsSinceEpoch(timeVec[i].toInt());
                QString timeStr = timeValue.toString("yyyy-MM-dd hh:mm:ss");
                xlsx.write(rowIndex, cloIndex, timeStr, format);
                xlsx.write(rowIndex, ++cloIndex, preVec[i].toDouble(), format);
                xlsx.write(rowIndex, ++cloIndex, tempVec[i].toDouble(), format);
            }
            //sheetIndex++;
        }
    }
    xlsx.saveAs(path);
}


void CustomPlotItem::clearValue()
{
    xAxisMaxValue = 0;
    yAxisMaxValue = 0;
    yAxis2MaxValue = 0;

    xAxisMinValue = 0;
    yAxisMinValue = 0;
    yAxis2MinValue = 0;
}

void CustomPlotItem::addLabel(const int type, const double xValue, const double yValue, const QString text)
{
    QCPItemText *textLabel = new QCPItemText(getPlot());
    textLabel->setFont(QFont(qApp->font().family(), 13));
    textLabel->setColor(Qt::white);
    textLabel->position->setCoords(xValue, yValue);
    textLabel->position->setType(QCPItemPosition::ptPlotCoords);
    textLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    textLabel->setBrush(QBrush(QColor(76, 217, 100)));
    textLabel->setPadding(QMargins(5,5,5,5));
    textLabel->setSelectedFont(QFont(qApp->font().family(), 11));
    textLabel->setSelectedBrush(QBrush(QColor(72, 201, 176, 156)));

    if(type == 1){
        textLabel->position->setAxes(getPlot()->xAxis, getPlot()->yAxis2);
    }
    else
    {
        textLabel->position->setAxes(getPlot()->xAxis, getPlot()->yAxis);
    }
    textLabel->setText(text);
    textLabel->setSelectable(true);

    // add the arrow:
    QCPItemLine *arrow = new QCPItemLine(getPlot());
    arrow->setPen(QPen(QColor(76, 217, 100), 1.8, Qt::SolidLine));
    arrow->start->setParentAnchor(textLabel->bottomLeft);
    arrow->end->setCoords(xValue, yValue);
    arrow->end->setType(QCPItemPosition::ptPlotCoords);
    if(type == 1){
        arrow->end->setAxes(getPlot()->xAxis, getPlot()->yAxis2);
    }
    else
    {
        arrow->end->setAxes(getPlot()->xAxis, getPlot()->yAxis);
    }
    arrow->setHead(QCPLineEnding::esSpikeArrow);
    arrow->setSelectable(false);
    replot();
}

void CustomPlotItem::deleteCurLabel()
{
    if(getPlot() && item_selected != nullptr)
    {
        // remove line
        QCPAbstractItem* closedItem = getPlot()->itemAt(item_selected->bottomLeft->pixelPosition());
        if(closedItem)
        {
            QCPItemLine* lineItem = dynamic_cast<QCPItemLine*>(closedItem);
            if(lineItem)
            {
                getPlot()->removeItem(lineItem);
            }
        }
        getPlot()->removeItem(item_selected);
        item_selected = nullptr;
        selectedTextChanged(false);
        replot();
    }
}

void CustomPlotItem::clearLabels()
{
    if(getPlot())
    {
        QList<QCPAbstractItem*> itemList;
        int itemCount = getPlot()->itemCount();
        for(int i = 0; i < itemCount; i++)
        {
            QCPAbstractItem* item = getPlot()->item(i);
            if(item)
            {
                if(QCPItemText* textItem = dynamic_cast<QCPItemText*>(item))
                {
                    if(textItem->selectable())
                    {
                        itemList.push_back(item);
                    }
                }
                else if(QCPItemLine* lineItem = dynamic_cast<QCPItemLine*>(item))
                {
                    itemList.push_back(item);
                }

            }
        }
        for(auto tItem : itemList)
        {
            if(tItem)
            {
                getPlot()->removeItem(tItem);
            }
        }
        item_selected = nullptr;
        selectedTextChanged(false);
        replot();
    }
}

void CustomPlotItem::editLabel(const QString text)
{
    if(getPlot() && item_selected != nullptr)
    {
        item_selected->setText(text);
        item_selected->setSelected(false);
        item_selected = nullptr;
        selectedTextChanged(false);
        replot();
    }
}

QString CustomPlotItem::getLabelText()
{
    if(getPlot() && item_selected != nullptr)
    {
        return item_selected->text();
    }
    return "";
}

double CustomPlotItem::getLabelTime()
{
    if(getPlot() && item_selected != nullptr)
    {
        // remove line
        QCPAbstractItem* closedItem = getPlot()->itemAt(item_selected->bottomLeft->pixelPosition());
        if(closedItem)
        {
            QCPItemLine* lineItem = dynamic_cast<QCPItemLine*>(closedItem);
            if(lineItem)
            {
                return lineItem->end->coords().x();
            }
        }
        return 0;
    }
}

QString CustomPlotItem::getTimeLabel()
{
    if(m_cur_Label_time->visible())
        return m_cur_Label_time->text();
    return "";
}
