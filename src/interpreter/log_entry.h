/**
 * @file log_entry.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Definice struktury LogEntry pro záznamy v logu událostí.
 */
/**
 * @file    log_entry.h
 * @brief   Jeden záznam v logu událostí.
 * @author  Lukáš
 * @date    2026-05-04
 */

#ifndef PETS_LOG_ENTRY_H
#define PETS_LOG_ENTRY_H

#include <string>
#include <chrono>

namespace pets {

/**
 * @brief Typ logované události.
 */
enum class LogType {
    TRANSITION_FIRED,      ///< Přechod byl odpálen
    INPUT_RECEIVED,        ///< Byl přijat externí vstup
    OUTPUT_SENT,           ///< Byl vygenerován výstup
    TIMEOUT_SCHEDULED,     ///< Byl naplánován časovač
    TIMEOUT_EXPIRED,       ///< Časovač vypršel a přechod se odpálil
    TIMEOUT_IGNORED,       ///< Časovač vypršel, ale přechod už nebyl enabled
    PLACE_TOKENS_CHANGED,  ///< Změnil se počet tokenů v místě
    VARIABLE_CHANGED,      ///< Změnila se hodnota proměnné
    INFO                   ///< Obecná informační zpráva
};

/**
 * @brief Jeden záznam v logu.
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp; ///< Datum a čas události
    LogType type;                                      ///< Typ události
    std::string message;                               ///< Čitelný popis
    std::string detail;                                ///< Volitelné doplňující info

    LogEntry() : timestamp(std::chrono::system_clock::now()) {}

    LogEntry(LogType t, std::string msg, std::string det = {})
        : timestamp(std::chrono::system_clock::now())
        , type(t)
        , message(std::move(msg))
        , detail(std::move(det))
    {}
};

} // namespace pets

#endif // PETS_LOG_ENTRY_H