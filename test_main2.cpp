#include <iostream>
#include <QString>
#include <QStringList>
#include "src/model/petrinet.h"
#include "src/interpreter/interpreter.h"

int main() {
    QStringList net_inputs_;
    net_inputs_.append("trigger");
    
    auto net = std::make_unique<pets::PetriNet>();
    for (const auto& name : net_inputs_) {
        net->inputs.emplace_back(name.toStdString());
    }
    
    pets::Interpreter interp(*net);
    
    interp.set_log_callback([](const pets::LogEntry& e) {
        std::cout << "LOG: " << e.message << "\n";
    });
    interp.start();
    
    QString injectName = "trigger";
    interp.inject_input(injectName.toStdString(), "start");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    interp.stop();
    return 0;
}
