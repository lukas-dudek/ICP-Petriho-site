/**
 * @file transition_item.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Implementace vykreslování přechodů se zvýrazněním v Qt scéně.
 */
/**
 * @brief Implementace grafické reprezentace přechodu.
 * @author Jan
 */
#include "transition_item.h"
#include "arc_item.h"
#include "../widgets/transition_dialog.h"

#include <QPainter>
#include <QFont>
#include <QPen>
#include <QGraphicsSceneMouseEvent>

TransitionItem::TransitionItem(const QString& id)
    : id_(id)
{
    setRect(QRectF(-30, -20, 60, 40));

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);

    setBrush(Qt::lightGray);
    setPen(QPen(Qt::black, 2));

    label_ = new QGraphicsTextItem(id, this);
    label_->setPos(QPointF(-25, -40));
}

TransitionItem::~TransitionItem()
{
    while (!arcs_.isEmpty()) {
        delete arcs_.takeFirst();
    }
}

void TransitionItem::setTransitionId(const QString& id)
{
    id_ = id;
    if (label_) label_->setPlainText(id);
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

void TransitionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);

    TransitionDialog dlg;
    dlg.setId(id_);
    dlg.setInputEvent(input_event_);
    dlg.setGuardExpression(guard_expression_);
    if (delay_ms_ > 0)
        dlg.setDelayMs(delay_ms_);
    if (!delay_expr_.isEmpty())
        dlg.setDelayExpr(delay_expr_);
    dlg.setActionCode(action_code_);
    dlg.setPriority(priority_);

    if (dlg.exec() == QDialog::Accepted) {
        setTransitionId(dlg.id());
        input_event_ = dlg.inputEvent();
        guard_expression_ = dlg.guardExpression();
        delay_ms_ = dlg.hasDelay() ? dlg.delayMs() : 0;
        delay_expr_ = dlg.delayExpr();
        action_code_ = dlg.actionCode();
        priority_ = dlg.priority();
    }
}

void TransitionItem::addArc(ArcItem* arc)
{
    if (!arcs_.contains(arc)) arcs_.append(arc);
}

void TransitionItem::removeArc(ArcItem* arc)
{
    arcs_.removeAll(arc);
}

QVariant TransitionItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange || change == ItemPositionHasChanged) {
        for (auto* arc : arcs_) {
            arc->updatePath();
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}