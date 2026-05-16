/** @brief Implementace grafické reprezentace hrany. @author Jan */
#include "arc_item.h"
#include "place_item.h"
#include "transition_item.h"
#include <QPainter>
#include <QPen>
#include <QtMath>
#include <QInputDialog>
#include <QGraphicsScene>

ArcItem::ArcItem(PlaceItem* fromPlace, TransitionItem* toTransition, int weight)
    : place_(fromPlace), transition_(toTransition), weight_(weight), is_input_(true)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 1.5));

    weight_label_ = new QGraphicsTextItem(QString::number(weight), this);
    if (place_) place_->addArc(this);
    if (transition_) transition_->addArc(this);
    updatePath();
}

ArcItem::ArcItem(TransitionItem* fromTransition, PlaceItem* toPlace, int weight)
    : place_(toPlace), transition_(fromTransition), weight_(weight), is_input_(false)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 1.5));

    weight_label_ = new QGraphicsTextItem(QString::number(weight), this);
    if (place_) place_->addArc(this);
    if (transition_) transition_->addArc(this);
    updatePath();
}

ArcItem::~ArcItem()
{
    if (place_) place_->removeArc(this);
    if (transition_) transition_->removeArc(this);
}

void ArcItem::setWeight(int w)
{
    weight_ = w;
    if (weight_label_) {
        weight_label_->setPlainText(w > 1 ? QString::number(w) : QString());
    }
}

void ArcItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    bool ok;
    int newWeight = QInputDialog::getInt(nullptr, "Nastavit váhu", "Váha hrany:", weight_, 1, 10000, 1, &ok);
    if (ok && newWeight >= 1) {
        setWeight(newWeight);
        updatePath();
        if (scene()) {
            scene()->update();
        }
    }
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
