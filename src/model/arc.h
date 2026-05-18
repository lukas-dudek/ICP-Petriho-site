/**
 * @file arc.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Definice struktury Arc reprezentující hranu v Petriho síti.
 */
#ifndef PETS_ARC_H
#define PETS_ARC_H

#include <string>

namespace pets {

/**
 * @brief Struktura reprezentující orientovanou hranu mezi místem a přechodem.
 * 
 * Váha udává, kolik tokenů se spotřebuje (vstupní hrana) nebo vyrobí (výstupní hrana).
 */
struct Arc {
    std::string place_id;
    int weight = 1;

    Arc() = default;
    Arc(const std::string& pid, int w) : place_id(pid), weight(w) {}
};

} // namespace pets

#endif // PETS_ARC_H