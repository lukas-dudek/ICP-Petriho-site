/**
 * @file interpreter.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#include "interpreter.h"
#include "inscription_engine.h"
#include <algorithm>
#include <cassert>
#include <iostream>

namespace pets {

// konstruktor a destruktor

Interpreter::Interpreter(const PetriNet& net)
    : net_(net)
    , inscription_engine_(std::make_unique<InscriptionEngine>())
{
    net_.reset();

    std::string error;
    if (!inscription_engine_->compile(net_, "/tmp/pets_" + net_.name, error)) {
        std::cerr << "Warning: JIT compilation failed: " << error << std::endl;
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

// rizeni smycky

void Interpreter::start()
{
    if (running_.load()) {
        return;
    }

    net_.reset();
    running_.store(true);
    main_thread_ = std::make_unique<std::thread>(&Interpreter::main_loop, this);
}

void Interpreter::stop()
{
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    event_queue_.push(Event::make_stop());

    if (main_thread_ && main_thread_->joinable()) {
        main_thread_->join();
    }
    main_thread_.reset();
}

bool Interpreter::is_running() const
{
    return running_.load();
}

// gui vstupy

void Interpreter::inject_input(const std::string& input_name, const std::string& value)
{
    event_queue_.push(Event::make_input(input_name, value));
}

// gui callbacky

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

// hlavni beh

void Interpreter::main_loop()
{
    log(LogType::INFO, "Interpret spuštěn pro síť: " + net_.name);
    notify_state();

    while (running_.load()) {

        // stabilizace
        stabilize();

        // casovace
        schedule_delayed();
        notify_state();

        // cekani
        int64_t next_timeout = timer_manager_.next_expiry_ms();
        Event event;

        if (next_timeout >= 0) {
            // Čekej do nejbližšího timeoutu, nebo na externí událost
            bool got_event = event_queue_.wait_for(next_timeout, event);

            if (!got_event) {
                // Timeout vypršel - zkontroluj timer manager
                auto expired = timer_manager_.get_expired();
                for (auto* t : expired) {
                    event_queue_.push(Event::make_timeout(t));
                }
                // Znovu načti z fronty (teď už by tam mělo něco být)
                if (!event_queue_.try_pop(event)) {
                    continue;  // Spurious wakeup - jdi zpět na začátek
                }
            }
        } else {
            // Žádné časovače - čekej na externí událost (nebo STOP)
            event = event_queue_.wait();
        }

        if (!running_.load()) {
            break;
        }

        // probuzeni
        switch (event.type) {

        case EventType::TIMEOUT:
            handle_timeout(event.transition);
            break;

        case EventType::INPUT:
            handle_input_event(event.input_name, event.input_value);
            break;

        case EventType::STOP:
            log(LogType::INFO, "Vyžádáno zastavení, ukončuji...");
            running_.store(false);
            break;
        }
    }

    log(LogType::INFO, "Interpret zastaven pro síť: " + net_.name);
}

// stabilizace (okamzite prechody)

void Interpreter::stabilize()
{
    bool progress = true;

    while (progress) {
        progress = false;

        auto candidates = get_enabled();
        if (candidates.empty()) {
            break;
        }

        auto to_fire = find_executable_transitions(candidates);
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


std::vector<Transition*> Interpreter::get_enabled()
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


bool Interpreter::is_transition_enabled(Transition* t) const
{
    if (!t) return false;

    for (const auto& arc : t->input_arcs) {
        auto* p = net_.find_place(arc.place_id);
        if (!p) return false;
        if (p->current_tokens < arc.weight) {
            return false;
        }
    }

    // Vyhodnocení stráže pomocí InscriptionEngine
    if (t->has_guard() && inscription_engine_->is_loaded()) {
        inscription_engine_->setup_context(net_);
        if (!inscription_engine_->evaluate_guard(*t)) {
            return false;
        }
    }

    return true;
}


std::vector<Transition*> Interpreter::find_executable_transitions(
    const std::vector<Transition*>& candidates)
{
    struct tmp_tokens {
        Place* place;
        int    available;
    };
    std::vector<tmp_tokens> shadow;
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
            auto* p = net_.find_place(arc.place_id);
            auto it = std::find_if(shadow.begin(), shadow.end(),
                                   [&](const tmp_tokens& ts) {
                                       return ts.place == p;
                                   });
            if (it == shadow.end() || it->available < arc.weight) {
                can_fire = false;
                break;
            }
        }

        if (can_fire) {
            selected.push_back(t);
            for (const auto& arc : t->input_arcs) {
                auto* p = net_.find_place(arc.place_id);
                auto it = std::find_if(shadow.begin(), shadow.end(),
                                       [&](const tmp_tokens& ts) {
                                           return ts.place == p;
                                       });
                if (it != shadow.end()) {
                    it->available -= arc.weight;
                }
            }
        }
    }

    return selected;
}


void Interpreter::fire_transition(Transition* t)
{
    if (!t) return;

    for (const auto& arc : t->input_arcs) {
        auto* p = net_.find_place(arc.place_id);
        if (p) {
            p->remove_tokens(arc.weight);
            log(LogType::PLACE_TOKENS_CHANGED,
                "Místo '" + p->id + "': odebráno " + std::to_string(arc.weight)
                + " tokenů, nyní má " + std::to_string(p->current_tokens));
        }
    }

    // proved akci
    if (t->action_code.has_value() && inscription_engine_->is_loaded()) {
        inscription_engine_->setup_context(net_);
        inscription_engine_->execute_transition_action(*t);
    }

    // pridej tokeny
    for (const auto& arc : t->output_arcs) {
        auto* p = net_.find_place(arc.place_id);
        if (p) {
            p->add_tokens(arc.weight);
            log(LogType::PLACE_TOKENS_CHANGED,
                "Místo '" + p->id + "': přidáno " + std::to_string(arc.weight)
                + " tokenů, nyní má " + std::to_string(p->current_tokens));

            // akce mista
            if (p->place_action.has_value() && inscription_engine_->is_loaded()) {
                inscription_engine_->setup_context(net_);
                inscription_engine_->execute_place_action(*p);
            }
        }
    }

    // zaloguj
    log(LogType::TRANSITION_FIRED,
        "Přechod '" + t->id + "' odpálen");

    // zrus casovace
    timer_manager_.cancel(t);
}


void Interpreter::schedule_delayed()
{
    for (auto& t : net_.transitions) {
        if (!t.is_delayed()) continue;
        if (t.is_event_driven()) continue;  // Ty se plánují jen při události

        if (is_transition_enabled(&t)) {
            // Spočítej "current_tokens" jako hash pro detekci duplicity
            int token_sum = 0;
            for (const auto& arc : t.input_arcs) {
                auto* p = net_.find_place(arc.place_id);
                if (p) token_sum += p->current_tokens;
            }

            timer_manager_.schedule(&t, t.delay_ms.value(), token_sum);
            log(LogType::TIMEOUT_SCHEDULED,
                "Naplánován timeout pro přechod '" + t.id
                + "' za " + std::to_string(t.delay_ms.value()) + " ms");
        }
    }
}


void Interpreter::handle_timeout(Transition* t)
{
    if (!t) return;

    log(LogType::TIMEOUT_EXPIRED,
        "Timeout vypršel pro přechod '" + t->id + "'");

    if (is_transition_enabled(t)) {
        fire_transition(t);
    } else {
        log(LogType::TIMEOUT_IGNORED,
            "Timeout pro přechod '" + t->id
            + "' ignorován - přechod již není povolen");
    }
}


void Interpreter::handle_input_event(const std::string& name, const std::string& value)
{
    
    auto* input = net_.find_input(name);
    if (!input) {
        log(LogType::INFO,
            "Ignorována vstupní událost '" + name + "' - neznámý vstup");
        return;
    }

    input->set_value(value);
    log(LogType::INPUT_RECEIVED,
        "Vstup '" + name + "' nastaven na '" + value + "'");
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

    // Okamžité - zařaď do stabilizační smyčky
    if (!immediate.empty()) {
        auto to_fire = find_executable_transitions(immediate);
        std::sort(to_fire.begin(), to_fire.end(),
                  [](Transition* a, Transition* b) {
                      return a->priority < b->priority;
                  });

        for (auto* t : to_fire) {
            fire_transition(t);
        }
    }

    // Zpožděné - naplánuj
    for (auto* t : delayed) {
        int token_sum = 0;
        for (const auto& arc : t->input_arcs) {
            auto* p = net_.find_place(arc.place_id);
            if (p) token_sum += p->current_tokens;
        }
        timer_manager_.schedule(t, t->delay_ms.value(), token_sum);
        log(LogType::TIMEOUT_SCHEDULED,
            "Naplánován (event) timeout pro přechod '" + t->id
            + "' za " + std::to_string(t->delay_ms.value()) + " ms");
    }
}


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

    // Místa - tokeny
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