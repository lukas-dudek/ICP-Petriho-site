Nástroj pro vizuální editaci a monitorování Petriho sítí
=========================================================

Autoři:
- Lukáš (xname01)
- Jan (xname02)
- Matěj (xname03)

Popis:
Tento projekt implementuje editor, interpret a monitor interpretovaných Petriho sítí 
v jazyce C++ s využitím frameworku Qt5. Aplikace umožňuje vizuální návrh sítí, 
vkládání C/C++ kódu do akcí a stráží (tzv. inskripční jazyk) a asynchronní běh.

Závislosti:
- Qt5 (Core, Gui, Widgets)
- C++17 kompilátor (g++)
- make
- (volitelně) Doxygen pro generování dokumentace

Překlad a spuštění:
1. `make` (přeloží projekt)
2. `make run` (přeloží a spustí aplikaci)
3. `make clean` (smaže dočasné soubory)
4. `make pack` (vytvoří archiv pro odevzdání)
5. `make doxygen` (vygeneruje HTML dokumentaci do složky doc/)

Implementovaná funkcionalita:
- Kompletní vizuální editor (Místa, Přechody, Hrany s vahami)
- Serializace a deserializace sítě (textový formát .petri)
- Podpora vstupních událostí a vkládání (injektování) vstupů
- Inskripční jazyk plně podporující C/C++ díky JIT kompilaci za běhu. 
  - Podpora proměnných v tělech akcí (do: { ... }) a ve strážích (when: event [ guard ])
- Odpočet časovačů (přechody se zpožděním) a asynchronní exekuce událostí na pozadí.

Omezení / Známé limity:
- U zpožděných přechodů (when: @ delay) parser očekává číselnou konstantu (např. @ 5000).
  Vyhodnocování dynamických proměnných přímo u zpoždění (např. @ timeout) není z 
  důvodu zjednodušení implementováno.
- Pro běh sítě (generování sdílených knihoven akcí a stráží) aplikace vyžaduje
  přístup ke kompilátoru gcc/g++ za běhu (JIT kompilace vytváří .so knihovny).

Licence a převzatý kód:
- Veškerý kód byl vytvořen členy týmu. Žádný externí kód (kromě standardních 
  Qt5 knihoven a C++ STL) nebyl použit.
