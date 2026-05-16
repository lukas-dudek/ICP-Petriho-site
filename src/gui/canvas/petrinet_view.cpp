/** @brief Implementace pohledu s podporou zoomu. @author Jan */
#include "petrinet_view.h"
#include "petrinet_scene.h"
#include <QWheelEvent>
#include <QtMath>

PetriNetView::PetriNetView(PetriNetScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void PetriNetView::zoomIn()
{
    scale(1.2, 1.2);
    current_zoom_ *= 1.2;
}

void PetriNetView::zoomOut()
{
    scale(1.0 / 1.2, 1.0 / 1.2);
    current_zoom_ /= 1.2;
}

void PetriNetView::fitInView()
{
    QGraphicsView::fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    current_zoom_ = transform().m11();
}

void PetriNetView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}