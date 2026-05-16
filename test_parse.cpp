#include <iostream>
#include "src/model/net_serializer.h"
#include "src/model/petrinet.h"

int main() {
    pets::PetriNet net;
    std::string err;
    if (!pets::NetSerializer::load("examples/simple_counter.petri", net, err)) {
        std::cout << "Error: " << err << "\n";
        return 1;
    }
    std::cout << "Inputs: " << net.inputs.size() << "\n";
    for(auto& i : net.inputs) std::cout << " - " << i.name << "\n";
    return 0;
}
