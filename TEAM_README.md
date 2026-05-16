# Příručka pro tým: Jak funguje náš editor a interpret Petriho sítí

Tento dokument slouží pro rychlé pochopení architektury a fungování našeho projektu. Je napsán tak, aby do něj mohl nahlédnout kdokoli, kdo aplikaci vidí poprvé, a ihned pochopil, jak se spouští, jak se ovládá a jak funguje "pod kapotou".

---

## 1. Jak aplikaci přeložit a spustit

Aplikace je napsaná v C++17 s využitím frameworku **Qt5**. Ke kompilaci používáme standardní `make`.

*   **Závislosti:** Nainstalované knihovny Qt5 (Core, Gui, Widgets) a C++ kompilátor (`g++`). Pro dynamické načítání kódů se využívá `dlopen`.
*   **Překlad:** Stačí v terminálu ve složce projektu zavolat:
    ```bash
    make
    ```
*   **Spuštění:** Pro rovnou přeložení a zapnutí aplikace přes jedinou zkratku volejte:
    ```bash
    make run
    ```
*   *(Tip: Příkazem `make pack` se vygeneruje `.zip` archiv připravený rovnou k odevzdání.)*

---

## 2. Uživatelský manuál: Jak pracovat s aplikací

Když se aplikace po `make run` zapne, uvidíte grafické okno s plátnem (Canvas) uprostřed a sadou panelů po pravé a levé straně. Zde je návod, k čemu jednotlivé části slouží a jak se používají:

### 🖌 Kreslící plátno (Canvas)
Plátno slouží k vizuálnímu návrhu Petriho sítě.
*   **Přidání Místa (Kruh):** Klikněte **pravým tlačítkem** kamkoliv do prázdného místa na plátně.
*   **Přidání Přechodu (Čtverec):** Podržte `Shift` a klikněte **pravým tlačítkem** na prázdné místo.
*   **Kreslení hran (Šipek):** Klikněte pravým tlačítkem na jakýkoliv existující uzel (Místo/Přechod), držte tlačítko a potáhněte myší k jinému uzlu. Hrana se vytvoří sama.
*   **Posouvání uzlů:** Levým tlačítkem chytněte uzel a táhněte. Připojené hrany se budou plynule posouvat spolu s ním.
*   **Editace vlastností:** **Dvojklikem** (levým tlačítkem) na Místo, Přechod nebo Hranu se otevře editační okno.
    *   *U Místa* můžete nastavit počáteční tokeny a jméno.
    *   *U Přechodu* můžete napsat jeho podmínku (Guard), akci (do: `{...}`) v jazyce C++ nebo nastavit zpoždění (Delay).
    *   *U Hrany* můžete změnit její váhu (pokud je váha > 1, vykreslí se na šipce).

### ⚙️ Konfigurace sítě (Vlevo nahoře)
*   **Vstupy, Výstupy, Proměnné:** Zde definujete "rozhraní" sítě. Můžete zde přidat například proměnnou `int counter = 0;` (tu pak můžete používat uvnitř akcí přechodů jako by to byl běžný C++ kód!). Nebo zde definujete vstupy (např. tlačítko), které do sítě později injektujete.
*   Veškeré parametry musí být oddělené čárkou, formát proměnných je `typ nazev = hodnota` (např. `int timeout = 5000`).

### ▶️ Horní lišta (Toolbar)
*   **Run / Stop:** Zapíná a vypíná exekuci (běh) sítě. Po kliknutí na Run plátno zešedne (editor se zamkne), aplikace vytvoří C++ sdílené knihovny z vašich skriptů a spustí se.
*   **Open / Save:** Ukládá celou nakreslenou síť (včetně rozložení na plátně) do přehledného textového formátu `.petri`, který specifikuje zadání. Zkuste si rovnou načíst hotové sítě ze složky `examples/`.

### ⚡ Injektování vstupů a Logy (Vpravo)
*   **Injection Panel:** Protože je síť tzv. "reaktivní" (Event-driven), za běhu většinou tiše spí a čeká. Pokud jste v levém panelu nadefinovali nějaký "Vstup" (např. `in`), z roletky v tomto panelu si ho vyberete, do políčka napíšete hodnotu a kliknete na **Injektovat**. Tím simulujete to, že uživatel (nebo jiný systém) venku něco udělal. Síť se probudí, vyhodnotí, jestli na daný vstup nějaký přechod nereaguje, a případně se odpálí.
*   **Log Panel:** Úplně dole vpravo se loguje každý krok sítě – od přesunů tokenů až po odpočty časovačů.

### 🔍 Přehled stavu sítě (Vpravo dole)
Během simulace (po kliknutí na Run) v tomto panelu vidíte všechny živé informace: 
*   **Tokeny:** Kolik je kde aktuálně tokenů.
*   **Proměnné:** Jaké jsou aktuální hodnoty C++ proměnných (např. při každém běhu přechodu můžete vidět, jak se zvedá váš `counter`).
*   **Timeouty:** Pokud nějaký přechod čeká na odpálení (např. zpoždění 5000 ms), zde uvidíte, kolik milisekund mu ještě zbývá. Přechod se na plátně po dobu odpočtu vybarví oranžově.

---

## 3. Architektura aplikace (Jak je to naprogramované)

Aplikace striktně dodržuje oddělení logiky (Model), uživatelského rozhraní (GUI) a exekuce (Interpreter).

### 🗂 `src/model/` (Datový model)
Základní datové struktury bez jakékoliv závislosti na GUI nebo spouštění.
* **`PetriNet`**: Hlavní obálka sítě obsahující seznamy míst (`Place`), přechodů (`Transition`), vstupů, výstupů a interních proměnných.
* **`Place` / `Transition` / `Arc`**: Třídy reprezentující jednotlivé prvky. Obsahují vlastnosti jako počáteční počet tokenů, C++ akce (např. `do: { ... }`) nebo stráže (např. `when: in [ ... ]`).
* **`NetSerializer`**: Slouží k ukládání sítě do textového `.petri` souboru a jejímu načítání zpět. Jde o formát vyžadovaný zadáním.

### 🎨 `src/gui/` (Grafické rozhraní)
Postaveno na frameworku **Qt5 (Graphics View Framework)**.
* **`MainWindow`**: Hlavní okno aplikace. Sdružuje plátno, logy a postranní panely.
* **`PetriNetScene`**: Spravuje plátno a kreslení uzlů (`PlaceItem`, `TransitionItem`) a hran (`ArcItem`).
* **Zpracování Run**: Po kliknutí na tlačítko "Run" se plátno z GUI nejprve *zeserializuje* (vyexportuje) do interního modelu a ten se pošle interpretu. Tím je zajištěno, že interpret pracuje vždy s aktuální sítí nezávisle na GUI.

### ⚙️ `src/interpreter/` (Výpočetní jádro)
Běží **asynchronně na vlastním vlákně na pozadí**, aby nezasekávalo GUI.
* **`Interpreter`**: Hlavní mozek sítě. Obsahuje smyčku, která reaguje na tokeny, vyhodnocuje zpoždění a čeká na externí vstupy.
* **`InscriptionEngine` (JIT Kompilátor)**: Toto je náš trumf! Kód napsaný uživatelem v C++ v rámci podmínky (guard) nebo akce (action) se vezme, na pozadí se skutečně zkompiluje přes kompilátor systému (`g++`) do malé sdílené knihovny `.so` a ihned se za běhu načte (`dlopen`). Díky tomu fungují skutečné proměnné a reálné C++ operace (např. aritmetika, if-else, zavolání funkcí).
* **`TimerManager`**: Stará se o přechody s časovačem (direktiva `@ delay`).
* **`EventQueue`**: Thread-safe fronta (chráněná mutexy a condition variables), přes kterou GUI posílá do běžící sítě *vstupy* a *příkazy k zastavení*.

---

## 4. Životní cyklus interpretu (Hlavní smyčka)

Jak přesně interpret pracuje na pozadí? Dodrželi jsme algoritmus popsaný v zadání:

1. **Reakce bez čekání (Stabilizace):**
   Interpret prochází všechny okamžité přechody (ty bez časovače). Pokud mají dostatek tokenů a splněnou stráž, odpálí je, sebere tokeny na vstupu, provede C++ kód přes JIT kompilátor a přidá tokeny na výstup. To dělá tak dlouho, dokud se síť "neuklidní" (už není co pálit).
   
2. **Naplánování zpožděných přechodů:**
   Projde přechody s nastaveným časovačem (např. `@ 5000`). Pokud jsou povoleny (mají tokeny), `TimerManager` pro ně založí odpočet.

3. **Pasivní čekání (Uspání vlákna):**
   Vlákno interpretu "usne". Probudí ho pouze 3 věci:
   * Vypršel nějaký časovač z `TimerManageru`.
   * Uživatel z GUI přes Injection panel poslal **Vstup**.
   * Uživatel klikl na tlačítko Stop.

4. **Vyhodnocení po probuzení:**
   Když se probudí zásahem uživatele (např. odeslání vstupu "start" z GUI), přečte hodnotu, uloží ji pro `valueof("in")` a pokud tento vstup spouští nějaký přechod, provede odpálení. Následně se smyčka vrátí na bod 1.

---

## 5. Co jsme naprogramovali & co je třeba zmínit u obhajoby

Máme kompletně připravený a robustní projekt, se kterým můžete předvádět následující věci:

* **Tvorba a ukládání sítí**: Ukazujte na případu, jak se nakliká síť a jak z toho padá hezký textový soubor.
* **JIT Kompilace kódu**: Tohle je nejsilnější stránka projektu. Můžeme v editoru přechodu používat skutečné C++ kódy. Ukažte `examples/simple_counter.petri`, kde se proměnná sčítá a live monitor na to reaguje. Proměnné (`int counter`) jsou tam plně podporovány.
* **Časovače a multithreading**: Síť není zaseklá. Zvládne mít spuštěné časovače na pozadí, které samy po expiraci dokončí přesun tokenů a UI se u toho neseká.
* **Záludnost s časovačem a proměnnou (Detail pro obhajobu):** Kdyby si vyučující hrál s časovači a ptal se, jestli se dají používat proměnné i do časovačů (direktiva `when: @ delay`)... Zde fungují natvrdo **čísla** (např. `@ 5000`). Proměnná tam dát nelze. Proč? Z důvodu zjednodušení parseru by napojení dynamického C++ stringu na timer dalo tolik práce, že jsme se rozhodli toto drobné omezení záměrně nechat a uvést ho v `README.txt`. Zbytek kódů a políček proměnné používat samozřejmě plnohodnotně umí!
* **Thread-safe ukončování**: Všechny smyčky a čtení proměnných běží bezpečně přes atomické proměnné a mutexy. Když načtete jinou síť, ta stará se na pozadí spolehlivě zničí a nedělá nepořádek.
