#ifndef PLACE_ITEM_H
#define PLACE_ITEM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>

class PlaceItem : public QGraphicsEllipseItem
{
public:
    explicit PlaceItem(const QString& id, int initialTokens = 0);

    QString placeId() const { return id_; }
    void    setTokens(int count);
    int     tokens() const { return token_count_; }

    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }

private:
    QString id_;
    int     token_count_ = 0;
    QGraphicsTextItem* label_       = nullptr;
    QGraphicsTextItem* token_label_ = nullptr;
};

#endif