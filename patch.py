import sys

def de_ai_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Doxygen headers
    if filepath.endswith('interpreter.cpp'):
        content = content.replace("/**\n * @file    interpreter.cpp\n * @brief   Implementace hlavní smyčky interpretované Petriho sítě.\n * @author  Lukáš\n * @date    2026-05-04\n *\n * Integruje:\n *  - InscriptionEngine (JIT kompilace C++ výrazů)\n *  - TimerManager (zpožděné přechody)\n *  - EventQueue (pasivní čekání s timeoutem)\n *  - Deterministický výběr max nezávislé množiny\n */\n", "// interpreter.cpp - behova smycka\n")
    
    # Block separators
    content = content.replace("// ======================================================================\n", "")
    content = content.replace("// Konstruktor / Destruktor\n", "// konstruktor a destruktor\n")
    content = content.replace("// Životní cyklus\n", "// rizeni smycky\n")
    content = content.replace("// Vstup z GUI\n", "// gui vstupy\n")
    content = content.replace("// Callbacky pro GUI\n", "// gui callbacky\n")
    content = content.replace("// Hlavní smyčka\n", "// hlavni beh\n")
    content = content.replace("// (1) Reakce bez čekání – stabilizace\n", "// stabilizace (okamzite prechody)\n")
    content = content.replace("// === (1) Reakce bez čekání – stabilizace ===\n", "// stabilizace\n")
    content = content.replace("// === (2) Naplánování zpožděných přechodů ===\n", "// casovace\n")
    content = content.replace("// === (3) Pasivní čekání (s timeoutem pro časovače) ===\n", "// cekani\n")
    content = content.replace("// === (4) Zpracování probuzení ===\n", "// probuzeni\n")
    content = content.replace("// Vyhledání enabled okamžitých přechodů\n", "")
    content = content.replace("// Kontrola, zda je přechod enabled\n", "")
    content = content.replace("// Maximální množina nezávislých přechodů\n", "")
    content = content.replace("// Odpálení přechodu (fire)\n", "")
    content = content.replace("// (2) Naplánování zpožděných přechodů\n", "")
    content = content.replace("// (4a) Zpracování timeoutu\n", "")
    content = content.replace("// (4b) Zpracování vstupní události\n", "")
    content = content.replace("// Logování a notifikace GUI\n", "")
    
    # memory ordering
    content = content.replace("running_.load(std::memory_order_acquire)", "running_.load()")
    content = content.replace("running_.store(true, std::memory_order_release)", "running_.store(true)")
    content = content.replace("running_.store(false, std::memory_order_release)", "running_.store(false)")

    # method renaming
    content = content.replace("do_reaction_without_waiting", "stabilize")
    content = content.replace("schedule_delayed_transitions", "schedule_delayed")
    content = content.replace("find_enabled_immediate_transitions", "get_enabled")
    content = content.replace("find_max_independent_set", "get_independent_set")

    # comments logic simplifications
    content = content.replace("// 1. Odeber tokeny", "// vezmi tokeny")
    content = content.replace("// 2. Proveď akci přechodu (InscriptionEngine)", "// proved akci")
    content = content.replace("// 3. Přidej tokeny", "// pridej tokeny")
    content = content.replace("// 4. Place action – při každém přidání tokenu", "// akce mista")
    content = content.replace("// 5. Zaloguj odpálení", "// zaloguj")
    content = content.replace("// 6. Zruš případné časovače pro tento přechod\n    //    (doporučená sémantika: timeout už není potřeba)", "// zrus casovace")

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == "__main__":
    for arg in sys.argv[1:]:
        de_ai_file(arg)
