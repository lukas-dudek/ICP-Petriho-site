/**
 * @file    inscription_engine.h
 * @brief   Engine pro vyhodnocování inskripčního jazyka (podmnožina C/C++)
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Strategie: Just-In-Time kompilace do sdílené knihovny.
 *
 * Pro každou Petriho síť se vygeneruje .cpp soubor, který obsahuje:
 *  - vestavěné funkce (valueof, defined, output, tokens, elapsed, now)
 *  - strážní funkce pro každý přechod (guard_<id>)
 *  - akční funkce pro každý přechod (action_<id>)
 *  - akční funkce pro každé místo (place_action_<id>)
 *
 * .cpp se zkompiluje do .so / .dll a dynamicky načte.
 * Před každým voláním guard/action se nastaví globální proměnné
 * (valueof_*, defined_*, tokens_*, elapsed_*, var_*, now_ms).
 */

#ifndef PETS_INSCRIPTION_ENGINE_H
#define PETS_INSCRIPTION_ENGINE_H

#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace pets {

class PetriNet;
class Place;
class Transition;
class Input;
class Variable;
class Output;

/**
 * @brief Engine pro dynamické vyhodnocování C++ výrazů.
 *
 * Používá JIT kompilaci – generuje, kompiluje a dynamicky načítá
 * sdílenou knihovnu s funkcemi pro stráže a akce.
 */
class InscriptionEngine {
public:
    /**
     * @brief Typ callbacku pro funkci output("name", value).
     *        První parametr: jméno výstupu, druhý: hodnota.
     */
    using OutputFunc = std::function<void(const std::string&, const std::string&)>;

    InscriptionEngine() = default;
    ~InscriptionEngine();

    InscriptionEngine(const InscriptionEngine&) = delete;
    InscriptionEngine& operator=(const InscriptionEngine&) = delete;

    /**
     * @brief Zkompiluje všechny akce/stráže sítě do sdílené knihovny.
     * @param net Model sítě.
     * @param work_dir Pracovní adresář pro dočasné soubory.
     * @param error_msg Pokud selže, obsahuje popis chyby.
     * @return true při úspěchu.
     */
    bool compile(const PetriNet& net,
                 const std::string& work_dir,
                 std::string& error_msg);

    /**
     * @brief Uvolní načtenou knihovnu.
     */
    void unload();

    /**
     * @brief Je knihovna načtená?
     */
    bool is_loaded() const { return lib_handle_ != nullptr; }

    // ==================== Nastavení kontextu před voláním ====================

    /**
     * @brief Nastaví hodnoty všech proměnných, vstupů, tokenů, elapsedů.
     *
     * Toto musí být voláno před každým evaluate_guard / execute_action.
     * Proměnné se namapují jako globální symboly v načtené knihovně.
     */
    void setup_context(const PetriNet& net);

    /**
     * @brief Nastaví callback pro output().
     */
    void set_output_callback(OutputFunc callback);

    // ==================== Vyhodnocování ====================

    /**
     * @brief Vyhodnotí strážní podmínku přechodu.
     * @param t Přechod, jehož guard se má vyhodnotit.
     * @return true pokud je podmínka splněna (nebo žádná neexistuje).
     */
    bool evaluate_guard(const Transition& t);

    /**
     * @brief Provede akci přechodu.
     * @param t Přechod, jehož akce se má provést.
     */
    void execute_transition_action(const Transition& t);

    /**
     * @brief Provede akci místa (při přidání tokenu).
     * @param p Místo, jehož akce se má provést.
     */
    void execute_place_action(const Place& p);

private:
    // ==================== Generování kódu ====================

    /**
     * @brief Vygeneruje kompletní .cpp zdroják pro danou síť.
     * @return C++ kód jako string.
     */
    std::string generate_source(const PetriNet& net) const;

    /**
     * @brief Vygeneruje deklarace extern proměnných pro kontext.
     */
    std::string generate_context_externs(const PetriNet& net) const;

    /**
     * @brief Vygeneruje vestavěné wrapper funkce (valueof, defined, atd.)
     */
    std::string generate_builtin_wrappers(const PetriNet& net) const;

    /**
     * @brief Vygeneruje strážní funkci pro jeden přechod.
     */
    std::string generate_guard_function(const Transition& t) const;

    /**
     * @brief Vygeneruje akční funkci.
     */
    std::string generate_action_function(const std::string& func_name,
                                         const std::string& code) const;

    // ==================== Dynamické načítání ====================

    /**
     * @brief Zavolá g++/clang++ pro kompilaci .cpp do .so/.dll.
     */
    bool compile_to_shared_lib(const std::string& src_path,
                               const std::string& lib_path,
                               std::string& error_msg);

    /**
     * @brief Načte sdílenou knihovnu.
     */
    bool load_library(const std::string& lib_path, std::string& error_msg);

    /**
     * @brief Získá pointer na symbol z knihovny.
     */
    void* get_symbol(const std::string& name);

    // ==================== Settery kontextových proměnných ====================

    void set_extern_int(const std::string& name, int value);
    void set_extern_bool(const std::string& name, bool value);
    void set_extern_string(const std::string& name, const std::string& value);
    void set_extern_int64(const std::string& name, int64_t value);

    // ==================== Data ====================

    void* lib_handle_ = nullptr;           ///< Handle dynamické knihovny
    std::string lib_path_;                 ///< Cesta k .so/.dll
    OutputFunc output_callback_;           ///< Callback pro output()

    /// Cache pro zjištěné symboly (jméno -> pointer na funkci)
    std::unordered_map<std::string, void*> symbol_cache_;
};

} // namespace pets

#endif // PETS_INSCRIPTION_ENGINE_H