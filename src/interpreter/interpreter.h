// interpreter.h - behova smycka interpretu

#ifndef PETS_INTERPRETER_H
#define PETS_INTERPRETER_H

#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>

#include "event_queue.h"
#include "timer_manager.h"
#include "runtime_state.h"
#include "log_entry.h"
#include "../model/petrinet.h"

namespace pets {

class InscriptionEngine;  // forward

// Hlavni trida interpretu Petriho site
class Interpreter {
public:
    using StateCallback = std::function<void(const RuntimeState&)>;
    using LogCallback = std::function<void(const LogEntry&)>;

    explicit Interpreter(const PetriNet& net);
    ~Interpreter();

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

    void start();
    void stop();
    bool is_running() const;

    void inject_input(const std::string& input_name, const std::string& value);

    void set_state_callback(StateCallback callback);
    void set_log_callback(LogCallback callback);

    RuntimeState get_current_state() const;

    // Verejne pro testovani
    std::vector<Transition*> get_independent_set(
        const std::vector<Transition*>& candidates);

private:
    void main_loop();
    void stabilize();
    void schedule_delayed();
    void handle_timeout(Transition* t);
    void handle_input_event(const std::string& name, const std::string& value);

    std::vector<Transition*> get_enabled();
    bool is_transition_enabled(Transition* t) const;
    void fire_transition(Transition* t);

    void log(LogType type, const std::string& message, const std::string& detail = {});
    void notify_state();

    PetriNet net_;
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> main_thread_;

    EventQueue    event_queue_;
    TimerManager  timer_manager_;
    std::unique_ptr<InscriptionEngine> inscription_engine_;  // <-- NOVÉ

    StateCallback state_callback_;
    LogCallback   log_callback_;
    mutable std::mutex callback_mutex_;
};

} // namespace pets

#endif // PETS_INTERPRETER_H