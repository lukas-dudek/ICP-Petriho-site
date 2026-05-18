/**
 * @file transition_item.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Grafická reprezentace přechodu (transition) v Qt scéně.
 */
#ifndef TRANSITION_ITEM_H
#define TRANSITION_ITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QString>
#include <cstdint>

enum class HighlightType {
    Normal,
    Enabled,
    Pending,
    Firing
};

class ArcItem; // forward deklarace

class TransitionItem : public QGraphicsRectItem
{
public:
    explicit TransitionItem(const QString& id);
    ~TransitionItem() override;

    QString transitionId() const { return id_; }
    void    setTransitionId(const QString& id);
    void    setHighlight(HighlightType type);
    HighlightType highlight() const { return highlight_; }

    // Editovatelné vlastnosti
    void    setInputEvent(const QString& e)    { input_event_ = e; }
    QString inputEvent() const                  { return input_event_; }

    void    setGuardExpression(const QString& g){ guard_expression_ = g; }
    QString guardExpression() const             { return guard_expression_; }

    void    setDelayMs(int64_t d)               { delay_ms_ = d; }
    int64_t delayMs() const                     { return delay_ms_; }

    void    setDelayExpr(const QString& e)       { delay_expr_ = e; }
    QString delayExpr() const                   { return delay_expr_; }

    bool    hasDelay() const                    { return delay_ms_ > 0 || !delay_expr_.isEmpty(); }

    void    setActionCode(const QString& c)     { action_code_ = c; }
    QString actionCode() const                  { return action_code_; }

    void    setPriority(int p)                   { priority_ = p; }
    int     priority() const                     { return priority_; }

    void addArc(ArcItem* arc);
    void removeArc(ArcItem* arc);

    enum { Type = QGraphicsItem::UserType + 2 };
    int type() const override { return Type; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QString       id_;
    HighlightType highlight_ = HighlightType::Normal;
    QGraphicsTextItem* label_ = nullptr;

    // Editovatelné vlastnosti přechodu
    QString input_event_;
    QString guard_expression_;
    int64_t delay_ms_ = 0;
    QString delay_expr_;
    QString action_code_;
    int     priority_ = 0;

    QList<ArcItem*> arcs_;
};

#endif