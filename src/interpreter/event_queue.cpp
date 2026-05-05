/**
 * @file    event_queue.cpp
 * @brief   Implementace thread-safe fronty událostí.
 * @author  Lukáš
 * @date    2026-05-04
 */

#include "event_queue.h"

namespace pets {

void EventQueue::push(const Event& event)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(event);
    }
    cv_.notify_one();
}

Event EventQueue::wait()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty(); });
    Event e = queue_.front();
    queue_.pop();
    return e;
}

bool EventQueue::wait_for(int64_t timeout_ms, Event& out_event)
{
    std::unique_lock<std::mutex> lock(mutex_);
    bool success = cv_.wait_for(lock,
                                std::chrono::milliseconds(timeout_ms),
                                [this] { return !queue_.empty(); });
    if (success) {
        out_event = queue_.front();
        queue_.pop();
        return true;
    }
    return false;
}

bool EventQueue::try_pop(Event& event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    event = queue_.front();
    queue_.pop();
    return true;
}

void EventQueue::wakeup()
{
    cv_.notify_one();
}

} // namespace pets