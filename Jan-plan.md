┌────────────────────────────────────────────────────────────────────┐
│                     Frontend GUI (Jan)                             │
├──────────────────┬──────────────────────┬─────────────────────────┤
│  Editor Canvas   │  Property Dialogs    │  Runtime Monitor        │
│  (vizuální       │  (editace míst,      │  (živé zobrazení stavu) │
│   kreslení)      │   přechodů, vstupů)  │                         │
├──────────────────┼──────────────────────┼─────────────────────────┤
│  PlaceItem       │  PlaceDialog         │  TokenOverlay           │
│  TransitionItem  │  TransitionDialog    │  HighlightManager       │
│  ArcItem         │  VariableDialog      │  LogPanel               │
│  PetriNetScene   │  InputOutputDialog   │  InjectionPanel         │
│  PetriNetView    │  ConditionEditor     │  ConnectionDialog       │
│  MainWindow      │  CodeEditor          │  StateReceiver          │
└──────────────────┴──────────────────────┴─────────────────────────┘
│               IPC Listener / Sender ( UDP klient )                 │
│               ← přijímá RuntimeState, posílá vstupy                │
└────────────────────────────────────────────────────────────────────┘
```

---

## 1. Konceptuálny prehľad frontendu

Frontend je postavený na **Qt 5.5+** a striktne dodržiava oddelenie modelu od GUI. GUI nikdy nemanipuluje priamo s dátovými štruktúrami backendu; všetko číta a zapisuje cez rozhranie, ktoré poskytuje Matějov parser/textový formát a Lukášov IPC bridge.

GUI má **tri hlavné módy**:

1. **Editor mód** – vytváranie a editácia Petriho siete (offline).
2. **Monitor mód** – pripojenie k bežiacemu interpretu a živé zobrazenie stavu (online).
3. **Hybridný mód** – pri editácii je možné sieť spustiť, sledovať a prípadne upravovať (ak to architektúra dovolí).

---

## 2. Hlavné triedy a ich zodpovednosť

### 2.1 Triedy pre Editor Canvas

```cpp
/**
 * @brief Grafická reprezentácia miesta (Place) na scéne
 * @author Jan ...
 * 
 * Vykresľuje sa ako kruh/elipsa s názvom a počtom tokenov.
 * Počas editácie je presúvateľný.
 */
class PlaceItem : public QGraphicsEllipseItem {
public:
    explicit PlaceItem(const QString& id, int initialTokens = 0);
    
    QString placeId() const;
    void setTokens(int count);                  // aktualizuje zobrazený počet tokenov
    void setHighlight(HighlightType type);      // zvýraznenie (enabled, firing, ...)
    
    // Spätná synchronizácia s dátovým modelom
    void updateFromModel(const Place& place);
    Place toModel() const;
    
    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    
private:
    QString id_;
    int token_count_ = 0;
    QGraphicsTextItem* label_;                  // meno miesta
    QGraphicsTextItem* token_label_;            // "2 tokens" pod menom
};
```

```cpp
/**
 * @brief Grafická reprezentácia prechodu (Transition) na scéne
 * @author Jan ...
 * 
 * Vykresľuje sa ako obdĺžnik. Pri zvýraznení mení farbu:
 *  - sivý = nie je enabled
 *  - zelený = enabled (okamžitý)
 *  - oranžový = naplánovaný timeout
 *  - modrý = práve odpálený
 */
class TransitionItem : public QGraphicsRectItem {
public:
    explicit TransitionItem(const QString& id);
    
    QString transitionId() const;
    void setHighlight(HighlightType type);
    
    void updateFromModel(const Transition& transition);
    Transition toModel() const;
    
    enum { Type = QGraphicsItem::UserType + 2 };
    int type() const override { return Type; }
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    
private:
    QString id_;
    HighlightType highlight_ = HighlightType::Normal;
    QGraphicsTextItem* label_;
};
```

```cpp
/**
 * @brief Grafická reprezentácia orientovanej hrany medzi Place ↔ Transition
 * @author Jan ...
 * 
 * Kreslí sa ako Bézierova krivka alebo lomená čiara s šípkou na konci.
 * Na strede hrany sa zobrazuje váha (weight >= 1).
 * Podporuje oba smery: Place → Transition (vstupná) aj Transition → Place (výstupná).
 */
class ArcItem : public QGraphicsPathItem {
public:
    ArcItem(PlaceItem* fromPlace, TransitionItem* toTransition, int weight = 1);
    ArcItem(TransitionItem* fromTransition, PlaceItem* toPlace, int weight = 1);
    
    // Zistí typ hrany
    bool isInputArc() const;    // Place → Transition
    bool isOutputArc() const;   // Transition → Place
    
    PlaceItem* placeItem() const;
    TransitionItem* transitionItem() const;
    int weight() const;
    void setWeight(int w);
    
    void updatePath();          // prepočíta krivku pri presune prvkov
    
    enum { Type = QGraphicsItem::UserType + 3 };
    int type() const override { return Type; }
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;
    
private:
    QGraphicsItem* from_;
    QGraphicsItem* to_;
    int weight_;
    QGraphicsTextItem* weight_label_;
    bool is_input_;             // true = Place → Transition
};
```

```cpp
/**
 * @brief Scéna spravujúca všetky grafické prvky Petriho siete
 * @author Jan ...
 * 
 * Vlastní zoznamy PlaceItem, TransitionItem a ArcItem.
 * Poskytuje metódy na pridávanie/mazanie prvkov a ich spätnú konverziu
 * do/z dátového modelu (PetriNet).
 */
class PetriNetScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit PetriNetScene(QObject* parent = nullptr);
    
    // Editor operácie
    PlaceItem* addPlace(const QPointF& pos, const QString& id, int initialTokens = 0);
    TransitionItem* addTransition(const QPointF& pos, const QString& id);
    ArcItem* addArc(QGraphicsItem* from, QGraphicsItem* to, int weight = 1);
    void removeItem(QGraphicsItem* item);
    
    // Konverzia do/z dátového modelu
    void loadFromPetriNet(const PetriNet& net);   // import z backend modelu
    PetriNet toPetriNet() const;                  // export do backend modelu
    
    // Vyhľadávanie
    PlaceItem* findPlace(const QString& id) const;
    TransitionItem* findTransition(const QString& id) const;
    QList<ArcItem*> arcsForPlace(PlaceItem* place) const;
    QList<ArcItem*> arcsForTransition(TransitionItem* transition) const;
    
    // Zvýraznenie v runtime
    void clearHighlights();
    void highlightEnabled(const QStringList& transitionIds);
    void highlightPending(const QMap<QString, int64_t>& timers); // id → remaining_ms
    void updateTokens(const QMap<QString, int>& placeTokens);
    
signals:
    void itemAdded(QGraphicsItem* item);
    void itemRemoved(const QString& id);
    void networkModified();     // signalizuje zmenu pre undo/redo a save
    
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    
private:
    bool drawing_arc_ = false;
    QGraphicsItem* arc_start_item_ = nullptr;
    QGraphicsLineItem* temp_line_ = nullptr;
    
    int place_counter_ = 1;     // pre auto-generovanie mien P1, P2, ...
    int transition_counter_ = 1;
};
```

```cpp
/**
 * @brief QGraphicsView s podporou zoomu, pan a režimu editácia/monitor
 * @author Jan ...
 */
class PetriNetView : public QGraphicsView {
    Q_OBJECT
public:
    explicit PetriNetView(PetriNetScene* scene, QWidget* parent = nullptr);
    
    void setEditMode(bool editing);     // true = editor, false = monitor (read-only)
    void setMonitorMode(bool monitoring);
    
    // Zoom & Pan
    void zoomIn();
    void zoomOut();
    void fitInView();
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override; // mriežka
    
private:
    bool edit_mode_ = true;
    qreal current_zoom_ = 1.0;
};
```

### 2.2 Triedy pre Property Dialogs

```cpp
/**
 * @brief Dialóg pre editáciu vlastností miesta
 * @author Jan ...
 */
class PlaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlaceDialog(PlaceItem* item, QWidget* parent = nullptr);
    
    QString placeId() const;
    int initialTokens() const;
    QString placeAction() const;            // C++ kód akcie miesta (voliteľné)
    
private:
    QLineEdit* id_edit_;
    QSpinBox* tokens_spin_;
    CodeEditor* action_editor_;             // zvýrazňovač syntaxe pre C++
    QPushButton* validate_btn_;             // validácia kódu (voliteľné)
    PlaceItem* item_;
};
```

```cpp
/**
 * @brief Dialóg pre editáciu vlastností prechodu
 * @author Jan ...
 * 
 * Umožňuje editovať:
 * - vstupné a výstupné hrany (s váhami) v tabuľkovom zobrazení
 * - udalostnú podmienku (input_event_name)
 * - strážnu podmienku (C++ bool výraz)
 * - zpoždění (@ delay_in_ms)
 * - akciu prechodu (C++ kód)
 */
class TransitionDialog : public QDialog {
    Q_OBJECT
public:
    explicit TransitionDialog(TransitionItem* item,
                               const QList<PlaceItem*>& allPlaces,
                               QWidget* parent = nullptr);
    
    QString transitionId() const;
    
    // Hrany
    struct ArcInfo {
        QString placeId;
        int weight;
    };
    QList<ArcInfo> inputArcs() const;
    QList<ArcInfo> outputArcs() const;
    
    // Podmienka odpálenia (3 časti)
    QString inputEventName() const;         // napr. "in"
    QString guardExpression() const;        // C++ bool výraz
    int delayMs() const;                    // -1 ak nie je zadané
    
    QString actionCode() const;             // C++ kód akcie
    
private:
    QLineEdit* id_edit_;
    
    // Vstupné a výstupné hrany – QTableWidget
    QTableWidget* input_arcs_table_;        // stĺpce: Place, Weight
    QTableWidget* output_arcs_table_;
    QComboBox* place_combo_;                // kombo pre výber existujúceho miesta
    QSpinBox* weight_spin_;
    QPushButton* add_input_btn_;
    QPushButton* add_output_btn_;
    QPushButton* remove_arc_btn_;
    
    // Podmienky
    QComboBox* event_combo_;                // predvyplnené zoznamom vstupov
    QCheckBox* no_event_check_;             // "žiadna udalosť"
    CodeEditor* guard_editor_;
    QSpinBox* delay_spin_;
    QCheckBox* delay_enabled_check_;
    
    // Akcia
    CodeEditor* action_editor_;
    
    TransitionItem* item_;
};
```

```cpp
/**
 * @brief Jednoduchý editor so zvýrazňovaním syntaxe C/C++
 * @author Jan ...
 * 
 * Použitý v PlaceDialog, TransitionDialog, ConditionEditor, atď.
 * Založený na QPlainTextEdit + QSyntaxHighlighter.
 */
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget* parent = nullptr);
    
    void setPlaceholderText(const QString& text);
    QString code() const;
    void setCode(const QString& code);
    
    // Validácia zátvoriek, atď.
    bool isBalanced() const;
    
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;  // čísla riadkov
    
private:
    class SyntaxHighlighter : public QSyntaxHighlighter {
    public:
        explicit SyntaxHighlighter(QTextDocument* parent);
    protected:
        void highlightBlock(const QString& text) override;
    };
    
    SyntaxHighlighter* highlighter_;
    QWidget* line_number_area_;
};
```

```cpp
/**
 * @brief Dialóg pre správu vstupov, výstupov a premenných siete
 * @author Jan ...
 */
class VariablesDialog : public QDialog {
    Q_OBJECT
public:
    explicit VariablesDialog(QWidget* parent = nullptr);
    
    struct InputInfo {
        QString name;
    };
    struct OutputInfo {
        QString name;
    };
    struct VariableInfo {
        QString type;       // "int", "float", "const char*", ...
        QString name;
        QString initialValue;
    };
    
    QList<InputInfo> inputs() const;
    QList<OutputInfo> outputs() const;
    QList<VariableInfo> variables() const;
    
    void setInputs(const QList<InputInfo>& inputs);
    void setOutputs(const QList<OutputInfo>& outputs);
    void setVariables(const QList<VariableInfo>& variables);
    
private:
    QTableWidget* inputs_table_;
    QTableWidget* outputs_table_;
    QTableWidget* variables_table_;         // stĺpce: Type, Name, Initial Value
    QComboBox* type_combo_;
};
```

### 2.3 Triedy pre runtime monitor

```cpp
/**
 * @brief Prijíma stav siete od bežiaceho interpretu cez IPC (UDP)
 * @author Jan ...
 * 
 * Beží v samostatnom vlákne, prijaté dáta emit ako Qt signály.
 * Tým neblokuje hlavné GUI vlákno.
 */
class StateReceiver : public QObject {
    Q_OBJECT
public:
    explicit StateReceiver(quint16 port, QObject* parent = nullptr);
    ~StateReceiver();
    
    void start();
    void stop();
    
    /// Pošle vstupnú udalosť do bežiaceho interpretu
    void injectInput(const QString& inputName, const QString& value);
    
signals:
    void runtimeStateReceived(const RuntimeState& state);
    void logEntryReceived(const LogEntry& entry);
    void connectionEstablished(const QString& networkName);
    void connectionLost();
    void errorOccurred(const QString& errorMsg);
    
private slots:
    void readPendingDatagrams();
    
private:
    QUdpSocket* socket_;
    quint16 port_;
    QHostAddress interpreter_address_;
    bool running_ = false;
};
```

```cpp
/**
 * @brief Panel zobrazujúci log udalostí v reálnom čase
 * @author Jan ...
 */
class LogPanel : public QWidget {
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);
    
    void appendLog(const LogEntry& entry);
    void clear();
    
    /// Export logu do súboru
    void exportToFile(const QString& path);
    
private:
    QTableWidget* log_table_;               // stĺpce: Čas, Typ udalosti, Popis
    QPushButton* clear_btn_;
    QPushButton* export_btn_;
    bool auto_scroll_ = true;
};
```

```cpp
/**
 * @brief Panel pre injektovanie vstupných hodnôt do bežiacej siete
 * @author Jan ...
 */
class InjectionPanel : public QWidget {
    Q_OBJECT
public:
    explicit InjectionPanel(QWidget* parent = nullptr);
    
    /// Nastaví zoznam dostupných vstupov (na základe načítanej siete)
    void setAvailableInputs(const QStringList& inputNames);
    
signals:
    /// GUI požiadalo o injekciu hodnoty do vstupu
    void injectRequested(const QString& inputName, const QString& value);
    
private:
    QComboBox* input_selector_;
    QLineEdit* value_edit_;
    QPushButton* inject_btn_;
    QLabel* last_value_label_;              // zobrazuje poslednú známu hodnotu
};
```

```cpp
/**
 * @brief Panel s prehľadom aktuálneho stavu premenných, vstupov a výstupov
 * @author Jan ...
 */
class StateOverviewPanel : public QWidget {
    Q_OBJECT
public:
    explicit StateOverviewPanel(QWidget* parent = nullptr);
    
    void updateState(const RuntimeState& state);
    
private:
    QTableWidget* variables_table_;         // Premenné: meno → hodnota
    QTableWidget* inputs_table_;            // Vstupy: meno → posledná hodnota
    QTableWidget* outputs_table_;           // Výstupy: meno → posledná hodnota
    QTableWidget* timers_table_;            // Timeouty: prechod → zostávajúci čas
};
```

### 2.4 Hlavné okno aplikácie

```cpp
/**
 * @brief Hlavné okno aplikácie – prepája editor, monitor a všetky panely
 * @author Jan ...
 */
class MainWindow : public QMainWindow {
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
    
    void onEditPlace(PlaceItem* place);
    void onEditTransition(TransitionItem* transition);
    void onEditVariables();
    
    // === Generovanie a spustenie ===
    void onGenerateAndRun();
    void onStopInterpreter();
    
    // === Monitor / Pripojenie ===
    void onConnectToRunning();
    void onDisconnect();
    
    // === Prichádzajúce dáta z interpretu ===
    void onRuntimeStateReceived(const RuntimeState& state);
    void onLogEntryReceived(const LogEntry& entry);
    
private:
    void setupMenuBar();
    void setupToolBar();
    void setupDockWidgets();
    void setupStatusBar();
    
    void updateWindowTitle();
    void updateActionsForEditorMode();
    void updateActionsForMonitorMode();
    
    // Hlavné komponenty
    PetriNetScene* scene_;
    PetriNetView* view_;
    
    // Panely
    LogPanel* log_panel_;
    InjectionPanel* injection_panel_;
    StateOverviewPanel* state_overview_;
    
    // IPC
    StateReceiver* receiver_;
    QThread* receiver_thread_;
    
    // Stav
    QString current_file_path_;
    bool is_modified_ = false;
    bool is_running_ = false;
};
```

---

## 3. Návrhové vzory (Design Patterns)

| Vzor | Kde a prečo |
|------|-------------|
| **MVC / Model-View-Controller** | Hlavná architektúra: dáta (PetriNet) sú oddelené od grafickej reprezentácie (Scene) aj od logiky ovládania (MainWindow). |
| **Observer** | GUI sa pripája ako observer na signály z `StateReceiver`‑a; reaguje na zmeny RuntimeState bez pollovania. |
| **Command** | Každá editorová akcia (pridanie, zmazanie, zmena vlastností) je zapuzdrená ako command pre undo/redo stack (`QUndoCommand`). |
| **State** | `MainWindow` má explicitné stavy: `EditorMode`, `MonitorMode`, `DisconnectedMode` – každý povoľuje iné sady akcií v menu/toolbare. |
| **Composite** | `QGraphicsScene` prirodzene umožňuje hierarchické zoskupovanie položiek (PlaceItem + jeho token_label + highlight overlay). |
| **Template Method** | `PlaceDialog` a `TransitionDialog` dedia spoločné chovanie z abstraktného `PropertyDialog` (validácia, potvrdenie, obnovenie). |
| **Singleton** | `Settings` trieda pre globálne konfigurácie (naposledy otvorené súbory, veľkosť okna, farby zvýraznenia). |

---

## 4. Dátové štruktúry pre výmenu s backendom

Tieto štruktúry sú **zdieľané** s backendom (definované v spoločnom hlavičkovom súbore, ktorý spravuje Matěj). GUI ich používa iba na čítanie.

```cpp
/**
 * @brief Stav siete v jednom okamihu – posielaný z interpretu do GUI
 * @author Matěj / Jan
 */
struct RuntimeState {
    // Marking
    QMap<QString, int> placeTokens;         // názov miesta → počet tokenov
    
    // Povolené prechody
    QStringList enabledTransitions;
    
    // Naplánované timeouty
    struct PendingTimer {
        QString transitionId;
        int64_t remainingMs;                // koľko ms ešte zostáva do odpálenia
    };
    QList<PendingTimer> pendingTimers;
    
    // Posledné známe hodnoty
    QMap<QString, QString> inputValues;     // názov vstupu → hodnota
    QMap<QString, QString> outputValues;    // názov výstupu → hodnota
    QMap<QString, QString> variableValues;  // názov premennej → hodnota
    
    // Parsovanie z JSON (ktorý posiela backend cez UDP)
    static RuntimeState fromJson(const QByteArray& json);
    QByteArray toJson() const;
};

/**
 * @brief Jeden záznam v logu udalostí
 */
struct LogEntry {
    enum class EventType {
        EXTERNAL_INPUT,
        TRANSITION_FIRED,
        TIMER_SCHEDULED,
        TIMER_EXPIRED,
        TIMER_IGNORED,
        PLACE_ACTION,
        VARIABLE_CHANGED,
        OUTPUT_GENERATED,
        NETWORK_STARTED,
        NETWORK_STOPPED
    };
    
    qint64 timestampMs;                     // čas od štartu (now())
    EventType type;
    QString description;                    // ľudsky čitateľný popis
    
    // Voliteľné detaily
    QString transitionId;                   // pre TRANSITION_FIRED, TIMER_*
    QMap<QString, int> tokenChanges;        // miesto → delta tokenov
    QMap<QString, QString> variableChanges;// premenná → nová hodnota
    QString outputGenerated;                // názov výstupu + hodnota
};
```

---

## 5. Menu a toolbar – štruktúra

```
┌─ File ─────────────────────────────┐
│  New Network          Ctrl+N       │
│  Open Network...      Ctrl+O       │
│  Save                 Ctrl+S       │
│  Save As...           Ctrl+Shift+S │
│  ─────────────────────────         │
│  Export as PNG...                  │
│  ─────────────────────────         │
│  Connect to Running...             │
│  Disconnect                        │
│  ─────────────────────────         │
│  Exit                  Alt+F4      │
└────────────────────────────────────┘

┌─ Edit ─────────────────────────────┐
│  Undo                 Ctrl+Z       │
│  Redo                 Ctrl+Y       │
│  ─────────────────────────         │
│  Add Place            P            │
│  Add Transition       T            │
│  Delete Selected      Del          │
│  ─────────────────────────         │
│  Edit Variables...                 │
│  Edit Network Properties...        │
└────────────────────────────────────┘

┌─ Run ──────────────────────────────┐
│  Generate & Run       F5           │
│  Stop                 Shift+F5     │
│  ─────────────────────────         │
│  Inject Input...      Ctrl+I       │
└────────────────────────────────────┘

┌─ View ─────────────────────────────┐
│  Zoom In              Ctrl++       │
│  Zoom Out             Ctrl+-       │
│  Fit to Window        Ctrl+0       │
│  ─────────────────────────         │
│  ✓ Show Grid                       │
│  ✓ Show Token Labels               │
│  ─────────────────────────         │
│  Log Panel                         │
│  Injection Panel                   │
│  State Overview Panel              │
└────────────────────────────────────┘

┌─ Help ─────────────────────────────┐
│  About...                          │
│  About Qt...                       │
└────────────────────────────────────┘
```

---

## 6. Sekvenčný diagram – Spustenie a monitorovanie

```
GUI (MainWindow)          IPC (StateReceiver)        Backend (Interpreter)
 │                              │                          │
 │  onGenerateAndRun()          │                          │
 │──┼───────────────────────────┼──────────────────────────┤
 │  │  vygeneruj a spusti       │                          │
 │  │  interpret (cez Mateje)   │                          │
 │  │                           │                          │
 │  │  start() receiver         │                          │
 │  │──────────────────────────>│                          │
 │  │                           │  UDP: REGISTER           │
 │  │                           │─────────────────────────>│
 │  │                           │                          │
 │  │                           │  UDP: STATE_UPDATE       │
 │  │                           │<─────────────────────────│
 │  │  runtimeStateReceived()   │                          │
 │  │<──────────────────────────│                          │
 │  │  (update scene, panels)   │                          │
 │  │                           │                          │
 │  │  [user injects input]     │                          │
 │  │  injectInput("in", "1")   │                          │
 │  │──────────────────────────>│                          │
 │  │                           │  UDP: INPUT in=1         │
 │  │                           │─────────────────────────>│
 │  │                           │                          │
 │  │                           │  UDP: STATE_UPDATE       │
 │  │                           │<─────────────────────────│
 │  │  runtimeStateReceived()   │                          │
 │  │<──────────────────────────│                          │
 │  │  (marking sa zmenil)      │                          │
```

---

## 7. Adresárová štruktúra (frontend časť)

```
src/
├── gui/
│   ├── CMakeLists.txt               (alebo pripojenie do hlavného Makefile)
│   ├── main_gui.cpp                 (vstupný bod pre GUI)
│   │
│   ├── canvas/
│   │   ├── place_item.h / place_item.cpp
│   │   ├── transition_item.h / transition_item.cpp
│   │   ├── arc_item.h / arc_item.cpp
│   │   ├── petrinet_scene.h / petrinet_scene.cpp
│   │   ├── petrinet_view.h / petrinet_view.cpp
│   │   └── highlight_manager.h / highlight_manager.cpp
│   │
│   ├── dialogs/
│   │   ├── place_dialog.h / place_dialog.cpp
│   │   ├── transition_dialog.h / transition_dialog.cpp
│   │   ├── variables_dialog.h / variables_dialog.cpp
│   │   ├── connection_dialog.h / connection_dialog.cpp
│   │   └── network_properties_dialog.h / network_properties_dialog.cpp
│   │
│   ├── widgets/
│   │   ├── code_editor.h / code_editor.cpp
│   │   ├── log_panel.h / log_panel.cpp
│   │   ├── injection_panel.h / injection_panel.cpp
│   │   └── state_overview_panel.h / state_overview_panel.cpp
│   │
│   ├── ipc/
│   │   ├── state_receiver.h / state_receiver.cpp
│   │   └── shared_types.h             (RuntimeState, LogEntry – zdieľané)
│   │
│   ├── commands/
│   │   ├── add_place_command.h / add_place_command.cpp
│   │   ├── remove_item_command.h / remove_item_command.cpp
│   │   └── move_item_command.h / move_item_command.cpp
│   │
│   ├── main_window.h / main_window.cpp
│   └── resources/
│       ├── icons/                     (ikony pre toolbar, kurzory)
│       ├── app.qrc                    (Qt resource file)
│       └── style.qss                  (Qt stylesheet)
```

---

## 8. Testovací plán pre GUI (čo sa dá testovať bez backendu)

```cpp
// test_place_item.cpp
#include <QApplication>
#include <cassert>
#include "gui/canvas/place_item.h"

void test_place_initial_state() {
    PlaceItem item("P1", 3);
    
    assert(item.placeId() == "P1");
    assert(item.token_count_ == 3);        // interný stav
    
    // Zmena počtu tokenov
    item.setTokens(0);
    assert(item.token_count_ == 0);
    
    std::cout << "✅ test_place_initial_state passed\n";
}

void test_arc_creation() {
    PlaceItem place("P1", 1);
    TransitionItem trans("T1");
    
    ArcItem inputArc(&place, &trans, 2);
    assert(inputArc.isInputArc());
    assert(inputArc.weight() == 2);
    assert(inputArc.placeItem() == &place);
    assert(inputArc.transitionItem() == &trans);
    
    ArcItem outputArc(&trans, &place, 1);
    assert(outputArc.isOutputArc());
    assert(outputArc.weight() == 1);
    
    std::cout << "✅ test_arc_creation passed\n";
}

void test_scene_add_remove() {
    PetriNetScene scene;
    
    // Pridanie miesta
    auto* p = scene.addPlace(QPointF(0, 0), "IDLE", 1);
    assert(p != nullptr);
    assert(scene.findPlace("IDLE") == p);
    assert(scene.items().count() >= 1);
    
    // Pridanie prechodu
    auto* t = scene.addTransition(QPointF(100, 0), "T_on");
    assert(t != nullptr);
    assert(scene.findTransition("T_on") == t);
    
    // Pridanie hrany
    auto* a = scene.addArc(p, t, 1);
    assert(a != nullptr);
    assert(scene.arcsForPlace(p).contains(a));
    assert(scene.arcsForTransition(t).contains(a));
    
    // Odstránenie
    scene.removeItem(p);
    assert(scene.findPlace("IDLE") == nullptr);
    
    std::cout << "✅ test_scene_add_remove passed\n";
}
```

---

## 9. Časový plán (Jan – 7 dní)

| Deň | Úloha | Výstup |
|-----|-------|--------|
| **1** | Založenie Qt projektu, základ `MainWindow`, `PetriNetScene`, `PetriNetView`. Prvý prototyp – kreslenie pevne daných Place+Transition na scénu. | Spustiteľné GUI so scénou. |
| **2** | Implementácia `PlaceItem`, `TransitionItem`, `ArcItem` – kreslenie, presúvanie, mazanie. Menu a toolbar pre editor. | Funkčný editor – pridávanie/mazanie prvkov. |
| **3** | Property dialogy: `PlaceDialog`, `TransitionDialog` (bez code editoru). `VariablesDialog`. | Editácia vlastností cez dialógy. |
| **4** | `CodeEditor` so zvýrazňovaním syntaxe. Integrácia do dialógov. Undo/redo commandy. Ukladanie/načítanie súborov cez Matějov parser. | Plne funkčný offline editor. |
| **5** | `StateReceiver`, `LogPanel`, `InjectionPanel`, `StateOverviewPanel`. Pripojenie signálov na scénu (`updateTokens`, `highlightEnabled`, ...). | Prijímanie stavu z interpretu. |
| **6** | Integrácia s backendom – testovanie celej slučky: Editor → Ulož → Generate & Run → Monitoruj → Injectuj. | End-to-end test so vzorovou sieťou (TOF_PN). |
| **7** | Ladanie, vizuálne doladenie (stylesheet, ikony), dokumentácia kódu (Doxygen komentáre), finalizácia. | Pripravené na odevzdanie. |

---

## 10. Poznámky a odporúčania

- **Inkrementálny vývoj:** Každý deň by mal skončiť spustiteľnou verziou GUI, aj keď nekompletnou. Nikdy nenechávať "rozpracované" na ďalší deň bez toho, aby sa to dalo spustiť.
- **Testovanie izolovane:** GUI by sa malo dať testovať aj bez backendu – použi mockované dáta (`RuntimeState` vytvorený ručne) na simuláciu behu siete.
- **Komunikácia s tímom:** Každý deň krátky sync s Lukášom a Matějom (stačí 5 minút), aby sa zladili rozhrania (najmä `RuntimeState`, `LogEntry`, formát JSON).
- **Qt verzia:** Testovať na Qt 5.5, ktoré je na merlinovi. Nepoužívať features z novších verzií.
- **Doxygen:** Každá trieda a každá verejná metóda musí mať `@brief` a `@author Jan ...`. Bez toho hrozí 0 bodov!
- **Oddelenie model-GUI:** GUI **nikdy** nepristupuje priamo k triedam `Place`, `Transition` z backendu. Používa iba vlastné `PlaceItem`, `TransitionItem` a konvertuje ich cez `toModel()`/`updateFromModel()`, resp. cez parser textového formátu.