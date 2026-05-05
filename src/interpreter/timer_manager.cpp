/**
 * @file    timer_manager.cpp
 * @brief   Implementace správy časovačů.
 * @author  Lukáš
 * @date    2026-05-04
 */

#include "timer_manager.h"
#include "../model/transition.h"
#include <algorithm>

namespace pets {

void TimerManager::schedule(Transition* t, int64_t delay_ms, int current_tokens)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Zkontroluj, zda už neběží časovač se stejnými parametry
    for (const auto& entry : timers_) {
        if (entry.transition == t
            && entry.token_count_at_schedule == current_tokens)
        {
            return;  // Už existuje, nic neděláme
        }
    }

    TimerEntry entry;
    entry.transition = t;
    entry.expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
    entry.token_count_at_schedule = current_tokens;
    timers_.push_back(entry);
}

std::vector<Transition*> TimerManager::get_expired()
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    std::vector<Transition*> expired;

    auto it = timers_.begin();
    while (it != timers_.end()) {
        if (it->expiry <= now) {
            expired.push_back(it->transition);
            it = timers_.erase(it);  // Odstraň vypršelý časovač
        } else {
            ++it;
        }
    }

    return expired;
}

void TimerManager::cancel(Transition* t)
{
    std::lock_guard<std::mutex> lock(mutex_);
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
                       [t](const TimerEntry& e) { return e.transition == t; }),
        timers_.end()
    );
}

void TimerManager::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    timers_.clear();
}

size_t TimerManager::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return timers_.size();
}

std::vector<TimerManager::TimerInfo> TimerManager::get_all() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TimerInfo> result;
    auto now = std::chrono::steady_clock::now();

    for (const auto& entry : timers_) {
        TimerInfo info;
        info.transition_id = entry.transition ? entry.transition->id : "";
        info.remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.expiry - now
        ).count();
        if (info.remaining_ms < 0) info.remaining_ms = 0;
        result.push_back(info);
    }
    return result;
}

int64_t TimerManager::next_expiry_ms() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (timers_.empty()) return -1;

    auto now = std::chrono::steady_clock::now();
    int64_t min_ms = INT64_MAX;

    for (const auto& entry : timers_) {
        int64_t remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.expiry - now
        ).count();
        if (remaining < min_ms) {
            min_ms = remaining;
        }
    }

    return min_ms > 0 ? min_ms : 0;
}

} // namespace pets