#ifndef TRANSITION_ITEM_H
#define TRANSITION_ITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

enum class HighlightType {
    Normal,
    Enabled,
    Pending,
    Firing
};

class TransitionItem : public QGraphicsRectItem
{
public:
    explicit TransitionItem(const QString& id);

    QString transitionId() const { return id_; }
    void    setHighlight(HighlightType type);
    HighlightType highlight() const { return highlight_; }

    enum { Type = QGraphicsItem::UserType + 2 };
    int type() const override { return Type; }

private:
    QString       id_;
    HighlightType highlight_ = HighlightType::Normal;
    QGraphicsTextItem* label_ = nullptr;
};

#endif