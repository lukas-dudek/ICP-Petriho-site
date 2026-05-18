/**
 * @file place.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Definice třídy Place reprezentující místo v Petriho síti.
 */
#ifndef PETS_PLACE_H
#define PETS_PLACE_H

#include <string>
#include <optional>
#include <chrono>

namespace pets {

/**
 * @brief Reprezentuje jedno místo v Petriho síti.
 * 
 * Každé místo má jednoznačný identifikátor, počáteční počet tokenů a volitelně
 * akci, která se provádí při každém přidání tokenu ("on add token").
 */
class Place {
public:
    
    std::string id;                          ///< Jednoznačné jméno místa
    int         initial_tokens = 0;          ///< Počáteční počet tokenů (>= 0)
    std::optional<std::string> place_action; ///< C/C++ kód akce při přidání tokenu

    
    int current_tokens = 0;  ///< Aktuální počet tokenů během běhu

    /// Čas poslední změny počtu tokenů (pro funkci elapsed())
    std::chrono::steady_clock::time_point last_token_change;

    
    Place() = default;

    Place(std::string id_, int initial = 0, std::optional<std::string> action = {})
        : id(std::move(id_))
        , initial_tokens(initial)
        , place_action(std::move(action))
        , current_tokens(initial)
    {}

    
    /// Reset do počátečního stavu (před spuštěním interpretace)
    void reset()
    {
        current_tokens = initial_tokens;
        last_token_change = std::chrono::steady_clock::now();
    }

    /// Přidá tokeny do místa (a aktualizuje čas změny)
    void add_tokens(int count)
    {
        if (count > 0) {
            current_tokens += count;
            last_token_change = std::chrono::steady_clock::now();
        }
    }

    /// Odebere tokeny z místa (a aktualizuje čas změny)
    void remove_tokens(int count)
    {
        if (count > 0 && current_tokens >= count) {
            current_tokens -= count;
            last_token_change = std::chrono::steady_clock::now();
        }
    }
};

} // namespace pets

#endif // PETS_PLACE_H
