/**
 * @file place_item.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
/**
 * @file    place_item.cpp
 * @brief   Implementace grafické reprezentace místa.
 * @author  Jan
 * @date    2026-05-04
 */

#include "place_item.h"
#include "arc_item.h"
#include "../widgets/place_dialog.h"

#include <QPainter>
#include <QFont>
#include <QGraphicsSceneMouseEvent>

PlaceItem::PlaceItem(const QString& id, int initialTokens)
    : id_(id),
      token_count_(initialTokens),
      initial_tokens_(initialTokens)
{
    // Kruh o průměru 60 px
    setRect(QRectF(-30, -30, 60, 60));

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    setBrush(Qt::white);
    setPen(QPen(Qt::black, 2));

    // Popisek – jméno
    label_ = new QGraphicsTextItem(id, this);
    label_->setPos(QPointF(-25, -50));

    // Popisek – počet tokenů
    token_label_ = new QGraphicsTextItem(QString::number(token_count_), this);
    token_label_->setPos(QPointF(-8, -8));
    QFont f = token_label_->font();
    f.setPointSize(16);
    f.setBold(true);
    token_label_->setFont(f);
}

void PlaceItem::setPlaceId(const QString& id)
{
    id_ = id;
    if (label_) label_->setPlainText(id);
}

void PlaceItem::setTokens(int count)
{
    token_count_ = count;
    if (token_label_) {
        token_label_->setPlainText(QString::number(count));
    }
}

void PlaceItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);

    PlaceDialog dlg;
    dlg.setId(id_);
    dlg.setInitialTokens(initial_tokens_);
    dlg.setPlaceAction(place_action_);

    if (dlg.exec() == QDialog::Accepted) {
        setPlaceId(dlg.id());
        initial_tokens_ = dlg.initialTokens();
        setTokens(dlg.initialTokens());
        place_action_ = dlg.placeAction();
    }
}

void PlaceItem::addArc(ArcItem* arc)
{
    if (!arcs_.contains(arc)) arcs_.append(arc);
}

void PlaceItem::removeArc(ArcItem* arc)
{
    arcs_.removeAll(arc);
}

QVariant PlaceItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange || change == ItemPositionHasChanged) {
        for (auto* arc : arcs_) {
            arc->updatePath();
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}
