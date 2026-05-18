/**
 * @file net_serializer.h
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Deklarace třídy NetSerializer pro serializaci a deserializaci sítí.
 */
#ifndef PETS_NET_SERIALIZER_H
#define PETS_NET_SERIALIZER_H

#include <string>

namespace pets {

class PetriNet;

// @brief Umožňuje uložit a načíst Petriho síť z/do textového souboru.
class NetSerializer {
public:
    // @brief Uloží síť do textového souboru.
// @param net Model sítě.
// @param path Cesta k výstupnímu souboru.
// @param error Pokud selže, obsahuje popis chyby.
// @return true při úspěchu.
    static bool save(const PetriNet& net, const std::string& path,
                     std::string& error);

    // @brief Serializuje síť do řetězce (textový formát).
    static std::string serialize(const PetriNet& net);

    // @brief Načte síť z textového souboru.
// @param path Cesta ke vstupnímu souboru.
// @param net Výstupní model sítě (bude vyčištěn a naplněn).
// @param error Pokud selže, obsahuje popis chyby.
// @return true při úspěchu.
    static bool load(const std::string& path, PetriNet& net,
                     std::string& error);

    // @brief Naparsuje síť z řetězce.
    static bool parse(const std::string& content, PetriNet& net,
                      std::string& error);
};

} // namespace pets

#endif // PETS_NET_SERIALIZER_H
