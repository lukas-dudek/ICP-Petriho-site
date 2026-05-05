/**
 * @file    arc.h
 * @brief   Hrana Petriho sítě – dvojice (místo, váha)
 * @author  Lukáš
 * @date    2026-05-04
 */

#ifndef PETS_ARC_H
#define PETS_ARC_H

namespace pets {

// Forward deklarace
class Place;

/**
 * @brief Struktura reprezentující orientovanou hranu mezi místem a přechodem.
 *
 * Váha udává, kolik tokenů se spotřebuje (vstupní hrana) nebo vyrobí (výstupní hrana).
 * Minimální váha je 1.
 */
struct Arc {
    Place* place = nullptr;   ///< Ukazatel na místo
    int    weight = 1;        ///< Váha hrany (>= 1)

    Arc() = default;
    Arc(Place* p, int w) : place(p), weight(w) {}
};

} // namespace pets

#endif // PETS_ARC_H