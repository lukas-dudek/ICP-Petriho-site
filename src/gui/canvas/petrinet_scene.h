/**
 * @file petrinet_scene.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef PETRINET_SCENE_H
#define PETRINET_SCENE_H

#include <QGraphicsScene>

class PlaceItem;
class TransitionItem;
class ArcItem;

namespace pets { struct RuntimeState; }

using pets::RuntimeState;

class PetriNetScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit PetriNetScene(QObject* parent = nullptr);

    PlaceItem*      addPlace(const QPointF& pos, const QString& id, int initialTokens = 0);
    TransitionItem* addTransition(const QPointF& pos, const QString& id);
    ArcItem*        addArc(QGraphicsItem* from, QGraphicsItem* to, int weight = 1);

    void removeSelected();
    void clear();

    PlaceItem*      findPlace(const QString& id) const;
    TransitionItem* findTransition(const QString& id) const;

    QString generatePlaceId()      const;
    QString generateTransitionId() const;

    // Runtime aktualizace
    void updateTokens(const RuntimeState& state);
    void updateHighlights(const RuntimeState& state);

signals:
    void networkModified();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    bool            drawing_arc_ = false;
    QGraphicsItem*  arc_start_item_ = nullptr;
    QGraphicsLineItem* temp_line_ = nullptr;

    mutable int     place_counter_      = 1;
    mutable int     transition_counter_ = 1;
};

#endif