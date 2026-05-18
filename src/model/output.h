/**
 * @file output.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#ifndef PETS_OUTPUT_H
#define PETS_OUTPUT_H

#include <string>

namespace pets {

// @brief Reprezentuje výstupní událost Petriho sítě.
// 
// Výstup má pouze jméno. Hodnoty se do něj odesílají voláním funkce output()
// v akcích přechodů nebo míst.
class Output {
public:
    std::string name; ///< Jméno výstupu (např. "out", "rt")

    Output() = default;
    explicit Output(std::string name_) : name(std::move(name_)) {}
};

} // namespace pets

#endif // PETS_OUTPUT_H