/**
 * @file    main_window.cpp
 * @brief   Implementace hlavního okna – menu, toolbar, dock panely.
 * @author  Jan
 * @date    2026-05-04
 */

#include "main_window.h"
#include "canvas/petrinet_scene.h"
#include "canvas/petrinet_view.h"
#include "widgets/log_panel.h"
#include "widgets/injection_panel.h"
#include "widgets/state_overview_panel.h"

#include "interpreter/runtime_state.h"
#include "interpreter/log_entry.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QAction>
#include <QKeySequence>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>

using pets::RuntimeState;
using pets::LogEntry;

// ======================================================================
// Konstruktor / Destruktor
// ======================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PetriNet Editor");
    resize(1200, 800);

    // Vytvoření scény a pohledu
    scene_ = new PetriNetScene(this);
    view_  = new PetriNetView(scene_, this);
    setCentralWidget(view_);

    setupMenuBar();
    setupToolBar();
    setupDockWidgets();
    setupStatusBar();
    setupConnections();

    updateWindowTitle();
    statusBar()->showMessage("Připraven", 3000);
}

MainWindow::~MainWindow()
{
    // vše uvolněno přes Qt parent-child
}

// ======================================================================
// Menu Bar
// ======================================================================

void MainWindow::setupMenuBar()
{
    // === File ===
    QMenu* file_menu = menuBar()->addMenu(tr("&File"));

    file_menu->addAction(tr("&New Network"),      this, &MainWindow::onNewNetwork,
                         QKeySequence::New);
    file_menu->addAction(tr("&Open Network..."),   this, &MainWindow::onOpenNetwork,
                         QKeySequence::Open);
    file_menu->addSeparator();
    file_menu->addAction(tr("&Save"),              this, &MainWindow::onSaveNetwork,
                         QKeySequence::Save);
    file_menu->addAction(tr("Save &As..."),        this, &MainWindow::onSaveNetworkAs,
                         QKeySequence(tr("Ctrl+Shift+S")));
    file_menu->addSeparator();
    file_menu->addAction(tr("Export as &PNG..."));
    file_menu->addSeparator();
    file_menu->addAction(tr("&Connect to Running..."), this, &MainWindow::onConnectToRunning);
    file_menu->addAction(tr("&Disconnect"),           this, &MainWindow::onDisconnect);
    file_menu->addSeparator();
    file_menu->addAction(tr("E&xit"), this, &QApplication::quit,
                         QKeySequence(tr("Alt+F4")));

    // === Edit ===
    QMenu* edit_menu = menuBar()->addMenu(tr("&Edit"));

    QAction* undoAction = edit_menu->addAction(tr("&Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);  // Not implemented yet
    QAction* redoAction = edit_menu->addAction(tr("&Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false);  // Not implemented yet
    edit_menu->addSeparator();
    edit_menu->addAction(tr("Add &Place"),      this, &MainWindow::onAddPlace,
                         QKeySequence(tr("P")));
    edit_menu->addAction(tr("Add &Transition"), this, &MainWindow::onAddTransition,
                         QKeySequence(tr("T")));
    edit_menu->addAction(tr("&Delete Selected"), this, &MainWindow::onDeleteSelected,
                         QKeySequence::Delete);
    edit_menu->addSeparator();
    edit_menu->addAction(tr("Edit &Variables..."),         this, &MainWindow::onEditVariables);
    edit_menu->addAction(tr("Edit Network &Properties..."), this, &MainWindow::onEditNetworkProperties);

    // === Run ===
    QMenu* run_menu = menuBar()->addMenu(tr("&Run"));

    run_menu->addAction(tr("Generate && Run"), this, &MainWindow::onGenerateAndRun,
                        QKeySequence(tr("F5")));
    run_menu->addAction(tr("&Stop"),           this, &MainWindow::onStopInterpreter,
                        QKeySequence(tr("Shift+F5")));

    // === View ===
    QMenu* view_menu = menuBar()->addMenu(tr("&View"));

    view_menu->addAction(tr("Zoom &In"),  this, &MainWindow::onZoomIn,
                         QKeySequence::ZoomIn);
    view_menu->addAction(tr("Zoom &Out"), this, &MainWindow::onZoomOut,
                         QKeySequence::ZoomOut);
    view_menu->addAction(tr("&Fit to Window"), this, &MainWindow::onFitToWindow,
                         QKeySequence(tr("Ctrl+0")));

    // === Help ===
    QMenu* help_menu = menuBar()->addMenu(tr("&Help"));

    help_menu->addAction(tr("&About..."), [this]() {
        QMessageBox::about(this, tr("About PetriNet Editor"),
            tr("Nástroj pro vizuální editaci, generování kódu\n"
               "a monitorování běhu interpretovaných Petriho sítí.\n\n"
               "ICP projekt 2026"));
    });
    help_menu->addAction(tr("About &Qt..."), []() {
        QMessageBox::aboutQt(nullptr);
    });
}

// ======================================================================
// Tool Bar
// ======================================================================

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName("MainToolBar");
    toolbar->setMovable(false);

    toolbar->addAction(tr("New"), this, &MainWindow::onNewNetwork);
    toolbar->addAction(tr("Open"), this, &MainWindow::onOpenNetwork);
    toolbar->addAction(tr("Save"), this, &MainWindow::onSaveNetwork);
    toolbar->addSeparator();
    toolbar->addAction(tr("Add Place"),      this, &MainWindow::onAddPlace);
    toolbar->addAction(tr("Add Transition"), this, &MainWindow::onAddTransition);
    toolbar->addAction(tr("Delete"),         this, &MainWindow::onDeleteSelected);
    toolbar->addSeparator();
    toolbar->addAction(tr("Run"),  this, &MainWindow::onGenerateAndRun);
    toolbar->addAction(tr("Stop"), this, &MainWindow::onStopInterpreter);
}

// ======================================================================
// Dock Widgets (panely)
// ======================================================================

void MainWindow::setupDockWidgets()
{
    // --- Spodní panel: Log ---
    log_panel_ = new LogPanel(this);
    QDockWidget* log_dock = new QDockWidget(tr("Log událostí"), this);
    log_dock->setObjectName("LogDock");
    log_dock->setWidget(log_panel_);
    addDockWidget(Qt::BottomDockWidgetArea, log_dock);

    // --- Pravý panel: Injekce ---
    injection_panel_ = new InjectionPanel(this);
    QDockWidget* injection_dock = new QDockWidget(tr("Injektování vstupů"), this);
    injection_dock->setObjectName("InjectionDock");
    injection_dock->setWidget(injection_panel_);
    addDockWidget(Qt::RightDockWidgetArea, injection_dock);

    // --- Pravý panel: Stav ---
    state_overview_ = new StateOverviewPanel(this);
    QDockWidget* state_dock = new QDockWidget(tr("Přehled stavu"), this);
    state_dock->setObjectName("StateDock");
    state_dock->setWidget(state_overview_);
    addDockWidget(Qt::RightDockWidgetArea, state_dock);

    // Seskupení pravých panelů do tabů
    tabifyDockWidget(injection_dock, state_dock);
    injection_dock->raise();  // Injekce jako první
}

// ======================================================================
// Status Bar
// ======================================================================

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage(tr("Režim: Editor"));
}

// ======================================================================
// Connections
// ======================================================================

void MainWindow::setupConnections()
{
    connect(scene_, &PetriNetScene::networkModified, this, [this]() {
        is_modified_ = true;
        updateWindowTitle();
    });
}

// ======================================================================
// Sloty – Editor
// ======================================================================

void MainWindow::onNewNetwork()
{
    scene_->clear();
    current_file_path_.clear();
    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Nová síť vytvořena"), 3000);
}

void MainWindow::onOpenNetwork()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Otevřít síť"), QString(),
        tr("Petriho sítě (*.petri *.txt);;Všechny soubory (*)"));

    if (path.isEmpty()) return;

    // TODO: zavolat Matějův parser pro načtení do scene_
    current_file_path_ = path;
    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Síť načtena"), 3000);
}

void MainWindow::onSaveNetwork()
{
    if (current_file_path_.isEmpty()) {
        onSaveNetworkAs();
        return;
    }

    // TODO: zavolat Matějův serializer pro uložení
    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Síť uložena"), 3000);
}

void MainWindow::onSaveNetworkAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Uložit síť"), QString(),
        tr("Petriho sítě (*.petri *.txt);;Všechny soubory (*)"));

    if (path.isEmpty()) return;

    current_file_path_ = path;
    onSaveNetwork();
}

void MainWindow::onAddPlace()
{
    scene_->addPlace(QPointF(100, 100), scene_->generatePlaceId(), 0);
    statusBar()->showMessage(tr("Místo přidáno"), 2000);
}

void MainWindow::onAddTransition()
{
    scene_->addTransition(QPointF(300, 100), scene_->generateTransitionId());
    statusBar()->showMessage(tr("Přechod přidán"), 2000);
}

void MainWindow::onDeleteSelected()
{
    scene_->removeSelected();
    statusBar()->showMessage(tr("Vybrané prvky smazány"), 2000);
}

void MainWindow::onEditVariables()
{
    // TODO: VariablesDialog
    statusBar()->showMessage(tr("Editace proměnných – TODO"), 2000);
}

void MainWindow::onEditNetworkProperties()
{
    // TODO: NetworkPropertiesDialog
    statusBar()->showMessage(tr("Vlastnosti sítě – TODO"), 2000);
}

// ======================================================================
// Sloty – Run
// ======================================================================

void MainWindow::onGenerateAndRun()
{
    // TODO: propojit s backendem
    is_running_ = true;
    updateActionsForMode();
    statusBar()->showMessage(tr("Interpret spuštěn"), 3000);
}

void MainWindow::onStopInterpreter()
{
    // TODO: propojit s backendem
    is_running_ = false;
    updateActionsForMode();
    statusBar()->showMessage(tr("Interpret zastaven"), 3000);
}

// ======================================================================
// Sloty – Monitor
// ======================================================================

void MainWindow::onConnectToRunning()
{
    // TODO: ConnectionDialog + StateReceiver start
    is_connected_ = true;
    updateActionsForMode();
    statusBar()->showMessage(tr("Připojeno k běžící síti"), 3000);
}

void MainWindow::onDisconnect()
{
    // TODO: StateReceiver stop
    is_connected_ = false;
    updateActionsForMode();
    statusBar()->showMessage(tr("Odpojeno"), 3000);
}

// ======================================================================
// Sloty – Data z interpretu
// ======================================================================

void MainWindow::onRuntimeStateReceived(const RuntimeState& state)
{
    // Aktualizuj scénu – tokeny, zvýraznění
    scene_->updateTokens(state);
    scene_->updateHighlights(state);

    // Aktualizuj panely
    state_overview_->updateState(state);

    statusBar()->showMessage(tr("Stav aktualizován"), 2000);
}

void MainWindow::onLogEntryReceived(const LogEntry& entry)
{
    log_panel_->appendLog(entry);
}

// ======================================================================
// Sloty – View
// ======================================================================

void MainWindow::onZoomIn()  { view_->zoomIn(); }
void MainWindow::onZoomOut() { view_->zoomOut(); }
void MainWindow::onFitToWindow() { view_->fitInView(); }

// ======================================================================
// Pomocné
// ======================================================================

void MainWindow::updateWindowTitle()
{
    QString title = tr("PetriNet Editor");

    if (!current_file_path_.isEmpty()) {
        QFileInfo fi(current_file_path_);
        title = fi.fileName() + " - " + title;
    } else {
        title = tr("[Nová síť]") + " - " + title;
    }

    if (is_modified_) {
        title = "* " + title;
    }

    setWindowTitle(title);
}

void MainWindow::updateActionsForMode()
{
    // TODO: povolit/zakázat akce podle režimu
}
