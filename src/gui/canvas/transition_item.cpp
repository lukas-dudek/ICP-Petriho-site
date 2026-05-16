/** @brief Implementace grafické reprezentace přechodu. @author Jan */
#include "transition_item.h"
#include <QPainter>
#include <QPen>

TransitionItem::TransitionItem(const QString& id)
    : id_(id)
{
    setRect(QRectF(-30, -20, 60, 40));

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    setBrush(Qt::lightGray);
    setPen(QPen(Qt::black, 2));

    label_ = new QGraphicsTextItem(id, this);
    label_->setPos(QPointF(-25, -40));
}

void TransitionItem::setHighlight(HighlightType type)
{
    highlight_ = type;
    switch (type) {
    case HighlightType::Normal:   setBrush(Qt::lightGray); break;
    case HighlightType::Enabled:  setBrush(Qt::green);     break;
    case HighlightType::Pending:  setBrush(QColor(255,165,0)); break;
    case HighlightType::Firing:   setBrush(Qt::cyan);      break;
    }
    update();
}