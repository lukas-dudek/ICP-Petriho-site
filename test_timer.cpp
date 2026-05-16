#include <iostream>
#include <thread>
#include "src/model/petrinet.h"
#include "src/interpreter/interpreter.h"

int main() {
    pets::PetriNet net;
    net.places.emplace_back("P1", 1, std::nullopt);
    net.places.emplace_back("P2", 0, std::nullopt);
    
    pets::Transition t("T1");
    t.input_arcs.push_back({&net.places[0], 1});
    t.output_arcs.push_back({&net.places[1], 1});
    t.delay_ms = 1000; // 1 second
    net.transitions.push_back(std::move(t));

    pets::Interpreter interp(net);
    interp.set_log_callback([](const pets::LogEntry& e) {
        std::cout << "LOG: " << e.message << "\n";
    });
    
    interp.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    interp.stop();
    return 0;
}
