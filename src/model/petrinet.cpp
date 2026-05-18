// petrinet.cpp

#include "petrinet.h"
#include <algorithm>
#include <unordered_set>

namespace pets {


Place* PetriNet::find_place(const std::string& id)
{
    auto it = std::find_if(places.begin(), places.end(),
                           [&](const Place& p) { return p.id == id; });
    return (it != places.end()) ? &(*it) : nullptr;
}

const Place* PetriNet::find_place(const std::string& id) const
{
    auto it = std::find_if(places.begin(), places.end(),
                           [&](const Place& p) { return p.id == id; });
    return (it != places.end()) ? &(*it) : nullptr;
}

Transition* PetriNet::find_transition(const std::string& id)
{
    auto it = std::find_if(transitions.begin(), transitions.end(),
                           [&](const Transition& t) { return t.id == id; });
    return (it != transitions.end()) ? &(*it) : nullptr;
}

const Transition* PetriNet::find_transition(const std::string& id) const
{
    auto it = std::find_if(transitions.begin(), transitions.end(),
                           [&](const Transition& t) { return t.id == id; });
    return (it != transitions.end()) ? &(*it) : nullptr;
}

Input* PetriNet::find_input(const std::string& name)
{
    auto it = std::find_if(inputs.begin(), inputs.end(),
                           [&](const Input& i) { return i.name == name; });
    return (it != inputs.end()) ? &(*it) : nullptr;
}

const Input* PetriNet::find_input(const std::string& name) const
{
    auto it = std::find_if(inputs.begin(), inputs.end(),
                           [&](const Input& i) { return i.name == name; });
    return (it != inputs.end()) ? &(*it) : nullptr;
}

Output* PetriNet::find_output(const std::string& name)
{
    auto it = std::find_if(outputs.begin(), outputs.end(),
                           [&](const Output& o) { return o.name == name; });
    return (it != outputs.end()) ? &(*it) : nullptr;
}

const Output* PetriNet::find_output(const std::string& name) const
{
    auto it = std::find_if(outputs.begin(), outputs.end(),
                           [&](const Output& o) { return o.name == name; });
    return (it != outputs.end()) ? &(*it) : nullptr;
}

Variable* PetriNet::find_variable(const std::string& name)
{
    auto it = std::find_if(variables.begin(), variables.end(),
                           [&](const Variable& v) { return v.name == name; });
    return (it != variables.end()) ? &(*it) : nullptr;
}

const Variable* PetriNet::find_variable(const std::string& name) const
{
    auto it = std::find_if(variables.begin(), variables.end(),
                           [&](const Variable& v) { return v.name == name; });
    return (it != variables.end()) ? &(*it) : nullptr;
}


bool PetriNet::validate(std::string& error_msg) const
{
    // 1. Unikátnost jmen míst
    {
        std::unordered_set<std::string> names;
        for (const auto& p : places) {
            if (!names.insert(p.id).second) {
                error_msg = "Duplicate place id: " + p.id;
                return false;
            }
        }
    }

    // 2. Unikátnost jmen přechodů
    {
        std::unordered_set<std::string> names;
        for (const auto& t : transitions) {
            if (!names.insert(t.id).second) {
                error_msg = "Duplicate transition id: " + t.id;
                return false;
            }
        }
    }

    // 3. Unikátnost jmen vstupů
    {
        std::unordered_set<std::string> names;
        for (const auto& i : inputs) {
            if (!names.insert(i.name).second) {
                error_msg = "Duplicate input name: " + i.name;
                return false;
            }
        }
    }

    // 4. Unikátnost jmen výstupů
    {
        std::unordered_set<std::string> names;
        for (const auto& o : outputs) {
            if (!names.insert(o.name).second) {
                error_msg = "Duplicate output name: " + o.name;
                return false;
            }
        }
    }

    // 5. Unikátnost jmen proměnných
    {
        std::unordered_set<std::string> names;
        for (const auto& v : variables) {
            if (!names.insert(v.name).second) {
                error_msg = "Duplicate variable name: " + v.name;
                return false;
            }
        }
    }

    // 6. Všechny hrany odkazují na existující místa
    for (const auto& t : transitions) {
        for (const auto& arc : t.input_arcs) {
            if (!arc.place) {
                error_msg = "Transition '" + t.id + "' has null input arc";
                return false;
            }
        }
        for (const auto& arc : t.output_arcs) {
            if (!arc.place) {
                error_msg = "Transition '" + t.id + "' has null output arc";
                return false;
            }
        }
    }

    // 7. Váhy hran >= 1
    for (const auto& t : transitions) {
        for (const auto& arc : t.input_arcs) {
            if (arc.weight < 1) {
                error_msg = "Transition '" + t.id + "' has input weight < 1";
                return false;
            }
        }
        for (const auto& arc : t.output_arcs) {
            if (arc.weight < 1) {
                error_msg = "Transition '" + t.id + "' has output weight < 1";
                return false;
            }
        }
    }

    // 8. Pokud přechod odkazuje na vstupní událost, musí existovat
    for (const auto& t : transitions) {
        if (t.input_event_name.has_value()) {
            if (!find_input(t.input_event_name.value())) {
                error_msg = "Transition '" + t.id + "' references unknown input '"
                            + t.input_event_name.value() + "'";
                return false;
            }
        }
    }

    return true;
}


void PetriNet::reset()
{
    for (auto& p : places) {
        p.reset();
    }
    for (auto& i : inputs) {
        i.reset();
    }
    for (auto& v : variables) {
        v.reset();
    }
}

void PetriNet::clear()
{
    name.clear();
    comment.clear();
    places.clear();
    transitions.clear();
    inputs.clear();
    outputs.clear();
    variables.clear();
}

} // namespace pets
