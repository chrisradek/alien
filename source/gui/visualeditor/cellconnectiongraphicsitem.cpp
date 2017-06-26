#include "cellconnectiongraphicsitem.h"

#include "Model/Entities/Descriptions.h"
#include "gui/Settings.h"
#include "gui/Settings.h"

#include <QPainter>
#include <qmath.h>

CellConnectionGraphicsItem::CellConnectionGraphicsItem(GraphicsItemConfig* config, CellDescription const & cell1, CellDescription const & cell2, QGraphicsItem * parent)
	: QGraphicsItem(parent)
{
	QGraphicsItem::setZValue(-1.0);
	update(cell1, cell2);
}

/*
CellConnectionGraphicsItem::CellConnectionGraphicsItem (qreal x1, qreal y1, qreal x2, qreal y2, ConnectionState s, QGraphicsItem* parent)
    : QGraphicsItem(parent), _dx(x2-x1), _dy(y2-y1), _connectionState(s)
{
    QGraphicsItem::setPos(x1, y1);
    QGraphicsItem::setZValue(-1.0);
}
*/

void CellConnectionGraphicsItem::update(CellDescription const & cell1, CellDescription const & cell2)
{
	auto const &pos1 = cell1.pos.getValue();
	auto const &pos2 = cell2.pos.getValue();
	_dx = (pos2.x() - pos1.x()) * GRAPHICS_ITEM_SIZE;
	_dy = (pos2.y() - pos1.y()) * GRAPHICS_ITEM_SIZE;

	QGraphicsItem::setPos(pos1.x() * GRAPHICS_ITEM_SIZE, pos1.y() * GRAPHICS_ITEM_SIZE);
	_connectionState = ConnectionState::NO_DIR_CONNECTION;
}

QRectF CellConnectionGraphicsItem::boundingRect () const
{
    qreal minX = qMin(0.0, _dx);
    qreal minY = qMin(0.0, _dy);
    qreal maxX = qMax(0.0, _dx);
    qreal maxY = qMax(0.0, _dy);
    return QRectF(minX, minY, (maxX-minX), (maxY-minY));
}

void CellConnectionGraphicsItem::paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if( _connectionState == NO_DIR_CONNECTION )
        painter->setPen(QPen(QBrush(LINE_INACTIVE_COLOR), 0.03 * GRAPHICS_ITEM_SIZE));
    else
        painter->setPen(QPen(QBrush(LINE_ACTIVE_COLOR), 0.03 * GRAPHICS_ITEM_SIZE));
    painter->drawLine(QPointF(0.0, 0.0), QPointF(_dx, _dy));

    if( (_connectionState == A_TO_B_CONNECTION) || (_connectionState == B_TO_A_CONNECTION) ) {
        qreal x2 = _dx;
        qreal y2 = _dy;
        qreal relPosX = -_dx;
        qreal relPosY = -_dy;
        if( _connectionState == B_TO_A_CONNECTION ) {
            x2 = 0.0;
            y2 = 0.0;
            relPosX = _dx;
            relPosY = _dy;
        }

        qreal len = qSqrt(relPosX*relPosX+relPosY*relPosY);
        relPosX = relPosX / len * GRAPHICS_ITEM_SIZE;
        relPosY = relPosY / len * GRAPHICS_ITEM_SIZE;

        //rotate 45 degree counterclockwise and scaling
        qreal aX = (relPosX-relPosY)/10.0;
        qreal aY = (relPosX+relPosY)/10.0;
        qreal bX = x2 + relPosX*0.35;
        qreal bY = y2 + relPosY*0.35;
        painter->drawLine(QPointF(bX, bY), QPointF(bX+aX, bY+aY));

        //rotate 45 degree clockwise
        aX = (relPosX+relPosY)/10.0;
        aY = (-relPosX+relPosY)/10.0;
        painter->drawLine(QPointF(bX, bY), QPointF(bX+aX, bY+aY));
    }
}

void CellConnectionGraphicsItem::setConnectionState (ConnectionState connectionState)
{
    _connectionState = connectionState;
}
