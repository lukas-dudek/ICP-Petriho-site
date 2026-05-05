┌─────────────────────────────────────────────────────────┐
│                    Backend (Lukáš)                       │
├───────────────┬────────────────┬────────────────────────┤
│  PetriNet     │  Interpreter   │  CodeGenerator         │
│  (datový      │  (běhové jádro)│  (překlad sítě do C++) │
│   model)      │                │                        │
├───────────────┼────────────────┼────────────────────────┤
│  Place        │  EventLoop     │  InscriptionCompiler   │
│  Transition   │  Scheduler     │  (vestavěný interpret  │
│  Arc          │  TimerManager  │   C/C++ výrazů)        │
│  Variable     │  MaxIndependent│                        │
│  Input/Output │  SetFinder     │                        │
├───────────────┴────────────────┴────────────────────────┤
│              IPC Bridge (UDP / Shared Memory)            │
│              → posílá stav, přijímá vstupy               │
└─────────────────────────────────────────────────────────┘/**
 * @brief Reprezentuje jedno místo v Petriho síti
 * @author Lukáš ...
 */
class Place {
public:
    std::string id;                          // jednoznačné jméno
    int initial_tokens = 0;                  // počáteční tokeny (>= 0)
    std::optional<std::string> place_action; // akce při vstupu tokenu (C++ kód)
    
    // Runtime data (během interpretace):
    int current_tokens = 0;                  // aktuální počet tokenů
    std::chrono::steady_clock::time_point last_token_change; // pro elapsed()
};class Transition {
public:
    std::string id;
    
    // Vstupní/výstupní hrany (váha >= 1)
    std::vector<std::pair<Place*, int>> input_arcs;   // place + weight
    std::vector<std::pair<Place*, int>> output_arcs;
    
    // Podmínka odpálení (3 části, každá může chybět)
    std::optional<std::string> input_event_name;       // např. "in"
    std::optional<std::string> guard_expression;       // např. "atoi(valueof(\"in\")) == 1"
    std::optional<int64_t> delay_ms;                   // @ 1000  (zpoždění)
    
    std::optional<std::string> action_code;            // akce při odpálení (C++ kód)
    
    // Priorita pro deterministické řešení konfliktů
    int priority = 0;                                  // dle pořadí definice
};class Input {
public:
    std::string name;
    std::string last_known_value;           // poslední známá hodnota
    bool has_been_set = false;              // pro defined()
    std::chrono::steady_clock::time_point last_set_time;
};

class Output {
public:
    std::string name;
};class Variable {
public:
    std::string type;                       // "int", "float", "const char*", ...
    std::string name;
    std::string initial_value;              // např. "5000"
    
    // Runtime – hodnota uložená jako union/variant
    std::string current_value;              // pro jednoduchost jako string
};class PetriNet {
public:
    std::string name;
    std::string comment;
    
    std::vector<Input> inputs;
    std::vector<Output> outputs;
    std::vector<Variable> variables;
    std::vector<Place> places;
    std::vector<Transition> transitions;
    
    // Metody pro přístup a modifikaci
    Place* findPlace(const std::string& id);
    Transition* findTransition(const std::string& id);
    Input* findInput(const std::string& name);
    // ...
    
    // Validace sítě (kontrola cyklů, nedosažitelných míst, atd.)
    bool validate(std::string& error_msg) const;
};class Interpreter {
public:
    explicit Interpreter(const PetriNet& net);
    ~Interpreter();
    
    // Spuštění / zastavení
    void start();                           // spustí hlavní smyčku v samostatném vlákně
    void stop();                            // nastaví požadavek na ukončení
    bool isRunning() const;
    
    // Injekce vstupní události zvenčí (z GUI)
    void injectInput(const std::string& input_name, const std::string& value);
    
    // Callbacky pro GUI (observer pattern)
    void setStateCallback(std::function<void(const RuntimeState&)> callback);
    void setLogCallback(std::function<void(const LogEntry&)> callback);
    
private:
    PetriNet net_;                          // kopie modelu sítě
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> main_thread_;
    
    // Runtime struktury
    TimerManager timer_manager_;
    InscriptionEngine inscription_engine_;
    
    void mainLoop();
    void doReactionWithoutWaiting();
    std::vector<Transition*> findEnabledImmediateTransitions();
    std::vector<Transition*> findMaxIndependentSet(const std::vector<Transition*>& candidates);
    void fireTransition(Transition* t);
    void scheduleDelayedTransitions();
    void handleInputEvent(const std::string& name, const std::string& value);
    void handleTimeout(Transition* t);
};void Interpreter::mainLoop() {
    while (running_) {
        // (1) Reakce bez čekání – stabilizace
        doReactionWithoutWaiting();
        
        // (2) Naplánování zpožděných přechodů
        scheduleDelayedTransitions();
        
        // (3) Pasivní čekání na událost
        auto event = event_queue_.wait();  // blocking wait
        
        // (4) Zpracování probuzení
        if (event.type == EventType::INPUT) {
            handleInputEvent(event.input_name, event.value);
        } else if (event.type == EventType::TIMEOUT) {
            handleTimeout(event.transition);
        } else if (event.type == EventType::STOP) {
            break;
        }
        
        // Po zpracování se vrací na začátek while – opět stabilizace, atd.
    }
}std::vector<Transition*> Interpreter::findMaxIndependentSet(
    const std::vector<Transition*>& candidates)
{
    // Kopie aktuálního markingu pro simulaci odebírání
    std::unordered_map<Place*, int> available_tokens;
    for (auto& place : net_.places) {
        available_tokens[&place] = place.current_tokens;
    }
    
    std::vector<Transition*> selected;
    
    // Seřazeno dle priority (definiční pořadí = determinismus)
    auto sorted = candidates;
    std::sort(sorted.begin(), sorted.end(),
              [](Transition* a, Transition* b) { return a->priority < b->priority; });
    
    for (auto* t : sorted) {
        // Zkontroluj, zda je stále dost tokenů (po odebrání již vybranými)
        bool can_fire = true;
        for (auto& [place, weight] : t->input_arcs) {
            if (available_tokens[place] < weight) {
                can_fire = false;
                break;
            }
        }
        
        if (can_fire) {
            selected.push_back(t);
            // Simuluj odebrání tokenů pro další iterace
            for (auto& [place, weight] : t->input_arcs) {
                available_tokens[place] -= weight;
            }
        }
    }
    
    return selected;
}class TimerManager {
public:
    // Naplánuje timeout pro přechod (pokud už neběží se stejnými parametry)
    void schedule(Transition* t);
    
    // Vrátí timeouty, které právě vypršely (voláno z event loopu)
    std::vector<Transition*> getExpired();
    
    // Zruší timeout pro daný přechod (volitelné – základní verze nemusí)
    void cancel(Transition* t);
    
private:
    struct TimerEntry {
        Transition* transition;
        std::chrono::steady_clock::time_point expiry;
        int token_count_at_schedule;  // pro detekci "stejných parametrů"
    };
    std::vector<TimerEntry> timers_;
    std::mutex mutex_;
};class InscriptionEngine {
public:
    // Zkompiluje všechny akce/stráže sítě do sdílené knihovny
    bool compile(const PetriNet& net, const std::string& output_path);
    
    // Vyhodnocení strážní podmínky
    bool evaluateGuard(Transition* t,
                       const std::unordered_map<std::string, std::string>& input_values,
                       const std::unordered_map<std::string, std::string>& variables);
    
    // Provedení akce přechodu / místa
    void executeAction(const std::string& action_code, /* ... */);
    
private:
    void* lib_handle_ = nullptr;  // handle k dynamické knihovně
    
    // Vygeneruje C++ kód pro jednu podmínku/akci
    std::string generateGuardWrapper(Transition* t);
    std::string generateActionWrapper(const std::string& action_code, 
                                       const std::string& func_name);
};// auto-generated guard for transition T_on
extern "C" bool guard_T_on(
    const char* valueof_in,    // valueof("in")
    bool defined_in,           // defined("in")
    int tokens_ACTIVE,         // tokens("ACTIVE")
    int elapsed_TIMING,        // elapsed("TIMING")
    int64_t now_ms             // now()
) {
    return (atoi(valueof_in) == 1);  // <-- původní guard expression
}extern "C" void action_T_on(
    // ... stejné parametry ...
    void (*output_func)(const char* name, const char* value)  // output() callback
) {
    // { if (defined("set_to")) { timeout = atoi(valueof("set_to")); } }
    if (defined_set_to) {
        timeout = atoi(valueof_set_to);
    }
}bool InscriptionEngine::compile(const PetriNet& net, const std::string& dir) {
    // 1. Vygeneruj .cpp soubor
    std::string cpp_code = generateFullSource(net);
    std::ofstream(dir + "/generated_net.cpp") << cpp_code;
    
    // 2. Zavolej g++ / clang++
    std::string cmd = "g++ -std=c++17 -shared -fPIC -o " + 
                      dir + "/libnet.so " + dir + "/generated_net.cpp";
    int result = std::system(cmd.c_str());
    
    // 3. Dynamicky načti
    lib_handle_ = dlopen((dir + "/libnet.so").c_str(), RTLD_NOW);
    return result == 0 && lib_handle_ != nullptr;
}struct RuntimeState {
    // Marking
    std::vector<std::pair<std::string, int>> places_tokens;  // place_id → tokens
    
    // Povolené přechody
    std::vector<std::string> enabled_transitions;
    
    // Naplánované timeouty
    struct PendingTimer {
        std::string transition_id;
        int64_t remaining_ms;
    };
    std::vector<PendingTimer> pending_timers;
    
    // Poslední známé hodnoty
    std::vector<std::pair<std::string, std::string>> input_values;
    std::vector<std::pair<std::string, std::string>> output_values;
    std::vector<std::pair<std::string, std::string>> variable_values;
};nlohmann::json RuntimeState::to_json() const {
    nlohmann::json j;
    j["places"] = nlohmann::json::object();
    for (auto& [id, tokens] : places_tokens) {
        j["places"][id] = tokens;
    }
    j["enabled_transitions"] = enabled_transitions;
    j["pending_timers"] = nlohmann::json::array();
    for (auto& [tid, remaining] : pending_timers) {
        j["pending_timers"].push_back({{"transition", tid}, {"remaining_ms", remaining}});
    }
    // ... atd.
    return j;
}class UdpBridge {
public:
    UdpBridge(int port);
    
    void sendState(const RuntimeState& state);       // pošle JSON na GUI
    void sendLog(const LogEntry& entry);             // pošle log záznam
    
    Event waitForEvent();                             // blokující čtení (vstupy/příkazy)
    
private:
    int socket_fd_;
    struct sockaddr_in gui_addr_;
};
6. Návrhové vzory (Design Patterns)
Dle zadání „používejte vhodné návrhové vzory“. Tady jsou klíčové:

Vzor	Kde
Observer	Interpreter → GUI callbacks (stav, log). GUI se registruje jako observer.
Singleton	InscriptionEngine – pouze jedna instance kompilátoru za běhu
Command	Vstupní události (injectInput) jako command objekty v event queue
State	Přechod mezi stavy sítě (IDLE → ACTIVE → TIMING v příkladu TOF)
Strategy	Různé strategie pro výběr maximální nezávislé množiny (můžeš implementovat víc a přepínat)
Factory	Vytváření Place, Transition atd. – parser textového formátu (Matěj) bude používat tvoji factory
// test_max_independent.cpp
#include <cassert>
#include "model/petrinet.h"
#include "interpreter/max_independent_set.h"

void test_two_independent_transitions() {
    PetriNet net;
    Place p1{"P1", 2};  // 2 tokeny
    Place p2{"P2", 1};  // 1 token
    net.places = {p1, p2};
    
    Transition t1{"T1"};
    t1.input_arcs = {{&net.places[0], 1}};  // spotřebuje 1 z P1
    t1.priority = 0;
    
    Transition t2{"T2"};
    t2.input_arcs = {{&net.places[0], 1}};  // spotřebuje 1 z P1 (nezávislé)
    t2.priority = 1;
    
    net.transitions = {t1, t2};
    
    Interpreter interp(net);
    auto set = interp.findMaxIndependentSet({&net.transitions[0], &net.transitions[1]});
    
    assert(set.size() == 2);  // oba by měly být vybrány
    assert(set[0]->id == "T1");  // deterministicky dle priority
    assert(set[1]->id == "T2");
    
    std::cout << "✅ test_two_independent_transitions passed\n";
}
src/
├── model/
│   ├── place.h / place.cpp
│   ├── transition.h / transition.cpp
│   ├── input.h / input.cpp
│   ├── output.h / output.cpp
│   ├── variable.h / variable.cpp
│   ├── petrinet.h / petrinet.cpp
│   └── arc.h                   (header-only, jen dvojice Place* + weight)
│
├── interpreter/
│   ├── interpreter.h / interpreter.cpp
│   ├── event_queue.h / event_queue.cpp
│   ├── timer_manager.h / timer_manager.cpp
│   ├── max_independent_set.h / max_independent_set.cpp
│   └── inscription_engine.h / inscription_engine.cpp
│
├── codegen/
│   ├── code_generator.h / code_generator.cpp
│   └── cpp_templates.h         (šablony pro generovaný kód)
│
├── ipc/
│   ├── udp_bridge.h / udp_bridge.cpp
│   ├── runtime_state.h / runtime_state.cpp
│   └── log_entry.h / log_entry.cpp
│
├── utils/
│   └── json_helper.h           (wrapper pro nlohmann/json)
│
└── main.cpp                    (pouze pro testování backendu zvlášť)