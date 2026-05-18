/**
 * @file main_backend.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#include <chrono>
#include <iostream>
#include <thread>

#include "interpreter/interpreter.h"
#include "model/petrinet.h"

using namespace pets;

int main() {
  std::cout << "--- Petri Net Backend Test ---\n";

  PetriNet net;
  net.name = "TestNet";
  net.comment = "jednoducha sit";
  net.inputs.emplace_back("in");
  net.outputs.emplace_back("out");
  net.variables.emplace_back("int", "counter", "0");
  net.places.emplace_back("P1", 1); // startovni token
  net.places.emplace_back("P2", 0);

  // T1
  Transition t1("T1");
  t1.input_arcs.push_back({"P1", 1});
  t1.output_arcs.push_back({"P2", 1});
  t1.input_event_name = "in";
  t1.priority = 0;
  net.transitions.push_back(t1);

  // T2
  Transition t2("T2");
  t2.input_arcs.push_back({"P2", 1});
  t2.output_arcs.push_back({"P1", 1});
  t2.delay_ms = 2000;
  t2.priority = 1;
  net.transitions.push_back(t2);

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

  interp.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "Posilam in = hello\n";
  interp.inject_input("in", "hello");
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Ukoncuji...\n";
  interp.stop();

  std::cout << "--- Hotovo ---\n";
  return 0;
}