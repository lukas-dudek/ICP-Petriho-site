/**
 * @file petrinet_view.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef PETRINET_VIEW_H
#define PETRINET_VIEW_H

#include <QGraphicsView>

class PetriNetScene;

class PetriNetView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PetriNetView(PetriNetScene* scene, QWidget* parent = nullptr);

    void zoomIn();
    void zoomOut();
    void fitInView();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    qreal current_zoom_ = 1.0;
};

#endif