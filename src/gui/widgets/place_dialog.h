/**
 * @file place_dialog.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
/**
 * @file    place_dialog.h
 * @brief   Dialog pro editaci vlastností místa.
 * @author  Jan
 * @date    2026-05-04
 */

#ifndef PLACE_DIALOG_H
#define PLACE_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

/**
 * @brief Dialog pro editaci vlastností místa (jméno, tokeny, place action).
 */
class PlaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlaceDialog(QWidget* parent = nullptr);

    void setId(const QString& id);
    void setInitialTokens(int tokens);
    void setPlaceAction(const QString& code);

    QString id() const;
    int     initialTokens() const;
    QString placeAction() const;

private:
    QLineEdit* id_edit_      = nullptr;
    QSpinBox*  tokens_spin_  = nullptr;
    QTextEdit* action_edit_  = nullptr;
};

#endif // PLACE_DIALOG_H
