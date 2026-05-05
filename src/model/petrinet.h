/**
 * @file    petrinet.h
 * @brief   Agregát celé Petriho sítě – obsahuje všechna místa, přechody,
 * vstupy, výstupy, proměnné
 * @author  Lukáš
 * @date    2026-05-04
 */

#ifndef PETS_PETRINET_H
#define PETS_PETRINET_H

#include <string>
#include <vector>

#include "input.h"
#include "output.h"
#include "place.h"
#include "transition.h"
#include "variable.h"

namespace pets {

/**
 * @brief Reprezentuje celou interpretovanou Petriho síť.
 *
 * Drží vlastnictví všech komponent (místa, přechody, vstupy, výstupy, proměnné)
 * a poskytuje pohodlné vyhledávací metody. Validace kontroluje konzistenci
 * sítě.
 */
class PetriNet {
public:
  // ==================== Metadata ====================

  std::string name;    ///< Jméno sítě
  std::string comment; ///< Komentář / popis

  // ==================== Kolekce komponent ====================

  std::vector<Place> places;           ///< Místa (kroky)
  std::vector<Transition> transitions; ///< Přechody
  std::vector<Input> inputs;           ///< Vstupní události
  std::vector<Output> outputs;         ///< Výstupní události
  std::vector<Variable> variables;     ///< Interní proměnné

  // ==================== Vyhledávací metody ====================

  /// Najde místo podle jména. Vrací nullptr, pokud neexistuje.
  Place *find_place(const std::string &id);
  const Place *find_place(const std::string &id) const;

  /// Najde přechod podle jména.
  Transition *find_transition(const std::string &id);
  const Transition *find_transition(const std::string &id) const;

  /// Najde vstup podle jména.
  Input *find_input(const std::string &name);
  const Input *find_input(const std::string &name) const;

  /// Najde výstup podle jména.
  Output *find_output(const std::string &name);
  const Output *find_output(const std::string &name) const;

  /// Najde proměnnou podle jména.
  Variable *find_variable(const std::string &name);
  const Variable *find_variable(const std::string &name) const;

  // ==================== Validace ====================

  /**
   * @brief Zvaliduje konzistenci sítě.
   * @param error_msg Pokud validace selže, obsahuje popis chyby.
   * @return true pokud je síť validní.
   */
  bool validate(std::string &error_msg) const;

  // ==================== Pomocné ====================

  /// Resetuje všechny runtime komponenty do počátečního stavu.
  void reset();

  /// Vyčistí celou síť.
  void clear();
};

} // namespace pets

#endif // PETS_PETRINET_H
