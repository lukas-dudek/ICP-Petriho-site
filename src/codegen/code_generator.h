/**
 * @file    code_generator.h
 * @brief   Generování C++ kódu interpretu z modelu Petriho sítě
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Třída vygeneruje plnohodnotný .cpp soubor, který obsahuje:
 * - vestavěné funkce (valueof, defined, output, tokens, elapsed, now)
 * - strážní funkce pro každý přechod
 * - akční funkce pro každý přechod / místo
 *
 * Vygenerovaný soubor lze přeložit do sdílené knihovny (.so/.dll)
 * a dynamicky načíst (InscriptionEngine).
 */

#ifndef PETS_CODE_GENERATOR_H
#define PETS_CODE_GENERATOR_H

#include <string>

namespace pets {

class PetriNet;

/**
 * @brief Generátor C++ zdrojového kódu pro interpretaci Petriho sítě.
 *
 * Používá šablonovací přístup – do předpřipravených šablon (cpp_template.h)
 * dosazuje konkrétní identifikátory a výrazy.
 */
class CodeGenerator {
public:
    /**
     * @brief Vygeneruje kompletní .cpp soubor pro danou síť.
     * @param net Model sítě.
     * @param output_path Cesta k výstupnímu souboru (např. "/tmp/pets_gen.cpp").
     * @return true při úspěchu.
     */
    bool generate(const PetriNet& net, const std::string& output_path);

    /**
     * @brief Zkompiluje vygenerovaný .cpp do sdílené knihovny.
     * @param source_path Cesta k .cpp souboru.
     * @param lib_path Cesta k výstupní knihovně (např. "/tmp/libpets.so").
     * @return true při úspěchu.
     */
    bool compile(const std::string& source_path, const std::string& lib_path);

private:
    /// Vygeneruje hlavičku souboru (includy, forward deklarace)
    std::string generate_header(const PetriNet& net) const;

    /// Vygeneruje vestavěné callback funkce (valueof, defined, ...)
    std::string generate_builtins() const;

    /// Vygeneruje strážní funkci pro jeden přechod
    std::string generate_guard(const class Transition& t) const;

    /// Vygeneruje akční funkci pro jeden přechod / místo
    std::string generate_action(const std::string& func_name,
                                const std::string& action_code) const;
};

} // namespace pets

#endif // PETS_CODE_GENERATOR_H
