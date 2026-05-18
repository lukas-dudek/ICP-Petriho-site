/**
 * @file petrinet.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
#include "petrinet.h"
#include <algorithm>
#include <unordered_set>

namespace pets {


template<typename Container, typename Predicate>
auto find_element(Container& c, Predicate p) -> decltype(&c.front()) {
    auto it = std::find_if(c.begin(), c.end(), p);
    return (it != c.end()) ? &(*it) : nullptr;
}

Place* PetriNet::find_place(const std::string& id)
{
    return find_element(places, [&](const Place& p){ return p.id == id; });
}

const Place* PetriNet::find_place(const std::string& id) const
{
    return find_element(places, [&](const Place& p){ return p.id == id; });
}

Transition* PetriNet::find_transition(const std::string& id)
{
    return find_element(transitions, [&](const Transition& t){ return t.id == id; });
}

const Transition* PetriNet::find_transition(const std::string& id) const
{
    return find_element(transitions, [&](const Transition& t){ return t.id == id; });
}

Input* PetriNet::find_input(const std::string& name)
{
    return find_element(inputs, [&](const Input& i){ return i.name == name; });
}

const Input* PetriNet::find_input(const std::string& name) const
{
    return find_element(inputs, [&](const Input& i){ return i.name == name; });
}

Output* PetriNet::find_output(const std::string& name)
{
    return find_element(outputs, [&](const Output& o){ return o.name == name; });
}

const Output* PetriNet::find_output(const std::string& name) const
{
    return find_element(outputs, [&](const Output& o){ return o.name == name; });
}

Variable* PetriNet::find_variable(const std::string& name)
{
    return find_element(variables, [&](const Variable& v){ return v.name == name; });
}

const Variable* PetriNet::find_variable(const std::string& name) const
{
    return find_element(variables, [&](const Variable& v){ return v.name == name; });
}


bool PetriNet::validate(std::string& error_msg) const
{
    auto check_unique = [&](const auto& container, const std::string& error_prefix) {
        std::unordered_set<std::string> names;
        for (const auto& item : container) {
            std::string name;
            if constexpr (std::is_same_v<std::decay_t<decltype(item)>, Place> || 
                          std::is_same_v<std::decay_t<decltype(item)>, Transition>) {
                name = item.id;
            } else {
                name = item.name;
            }
            if (!names.insert(name).second) {
                error_msg = error_prefix + name;
                return false;
            }
        }
        return true;
    };

    if (!check_unique(places, "Duplicitní název místa: ")) return false;
    if (!check_unique(transitions, "Duplicitní název přechodu: ")) return false;
    if (!check_unique(inputs, "Duplicitní název vstupu: ")) return false;
    if (!check_unique(outputs, "Duplicitní název výstupu: ")) return false;
    if (!check_unique(variables, "Duplicitní název proměnné: ")) return false;

    // Odkazy a váhy
    for (const auto& t : transitions) {
        for (const auto& arc : t.input_arcs) {
            if (!find_place(arc.place_id)) {
                error_msg = "Přechod '" + t.id + "' odkazuje na neexistující vstupní místo";
                return false;
            }
            if (arc.weight < 1) {
                error_msg = "Přechod '" + t.id + "' má vstupní váhu < 1";
                return false;
            }
        }
        for (const auto& arc : t.output_arcs) {
            if (!find_place(arc.place_id)) {
                error_msg = "Přechod '" + t.id + "' odkazuje na neexistující výstupní místo";
                return false;
            }
            if (arc.weight < 1) {
                error_msg = "Přechod '" + t.id + "' má výstupní váhu < 1";
                return false;
            }
        }
        
        if (t.input_event_name.has_value()) {
            if (!find_input(t.input_event_name.value())) {
                error_msg = "Přechod '" + t.id + "' odkazuje na neznámou událost '"
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
