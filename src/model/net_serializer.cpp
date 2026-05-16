/**
 * @file    net_serializer.cpp
 * @brief   Implementace parseru a serializeru textového formátu sítě.
 * @author  Matěj
 * @date    2026-05-04
 */

#include "net_serializer.h"
#include "petrinet.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace pets {

// ======================================================================
// Pomocné funkce
// ======================================================================

static std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

/**
 * @brief Extrahuje blok kódu ohraničený { ... } ze stringu.
 *        Vrací obsah bez vnějších závorek, trimovaný.
 */
static std::string extract_braces(const std::string& s)
{
    auto first = s.find('{');
    auto last = s.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last <= first) {
        return {};
    }
    return trim(s.substr(first + 1, last - first - 1));
}

// ======================================================================
// Serialize (model → text)
// ======================================================================

std::string NetSerializer::serialize(const PetriNet& net)
{
    std::ostringstream ss;

    ss << "Jméno sítě:\n";
    ss << "    " << net.name << "\n";

    ss << "Komentář:\n";
    ss << "    " << net.comment << "\n";

    // Vstupy
    ss << "Vstupy:\n";
    for (const auto& i : net.inputs) {
        ss << "    " << i.name << "\n";
    }

    // Výstupy
    ss << "Výstupy:\n";
    for (const auto& o : net.outputs) {
        ss << "    " << o.name << "\n";
    }

    // Proměnné
    ss << "Proměnné:\n";
    for (const auto& v : net.variables) {
        ss << "    " << v.type << " " << v.name;
        if (!v.initial_value.empty()) {
            ss << " = " << v.initial_value;
        }
        ss << "\n";
    }

    // Místa
    ss << "Místa:\n";
    for (const auto& p : net.places) {
        ss << "    " << p.id << " (" << p.initial_tokens << ")";
        if (p.place_action.has_value()) {
            ss << " : { " << p.place_action.value() << " }";
        }
        ss << "\n";
    }

    // Přechody
    ss << "Přechody:\n";
    for (const auto& t : net.transitions) {
        ss << t.id << " :\n";

        // Vstupní hrany
        ss << "    in:";
        for (size_t i = 0; i < t.input_arcs.size(); ++i) {
            if (i > 0) ss << ",";
            if (t.input_arcs[i].place) {
                ss << "  " << t.input_arcs[i].place->id << "*" << t.input_arcs[i].weight;
            }
        }
        ss << "\n";

        // Výstupní hrany
        ss << "    out:";
        for (size_t i = 0; i < t.output_arcs.size(); ++i) {
            if (i > 0) ss << ",";
            if (t.output_arcs[i].place) {
                ss << " " << t.output_arcs[i].place->id << "*" << t.output_arcs[i].weight;
            }
        }
        ss << "\n";

        // Podmínka
        ss << "    when:";
        if (t.input_event_name.has_value()) {
            ss << " " << t.input_event_name.value();
        }
        if (t.guard_expression.has_value()) {
            ss << " [ " << t.guard_expression.value() << " ]";
        }
        if (t.delay_ms.has_value()) {
            ss << " @ " << t.delay_ms.value();
        }
        ss << "\n";

        // Akce
        ss << "    do: { ";
        if (t.action_code.has_value()) {
            ss << t.action_code.value();
        }
        ss << " }\n";

        // Priorita (rozšíření formátu)
        if (t.priority != 0) {
            ss << "    priority: " << t.priority << "\n";
        }
    }

    return ss.str();
}

bool NetSerializer::save(const PetriNet& net, const std::string& path,
                         std::string& error)
{
    std::ofstream out(path);
    if (!out.is_open()) {
        error = "Cannot open file for writing: " + path;
        return false;
    }
    out << serialize(net);
    return true;
}

// ======================================================================
// Parse (text → model)
// ======================================================================

bool NetSerializer::parse(const std::string& content, PetriNet& net,
                          std::string& error)
{
    net.clear();

    std::istringstream stream(content);
    std::string line;

    // Stav parseru
    enum class Section {
        NONE, NAME, COMMENT, INPUTS, OUTPUTS, VARIABLES, PLACES, TRANSITIONS
    };
    Section section = Section::NONE;

    // Pomocná struktura pro odložené propojení hran
    struct PendingArc {
        std::string place_id;
        int weight;
    };
    struct PendingTransition {
        Transition t;
        std::vector<PendingArc> pending_in;
        std::vector<PendingArc> pending_out;
    };
    std::vector<PendingTransition> pending_transitions;

    // Dočasný stack
    std::unique_ptr<PendingTransition> current_pending;

    auto start_new_transition = [&](const std::string& id) {
        if (current_pending) {
            pending_transitions.push_back(std::move(*current_pending));
        }
        current_pending = std::make_unique<PendingTransition>();
        current_pending->t = Transition(id);
    };

    auto finalize_all = [&]() {
        if (current_pending) {
            pending_transitions.push_back(std::move(*current_pending));
            current_pending.reset();
        }
    };

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        // Kontrola sekcí
        if (trimmed == "Jméno sítě:" || trimmed == "Jmeno site:") {
            finalize_all();
            section = Section::NAME;
            continue;
        }
        if (trimmed == "Komentář:" || trimmed == "Komentar:") {
            finalize_all();
            section = Section::COMMENT;
            continue;
        }
        if (trimmed == "Vstupy:") {
            finalize_all();
            section = Section::INPUTS;
            continue;
        }
        if (trimmed == "Výstupy:" || trimmed == "Vystupy:") {
            finalize_all();
            section = Section::OUTPUTS;
            continue;
        }
        if (trimmed == "Proměnné:" || trimmed == "Promenne:") {
            finalize_all();
            section = Section::VARIABLES;
            continue;
        }
        // Místa – match variants
        if (trimmed.find("Místa") == 0 || trimmed.find("Mista") == 0) {
            finalize_all();
            section = Section::PLACES;
            continue;
        }
        // Přechody – match variants
        if (trimmed.find("Přechody") == 0 || trimmed.find("Prechody") == 0) {
            finalize_all();
            section = Section::TRANSITIONS;
            continue;
        }
        // Pozn.: – komentáře, ignorovat
        if (trimmed.find("Pozn.:") == 0 || trimmed.find("Pozn:") == 0) {
            continue;
        }

        // Zpracuj dle aktuální sekce
        switch (section) {
        case Section::NAME:
            net.name = trimmed;
            break;

        case Section::COMMENT:
            if (!net.comment.empty()) net.comment += " ";
            net.comment += trimmed;
            break;

        case Section::INPUTS:
            net.inputs.emplace_back(trimmed);
            break;

        case Section::OUTPUTS:
            net.outputs.emplace_back(trimmed);
            break;

        case Section::VARIABLES: {
            // Formát: "typ jméno = hodnota" nebo "typ jméno"
            std::istringstream vs(trimmed);
            std::string type, name, eq, value;
            vs >> type >> name;
            if (vs >> eq && eq == "=") {
                std::getline(vs, value);
                value = trim(value);
            }
            net.variables.emplace_back(type, name, value);
            break;
        }

        case Section::PLACES: {
            // Formát: "ID (tokens)" nebo "ID (tokens) : { code }"
            std::regex place_re(R"((\w+)\s*\((\d+)\)\s*(?::\s*(\{.*\}))?)");
            std::smatch m;
            if (std::regex_search(trimmed, m, place_re)) {
                std::string id = m[1].str();
                int tokens = std::stoi(m[2].str());
                std::optional<std::string> action;
                if (m[3].matched) {
                    std::string code = extract_braces(m[3].str());
                    if (!code.empty()) action = code;
                }
                net.places.emplace_back(id, tokens, action);
            }
            break;
        }

        case Section::TRANSITIONS: {
            // Detekce nového přechodu: "ID :"
            // ale NE "in:", "out:", "when:", "do:", "priority:"
            if (trimmed.find("in:") == 0) {
                // Vstupní hrany: "in:  P1*1, P2*2"
                if (!current_pending) break;
                std::string arcs_str = trim(trimmed.substr(3));
                std::istringstream as(arcs_str);
                std::string token;
                while (std::getline(as, token, ',')) {
                    token = trim(token);
                    auto star = token.find('*');
                    if (star != std::string::npos) {
                        std::string pid = trim(token.substr(0, star));
                        int weight = std::stoi(trim(token.substr(star + 1)));
                        current_pending->pending_in.push_back({pid, weight});
                    }
                }
            } else if (trimmed.find("out:") == 0) {
                // Výstupní hrany
                if (!current_pending) break;
                std::string arcs_str = trim(trimmed.substr(4));
                std::istringstream as(arcs_str);
                std::string token;
                while (std::getline(as, token, ',')) {
                    token = trim(token);
                    auto star = token.find('*');
                    if (star != std::string::npos) {
                        std::string pid = trim(token.substr(0, star));
                        int weight = std::stoi(trim(token.substr(star + 1)));
                        current_pending->pending_out.push_back({pid, weight});
                    }
                }
            } else if (trimmed.find("when:") == 0) {
                // Podmínka: "when: event_name [ guard ] @ delay"
                if (!current_pending) break;
                std::string cond = trim(trimmed.substr(5));

                // Parse @ delay
                auto at_pos = cond.find('@');
                if (at_pos != std::string::npos) {
                    std::string delay_str = trim(cond.substr(at_pos + 1));
                    try {
                        current_pending->t.delay_ms = std::stoll(delay_str);
                    } catch (...) {
                        // delay může být jméno proměnné – uložit jako guard_expression...
                        // Pro jednoduchost: pokud číslo, použij; jinak ignoruj
                    }
                    cond = trim(cond.substr(0, at_pos));
                }

                // Parse [ guard ]
                auto bracket_start = cond.find('[');
                auto bracket_end = cond.rfind(']');
                if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
                    std::string guard = trim(cond.substr(bracket_start + 1,
                                                        bracket_end - bracket_start - 1));
                    if (!guard.empty()) {
                        current_pending->t.guard_expression = guard;
                    }
                    cond = trim(cond.substr(0, bracket_start));
                }

                // Co zbude = event name
                if (!cond.empty()) {
                    current_pending->t.input_event_name = cond;
                }
            } else if (trimmed.find("do:") == 0) {
                // Akce: "do: { code }"
                if (!current_pending) break;
                std::string code = extract_braces(trimmed);
                if (!code.empty()) {
                    current_pending->t.action_code = code;
                }
            } else if (trimmed.find("priority:") == 0) {
                if (!current_pending) break;
                std::string prio = trim(trimmed.substr(9));
                try {
                    current_pending->t.priority = std::stoi(prio);
                } catch (...) {}
            } else {
                // Nový přechod: "ID :"
                // Odfiltruj řádky které nejsou deklarace přechodů
                auto colon_pos = trimmed.find(':');
                if (colon_pos != std::string::npos) {
                    std::string id = trim(trimmed.substr(0, colon_pos));
                    std::string rest = trim(trimmed.substr(colon_pos + 1));
                    // ID nesmí obsahovat mezery a nesmí to být klíčové slovo
                    if (!id.empty() && id.find(' ') == std::string::npos &&
                        rest.empty()) {
                        start_new_transition(id);
                    }
                }
            }
            break;
        }

        default:
            break;
        }
    }

    // Dokončení posledního přechodu
    finalize_all();

    // Propoj arc pointery na reálná místa
    for (auto& pt : pending_transitions) {
        for (const auto& pa : pt.pending_in) {
            Place* p = net.find_place(pa.place_id);
            if (!p) {
                error = "Transition '" + pt.t.id + "' references unknown place '" + pa.place_id + "'";
                return false;
            }
            pt.t.input_arcs.push_back({p, pa.weight});
        }
        for (const auto& pa : pt.pending_out) {
            Place* p = net.find_place(pa.place_id);
            if (!p) {
                error = "Transition '" + pt.t.id + "' references unknown place '" + pa.place_id + "'";
                return false;
            }
            pt.t.output_arcs.push_back({p, pa.weight});
        }
        net.transitions.push_back(std::move(pt.t));
    }

    return true;
}

bool NetSerializer::load(const std::string& path, PetriNet& net,
                         std::string& error)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        error = "Cannot open file: " + path;
        return false;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return parse(ss.str(), net, error);
}

} // namespace pets
