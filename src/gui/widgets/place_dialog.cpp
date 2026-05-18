/**
 * @file place_dialog.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Implementace dialogu pro editaci tokenů a akce místa.
 */
/**
 * @file    place_dialog.cpp
 * @brief   Implementace dialogu pro editaci místa.
 * @author  Jan
 * @date    2026-05-04
 */

#include "place_dialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFont>

PlaceDialog::PlaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Vlastnosti místa"));
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);

    auto* form = new QFormLayout();

    id_edit_ = new QLineEdit(this);
    form->addRow(tr("Identifikátor:"), id_edit_);

    tokens_spin_ = new QSpinBox(this);
    tokens_spin_->setRange(0, 9999);
    form->addRow(tr("Počáteční tokeny:"), tokens_spin_);

    mainLayout->addLayout(form);

    // Akce místa
    mainLayout->addWidget(new QLabel(tr("Akce místa (on_add_token, C/C++):"), this));
    action_edit_ = new QTextEdit(this);
    action_edit_->setMinimumHeight(80);
    QFont mono("Monospace", 10);
    mono.setStyleHint(QFont::Monospace);
    action_edit_->setFont(mono);
    action_edit_->setPlaceholderText(tr("// kód provedený při přidání tokenu\n// Např.: output(\"out\", 1);"));
    mainLayout->addWidget(action_edit_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void PlaceDialog::setId(const QString& id) { id_edit_->setText(id); }
void PlaceDialog::setInitialTokens(int t) { tokens_spin_->setValue(t); }
void PlaceDialog::setPlaceAction(const QString& code) { action_edit_->setPlainText(code); }

QString PlaceDialog::id() const { return id_edit_->text().trimmed(); }
int     PlaceDialog::initialTokens() const { return tokens_spin_->value(); }
QString PlaceDialog::placeAction() const { return action_edit_->toPlainText().trimmed(); }
