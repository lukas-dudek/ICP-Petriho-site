/**
 * @file event_queue.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Deklarace thread-safe fronty událostí pro interpret.
 */
#ifndef PETS_EVENT_QUEUE_H
#define PETS_EVENT_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

namespace pets {

class Transition;  // forward

// @brief Typy událostí, na které může smyčka reagovat.
enum class EventType {
    INPUT,      ///< Externí vstupní událost (GUI nastavilo hodnotu)
    TIMEOUT,    ///< Vypršel časovač zpožděného přechodu
    STOP        ///< Požadavek na ukončení interpretu
};

// @brief Jedna událost ve frontě.
struct Event {
    EventType type = EventType::STOP;

    // Pro INPUT:
    std::string input_name;
    std::string input_value;

    // Pro TIMEOUT:
    Transition* transition = nullptr;

    /// Factory metody
    static Event make_input(const std::string& name, const std::string& value)
    {
        Event e;
        e.type = EventType::INPUT;
        e.input_name = name;
        e.input_value = value;
        return e;
    }

    static Event make_timeout(Transition* t)
    {
        Event e;
        e.type = EventType::TIMEOUT;
        e.transition = t;
        return e;
    }

    static Event make_stop()
    {
        Event e;
        e.type = EventType::STOP;
        return e;
    }
};

// @brief Thread-safe fronta událostí s možností blokujícího čekání.
// 
// Používá se v hlavní smyčce interpretu. wait() blokuje, dokud není
// událost k dispozici. wait_for() čeká maximálně zadanou dobu.
class EventQueue {
public:
    /// Vloží událost do fronty a probudí čekající vlákno.
    void push(const Event& event);

    /// Blokující čekání na další událost.
    Event wait();

    // @brief Blokující čekání s timeoutem.
// @param timeout_ms Maximální doba čekání v ms.
// @param out_event Výstupní událost (platná jen pokud návratová hodnota = true).
// @return true pokud událost nastala, false pokud vypršel timeout.
    bool wait_for(int64_t timeout_ms, Event& out_event);

    /// Pokusí se neblokujícně vybrat událost.
    bool try_pop(Event& event);

    /// Probudí čekající vlákno (např. při požadavku na ukončení).
    void wakeup();

private:
    std::queue<Event> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace pets

#endif // PETS_EVENT_QUEUE_H