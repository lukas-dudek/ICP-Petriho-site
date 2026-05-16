#ifndef ARC_ITEM_H
#define ARC_ITEM_H

#include <QGraphicsPathItem>
#include <QGraphicsTextItem>

class PlaceItem;
class TransitionItem;

class ArcItem : public QGraphicsPathItem
{
public:
    ArcItem(PlaceItem* fromPlace, TransitionItem* toTransition, int weight = 1);
    ArcItem(TransitionItem* fromTransition, PlaceItem* toPlace, int weight = 1);
    ~ArcItem() override;

    bool isInputArc() const { return is_input_; }
    bool isOutputArc() const { return !is_input_; }

    PlaceItem*      placeItem()      const { return place_; }
    TransitionItem* transitionItem() const { return transition_; }

    int  weight() const { return weight_; }
    void setWeight(int w);

    void updatePath();

    enum { Type = QGraphicsItem::UserType + 3 };
    int type() const override { return Type; }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    PlaceItem*      place_      = nullptr;
    TransitionItem* transition_ = nullptr;
    int             weight_     = 1;
    QGraphicsTextItem* weight_label_ = nullptr;
    bool            is_input_   = true;  // Place→Transition (input), Transition→Place (output)
};

#endif