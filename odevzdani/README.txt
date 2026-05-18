Nástroj pro vizuální editaci, generování kódu a monitorování Petriho sítí
==========================================================================

Autoři:
  - Lukáš Dudek  (xdudekl00)
  - Jan Raška    (xraskaj00)
  - Matěj Zaoral (xzaoram00)


Popis projektu
--------------
Aplikace implementuje editor, JIT interpret a runtime monitor interpretovaných
event-driven časovaných Petriho sítí v jazyce C++17 s využitím frameworku Qt5.

Uživatel může vizuálně navrhnout síť (místa, přechody, orientované hrany
s vahami), definovat vstupy, výstupy a interní proměnné, specifikovat podmínky
přechodů a akce v inskripčním jazyce (podmnožina C/C++) a jedním kliknutím
vygenerovat kód interpretu, přeložit ho a spustit. Za běhu lze asynchronně
injektovat vstupní hodnoty a v reálném čase sledovat kompletní stav sítě.


Závislosti
----------
  - Qt5 (Core, Gui, Widgets) – minimálně verze 5.5
  - C++17 kompilátor (g++) – potřebný jak pro překlad projektu, tak pro
    JIT kompilaci akcí/stráží za běhu
  - GNU Make
  - Doxygen (volitelně, pro generování dokumentace)


Překlad a spuštění
-------------------
  make            – přeloží projekt
  make run        – přeloží a spustí aplikaci
  make clean      – smaže produkty překladu
  make pack       – vytvoří archiv pro odevzdání
  make doxygen    – vygeneruje HTML dokumentaci do doc/


Struktura projektu
------------------
  src/model/        – datový model sítě (PetriNet, Place, Transition, Arc,
                      Input, Output, Variable, NetSerializer)
  src/interpreter/  – interpret a JIT engine (Interpreter, InscriptionEngine,
                      EventQueue, TimerManager)
  src/gui/          – Qt5 grafické rozhraní (MainWindow, canvas, widgety)
  examples/         – příklady sítí ve formátu .petri
  doc/              – konceptuální návrh (class_diagram.pdf), Doxygen výstup


Implementovaná funkcionalita – KOMPLETNÍ
-----------------------------------------

1) Vizuální editor
   - Přidávání a mazání míst (kroužky) a přechodů (obdélníky).
   - Kreslení orientovaných hran s vahami (šipky s číslem).
   - Editace vlastností dvojklikem (identifikátor, tokeny, akce místa,
     vstupní událost, stráž, zpoždění, akce přechodu, priorita).
   - Editace vstupů, výstupů a interních proměnných v hlavním okně.

2) Textový formát sítě (.petri)
   - Čitelný textový soubor, snadno editovatelný ručně.
   - Uložení (File → Save / Save As) a načtení (File → Open).
   - Obsahuje: jméno sítě, komentář, vstupy, výstupy, proměnné,
     místa (s tokeny a place actions), přechody (hrany, podmínky, akce).

3) Inskripční jazyk (podmnožina C/C++)
   - JIT kompilace: z definice sítě se vygeneruje C++ zdrojový kód,
     zkompiluje přes g++ do sdílené knihovny (.so) a načte přes dlopen.
   - Všechny vestavěné funkce dle zadání:
       valueof("in_name")     → const char*  (poslední známá hodnota vstupu)
       defined("in_name")     → bool         (zda byl vstup někdy nastaven)
       output("out_name", x)  → void         (vygeneruje výstupní událost)
       tokens("place")        → int          (aktuální počet tokenů v místě)
       elapsed("place")       → int          (ms od poslední změny tokenů)
       elapsed("transition")  → int          (ms nepřetržité proveditelnosti)
       now()                  → int64        (ms od startu sítě)
   - Proměnné jsou přístupné přímo pod svým jménem (např. "timeout"),
     engine generuje C++ aliasy automaticky.
   - Place actions – akce místa se provádí při každém přidání tokenu.

4) Sémantika odpálení
   - Přechod je povolen pokud: dostatek tokenů, splněna událost, splněna stráž.
   - Při odpálení: odebrání tokenů → akce přechodu → přidání tokenů → place actions.
   - Nezávislé odpálení: v jedné reakci se odpálí maximální množina nezávislých
     přechodů (shadow token counting).
   - Konflikty řešeny deterministicky dle priority (nižší číslo = vyšší priorita).
   - Stabilizační smyčka (mikrokroky) – opakuje se dokud existují proveditelné
     okamžité přechody.

5) Zpožděné přechody
   - Podpora numerických i dynamických zpoždění (@ 5000 i @ timeout).
   - Přechody s input_event_name se plánují jen jako reakce na událost.
   - Přechody bez input_event_name se plánují kdykoli jsou povoleny.
   - Po vypršení časovače se ověří povolení – pokud nepovolen, timeout ignorován.
   - Aktivní čištění časovačů při odpálení přechodu.

6) Hlavní smyčka interpretu
   - Implementuje přesně algoritmus ze zadání:
     stabilize → schedule_delayed → wait → handle event → repeat.
   - Pasivní čekání (condition variable) na vstup, timeout nebo stop.
   - Zpracování událostí: TIMEOUT, INPUT, STOP.

7) Runtime monitor (za běhu)
   - Živé počty tokenů přímo v diagramu.
   - Barevné zvýraznění enabled přechodů (zelená) a pending timeoutů (oranžová).
   - Přehled hodnot vstupů, výstupů a proměnných (StateOverviewPanel).
   - Tabulka naplánovaných timeoutů s ID přechodu a zbývajícím časem.
   - Log událostí s časovým razítkem: vstupy, odpálení, timeouty, výstupy,
     změny tokenů.

8) Injektování vstupů
   - InjectionPanel umožňuje kdykoli za běhu zadat hodnotu libovolného vstupu.
   - Asynchronní zpracování přes EventQueue.

9) Generování kódu
   - Jedním kliknutím (Run → Generate & Run) se vygeneruje C++ kód interpretu,
     přeloží do sdílené knihovny a spustí.
   - Komunikace přes sdílenou knihovnu (dlopen/dlsym), ne UDP sockety.


Implementovaná funkcionalita – S OMEZENÍMI
-------------------------------------------

1) Sync proměnných
   - Po provedení akce se proměnné synchronizují zpět do modelu
     (sync_back_variables). Podporovány jsou typy int a string.
     Složitější datové typy (pole, struktury) nejsou podporovány.

2) Parser .petri formátu
   - Zpracovává standardní strukturu. Velmi složité vnořené C++ výrazy
     obsahující znaky interferující s formátem (např. '}' uvnitř akce)
     mohou vyžadovat opatrnost.


Neimplementovaná funkcionalita
-------------------------------

1) Připojení k běžící síti při startu
   - Zadání popisuje možnost "při startu se pokusit připojit k běžící síti".
     Toto není implementováno. Interpret běží jako vlákno uvnitř GUI aplikace
     s komunikací přes sdílenou knihovnu, nikoliv jako samostatný síťový proces.
     Zadání na řádku 106 explicitně uvádí, že "spuštění a komunikace může být
     řešena i jinak – např. sdílenou knihovnou", což je náš přístup.

2) UDP sockety
   - Komunikace mezi GUI a interpretem je řešena in-process (sdílená knihovna +
     callbacky), nikoli přes síťové sockety.


Příklady sítí
-------------
  examples/TOF_PN.petri         – Timer Off-Delay (kompletní příklad ze zadání)
  examples/TOF_PN_5s.petri      – TOF s pevným 5s timeoutem
  examples/simple_counter.petri – jednoduchý čítač
  examples/output_test.petri    – test výstupních událostí
  examples/test.petri           – TOF_PN pro testování


Licence a převzatý kód
-----------------------
Veškerý kód byl vytvořen členy týmu. Žádný externí kód (kromě standardních
knihoven Qt5 a C++ STL) nebyl použit ani převzat.
