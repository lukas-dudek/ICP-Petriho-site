/**
 * @file transition_dialog.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Deklarace dialogu pro editaci vlastností přechodu.
 */
/**
 * @file    transition_dialog.h
 * @brief   Dialog pro editaci vlastností přechodu.
 * @author  Jan
 * @date    2026-05-04
 */

#ifndef TRANSITION_DIALOG_H
#define TRANSITION_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>

/**
 * @brief Dialog pro editaci vlastností přechodu (jméno, guard, event, delay, akce).
 */
class TransitionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransitionDialog(QWidget* parent = nullptr);

    void setId(const QString& id);
    void setInputEvent(const QString& event);
    void setGuardExpression(const QString& guard);
    void setDelayMs(int64_t delay);
    void setDelayExpr(const QString& expr);
    void setActionCode(const QString& code);
    void setPriority(int priority);

    QString id() const;
    QString inputEvent() const;
    QString guardExpression() const;
    int64_t delayMs() const;
    QString delayExpr() const;
    QString actionCode() const;
    int     priority() const;
    bool    hasDelay() const;

private:
    QLineEdit* id_edit_      = nullptr;
    QLineEdit* event_edit_   = nullptr;
    QLineEdit* guard_edit_   = nullptr;
    QLineEdit* delay_edit_   = nullptr;
    QTextEdit* action_edit_  = nullptr;
    QSpinBox*  priority_spin_ = nullptr;
    QCheckBox* delay_check_  = nullptr;
};

#endif // TRANSITION_DIALOG_H
