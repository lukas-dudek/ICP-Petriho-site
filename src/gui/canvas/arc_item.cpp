/** @brief Implementace grafické reprezentace hrany. @author Jan */
#include "arc_item.h"
#include "place_item.h"
#include "transition_item.h"
#include <QPainter>
#include <QPen>
#include <QtMath>

ArcItem::ArcItem(PlaceItem* fromPlace, TransitionItem* toTransition, int weight)
    : place_(fromPlace), transition_(toTransition), weight_(weight), is_input_(true)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 1.5));

    weight_label_ = new QGraphicsTextItem(QString::number(weight), this);
    updatePath();
}

ArcItem::ArcItem(TransitionItem* fromTransition, PlaceItem* toPlace, int weight)
    : place_(toPlace), transition_(fromTransition), weight_(weight), is_input_(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 1.5));

    weight_label_ = new QGraphicsTextItem(QString::number(weight), this);
    updatePath();
}

void ArcItem::setWeight(int w)
{
    weight_ = w;
    if (weight_label_) weight_label_->setPlainText(QString::number(w));
}

void ArcItem::updatePath()
{
    if (!place_ || !transition_) return;

    QPointF p = place_->scenePos();
    QPointF t = transition_->scenePos();

    QPainterPath path;
    if (is_input_) path.moveTo(p); else path.moveTo(t);

    // Bézier křivka pro hezčí zobrazení
    QPointF ctrl = (p + t) / 2;
    if (is_input_) {
        path.cubicTo(QPointF(p.x(), ctrl.y()), QPointF(t.x(), ctrl.y()), t);
    } else {
        path.cubicTo(QPointF(t.x(), ctrl.y()), QPointF(p.x(), ctrl.y()), p);
    }

    setPath(path);

    // Šipka na konci
    // (zjednodušeně se vykreslí v paint, nebo se může přidat polygon)

    // Váha doprostřed
    if (weight_label_) {
        QPointF mid = (p + t) / 2;
        weight_label_->setPos(mid + QPointF(5, 5));
    }
}
