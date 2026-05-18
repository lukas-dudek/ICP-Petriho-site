/**
 * @file place_item.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Grafická reprezentace místa (place) v Qt scéně.
 */
#ifndef PLACE_ITEM_H
#define PLACE_ITEM_H

#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QString>

/**
 * @brief Grafická reprezentace místa (place) v Petriho síti.
 * @author Jan
 */
class ArcItem; // forward deklarace

class PlaceItem : public QGraphicsEllipseItem
{
public:
    explicit PlaceItem(const QString& id, int initialTokens = 0);
    ~PlaceItem() override;

    QString placeId() const { return id_; }
    void    setPlaceId(const QString& id);
    void    setTokens(int count);
    int     tokens() const { return token_count_; }

    void    setInitialTokens(int t) { initial_tokens_ = t; }
    int     initialTokens() const { return initial_tokens_; }

    void    setPlaceAction(const QString& code) { place_action_ = code; }
    QString placeAction() const { return place_action_; }

    void addArc(ArcItem* arc);
    void removeArc(ArcItem* arc);

    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString id_;
    int     token_count_    = 0;
    int     initial_tokens_ = 0;
    QString place_action_;
    QGraphicsTextItem* label_       = nullptr;
    QGraphicsTextItem* token_label_ = nullptr;
    QList<ArcItem*>    arcs_;
};

#endif