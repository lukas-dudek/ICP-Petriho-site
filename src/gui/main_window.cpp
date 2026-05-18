/**
 * @file main_window.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#include "main_window.h"
#include "canvas/petrinet_scene.h"
#include "canvas/petrinet_view.h"
#include "canvas/place_item.h"
#include "canvas/transition_item.h"
#include "canvas/arc_item.h"
#include "widgets/log_panel.h"
#include "widgets/injection_panel.h"
#include "widgets/state_overview_panel.h"

#include "interpreter/runtime_state.h"
#include "interpreter/log_entry.h"
#include "interpreter/interpreter.h"
#include "model/petrinet.h"
#include "model/net_serializer.h"

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
#include <QInputDialog>
#include <QTimer>

using pets::Interpreter;
using pets::PetriNet;
using pets::NetSerializer;

// Konstruktor / Destruktor

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
    if (interpreter_) {
        interpreter_->stop();
    }
}

// Menu Bar

void MainWindow::setupMenuBar()
{
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
    file_menu->addAction(tr("E&xit"), this, &QApplication::quit,
                         QKeySequence(tr("Alt+F4")));

    QMenu* edit_menu = menuBar()->addMenu(tr("&Edit"));

    edit_menu->addAction(tr("Add &Place"),      this, &MainWindow::onAddPlace,
                         QKeySequence(tr("P")));
    edit_menu->addAction(tr("Add &Transition"), this, &MainWindow::onAddTransition,
                         QKeySequence(tr("T")));
    edit_menu->addAction(tr("&Delete Selected"), this, &MainWindow::onDeleteSelected,
                         QKeySequence::Delete);
    edit_menu->addSeparator();
    edit_menu->addAction(tr("Edit &Variables..."),         this, &MainWindow::onEditVariables);
    edit_menu->addAction(tr("Edit Network &Properties..."), this, &MainWindow::onEditNetworkProperties);

    QMenu* run_menu = menuBar()->addMenu(tr("&Run"));

    run_menu->addAction(tr("Generate && &Run"), this, &MainWindow::onGenerateAndRun,
                        QKeySequence(tr("F5")));
    run_menu->addAction(tr("&Stop"),           this, &MainWindow::onStopInterpreter,
                        QKeySequence(tr("Shift+F5")));

    QMenu* view_menu = menuBar()->addMenu(tr("&View"));

    view_menu->addAction(tr("Zoom &In"),  this, &MainWindow::onZoomIn,
                         QKeySequence::ZoomIn);
    view_menu->addAction(tr("Zoom &Out"), this, &MainWindow::onZoomOut,
                         QKeySequence::ZoomOut);
    view_menu->addAction(tr("&Fit to Window"), this, &MainWindow::onFitToWindow,
                         QKeySequence(tr("Ctrl+0")));

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

// Tool Bar

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
    toolbar->addAction(tr("▶ Run"),  this, &MainWindow::onGenerateAndRun);
    toolbar->addAction(tr("■ Stop"), this, &MainWindow::onStopInterpreter);
}

// Dock Widgets (panely)

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
    injection_dock->raise();
}

// Status Bar

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage(tr("Režim: Editor"));
}

// Connections

void MainWindow::setupConnections()
{
    connect(scene_, &PetriNetScene::networkModified, this, [this]() {
        is_modified_ = true;
        updateWindowTitle();
    });

    // Propojení injection panelu
    connect(injection_panel_, &InjectionPanel::injectRequested,
            this, &MainWindow::onInjectInput);
}

// Sloty - Editor

void MainWindow::onNewNetwork()
{
    if (is_running_) {
        onStopInterpreter();
    }
    scene_->clear();
    current_file_path_.clear();
    net_name_ = "NewNet";
    net_comment_.clear();
    net_inputs_.clear();
    net_outputs_.clear();
    net_variables_.clear();
    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Nová síť vytvořena"), 3000);
}

void MainWindow::onOpenNetwork()
{
    if (is_running_) {
        onStopInterpreter();
    }
    
    QString path = QFileDialog::getOpenFileName(
        this, tr("Otevřít síť"), QString(),
        tr("Petriho sítě (*.petri *.txt);;Všechny soubory (*)"));

    if (path.isEmpty()) return;

    PetriNet net;
    std::string error;
    if (!NetSerializer::load(path.toStdString(), net, error)) {
        QMessageBox::critical(this, tr("Chyba"),
            tr("Nelze načíst síť: %1").arg(QString::fromStdString(error)));
        return;
    }

    modelToScene(net);
    current_file_path_ = path;
    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Síť '%1' načtena").arg(QString::fromStdString(net.name)), 3000);
}

void MainWindow::onSaveNetwork()
{
    if (current_file_path_.isEmpty()) {
        onSaveNetworkAs();
        return;
    }

    auto net = sceneToModel();
    std::string error;
    if (!NetSerializer::save(*net, current_file_path_.toStdString(), error)) {
        QMessageBox::critical(this, tr("Chyba"),
            tr("Nelze uložit síť: %1").arg(QString::fromStdString(error)));
        return;
    }

    is_modified_ = false;
    updateWindowTitle();
    statusBar()->showMessage(tr("Síť uložena"), 3000);
}

void MainWindow::onSaveNetworkAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Uložit síť"), QString(),
        tr("Petriho sítě (*.petri);;Textové soubory (*.txt);;Všechny soubory (*)"));

    if (path.isEmpty()) return;

    current_file_path_ = path;
    onSaveNetwork();
}

void MainWindow::onAddPlace()
{
    scene_->addPlace(QPointF(100, 100), scene_->generatePlaceId(), 0);
    statusBar()->showMessage(tr("Místo přidáno (double-click pro editaci)"), 2000);
}

void MainWindow::onAddTransition()
{
    scene_->addTransition(QPointF(300, 100), scene_->generateTransitionId());
    statusBar()->showMessage(tr("Přechod přidán (double-click pro editaci)"), 2000);
}

void MainWindow::onDeleteSelected()
{
    scene_->removeSelected();
    statusBar()->showMessage(tr("Vybrané prvky smazány"), 2000);
}

void MainWindow::onEditVariables()
{
    // Jednoduchý dialog pro vstup proměnných ve formátu "typ jméno = hodnota"
    QString current;
    for (const auto& v : net_variables_) {
        current += v.type + " " + v.name;
        if (!v.init.isEmpty()) current += " = " + v.init;
        current += "\n";
    }

    bool ok;
    QString text = QInputDialog::getMultiLineText(
        this, tr("Editace proměnných"),
        tr("Zadejte proměnné (jedna na řádek, formát: typ jméno = hodnota):"),
        current, &ok);

    if (!ok) return;

    net_variables_.clear();
    for (const QString& line : text.split('\n', Qt::SkipEmptyParts)) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            VarDef v;
            v.type = parts[0];
            v.name = parts[1];
            if (parts.size() >= 4 && parts[2] == "=") {
                v.init = parts[3];
            }
            net_variables_.append(v);
        }
    }

    is_modified_ = true;
    updateWindowTitle();
    statusBar()->showMessage(tr("Proměnné aktualizovány"), 2000);
}

void MainWindow::onEditNetworkProperties()
{
    bool ok;

    // Vstupy
    QString inputText = QInputDialog::getMultiLineText(
        this, tr("Vstupy sítě"),
        tr("Zadejte jména vstupů (jeden na řádek):"),
        net_inputs_.join("\n"), &ok);
    if (!ok) return;
    net_inputs_ = inputText.split('\n', Qt::SkipEmptyParts);
    for (auto& s : net_inputs_) s = s.trimmed();
    net_inputs_.removeAll("");

    // Výstupy
    QString outputText = QInputDialog::getMultiLineText(
        this, tr("Výstupy sítě"),
        tr("Zadejte jména výstupů (jeden na řádek):"),
        net_outputs_.join("\n"), &ok);
    if (!ok) return;
    net_outputs_ = outputText.split('\n', Qt::SkipEmptyParts);
    for (auto& s : net_outputs_) s = s.trimmed();
    net_outputs_.removeAll("");

    // Jméno
    QString name = QInputDialog::getText(
        this, tr("Jméno sítě"), tr("Jméno:"),
        QLineEdit::Normal, net_name_, &ok);
    if (ok && !name.isEmpty()) net_name_ = name;

    // Aktualizace injection panelu
    injection_panel_->setAvailableInputs(net_inputs_);

    is_modified_ = true;
    updateWindowTitle();
    statusBar()->showMessage(tr("Vlastnosti sítě aktualizovány"), 2000);
}

// Sloty - Run

void MainWindow::onGenerateAndRun()
{
    if (is_running_) {
        statusBar()->showMessage(tr("Interpret už běží!"), 2000);
        return;
    }

    auto net = sceneToModel();
    std::string error;
    if (!net->validate(error)) {
        QMessageBox::warning(this, tr("Validace selhala"),
            tr("Síť je nevalidní: %1").arg(QString::fromStdString(error)));
        return;
    }

    interpreter_ = std::make_unique<Interpreter>(*net);

    // Callbacky - musíme posílat přes Qt event loop (z jiného vlákna)
    interpreter_->set_state_callback(
        [this](const RuntimeState& state) {
            QMetaObject::invokeMethod(this, [this, state]() {
                onRuntimeStateReceived(state);
            }, Qt::QueuedConnection);
        });

    interpreter_->set_log_callback(
        [this](const LogEntry& entry) {
            QMetaObject::invokeMethod(this, [this, entry]() {
                onLogEntryReceived(entry);
            }, Qt::QueuedConnection);
        });

    interpreter_->start();
    is_running_ = true;

    // Aktualizuj injection panel
    injection_panel_->setAvailableInputs(net_inputs_);

    updateActionsForMode();
    statusBar()->showMessage(tr("Interpret spuštěn pro síť '%1'").arg(net_name_), 3000);
}

void MainWindow::onStopInterpreter()
{
    if (!is_running_ || !interpreter_) {
        statusBar()->showMessage(tr("Interpret neběží"), 2000);
        return;
    }

    interpreter_->stop();
    interpreter_.reset();
    is_running_ = false;

    updateActionsForMode();
    statusBar()->showMessage(tr("Interpret zastaven"), 3000);
}

// Sloty - Data z interpretu

void MainWindow::onRuntimeStateReceived(const RuntimeState& state)
{
    // Aktualizuj scénu - tokeny, zvýraznění
    scene_->updateTokens(state);
    scene_->updateHighlights(state);

    // Aktualizuj panely
    state_overview_->updateState(state);
}

void MainWindow::onLogEntryReceived(const LogEntry& entry)
{
    log_panel_->appendLog(entry);
}

// Sloty - Injekce

void MainWindow::onInjectInput(const QString& inputName, const QString& value)
{
    if (interpreter_ && is_running_) {
        interpreter_->inject_input(inputName.toStdString(), value.toStdString());
        statusBar()->showMessage(
            tr("Vstup '%1' = '%2' injektován").arg(inputName, value), 2000);
    } else {
        statusBar()->showMessage(tr("Interpret neběží - nelze injektovat"), 2000);
    }
}

// Sloty - View

void MainWindow::onZoomIn()  { view_->zoomIn(); }
void MainWindow::onZoomOut() { view_->zoomOut(); }
void MainWindow::onFitToWindow() { view_->fitInView(); }

// Konverze scéna ↔ model

void MainWindow::parsePlacesToModel(pets::PetriNet* net) const {
    for (auto* item : scene_->items()) {
        if (item->type() == PlaceItem::Type) {
            auto* pi = static_cast<PlaceItem*>(item);
            std::optional<std::string> action;
            if (!pi->placeAction().isEmpty()) {
                action = pi->placeAction().toStdString();
            }
            net->places.emplace_back(
                pi->placeId().toStdString(),
                pi->initialTokens(),
                action);
        }
    }
}

void MainWindow::parseTransitionsToModel(pets::PetriNet* net) const {
    for (auto* item : scene_->items()) {
        if (item->type() == TransitionItem::Type) {
            auto* ti = static_cast<TransitionItem*>(item);
            pets::Transition t(ti->transitionId().toStdString());
            t.priority = ti->priority();

            if (!ti->inputEvent().isEmpty())
                t.input_event_name = ti->inputEvent().toStdString();
            if (!ti->guardExpression().isEmpty())
                t.guard_expression = ti->guardExpression().toStdString();
            if (ti->hasDelay())
                t.delay_ms = ti->delayMs();
            if (!ti->actionCode().isEmpty())
                t.action_code = ti->actionCode().toStdString();

            for (auto* arcItem : scene_->items()) {
                if (arcItem->type() == ArcItem::Type) {
                    auto* ai = static_cast<ArcItem*>(arcItem);
                    if (ai->transitionItem() == ti) {
                        std::string pid = ai->placeItem()->placeId().toStdString();
                        if (ai->isInputArc()) {
                            t.input_arcs.push_back({pid, ai->weight()});
                        } else {
                            t.output_arcs.push_back({pid, ai->weight()});
                        }
                    }
                }
            }

            net->transitions.push_back(std::move(t));
        }
    }
}

std::unique_ptr<PetriNet> MainWindow::sceneToModel() const
{
    auto net = std::make_unique<PetriNet>();
    net->name = net_name_.toStdString();
    net->comment = net_comment_.toStdString();

    for (const auto& name : net_inputs_) {
        net->inputs.emplace_back(name.toStdString());
    }

    for (const auto& name : net_outputs_) {
        net->outputs.emplace_back(name.toStdString());
    }

    for (const auto& v : net_variables_) {
        net->variables.emplace_back(
            v.type.toStdString(), v.name.toStdString(), v.init.toStdString());
    }

    parsePlacesToModel(net.get());
    parseTransitionsToModel(net.get());

    return net;
}

void MainWindow::parsePlacesToScene(const PetriNet& net, QMap<QString, PlaceItem*>& placeMap) {
    int placeIdx = 0;
    for (const auto& p : net.places) {
        int x = 150 + (placeIdx % 4) * 200;
        int y = 100 + (placeIdx / 4) * 200;
        auto* pi = scene_->addPlace(QPointF(x, y),
            QString::fromStdString(p.id), p.initial_tokens);
        if (p.place_action.has_value()) {
            pi->setPlaceAction(QString::fromStdString(p.place_action.value()));
        }
        placeMap[QString::fromStdString(p.id)] = pi;
        placeIdx++;
    }
}

void MainWindow::parseTransitionsToScene(const PetriNet& net, const QMap<QString, PlaceItem*>& placeMap) {
    int transIdx = 0;
    for (const auto& t : net.transitions) {
        int x = 350 + (transIdx % 4) * 200;
        int y = 200 + (transIdx / 4) * 200;
        auto* ti = scene_->addTransition(QPointF(x, y),
            QString::fromStdString(t.id));

        if (t.input_event_name.has_value())
            ti->setInputEvent(QString::fromStdString(t.input_event_name.value()));
        if (t.guard_expression.has_value())
            ti->setGuardExpression(QString::fromStdString(t.guard_expression.value()));
        if (t.delay_ms.has_value())
            ti->setDelayMs(t.delay_ms.value());
        if (t.action_code.has_value())
            ti->setActionCode(QString::fromStdString(t.action_code.value()));
        ti->setPriority(t.priority);

        for (const auto& arc : t.input_arcs) {
            auto* pi = placeMap.value(QString::fromStdString(arc.place_id), nullptr);
            if (pi) {
                scene_->addArc(pi, ti, arc.weight);
            }
        }

        for (const auto& arc : t.output_arcs) {
            auto* pi = placeMap.value(QString::fromStdString(arc.place_id), nullptr);
            if (pi) {
                scene_->addArc(ti, pi, arc.weight);
            }
        }

        transIdx++;
    }
}

void MainWindow::modelToScene(const PetriNet& net)
{
    scene_->clear();

    net_name_ = QString::fromStdString(net.name);
    net_comment_ = QString::fromStdString(net.comment);

    net_inputs_.clear();
    for (const auto& i : net.inputs) {
        net_inputs_.append(QString::fromStdString(i.name));
    }

    net_outputs_.clear();
    for (const auto& o : net.outputs) {
        net_outputs_.append(QString::fromStdString(o.name));
    }

    net_variables_.clear();
    for (const auto& v : net.variables) {
        net_variables_.append({
            QString::fromStdString(v.type),
            QString::fromStdString(v.name),
            QString::fromStdString(v.initial_value)
        });
    }

    injection_panel_->setAvailableInputs(net_inputs_);

    QMap<QString, PlaceItem*> placeMap;
    parsePlacesToScene(net, placeMap);
    parseTransitionsToScene(net, placeMap);
}

// Pomocné

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

    if (is_running_) {
        title += " [RUNNING]";
    }

    setWindowTitle(title);
}

void MainWindow::updateActionsForMode()
{
    updateWindowTitle();
    // Při běhu změníme status bar
    if (is_running_) {
        statusBar()->showMessage(tr("Režim: Monitor (interpret běží)"));
    } else {
        statusBar()->showMessage(tr("Režim: Editor"));
    }
}
