/**
 * @file    interpreter.cpp
 * @brief   Implementace hlavní smyčky interpretované Petriho sítě.
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Integruje:
 *  - InscriptionEngine (JIT kompilace C++ výrazů)
 *  - TimerManager (zpožděné přechody)
 *  - EventQueue (pasivní čekání s timeoutem)
 *  - Deterministický výběr max nezávislé množiny
 */

#include "interpreter.h"
#include "inscription_engine.h"
#include <algorithm>
#include <cassert>

namespace pets {

// ======================================================================
// Konstruktor / Destruktor
// ======================================================================

Interpreter::Interpreter(const PetriNet& net)
    : net_(net)
    , inscription_engine_(std::make_unique<InscriptionEngine>())
{
    // Fix pointers in net_ to point to net_.places instead of net.places
    for (auto& t : net_.transitions) {
        for (auto& arc : t.input_arcs) {
            if (arc.place) {
                auto it = std::find_if(net.places.begin(), net.places.end(),
                    [&](const Place& p) { return &p == arc.place; });
                if (it != net.places.end()) {
                    auto index = std::distance(net.places.begin(), it);
                    arc.place = &net_.places[index];
                }
            }
        }
        for (auto& arc : t.output_arcs) {
            if (arc.place) {
                auto it = std::find_if(net.places.begin(), net.places.end(),
                    [&](const Place& p) { return &p == arc.place; });
                if (it != net.places.end()) {
                    auto index = std::distance(net.places.begin(), it);
                    arc.place = &net_.places[index];
                }
            }
        }
    }

    net_.reset();

    // Zkus zkompilovat inskripční kód
    std::string error;
    if (!inscription_engine_->compile(net_, "/tmp/pets_" + net_.name, error)) {
        // Fallback: ponecháme bez akcí/stráží
        // (v ostré verzi: logovat varování)
    }

    // Nastav output callback
    inscription_engine_->set_output_callback(
        [this](const std::string& name, const std::string& value) {
            // Najdi výstup a zaloguj
            auto* out = net_.find_output(name);
            if (out) {
                log(LogType::OUTPUT_SENT,
                    "Output '" + name + "' = '" + value + "'");
            }
        }
    );
}

Interpreter::~Interpreter()
{
    stop();
}

// ======================================================================
// Životní cyklus
// ======================================================================

void Interpreter::start()
{
    if (running_.load(std::memory_order_acquire)) {
        return;
    }

    net_.reset();
    running_.store(true, std::memory_order_release);
    main_thread_ = std::make_unique<std::thread>(&Interpreter::main_loop, this);
}

void Interpreter::stop()
{
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    running_.store(false, std::memory_order_release);
    event_queue_.push(Event::make_stop());

    if (main_thread_ && main_thread_->joinable()) {
        main_thread_->join();
    }
    main_thread_.reset();
}

bool Interpreter::is_running() const
{
    return running_.load(std::memory_order_acquire);
}

// ======================================================================
// Vstup z GUI
// ======================================================================

void Interpreter::inject_input(const std::string& input_name, const std::string& value)
{
    event_queue_.push(Event::make_input(input_name, value));
}

// ======================================================================
// Callbacky pro GUI
// ======================================================================

void Interpreter::set_state_callback(StateCallback callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    state_callback_ = std::move(callback);
}

void Interpreter::set_log_callback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    log_callback_ = std::move(callback);
}

// ======================================================================
// Hlavní smyčka
// ======================================================================

void Interpreter::main_loop()
{
    log(LogType::INFO, "Interpreter started for net: " + net_.name);
    notify_state();

    while (running_.load(std::memory_order_acquire)) {

        // === (1) Reakce bez čekání – stabilizace ===
        do_reaction_without_waiting();

        // === (2) Naplánování zpožděných přechodů ===
        schedule_delayed_transitions();
        notify_state();

        // === (3) Pasivní čekání (s timeoutem pro časovače) ===
        int64_t next_timeout = timer_manager_.next_expiry_ms();
        Event event;

        if (next_timeout > 0) {
            // Čekej do nejbližšího timeoutu, nebo na externí událost
            bool got_event = event_queue_.wait_for(next_timeout, event);

            if (!got_event) {
                // Timeout vypršel – zkontroluj timer manager
                auto expired = timer_manager_.get_expired();
                for (auto* t : expired) {
                    event_queue_.push(Event::make_timeout(t));
                }
                // Znovu načti z fronty (teď už by tam mělo něco být)
                if (!event_queue_.try_pop(event)) {
                    continue;  // Spurious wakeup – jdi zpět na začátek
                }
            }
        } else {
            // Žádné časovače – čekej na externí událost (nebo STOP)
            event = event_queue_.wait();
        }

        if (!running_.load(std::memory_order_acquire)) {
            break;
        }

        // === (4) Zpracování probuzení ===
        switch (event.type) {

        case EventType::TIMEOUT:
            handle_timeout(event.transition);
            break;

        case EventType::INPUT:
            handle_input_event(event.input_name, event.input_value);
            break;

        case EventType::STOP:
            log(LogType::INFO, "Stop requested, shutting down...");
            running_.store(false, std::memory_order_release);
            break;
        }
    }

    log(LogType::INFO, "Interpreter stopped for net: " + net_.name);
}

// ======================================================================
// (1) Reakce bez čekání – stabilizace
// ======================================================================

void Interpreter::do_reaction_without_waiting()
{
    bool progress = true;

    while (progress) {
        progress = false;

        auto candidates = find_enabled_immediate_transitions();
        if (candidates.empty()) {
            break;
        }

        auto to_fire = find_max_independent_set(candidates);
        if (to_fire.empty()) {
            break;
        }

        progress = true;

        // Seřaď deterministicky dle priority
        std::sort(to_fire.begin(), to_fire.end(),
                  [](Transition* a, Transition* b) {
                      return a->priority < b->priority;
                  });

        for (auto* t : to_fire) {
            fire_transition(t);
        }
    }
}

// ======================================================================
// Vyhledání enabled okamžitých přechodů
// ======================================================================

std::vector<Transition*> Interpreter::find_enabled_immediate_transitions()
{
    std::vector<Transition*> result;

    for (auto& t : net_.transitions) {
        if (t.is_delayed()) continue;
        if (t.is_event_driven()) continue;
        if (is_transition_enabled(&t)) {
            result.push_back(&t);
        }
    }

    return result;
}

// ======================================================================
// Kontrola, zda je přechod enabled
// ======================================================================

bool Interpreter::is_transition_enabled(Transition* t) const
{
    if (!t) return false;

    // 1. Multiset enabling – kontrola tokenů
    for (const auto& arc : t->input_arcs) {
        if (!arc.place) return false;
        if (arc.place->current_tokens < arc.weight) {
            return false;
        }
    }

    // 2. Vyhodnocení stráže pomocí InscriptionEngine
    if (t->has_guard() && inscription_engine_->is_loaded()) {
        inscription_engine_->setup_context(net_);
        if (!inscription_engine_->evaluate_guard(*t)) {
            return false;
        }
    }

    return true;
}

// ======================================================================
// Maximální množina nezávislých přechodů
// ======================================================================

std::vector<Transition*> Interpreter::find_max_independent_set(
    const std::vector<Transition*>& candidates)
{
    struct TokenShadow {
        Place* place;
        int    available;
    };
    std::vector<TokenShadow> shadow;
    shadow.reserve(net_.places.size());
    for (auto& p : net_.places) {
        shadow.push_back({&p, p.current_tokens});
    }

    auto sorted = candidates;
    std::sort(sorted.begin(), sorted.end(),
              [](Transition* a, Transition* b) {
                  return a->priority < b->priority;
              });

    std::vector<Transition*> selected;

    for (auto* t : sorted) {
        bool can_fire = true;
        for (const auto& arc : t->input_arcs) {
            auto it = std::find_if(shadow.begin(), shadow.end(),
                                   [&](const TokenShadow& ts) {
                                       return ts.place == arc.place;
                                   });
            if (it == shadow.end() || it->available < arc.weight) {
                can_fire = false;
                break;
            }
        }

        if (can_fire) {
            selected.push_back(t);
            for (const auto& arc : t->input_arcs) {
                auto it = std::find_if(shadow.begin(), shadow.end(),
                                       [&](const TokenShadow& ts) {
                                           return ts.place == arc.place;
                                       });
                if (it != shadow.end()) {
                    it->available -= arc.weight;
                }
            }
        }
    }

    return selected;
}

// ======================================================================
// Odpálení přechodu (fire)
// ======================================================================

void Interpreter::fire_transition(Transition* t)
{
    if (!t) return;

    // 1. Odeber tokeny
    for (const auto& arc : t->input_arcs) {
        if (arc.place) {
            arc.place->remove_tokens(arc.weight);
            log(LogType::PLACE_TOKENS_CHANGED,
                "Place '" + arc.place->id + "': removed " + std::to_string(arc.weight)
                + " token(s), now has " + std::to_string(arc.place->current_tokens));
        }
    }

    // 2. Proveď akci přechodu (InscriptionEngine)
    if (t->action_code.has_value() && inscription_engine_->is_loaded()) {
        inscription_engine_->setup_context(net_);
        inscription_engine_->execute_transition_action(*t);
    }

    // 3. Přidej tokeny
    for (const auto& arc : t->output_arcs) {
        if (arc.place) {
            arc.place->add_tokens(arc.weight);
            log(LogType::PLACE_TOKENS_CHANGED,
                "Place '" + arc.place->id + "': added " + std::to_string(arc.weight)
                + " token(s), now has " + std::to_string(arc.place->current_tokens));

            // 4. Place action – při každém přidání tokenu
            if (arc.place->place_action.has_value() && inscription_engine_->is_loaded()) {
                inscription_engine_->setup_context(net_);
                inscription_engine_->execute_place_action(*arc.place);
            }
        }
    }

    // 5. Zaloguj odpálení
    log(LogType::TRANSITION_FIRED,
        "Transition '" + t->id + "' fired");

    // 6. Zruš případné časovače pro tento přechod
    //    (doporučená sémantika: timeout už není potřeba)
    timer_manager_.cancel(t);
}

// ======================================================================
// (2) Naplánování zpožděných přechodů
// ======================================================================

void Interpreter::schedule_delayed_transitions()
{
    for (auto& t : net_.transitions) {
        if (!t.is_delayed()) continue;
        if (t.is_event_driven()) continue;  // Ty se plánují jen při události

        if (is_transition_enabled(&t)) {
            // Spočítej "current_tokens" jako hash pro detekci duplicity
            int token_sum = 0;
            for (const auto& arc : t.input_arcs) {
                if (arc.place) token_sum += arc.place->current_tokens;
            }

            timer_manager_.schedule(&t, t.delay_ms.value(), token_sum);
            log(LogType::TIMEOUT_SCHEDULED,
                "Scheduled timeout for transition '" + t.id
                + "' in " + std::to_string(t.delay_ms.value()) + " ms");
        }
    }
}

// ======================================================================
// (4a) Zpracování timeoutu
// ======================================================================

void Interpreter::handle_timeout(Transition* t)
{
    if (!t) return;

    log(LogType::TIMEOUT_EXPIRED,
        "Timeout expired for transition '" + t->id + "'");

    if (is_transition_enabled(t)) {
        fire_transition(t);
    } else {
        log(LogType::TIMEOUT_IGNORED,
            "Timeout for transition '" + t->id
            + "' ignored – no longer enabled");
    }
}

// ======================================================================
// (4b) Zpracování vstupní události
// ======================================================================

void Interpreter::handle_input_event(const std::string& name, const std::string& value)
{
    auto* input = net_.find_input(name);
    if (!input) {
        log(LogType::INFO,
            "Ignored input event '" + name + "' – no such input declared");
        return;
    }

    input->set_value(value);
    log(LogType::INPUT_RECEIVED,
        "Input '" + name + "' set to '" + value + "'");
    notify_state();

    // Aktivuj přechody navázané na tuto událost
    std::vector<Transition*> event_transitions;
    for (auto& t : net_.transitions) {
        if (t.input_event_name.has_value()
            && t.input_event_name.value() == name)
        {
            event_transitions.push_back(&t);
        }
    }

    // Rozděl na okamžité a zpožděné
    std::vector<Transition*> immediate;
    std::vector<Transition*> delayed;

    for (auto* t : event_transitions) {
        if (!is_transition_enabled(t)) {
            continue;
        }

        if (t->is_delayed()) {
            delayed.push_back(t);
        } else {
            immediate.push_back(t);
        }
    }

    // Okamžité – zařaď do stabilizační smyčky
    if (!immediate.empty()) {
        auto to_fire = find_max_independent_set(immediate);
        std::sort(to_fire.begin(), to_fire.end(),
                  [](Transition* a, Transition* b) {
                      return a->priority < b->priority;
                  });

        for (auto* t : to_fire) {
            fire_transition(t);
        }
    }

    // Zpožděné – naplánuj
    for (auto* t : delayed) {
        int token_sum = 0;
        for (const auto& arc : t->input_arcs) {
            if (arc.place) token_sum += arc.place->current_tokens;
        }
        timer_manager_.schedule(t, t->delay_ms.value(), token_sum);
        log(LogType::TIMEOUT_SCHEDULED,
            "Scheduled (event-driven) timeout for transition '" + t->id
            + "' in " + std::to_string(t->delay_ms.value()) + " ms");
    }
}

// ======================================================================
// Logování a notifikace GUI
// ======================================================================

void Interpreter::log(LogType type, const std::string& message, const std::string& detail)
{
    LogEntry entry(type, message, detail);

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (log_callback_) {
        log_callback_(entry);
    }
}

void Interpreter::notify_state()
{
    RuntimeState state = get_current_state();

    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (state_callback_) {
        state_callback_(state);
    }
}

RuntimeState Interpreter::get_current_state() const
{
    RuntimeState state;

    // Místa – tokeny
    for (const auto& p : net_.places) {
        state.places_tokens.push_back({p.id, std::to_string(p.current_tokens)});
    }

    // Povolené přechody (všechny, nejen okamžité)
    for (const auto& t : net_.transitions) {
        if (is_transition_enabled(const_cast<Transition*>(&t))) {
            state.enabled_transitions.push_back(t.id);
        }
    }

    // Naplánované timeouty
    auto timer_infos = timer_manager_.get_all();
    for (const auto& ti : timer_infos) {
        PendingTimer pt;
        pt.transition_id = ti.transition_id;
        pt.remaining_ms = ti.remaining_ms;
        state.pending_timers.push_back(pt);
    }

    // Vstupy
    for (const auto& i : net_.inputs) {
        state.input_values.push_back({i.name, i.last_known_value});
    }

    // Proměnné
    for (const auto& v : net_.variables) {
        state.variable_values.push_back({v.name, v.current_value});
    }

    return state;
}

} // namespace pets