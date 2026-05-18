/**
 * @file timer_manager.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Deklarace správce časovačů pro zpožděné přechody.
 */

// timer_manager.h

#ifndef PETS_TIMER_MANAGER_H
#define PETS_TIMER_MANAGER_H

#include <vector>
#include <mutex>
#include <chrono>
#include <string>
#include <cstdint>

namespace pets {

class Transition;

// @brief Spravuje kolekci aktivních časovačů.
class TimerManager {
public:
    // @brief Naplánuje timeout pro přechod (pokud ještě neběží se stejnými parametry).
// @param t Přechod, který se má odpálit po timeoutu.
// @param delay_ms Zpoždění v milisekundách.
// @param current_tokens Aktuální marking pro porovnání "stejných parametrů".
    void schedule(Transition* t, int64_t delay_ms, int current_tokens);

    // @brief Najde všechny přechody, jejichž časovač právě vypršel.
// @return Seznam přechodů k odpálení (může být prázdný).
    std::vector<Transition*> get_expired();

    /// Zruší všechny časovače pro daný přechod.
    void cancel(Transition* t);

    /// Zruší všechny časovače.
    void clear();

    /// Zjistí, zda pro daný přechod už běží časovač.
    bool has_timer(Transition* t) const;

    /// Vrátí počet aktivních časovačů.
    size_t size() const;

    /// Informace o jednom časovači pro GUI.
    struct TimerInfo {
        std::string transition_id;
        int64_t     remaining_ms;
    };

    /// Vrátí stav všech časovačů (pro zobrazení v GUI).
    std::vector<TimerInfo> get_all() const;

    /// Vrátí čas do vypršení dalšího časovače v ms (nebo -1 pokud žádný není).
    int64_t next_expiry_ms() const;

private:
    struct TimerEntry {
        Transition* transition = nullptr;
        std::chrono::steady_clock::time_point expiry;
        int token_count_at_schedule = 0;  ///< Pro detekci duplicity
    };

    std::vector<TimerEntry> timers_;
    mutable std::mutex mutex_;
};

} // namespace pets

#endif // PETS_TIMER_MANAGER_H