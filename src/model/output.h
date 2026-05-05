/**
 * @file    output.h
 * @brief   Výstupní událost interpretované Petriho sítě
 * @author  Lukáš
 * @date    2026-05-04
 */

#ifndef PETS_OUTPUT_H
#define PETS_OUTPUT_H

#include <string>

namespace pets {

/**
 * @brief Reprezentuje výstupní událost Petriho sítě.
 *
 * Výstup má pouze jméno. Hodnoty se do něj odesílají voláním funkce output()
 * v akcích přechodů nebo míst.
 */
class Output {
public:
    std::string name; ///< Jméno výstupu (např. "out", "rt")

    Output() = default;
    explicit Output(std::string name_) : name(std::move(name_)) {}
};

} // namespace pets

#endif // PETS_OUTPUT_H