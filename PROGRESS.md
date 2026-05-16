# Stav implementace projektu (PROGRESS)

Tento dokument mapuje všechny požadavky specifikované v `zadani.txt` vůči reálnému stavu naší aplikace.

---

## 1. Interpretovaná Petriho síť (Sémantika a základy)
- [x] **HOTOVO** - Event-driven (reaktivní) chování (čekání na event/timeout namísto neustálého cyklení).
- [x] **HOTOVO** - Multiset sémantika (místa obsahují počet tokenů 0..N, tokeny jsou nerozlišené).
- [x] **HOTOVO** - Vstupní události (asynchronní injektování hodnot z GUI do sítě).
- [x] **HOTOVO** - Interní proměnné a akce (podpora globálních proměnných v C/C++ kontextu).
- [x] **HOTOVO** - Strážní podmínky přechodů (vyhodnocování boolean výrazů v C/C++).
- [x] **HOTOVO** - Generování výstupních událostí.
- [x] **HOTOVO** - Plánování zpožděných odpálení (timeouty a zpožděné přechody).

---

## 2. Prvky sítě
- [x] **HOTOVO** - **Místa (Places):** Identifikátor, počáteční počet tokenů, volitelná akce místa.
- [x] **HOTOVO** - **Přechody (Transitions):** Identifikátor.
- [x] **HOTOVO** - **Hrany (Arcs):** Vstupní a výstupní hrany s explicitní vahou (w >= 1).
- [x] **HOTOVO** - **Podmínka odpálení:** Složena volitelně ze 3 částí: `událost [ stráž ] @ zpoždění`.
- [x] **HOTOVO** - **Akce přechodu:** Vykonání vlastní C/C++ akce při odpálení (`do: { ... }`).

---

## 3. Inskripční jazyk (JIT Kompilace C/C++)
Implementováno vytvořením vlastní JIT (Just-In-Time) kompilace do dynamických knihoven na pozadí, díky čemuž je podporováno čisté C/C++.
- [x] **HOTOVO** - `valueof("in_name")` (Získání hodnoty vstupu).
- [x] **HOTOVO** - `defined("in_name")` (Zda byl vstup vůbec aktivován).
- [x] **HOTOVO** - `output("out_name", x)` (Generování výstupu).
- [x] **HOTOVO** - `tokens("place")` (Zjištění počtu tokenů).
- [x] **HOTOVO** - `elapsed("place")` (Čas od poslední změny v místě).
- [x] **HOTOVO** - `now()` (Čas běhu sítě v milisekundách).

---

## 4. Nástroj (GUI a monitorování)
- [x] **HOTOVO** - Vizuální specifikace sítě (Místa jako elipsy, přechody jako obdélníky, šipky s vahami).
- [x] **HOTOVO** - Editace všech prvků přes GUI (Vstupy, výstupy, proměnné, podmínky, akce).
- [x] **HOTOVO** - Textový formát ukládání: serializace a deserializace vlastního čitelného `.petri` formátu.
- [x] **HOTOVO** - Jedním kliknutím (`Run`) vygenerování, přeložení a spuštění sítě na pozadí.
- [x] **HOTOVO** - Živé zobrazení počtů tokenů v místech přímo v diagramu.
- [x] **HOTOVO** - Živé zvýraznění povolených (enabled) a čekajících (pending timers) přechodů.
- [x] **HOTOVO** - Přehled hodnot vstupů, výstupů a interních proměnných (State Overview Panel).
- [x] **HOTOVO** - Log událostí: zaznamenávání časů, odpálení, timeoutů a změn tokenů.
- [x] **HOTOVO** - Asynchronní vkládání vstupů (Injection panel).
- [x] **HOTOVO** - Bezpečné ukončení běžící sítě při požadavku z uživatelského rozhraní.

---

## 5. Sémantika odpálení a algoritmus smyčky
Byl striktně dodržen "Algoritmus provádění interpretované sítě" popsaný v zadání:
- [x] **HOTOVO** - **Krok 1 (Reakce bez čekání):** Vybrání maximální nezávislé množiny kandidátů a její odpálení až do stabilizace sítě.
- [x] **HOTOVO** - **Krok 2 (Plánování):** Správná implementace odpočtu zpožděných přechodů bez konfliktu na časovačích.
- [x] **HOTOVO** - **Krok 3 (Pasivní čekání):** Vlákno se reálně uspává (pomocí condition variables), nehltá procesor (no busy-waiting).
- [x] **HOTOVO** - Deterministické řešení konfliktů (využití priorit u přechodů v případě konfliktu o stejný token).
- [x] **HOTOVO** - Bezpečné čištění vypršených časovačů ("timeout ignored", pokud mezitím přechod ztratil tokeny).

---

## 6. Odevzdání a technické požadavky
- [x] **HOTOVO** - Projekt přeložitelný pouhým `make` bez absolutních cest v `Makefile`.
- [x] **HOTOVO** - Multiplatformní toolkit (`Qt5`), C++17.
- [x] **HOTOVO** - Architektura: Oddělení logiky (Model), GUI a Interpretru (viz UML).
- [x] **HOTOVO** - `make doxygen` generuje plnohodnotnou HTML dokumentaci s `SOURCE_BROWSER=YES`.
- [x] **HOTOVO** - Konceptuální návrh uložen ve složce `doc/` jako UML diagram (`.pdf` / `.puml`).
- [x] **HOTOVO** - Vytvořen soubor `README.txt`.
- [x] **HOTOVO** - Přítomnost autorských komentářů (`@author xname0X`) v hlavičkách všech zdrojových kódů.
- [x] **HOTOVO** - Možnost sestavit odevzdávkový archiv příkazem `make pack`.
