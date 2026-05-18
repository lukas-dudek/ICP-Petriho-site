/**
 * @file state_overview_panel.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
/**
 * @file    state_overview_panel.cpp
 * @brief   Implementace panelu přehledu stavu sítě.
 * @author  Jan
 * @date    2026-05-04
 */

#include "state_overview_panel.h"
#include "interpreter/runtime_state.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTabWidget>

using pets::RuntimeState;

StateOverviewPanel::StateOverviewPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);

    // --- Proměnné ---
    variables_table_ = new QTableWidget(0, 2, this);
    variables_table_->setHorizontalHeaderLabels({"Proměnná", "Hodnota"});
    variables_table_->horizontalHeader()->setStretchLastSection(true);
    variables_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabs->addTab(variables_table_, "Proměnné");

    // --- Vstupy ---
    inputs_table_ = new QTableWidget(0, 2, this);
    inputs_table_->setHorizontalHeaderLabels({"Vstup", "Hodnota"});
    inputs_table_->horizontalHeader()->setStretchLastSection(true);
    inputs_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabs->addTab(inputs_table_, "Vstupy");

    // --- Timeouty ---
    timers_table_ = new QTableWidget(0, 2, this);
    timers_table_->setHorizontalHeaderLabels({"Přechod", "Zbývá [ms]"});
    timers_table_->horizontalHeader()->setStretchLastSection(true);
    timers_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabs->addTab(timers_table_, "Timeouty");

    layout->addWidget(tabs);
}

void StateOverviewPanel::updateState(const RuntimeState& state)
{
    // Proměnné
    variables_table_->setRowCount(static_cast<int>(state.variable_values.size()));
    for (int i = 0; i < static_cast<int>(state.variable_values.size()); ++i) {
        variables_table_->setItem(i, 0, new QTableWidgetItem(
            QString::fromStdString(state.variable_values[i].name)));
        variables_table_->setItem(i, 1, new QTableWidgetItem(
            QString::fromStdString(state.variable_values[i].value)));
    }

    // Vstupy
    inputs_table_->setRowCount(static_cast<int>(state.input_values.size()));
    for (int i = 0; i < static_cast<int>(state.input_values.size()); ++i) {
        inputs_table_->setItem(i, 0, new QTableWidgetItem(
            QString::fromStdString(state.input_values[i].name)));
        inputs_table_->setItem(i, 1, new QTableWidgetItem(
            QString::fromStdString(state.input_values[i].value)));
    }

    // Timeouty
    timers_table_->setRowCount(static_cast<int>(state.pending_timers.size()));
    for (int i = 0; i < static_cast<int>(state.pending_timers.size()); ++i) {
        timers_table_->setItem(i, 0, new QTableWidgetItem(
            QString::fromStdString(state.pending_timers[i].transition_id)));
        timers_table_->setItem(i, 1, new QTableWidgetItem(
            QString::number(state.pending_timers[i].remaining_ms)));
    }
}
