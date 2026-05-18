/**
 * @file runtime_state.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Struktury pro kompletní runtime stav sítě přenášený do GUI.
 */
/**
 * @file    runtime_state.h
 * @brief   Struktura pro kompletní stav sítě v reálném čase (pro GUI).
 * @author  Lukáš
 * @date    2026-05-04
 */

#ifndef PETS_RUNTIME_STATE_H
#define PETS_RUNTIME_STATE_H

#include <string>
#include <vector>
#include <cstdint>

namespace pets {

/**
 * @brief Informace o jednom naplánovaném časovači.
 */
struct PendingTimer {
    std::string transition_id;   ///< Jméno přechodu
    int64_t     remaining_ms = 0; ///< Zbývající čas do timeoutu v ms
};

/**
 * @brief Dvojice (jméno, hodnota) pro vstupy/výstupy/proměnné.
 */
struct NamedValue {
    std::string name;
    std::string value;
};

/**
 * @brief Kompletní runtime stav Petriho sítě.
 *
 * Tato struktura se serializuje (např. do JSON) a posílá do GUI pro
 * živé zobrazení markingu, povolených přechodů, timeoutů atd.
 */
struct RuntimeState {
    // Marking - aktuální počty tokenů ve všech místech
    std::vector<NamedValue> places_tokens;

    // Seznam právě povolených (enabled) přechodů
    std::vector<std::string> enabled_transitions;

    // Seznam naplánovaných timeoutů
    std::vector<PendingTimer> pending_timers;

    // Poslední známé hodnoty vstupů
    std::vector<NamedValue> input_values;

    // Poslední vygenerované hodnoty výstupů
    std::vector<NamedValue> output_values;

    // Aktuální hodnoty interních proměnných
    std::vector<NamedValue> variable_values;
};

} // namespace pets

#endif // PETS_RUNTIME_STATE_H
