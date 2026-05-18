/**
 * @file state_overview_panel.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Deklarace panelu přehledu stavu sítě za běhu.
 */
#ifndef STATE_OVERVIEW_PANEL_H
#define STATE_OVERVIEW_PANEL_H

#include <QWidget>
#include <QTableWidget>

namespace pets { struct RuntimeState; }

using pets::RuntimeState;

class StateOverviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StateOverviewPanel(QWidget* parent = nullptr);

    void updateState(const RuntimeState& state);

private:
    QTableWidget* variables_table_ = nullptr;
    QTableWidget* inputs_table_    = nullptr;
    QTableWidget* outputs_table_   = nullptr;
    QTableWidget* timers_table_    = nullptr;
};

#endif