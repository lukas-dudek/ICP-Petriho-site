/**
 * @file    main_window.h
 * @brief   Hlavní okno aplikace – propojuje editor, monitor a všechny panely.
 * @author  Jan
 * @date    2026-05-04
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>

#include "interpreter/runtime_state.h"
#include "interpreter/log_entry.h"

using pets::RuntimeState;
using pets::LogEntry;

namespace pets {
    class PetriNet;
    class Interpreter;
}

class PetriNetScene;
class PetriNetView;
class LogPanel;
class InjectionPanel;
class StateOverviewPanel;

/**
 * @brief Hlavní okno aplikace.
 *
 * Spravuje přepínání mezi editačním a monitorovacím režimem,
 * menu, toolbar a dokovací panely. Drží vlastnictví interpreteru.
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

    // === Injekce ===
    void onInjectInput(const QString& inputName, const QString& value);

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

    /// Sestaví PetriNet model z aktuální scény
    std::unique_ptr<pets::PetriNet> sceneToModel() const;

    /// Načte PetriNet model do scény
    void modelToScene(const pets::PetriNet& net);

    // === Hlavní komponenty ===
    PetriNetScene*      scene_ = nullptr;
    PetriNetView*       view_  = nullptr;

    // === Panely ===
    LogPanel*           log_panel_          = nullptr;
    InjectionPanel*     injection_panel_    = nullptr;
    StateOverviewPanel* state_overview_     = nullptr;

    // === Interpreter (in-process) ===
    std::unique_ptr<pets::Interpreter> interpreter_;

    // === Stav ===
    QString current_file_path_;
    bool    is_modified_ = false;
    bool    is_running_  = false;
    bool    is_connected_ = false;

    // === Síťová metadata (z editace) ===
    QString net_name_ = "NewNet";
    QString net_comment_;
    QStringList net_inputs_;
    QStringList net_outputs_;

    struct VarDef {
        QString type, name, init;
    };
    QList<VarDef> net_variables_;
};

#endif // MAIN_WINDOW_H