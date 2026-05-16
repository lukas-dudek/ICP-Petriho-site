/**
 * @file    place_item.cpp
 * @brief   Implementace grafické reprezentace místa.
 * @author  Jan
 * @date    2026-05-04
 */

#include "place_item.h"
#include <QPainter>
#include <QFont>

PlaceItem::PlaceItem(const QString& id, int initialTokens)
    : id_(id),
      token_count_(initialTokens)
{
    // Kruh o průměru 60 px
    setRect(QRectF(-30, -30, 60, 60));

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

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

void PlaceItem::setTokens(int count)
{
    token_count_ = count;
    if (token_label_) {
        token_label_->setPlainText(QString::number(count));
    }
}
