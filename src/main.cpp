/**
 * @file    main.cpp
 * @brief   Hlavní vstupní bod programu – testování backendu Petriho sítě.
 * @author  Lukáš
 * @date    2026-05-04
 *
 * Tento soubor slouží pro samostatné testování backendu bez GUI.
 * Produkčně bude backend spouštěn jako součást hlavní aplikace (Qt + GUI).
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "interpreter/interpreter.h"
#include "model/petrinet.h"

using namespace pets;

/**
 * @brief Vytvoří testovací Petriho síť (jednoduchý TOF_PN_5s z příkladu v
 * zadání)
 */
PetriNet create_test_net() {
  PetriNet net;
  net.name = "TestNet";
  net.comment = "Jednoduchá testovací síť";

  // Vstupy
  net.inputs.emplace_back("in");

  // Výstupy
  net.outputs.emplace_back("out");

  // Proměnné
  net.variables.emplace_back("int", "counter", "0");

  // Místa
  net.places.emplace_back("P1", 1); // Počáteční token
  net.places.emplace_back("P2", 0);

  // Přechod T1: P1 -> P2, na událost "in"
  {
    Transition t("T1");
    t.input_arcs.push_back({&net.places[0], 1});
    t.output_arcs.push_back({&net.places[1], 1});
    t.input_event_name = "in";
    t.priority = 0;
    net.transitions.push_back(t);
  }

  // Přechod T2: P2 -> P1, interní, zpožděný o 2000 ms
  {
    Transition t("T2");
    t.input_arcs.push_back({&net.places[1], 1});
    t.output_arcs.push_back({&net.places[0], 1});
    t.delay_ms = 2000;
    t.priority = 1;
    net.transitions.push_back(t);
  }

  return net;
}

int main() {
  std::cout << "=== Petri Net Interpreter - Backend Test ===" << std::endl;

  // Vytvoř síť
  auto net = create_test_net();

  std::string error;
  if (!net.validate(error)) {
    std::cerr << "Validation failed: " << error << std::endl;
    return 1;
  }

  std::cout << "Net '" << net.name << "' validated OK." << std::endl;

  // Vytvoř a spusť interpreter
  Interpreter interp(net);

  // Registruj callbacky
  interp.set_log_callback([](const LogEntry &entry) {
    std::cout << "[LOG] " << entry.message << std::endl;
  });

  interp.set_state_callback([](const RuntimeState &state) {
    std::cout << "[STATE] Places: ";
    for (const auto &p : state.places_tokens) {
      std::cout << p.name << "=" << p.value << " ";
    }
    std::cout << std::endl;
  });

  // Start
  interp.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Injektuj vstup
  std::cout << "\n>>> Injecting input 'in' = 'hello'" << std::endl;
  interp.inject_input("in", "hello");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  // Zastav
  std::cout << "\n>>> Stopping interpreter..." << std::endl;
  interp.stop();

  std::cout << "\n=== Test Complete ===" << std::endl;
  return 0;
}