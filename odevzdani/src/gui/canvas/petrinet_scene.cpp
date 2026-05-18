/**
 * @file petrinet_scene.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Implementace scény - přidávání prvků, kreslení hran myší.
 */
#include "petrinet_scene.h"
#include "place_item.h"
#include "transition_item.h"
#include "arc_item.h"
#include "interpreter/runtime_state.h"

#include <QGraphicsSceneMouseEvent>

using pets::RuntimeState;

PetriNetScene::PetriNetScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(QRectF(-2000, -2000, 4000, 4000));
}

PlaceItem* PetriNetScene::addPlace(const QPointF& pos, const QString& id, int initialTokens)
{
    auto* item = new PlaceItem(id, initialTokens);
    item->setPos(pos);
    addItem(item);
    emit networkModified();
    return item;
}

TransitionItem* PetriNetScene::addTransition(const QPointF& pos, const QString& id)
{
    auto* item = new TransitionItem(id);
    item->setPos(pos);
    addItem(item);
    emit networkModified();
    return item;
}

ArcItem* PetriNetScene::addArc(QGraphicsItem* from, QGraphicsItem* to, int weight)
{
    auto* fromPlace = dynamic_cast<PlaceItem*>(from);
    auto* toTransition = dynamic_cast<TransitionItem*>(to);
    auto* fromTransition = dynamic_cast<TransitionItem*>(from);
    auto* toPlace = dynamic_cast<PlaceItem*>(to);

    ArcItem* arc = nullptr;
    if (fromPlace && toTransition) {
        arc = new ArcItem(fromPlace, toTransition, weight);
    } else if (fromTransition && toPlace) {
        arc = new ArcItem(fromTransition, toPlace, weight);
    }

    if (arc) {
        addItem(arc);
        emit networkModified();
    }
    return arc;
}

void PetriNetScene::removeSelected()
{
    auto selected = selectedItems();
    if (selected.isEmpty()) return;

    for (auto* item : selected) {
        if (!item->scene()) continue; // Položka už byla smazána kaskádově
        delete item;
    }
    emit networkModified();
}

void PetriNetScene::clear()
{
    QGraphicsScene::clear();
    place_counter_ = 1;
    transition_counter_ = 1;
}

PlaceItem* PetriNetScene::findPlace(const QString& id) const
{
    for (auto* item : items()) {
        if (item->type() == PlaceItem::Type && static_cast<PlaceItem*>(item)->placeId() == id)
            return static_cast<PlaceItem*>(item);
    }
    return nullptr;
}

TransitionItem* PetriNetScene::findTransition(const QString& id) const
{
    for (auto* item : items()) {
        if (item->type() == TransitionItem::Type && static_cast<TransitionItem*>(item)->transitionId() == id)
            return static_cast<TransitionItem*>(item);
    }
    return nullptr;
}

QString PetriNetScene::generatePlaceId() const
{
    return QString("P%1").arg(place_counter_++);
}

QString PetriNetScene::generateTransitionId() const
{
    return QString("T%1").arg(transition_counter_++);
}

void PetriNetScene::updateTokens(const RuntimeState& state)
{
    for (const auto& nv : state.places_tokens) {
        auto* place = findPlace(QString::fromStdString(nv.name));
        if (place) {
            place->setTokens(std::stoi(nv.value));
        }
    }
}

void PetriNetScene::updateHighlights(const RuntimeState& state)
{
    // Reset všech
    for (auto* item : items()) {
        if (item->type() == TransitionItem::Type)
            static_cast<TransitionItem*>(item)->setHighlight(HighlightType::Normal);
    }

    // Enabled
    for (const auto& id : state.enabled_transitions) {
        auto* t = findTransition(QString::fromStdString(id));
        if (t) t->setHighlight(HighlightType::Enabled);
    }

    // Pending timers
    for (const auto& timer : state.pending_timers) {
        auto* t = findTransition(QString::fromStdString(timer.transition_id));
        if (t) t->setHighlight(HighlightType::Pending);
    }
}

// ============================================================
// Mouse - kreslení hran
// ============================================================

void PetriNetScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::RightButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    auto* item = itemAt(event->scenePos(), QTransform());
    if (item && (item->type() == PlaceItem::Type || item->type() == TransitionItem::Type)) {
        drawing_arc_ = true;
        arc_start_item_ = item;

        temp_line_ = new QGraphicsLineItem();
        temp_line_->setPen(QPen(Qt::darkGray, 1.5, Qt::DashLine));
        QLineF line(item->scenePos(), event->scenePos());
        temp_line_->setLine(line);
        addItem(temp_line_);
    }
}

void PetriNetScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (drawing_arc_ && temp_line_) {
        QLineF line(arc_start_item_->scenePos(), event->scenePos());
        temp_line_->setLine(line);
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void PetriNetScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (drawing_arc_ && event->button() == Qt::RightButton) {
        drawing_arc_ = false;
        if (temp_line_) {
            removeItem(temp_line_);
            delete temp_line_;
            temp_line_ = nullptr;
        }

        auto* toItem = itemAt(event->scenePos(), QTransform());
        if (toItem && toItem != arc_start_item_ &&
            (toItem->type() == PlaceItem::Type || toItem->type() == TransitionItem::Type)) {
            addArc(arc_start_item_, toItem, 1);
        }
        arc_start_item_ = nullptr;
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}