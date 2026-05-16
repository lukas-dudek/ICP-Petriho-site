/** @brief Implementace panelu pro injektování vstupů. @author Jan */
#include "injection_panel.h"
#include <QVBoxLayout>
#include <QLabel>

InjectionPanel::InjectionPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Vstup:", this));
    input_selector_ = new QComboBox(this);
    layout->addWidget(input_selector_);

    layout->addWidget(new QLabel("Hodnota:", this));
    value_edit_ = new QLineEdit(this);
    layout->addWidget(value_edit_);

    inject_btn_ = new QPushButton("Odeslat vstup", this);
    layout->addWidget(inject_btn_);
    layout->addStretch();

    connect(inject_btn_, &QPushButton::clicked, this, [this]() {
        if (!input_selector_->currentText().isEmpty() && !value_edit_->text().isEmpty()) {
            emit injectRequested(input_selector_->currentText(), value_edit_->text());
        }
    });
}

void InjectionPanel::setAvailableInputs(const QStringList& inputNames)
{
    input_selector_->clear();
    input_selector_->addItems(inputNames);
}