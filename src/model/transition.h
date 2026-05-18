/**
 * @file transition.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef PETS_TRANSITION_H
#define PETS_TRANSITION_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "arc.h"

namespace pets {

/**
 * @brief Reprezentuje jeden přechod v Petriho síti.
 * 
 * Přechod obsahuje vstupní a výstupní hrany s vahami, podmínku odpálení
 * složenou ze tří volitelných částí (input_event_name, guard_expression, delay_ms)
 * a akci provedenou při odpálení.
 */
class Transition {
public:
    
    std::string id;  ///< Jednoznačné jméno přechodu
    int priority = 0; ///< Priorita pro deterministické řešení konfliktů (nižší = dřívější)

    
    std::vector<Arc> input_arcs;  ///< Vstupní hrany (místo → přechod) s vahami
    std::vector<Arc> output_arcs; ///< Výstupní hrany (přechod → místo) s vahami

    
    /**
     * @brief Jméno vstupní události (např. "in").
     *        Pokud je prázdné/nullopt, přechod je "interní" a neváže se na událost.
     */
    std::optional<std::string> input_event_name;

    /**
     * @brief Booleovský výraz v C/C++ nad proměnnými a valueof(...).
     *        Pokud je prázdné/nullopt, stráž se nevyhodnocuje (vždy true).
     */
    std::optional<std::string> guard_expression;

    /**
     * @brief Zpoždění odpálení v milisekundách (@ delay).
     *        Pokud je nullopt, přechod je okamžitý.
     */
    std::optional<int64_t> delay_ms;

    
    /**
     * @brief Kód akce přechodu v C/C++.
     *        Provede se při odpálení (po odebrání tokenů, před přidáním).
     */
    std::optional<std::string> action_code;

    
    Transition() = default;
    explicit Transition(std::string id_) : id(std::move(id_)) {}

    
    /// Je přechod vázaný na konkrétní vstupní událost?
    bool is_event_driven() const { return input_event_name.has_value(); }

    /// Je přechod zpožděný?
    bool is_delayed() const { return delay_ms.has_value(); }

    /// Má přechod strážní podmínku?
    bool has_guard() const { return guard_expression.has_value(); }
};

} // namespace pets

#endif // PETS_TRANSITION_H
