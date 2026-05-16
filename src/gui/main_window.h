/**
 * @file    main_window.h
 * @brief   Hlavní okno aplikace – propojuje editor, monitor a všechny panely.
 * @author  Jan
 * @date    2026-05-04
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "interpreter/runtime_state.h"
#include "interpreter/log_entry.h"

using pets::RuntimeState;
using pets::LogEntry;

class PetriNetScene;
class PetriNetView;
class LogPanel;
class InjectionPanel;
class StateOverviewPanel;
class StateReceiver;

/**
 * @brief Hlavní okno aplikace.
 *
 * Spravuje přepínání mezi editačním a monitorovacím režimem,
 * menu, toolbar a dokovací panely.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // === Editor ===
    void onNewNetwork();
    void onOpenNetwork();
    void onSaveNetwork();
    void onSaveNetworkAs();

    void onAddPlace();
    void onAddTransition();
    void onDeleteSelected();

    void onEditVariables();
    void onEditNetworkProperties();

    // === Generování a spuštění ===
    void onGenerateAndRun();
    void onStopInterpreter();

    // === Monitor / Připojení ===
    void onConnectToRunning();
    void onDisconnect();

    // === Pricházející data z interpretu ===
    void onRuntimeStateReceived(const RuntimeState& state);
    void onLogEntryReceived(const LogEntry& entry);

    // === View ===
    void onZoomIn();
    void onZoomOut();
    void onFitToWindow();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupStatusBar();
    void setupConnections();

    void updateWindowTitle();
    void updateActionsForMode();

    // === Hlavní komponenty ===
    PetriNetScene*      scene_ = nullptr;
    PetriNetView*       view_  = nullptr;

    // === Panely ===
    LogPanel*           log_panel_          = nullptr;
    InjectionPanel*     injection_panel_    = nullptr;
    StateOverviewPanel* state_overview_     = nullptr;

    // === IPC ===
    StateReceiver*      receiver_        = nullptr;
    QThread*            receiver_thread_ = nullptr;

    // === Stav ===
    QString current_file_path_;
    bool    is_modified_ = false;
    bool    is_running_  = false;
    bool    is_connected_ = false;
};

#endif // MAIN_WINDOW_H