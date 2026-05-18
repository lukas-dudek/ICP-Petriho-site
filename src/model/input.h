// input.h

#ifndef PETS_INPUT_H
#define PETS_INPUT_H

#include <string>
#include <chrono>

namespace pets {

// @brief Reprezentuje vstupní událost (event) Petriho sítě.
// 
// Vstup má jméno, poslední známou hodnotu (typicky String) a příznak,
// zda byl někdy nastaven. Hodnota je vždy k dispozici pro čtení ve strážích
// i akcích přechodů.
class Input {
public:
    
    std::string name;                ///< Jméno vstupu (např. "in", "set_to")

    
    std::string last_known_value;    ///< Poslední známá hodnota (pro valueof())
    bool        has_been_set = false; ///< Byl vstup někdy nastaven? (pro defined())
    std::chrono::steady_clock::time_point last_set_time; ///< Čas posledního nastavení

    
    Input() = default;
    explicit Input(std::string name_) : name(std::move(name_)) {}

    
    /// Nastaví novou hodnotu vstupu
    void set_value(const std::string& value)
    {
        last_known_value = value;
        has_been_set = true;
        last_set_time = std::chrono::steady_clock::now();
    }

    /// Reset do počátečního stavu
    void reset()
    {
        last_known_value.clear();
        has_been_set = false;
    }
};

} // namespace pets

#endif // PETS_INPUT_H