/**
 * @file transition_dialog.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
/**
 * @file    transition_dialog.cpp
 * @brief   Implementace dialogu pro editaci přechodu.
 * @author  Jan
 * @date    2026-05-04
 */

#include "transition_dialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QCheckBox>
#include <QFont>

TransitionDialog::TransitionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Vlastnosti přechodu"));
    setMinimumWidth(450);

    auto* mainLayout = new QVBoxLayout(this);

    // Formulář
    auto* form = new QFormLayout();

    id_edit_ = new QLineEdit(this);
    form->addRow(tr("Identifikátor:"), id_edit_);

    event_edit_ = new QLineEdit(this);
    event_edit_->setPlaceholderText(tr("(prázdné = interní přechod)"));
    form->addRow(tr("Vstupní událost:"), event_edit_);

    guard_edit_ = new QLineEdit(this);
    guard_edit_->setPlaceholderText(tr("C/C++ booleovský výraz"));
    form->addRow(tr("Stráž [ ]:"), guard_edit_);

    delay_check_ = new QCheckBox(tr("Zpožděný přechod"), this);
    form->addRow(delay_check_);

    delay_spin_ = new QSpinBox(this);
    delay_spin_->setRange(0, 999999);
    delay_spin_->setSuffix(" ms");
    delay_spin_->setEnabled(false);
    form->addRow(tr("Zpoždění @:"), delay_spin_);

    connect(delay_check_, &QCheckBox::toggled, delay_spin_, &QSpinBox::setEnabled);

    priority_spin_ = new QSpinBox(this);
    priority_spin_->setRange(0, 100);
    form->addRow(tr("Priorita:"), priority_spin_);

    mainLayout->addLayout(form);

    // Akce – větší text edit
    mainLayout->addWidget(new QLabel(tr("Akce (C/C++ kód):"), this));
    action_edit_ = new QTextEdit(this);
    action_edit_->setMinimumHeight(100);
    QFont mono("Monospace", 10);
    mono.setStyleHint(QFont::Monospace);
    action_edit_->setFont(mono);
    action_edit_->setPlaceholderText(tr("// kód akce přechodu\n// Např.: output(\"out\", \"1\");"));
    mainLayout->addWidget(action_edit_);

    // Tlačítka
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void TransitionDialog::setId(const QString& id) { id_edit_->setText(id); }
void TransitionDialog::setInputEvent(const QString& event) { event_edit_->setText(event); }
void TransitionDialog::setGuardExpression(const QString& guard) { guard_edit_->setText(guard); }

void TransitionDialog::setDelayMs(int64_t delay)
{
    if (delay > 0) {
        delay_check_->setChecked(true);
        delay_spin_->setValue(static_cast<int>(delay));
    } else {
        delay_check_->setChecked(false);
        delay_spin_->setValue(0);
    }
}

void TransitionDialog::setActionCode(const QString& code) { action_edit_->setPlainText(code); }
void TransitionDialog::setPriority(int priority) { priority_spin_->setValue(priority); }

QString TransitionDialog::id() const { return id_edit_->text().trimmed(); }
QString TransitionDialog::inputEvent() const { return event_edit_->text().trimmed(); }
QString TransitionDialog::guardExpression() const { return guard_edit_->text().trimmed(); }
int64_t TransitionDialog::delayMs() const { return delay_spin_->value(); }
QString TransitionDialog::actionCode() const { return action_edit_->toPlainText().trimmed(); }
int     TransitionDialog::priority() const { return priority_spin_->value(); }
bool    TransitionDialog::hasDelay() const { return delay_check_->isChecked(); }
