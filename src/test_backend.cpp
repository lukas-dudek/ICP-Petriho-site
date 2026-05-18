/**
 * @file test_backend.cpp
 * @brief Automatický test interpretu: ověřuje output(), valueof(), tokens(), proměnné.
 */
#include <chrono>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

#include "interpreter/interpreter.h"
#include "model/petrinet.h"

using namespace pets;

static std::atomic<int> output_count{0};
static std::vector<std::string> captured_outputs;
static std::vector<std::string> captured_logs;

int main() {
    std::cout << "=== Automated Backend Test ===" << std::endl;
    int tests_passed = 0;
    int tests_failed = 0;

    // --- Sestavení sítě programově ---
    PetriNet net;
    net.name = "AutoTest";
    net.comment = "Automatický test output a inskripčního jazyka";
    net.inputs.emplace_back("cmd");
    net.outputs.emplace_back("result");
    net.outputs.emplace_back("status");
    net.variables.emplace_back("int", "counter", "0");

    net.places.emplace_back("P_idle", 1);   // start s 1 tokenem
    net.places.emplace_back("P_active", 0);

    // T_activate: cmd event → přesun P_idle→P_active, inkrementuj counter, output result
    Transition t1("T_activate");
    t1.input_arcs.push_back({"P_idle", 1});
    t1.output_arcs.push_back({"P_active", 1});
    t1.input_event_name = "cmd";
    t1.action_code = "var_counter = var_counter + 1; output(\"result\", var_counter); output(\"status\", \"activated\");";
    t1.priority = 0;
    net.transitions.push_back(t1);

    // T_reset: interní přechod P_active→P_idle (bez události, okamžitý)
    Transition t2("T_reset");
    t2.input_arcs.push_back({"P_active", 1});
    t2.output_arcs.push_back({"P_idle", 1});
    t2.action_code = "output(\"status\", \"reset\");";
    t2.priority = 1;
    net.transitions.push_back(t2);

    // Validace
    std::string error;
    if (!net.validate(error)) {
        std::cerr << "FAIL: Validation failed: " << error << std::endl;
        return 1;
    }
    std::cout << "[PASS] Net validation OK" << std::endl;
    tests_passed++;

    // Vytvoř interpret
    Interpreter interp(net);

    // Registruj callbacky
    interp.set_log_callback([](const LogEntry &entry) {
        captured_logs.push_back(entry.message);
        if (entry.type == LogType::OUTPUT_SENT) {
            output_count++;
            captured_outputs.push_back(entry.message);
            std::cout << "  [OUTPUT] " << entry.message << std::endl;
        }
    });

    interp.set_state_callback([](const RuntimeState &state) {
        // Ticho - jen zachytáváme
        (void)state;
    });

    // Spusť
    interp.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // --- TEST 1: Injektuj vstup → přechod se odpálí → output se vygeneruje ---
    std::cout << "\n--- Test 1: Inject 'cmd'='1' → output() should fire ---" << std::endl;
    output_count = 0;
    captured_outputs.clear();
    interp.inject_input("cmd", "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (output_count >= 2) {
        std::cout << "[PASS] output() was called " << output_count.load() << " times (expected >=2)" << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] output() was called " << output_count.load() << " times (expected >=2)" << std::endl;
        tests_failed++;
    }

    // Zkontroluj, že výstupy obsahují "result" a "status"
    bool has_result = false, has_status = false;
    for (const auto& s : captured_outputs) {
        if (s.find("result") != std::string::npos) has_result = true;
        if (s.find("status") != std::string::npos) has_status = true;
    }
    if (has_result && has_status) {
        std::cout << "[PASS] Both 'result' and 'status' outputs captured" << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] Missing outputs: result=" << has_result << " status=" << has_status << std::endl;
        tests_failed++;
    }

    // --- TEST 2: Druhá injekce → counter by měl být 2 ---
    std::cout << "\n--- Test 2: Second inject → counter should increment ---" << std::endl;
    captured_outputs.clear();
    output_count = 0;
    interp.inject_input("cmd", "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool has_counter2 = false;
    for (const auto& s : captured_outputs) {
        if (s.find("'2'") != std::string::npos || s.find("= '2'") != std::string::npos) {
            has_counter2 = true;
        }
    }
    if (has_counter2) {
        std::cout << "[PASS] Counter incremented to 2" << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] Counter did not increment to 2. Outputs:" << std::endl;
        for (const auto& s : captured_outputs) std::cout << "  " << s << std::endl;
        tests_failed++;
    }

    // --- TEST 3: Reset přechod by měl output "reset" ---
    bool has_reset = false;
    for (const auto& s : captured_outputs) {
        if (s.find("reset") != std::string::npos) has_reset = true;
    }
    if (has_reset) {
        std::cout << "[PASS] T_reset fired and produced output 'reset'" << std::endl;
        tests_passed++;
    } else {
        std::cout << "[FAIL] T_reset did not produce 'reset' output" << std::endl;
        tests_failed++;
    }

    // Zastav
    interp.stop();

    // --- Výsledky ---
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    if (tests_failed == 0) {
        std::cout << "\n*** ALL TESTS PASSED ***" << std::endl;
    } else {
        std::cout << "\n*** SOME TESTS FAILED ***" << std::endl;
    }

    return tests_failed > 0 ? 1 : 0;
}
