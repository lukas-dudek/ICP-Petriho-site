#include <iostream>
#include "src/model/petrinet.h"
#include "src/interpreter/interpreter.h"

int main() {
    pets::PetriNet net;
    net.inputs.emplace_back("trigger");
    pets::Interpreter interp(net);
    // Let's inject input
    interp.set_log_callback([](const pets::LogEntry& e) {
        std::cout << "LOG: " << e.message << "\n";
    });
    interp.start();
    interp.inject_input("trigger", "start");
    
    // Wait for event to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    interp.stop();
    return 0;
}
