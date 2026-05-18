/**
 * @file injection_panel.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef INJECTION_PANEL_H
#define INJECTION_PANEL_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

class InjectionPanel : public QWidget
{
    Q_OBJECT

public:
    explicit InjectionPanel(QWidget* parent = nullptr);

    void setAvailableInputs(const QStringList& inputNames);

signals:
    void injectRequested(const QString& inputName, const QString& value);

private:
    QComboBox*   input_selector_ = nullptr;
    QLineEdit*   value_edit_     = nullptr;
    QPushButton* inject_btn_     = nullptr;
};

#endif