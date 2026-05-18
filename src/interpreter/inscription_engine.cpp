/**
 * @file inscription_engine.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Implementace JIT kompilačního engine pro inskripční jazyk Petriho sítí.
 */
#include "inscription_engine.h"
#include "../model/petrinet.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <list>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
  #define DLOPEN(path) LoadLibraryA(path)
  #define DLSYM(handle, name) GetProcAddress((HMODULE)handle, name)
  #define DLCLOSE(handle) FreeLibrary((HMODULE)handle)
  #define DLERROR() std::to_string(GetLastError())
#else
  #include <dlfcn.h>
  #define DLOPEN(path) dlopen(path, RTLD_NOW)
  #define DLSYM(handle, name) dlsym(handle, name)
  #define DLCLOSE(handle) dlclose(handle)
  #define DLERROR() dlerror()
#endif

namespace pets {

/// Globální ukazatel na aktuální InscriptionEngine instanci (pro C trampoline)
static InscriptionEngine* g_active_engine = nullptr;

/// C trampoline funkce pro output() volání z JIT kódu
static void output_trampoline(const char* name, const char* value)
{
    if (g_active_engine) {
        g_active_engine->invoke_output_callback(
            name ? name : "", value ? value : "");
    }
}

// Destruktor

InscriptionEngine::~InscriptionEngine()
{
    if (g_active_engine == this) {
        g_active_engine = nullptr;
    }
    unload();
}

// Kompilace sítě do sdílené knihovny

bool InscriptionEngine::compile(const PetriNet& net,
                                const std::string& work_dir,
                                std::string& error_msg)
{
    unload();  // Uvolni případnou předchozí knihovnu

    // Generování .cpp zdrojáku sítě
    std::string source = generate_source(net);
    std::string src_path = work_dir + "/pets_net.cpp";
    {
        std::ofstream out(src_path);
        if (!out.is_open()) {
            error_msg = "Cannot write to " + src_path;
            return false;
        }
        out << source;
    }

    // Kompilace do sdílené knihovny
    lib_path_ = work_dir + "/libpets_net"
#ifdef _WIN32
                + ".dll";
#else
                + ".so";
#endif
    if (!compile_to_shared_lib(src_path, lib_path_, error_msg)) {
        return false;
    }

    // Načtení zkompilované knihovny
    if (!load_library(lib_path_, error_msg)) {
        return false;
    }

    return true;
}

void InscriptionEngine::unload()
{
    symbol_cache_.clear();

    if (lib_handle_) {
        DLCLOSE(lib_handle_);
        lib_handle_ = nullptr;
    }

    // Smaž dočasné soubory
    if (!lib_path_.empty()) {
        std::remove(lib_path_.c_str());
        lib_path_.clear();
    }
}

// Nastavení kontextu

void InscriptionEngine::setup_context(const PetriNet& net)
{
    if (!lib_handle_) return;

    // Nastav globální engine pro output trampoline
    g_active_engine = this;

    // Nastav output_func pointer v knihovně na trampoline
    void* output_func_ptr = get_symbol("output_func");
    if (output_func_ptr) {
        using OutputFuncType = void(*)(const char*, const char*);
        *reinterpret_cast<OutputFuncType*>(output_func_ptr) = &output_trampoline;
    }

    auto now = std::chrono::steady_clock::now();
    static auto start_time = now;
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time
    ).count();

    // Nastav valueof_* a defined_* pro každý vstup
    for (const auto& input : net.inputs) {
        set_extern_string("valueof_" + input.name, input.last_known_value);
        set_extern_bool("defined_" + input.name, input.has_been_set);
    }

    // Nastav tokens_* a elapsed_* pro každé místo
    for (const auto& place : net.places) {
        set_extern_int("tokens_" + place.id, place.current_tokens);

        int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - place.last_token_change
        ).count();
        set_extern_int("elapsed_" + place.id, elapsed);
    }

    // Nastav elapsed_* pro přechody (doba nepřetržité proveditelnosti)
    for (const auto& t : net.transitions) {
        if (t.is_delayed() && t.last_enabled_change != std::chrono::steady_clock::time_point{}) {
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - t.last_enabled_change
            ).count();
            set_extern_int("elapsed_" + t.id, elapsed);
        } else {
            set_extern_int("elapsed_" + t.id, 0);
        }
    }

    // Nastav var_* pro každou proměnnou
    for (const auto& var : net.variables) {
        if (var.type == "int") {
            int val = var.current_value.empty() ? 0 : std::stoi(var.current_value);
            set_extern_int("var_" + var.name, val);
        } else {
            set_extern_string("var_" + var.name, var.current_value);
        }
    }

    // Nastav now_ms
    set_extern_int64("now_ms", now_ms);
}

void InscriptionEngine::sync_back_variables(PetriNet& net)
{
    if (!lib_handle_) return;

    for (auto& var : net.variables) {
        if (var.type == "int") {
            void* ptr = get_symbol("var_" + var.name);
            if (ptr) {
                var.current_value = std::to_string(*static_cast<int*>(ptr));
            }
        } else {
            void* ptr = get_symbol("var_" + var.name);
            if (ptr) {
                const char* val = *static_cast<const char**>(ptr);
                if (val) var.current_value = val;
            }
        }
    }
}

void InscriptionEngine::set_output_callback(OutputFunc callback)
{
    output_callback_ = std::move(callback);
}

void InscriptionEngine::invoke_output_callback(const std::string& name, const std::string& value)
{
    if (output_callback_) {
        output_callback_(name, value);
    }
}

// Vyhodnocení stráže / akce

bool InscriptionEngine::evaluate_guard(const Transition& t)
{
    if (!t.has_guard()) {
        return true;  // Žádná stráž = vždy splněno
    }

    if (!lib_handle_) {
        return false;  // Knihovna není načtena
    }

    // Získej pointer na funkci guard_<id>
    std::string func_name = "guard_" + t.id;
    void* func_ptr = get_symbol(func_name);
    if (!func_ptr) {
        return false;
    }

    // Zavolej - signatura: bool (*)();
    using GuardFunc = bool (*)();
    auto fn = reinterpret_cast<GuardFunc>(func_ptr);
    return fn();
}

void InscriptionEngine::execute_transition_action(const Transition& t)
{
    if (!t.action_code.has_value()) {
        return;
    }

    if (!lib_handle_) {
        return;
    }

    std::string func_name = "action_" + t.id;
    void* func_ptr = get_symbol(func_name);
    if (!func_ptr) {
        return;
    }

    using ActionFunc = void (*)();
    auto fn = reinterpret_cast<ActionFunc>(func_ptr);
    fn();
}

void InscriptionEngine::execute_place_action(const Place& p)
{
    if (!p.place_action.has_value()) {
        return;
    }

    if (!lib_handle_) {
        return;
    }

    std::string func_name = "place_action_" + p.id;
    void* func_ptr = get_symbol(func_name);
    if (!func_ptr) {
        return;
    }

    using ActionFunc = void (*)();
    auto fn = reinterpret_cast<ActionFunc>(func_ptr);
    fn();
}

int64_t InscriptionEngine::evaluate_delay(const Transition& t)
{
    // Pokud je delay konstantní číslo, vrať ho přímo
    if (t.delay_ms.has_value()) {
        return t.delay_ms.value();
    }

    // Dynamický výraz – zavolej JIT funkci delay_<id>()
    if (t.delay_expr.has_value() && lib_handle_) {
        std::string func_name = "delay_" + t.id;
        void* func_ptr = get_symbol(func_name);
        if (func_ptr) {
            using DelayFunc = int64_t (*)();
            auto fn = reinterpret_cast<DelayFunc>(func_ptr);
            return fn();
        }
    }

    return 0;  // Fallback
}

// Generování C++ zdrojáku

std::string InscriptionEngine::generate_source(const PetriNet& net) const
{
    std::ostringstream ss;

    ss << "// ========================================================\n";
    ss << "// Auto-generated Petri Net Code\n";
    ss << "// Net: " << net.name << "\n";
    ss << "// Generated by pets::InscriptionEngine\n";
    ss << "// DO NOT EDIT MANUALLY\n";
    ss << "// ========================================================\n\n";

    ss << "#include <cstdint>\n";
    ss << "#include <cstring>\n";
    ss << "#include <cstdlib>\n";
    ss << "#include <string>\n\n";

    ss << generate_context_externs(net);
    ss << generate_builtin_wrappers(net);

    ss << "\n// Guards\n\n";
    for (const auto& t : net.transitions) {
        if (t.has_guard()) {
            ss << generate_guard_function(t) << "\n";
        }
    }

    ss << "\n// Transition Actions\n\n";
    for (const auto& t : net.transitions) {
        if (t.action_code.has_value()) {
            ss << generate_action_function("action_" + t.id, t.action_code.value()) << "\n";
        }
    }

    ss << "\n// Place Actions\n\n";
    for (const auto& p : net.places) {
        if (p.place_action.has_value()) {
            ss << generate_action_function("place_action_" + p.id,
                                           p.place_action.value()) << "\n";
        }
    }

    return ss.str();
}

std::string InscriptionEngine::generate_context_externs(const PetriNet& net) const
{
    std::ostringstream ss;
    ss << "// ==================== Context Variables ====================\n";
    ss << "// These are set by InscriptionEngine::setup_context()\n";
    ss << "// before each guard/action call.\n\n";
    ss << "extern \"C\" {\n";

    for (const auto& input : net.inputs) {
        ss << "    const char* valueof_" << input.name << " = nullptr;\n";
        ss << "    bool        defined_" << input.name << " = false;\n";
    }

    for (const auto& place : net.places) {
        ss << "    int tokens_" << place.id << " = 0;\n";
        ss << "    int elapsed_" << place.id << " = 0;\n";
    }

    // Elapsed pro přechody
    for (const auto& t : net.transitions) {
        ss << "    int elapsed_" << t.id << " = 0;\n";
    }

    for (const auto& var : net.variables) {
        if (var.type == "int") {
            std::string init_val = var.initial_value.empty() ? "0" : var.initial_value;
            ss << "    int var_" << var.name << " = " << init_val << ";\n";
        } else {
            ss << "    const char* var_" << var.name << " = \"" << var.initial_value << "\";\n";
        }
    }

    ss << "    int64_t now_ms = 0;\n";
    ss << "\n";

    // Output callback - funkce pointer nastavený zvenčí
    ss << "    void (*output_func)(const char* name, const char* value) = nullptr;\n";

    ss << "} // extern \"C\"\n\n";

    return ss.str();
}

std::string InscriptionEngine::generate_builtin_wrappers(const PetriNet& net) const
{
    std::ostringstream ss;
    ss << "// ==================== Builtin Wrappers ====================\n\n";

    ss << "#include <cstdlib>\n";
    ss << "#include <cstring>\n\n";

    // ====== output() – dvě přetížení: (const char*, const char*) a (const char*, int) ======
    ss << "static void output(const char* name, const char* value)\n";
    ss << "{\n";
    ss << "    if (output_func) {\n";
    ss << "        output_func(name, value);\n";
    ss << "    }\n";
    ss << "}\n\n";

    ss << "static void output(const char* name, int value)\n";
    ss << "{\n";
    ss << "    char buf[32];\n";
    ss << "    snprintf(buf, sizeof(buf), \"%d\", value);\n";
    ss << "    output(name, buf);\n";
    ss << "}\n\n";

    // ====== valueof("in_name") – mapovací funkce ======
    ss << "static const char* valueof(const char* name)\n";
    ss << "{\n";
    for (const auto& input : net.inputs) {
        ss << "    if (strcmp(name, \"" << input.name << "\") == 0) return valueof_" << input.name << ";\n";
    }
    ss << "    return \"\";\n";
    ss << "}\n\n";

    // ====== defined("in_name") ======
    ss << "static bool defined(const char* name)\n";
    ss << "{\n";
    for (const auto& input : net.inputs) {
        ss << "    if (strcmp(name, \"" << input.name << "\") == 0) return defined_" << input.name << ";\n";
    }
    ss << "    return false;\n";
    ss << "}\n\n";

    // ====== tokens("place") ======
    ss << "static int tokens(const char* name)\n";
    ss << "{\n";
    for (const auto& place : net.places) {
        ss << "    if (strcmp(name, \"" << place.id << "\") == 0) return tokens_" << place.id << ";\n";
    }
    ss << "    return 0;\n";
    ss << "}\n\n";

    // ====== elapsed("place_or_transition") ======
    ss << "static int elapsed(const char* name)\n";
    ss << "{\n";
    for (const auto& place : net.places) {
        ss << "    if (strcmp(name, \"" << place.id << "\") == 0) return elapsed_" << place.id << ";\n";
    }
    for (const auto& t : net.transitions) {
        ss << "    if (strcmp(name, \"" << t.id << "\") == 0) return elapsed_" << t.id << ";\n";
    }
    ss << "    return 0;\n";
    ss << "}\n\n";

    // ====== now() ======
    ss << "static int64_t now()\n";
    ss << "{\n";
    ss << "    return now_ms;\n";
    ss << "}\n\n";

    // ====== Aliasy proměnných: timeout → var_timeout ======
    ss << "// ==================== Variable Aliases ====================\n";
    ss << "// Umožňuje psát 'timeout' místo 'var_timeout' v inskripčním kódu.\n\n";
    for (const auto& var : net.variables) {
        if (var.type == "int") {
            ss << "static int& " << var.name << " = var_" << var.name << ";\n";
        } else {
            ss << "static const char*& " << var.name << " = var_" << var.name << ";\n";
        }
    }
    ss << "\n";

    // ====== Delay resolver funkce pro dynamické zpoždění ======
    ss << "// ==================== Delay Resolvers ====================\n\n";
    for (const auto& t : net.transitions) {
        if (t.delay_expr.has_value() && !t.delay_ms.has_value()) {
            // Dynamický výraz – generuj funkci
            ss << "extern \"C\" int64_t delay_" << t.id << "()\n";
            ss << "{\n";
            ss << "    return static_cast<int64_t>(" << t.delay_expr.value() << ");\n";
            ss << "}\n\n";
        }
    }

    return ss.str();
}

std::string InscriptionEngine::generate_guard_function(const Transition& t) const
{
    std::ostringstream ss;
    ss << "extern \"C\" bool guard_" << t.id << "()\n";
    ss << "{\n";
    ss << "    return (" << t.guard_expression.value() << ");\n";
    ss << "}\n";

    return ss.str();
}

std::string InscriptionEngine::generate_action_function(const std::string& func_name,
                                                         const std::string& code) const
{
    std::ostringstream ss;
    ss << "extern \"C\" void " << func_name << "()\n";
    ss << "{\n";

    // Stejně jako u stráže - kód musí být platné C++ používající
    // extern proměnné valueof_X, defined_X, tokens_X, elapsed_X, now_ms.
    ss << code << "\n";

    ss << "}\n";

    return ss.str();
}

// Dynamická kompilace a načítání

bool InscriptionEngine::compile_to_shared_lib(const std::string& src_path,
                                              const std::string& lib_path,
                                              std::string& error_msg)
{
#ifdef _WIN32
    std::string cmd = "g++ -std=c++17 -shared -O2 -o \"" + lib_path + "\" \"" + src_path + "\" 2>&1";
#else
    std::string cmd = "g++ -std=c++17 -shared -fPIC -O2 -o \"" + lib_path + "\" \"" + src_path + "\" 2>&1";
#endif

    // TODO: Přidat lepší detekci cesty ke kompilátoru, popen nemusí vždy fungovat spolehlivě.
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        error_msg = "Cannot execute compiler (g++ nenalezen?)";
        return false;
    }

    std::ostringstream output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    int ret = pclose(pipe);

    if (ret != 0) {
        error_msg = "Compilation failed:\n" + output.str();
        return false;
    }

    return true;
}

bool InscriptionEngine::load_library(const std::string& lib_path,
                                     std::string& error_msg)
{
    lib_handle_ = DLOPEN(lib_path.c_str());
    if (!lib_handle_) {
        error_msg = "dlopen failed: " + std::string(DLERROR());
        return false;
    }



    return true;
}

void* InscriptionEngine::get_symbol(const std::string& name)
{
    if (!lib_handle_) return nullptr;

    // Mrkni do cache
    auto it = symbol_cache_.find(name);
    if (it != symbol_cache_.end()) {
        return it->second;
    }

    void* ptr = DLSYM(lib_handle_, name.c_str());
    if (ptr) {
        symbol_cache_[name] = ptr;
    }
    return ptr;
}

// Settery kontextových proměnných

void InscriptionEngine::set_extern_int(const std::string& name, int value)
{
    if (!lib_handle_) return;
    void* ptr = get_symbol(name);
    if (ptr) {
        *static_cast<int*>(ptr) = value;
    }
}

void InscriptionEngine::set_extern_bool(const std::string& name, bool value)
{
    if (!lib_handle_) return;
    void* ptr = get_symbol(name);
    if (ptr) {
        *static_cast<bool*>(ptr) = value;
    }
}

void InscriptionEngine::set_extern_string(const std::string& name, const std::string& value)
{
    if (!lib_handle_) return;
    void* ptr = get_symbol(name);
    if (ptr) {
        // FIXME: Udržovat paměť vyčištěnou, list string_storage může nekontrolovaně růst.
        static std::list<std::string> string_storage;
        string_storage.push_back(value);
        *static_cast<const char**>(ptr) = string_storage.back().c_str();
    }
}

void InscriptionEngine::set_extern_int64(const std::string& name, int64_t value)
{
    if (!lib_handle_) return;
    void* ptr = get_symbol(name);
    if (ptr) {
        *static_cast<int64_t*>(ptr) = value;
    }
}

} // namespace pets