Ahoj! Vzhledem k tomu, že dnes je 4. května 2026 a termín odevzdání máte už 11. května 2026, zbývá vám přesně týden. Jako umělá inteligence sice nezažívám stres ze zkouškového, ale naprosto chápu, že teď musíte jet na 100 % a efektivně si práci rozdělit.

Zadání je poměrně komplexní a striktně vyžaduje oddělení modelu od GUI. Pro tříčlenný tým se přímo nabízí rozdělit projekt na tři hlavní pilíře:  **Backend (Jádro)** , **Frontend (GUI)** a  **Integraci/Komunikaci** .

Zde je návrh rozdělení rolí a harmonogramu pro Lukáše, Jana a Matěje, tak abyste mohli pracovat co nejvíce paralelně a nekřížili si cesty:

### Rozdělení rolí (Lukáš, Jan, Matěj)

**1. Lukáš: Architekt jádra (Model a Sémantika Petriho sítě)**
Lukáš se zaměří na čisté C++ (C++17) a logiku sítě bez ohledu na to, jak vypadá na obrazovce.

* **Datové struktury:** Datové struktury: Vytvoří třídy pro místa, přechody, tokeny a interní proměnnéDatové struktury: Vytvoří třídy pro místa, přechody, tokeny a interní proměnnéDatové struktury: Vytvoří třídy pro místa, přechody, tokeny a interní proměnné.
* **Algoritmus smyčky:** Algoritmus smyčky: Naimplementuje hlavní event-driven smyčku sítě, čekání na události a vyhodnocování maximální množiny nezávislých přechodůAlgoritmus smyčky: Naimplementuje hlavní event-driven smyčku sítě, čekání na události a vyhodnocování maximální množiny nezávislých přechodůAlgoritmus smyčky: Naimplementuje hlavní event-driven smyčku sítě, čekání na události a vyhodnocování maximální množiny nezávislých přechodů.
* **Sémantika odpálení:** Sémantika odpálení: Zajistí odebírání a přidávání tokenů dle vah hran a zpracování zpožděných přechodů (timeoutů)Sémantika odpálení: Zajistí odebírání a přidávání tokenů dle vah hran a zpracování zpožděných přechodů (timeoutů).
* **Generování kódu:** Generování kódu: Napíše logiku, která z modelu v paměti vygeneruje kód interpretu (obsahující C/C++ inskripční jazyk) a přeloží hoGenerování kódu: Napíše logiku, která z modelu v paměti vygeneruje kód interpretu (obsahující C/C++ inskripční jazyk) a přeloží ho.

**2. Jan: Mistr Frontendu (Qt Vizuální Editor)**
Jan si vezme na starost uživatelské rozhraní. Bude pracovat s toolkitem Qt (min. verze 5.5).

* **Kreslící plátno (Canvas):** Kreslící plátno (Canvas): Vytvoří editor pro přidávání/mazání míst (kroužky) a přechodů (čtverečky) a kreslení orientovaných hranKreslící plátno (Canvas): Vytvoří editor pro přidávání/mazání míst (kroužky) a přechodů (čtverečky) a kreslení orientovaných hran.
* **Dialogy a vlastnosti:** Dialogy a vlastnosti: Naimplementuje formuláře pro editaci podmínek, akcí, vstupů a výstupů přes inskripční jazykDialogy a vlastnosti: Naimplementuje formuláře pro editaci podmínek, akcí, vstupů a výstupů přes inskripční jazyk.
* **Vizuální oddělení:** Vizuální oddělení: Zajistí, že GUI bude pouze vizualizovat data, čímž splní požadavek na oddělení modelu od rozhraní.
* **Příprava pro monitor:** Příprava pro monitor: Připraví v diagramu prostor pro živé zobrazení počtu tokenů, log událostí a panel pro injektování vstupůPříprava pro monitor: Připraví v diagramu prostor pro živé zobrazení počtu tokenů, log událostí a panel pro injektování vstupůPříprava pro monitor: Připraví v diagramu prostor pro živé zobrazení počtu tokenů, log událostí a panel pro injektování vstupů.

**3. Matěj: Integrátor (Komunikace, Soubory a Infrastruktura)**
Matěj vytvoří "lepidlo", které spojí Lukášovo jádro s Janovým GUI, a postará se o formální požadavky projektu.

* **Ukládání a načítání:** Ukládání a načítání: Naimplementuje parsování a generování čitelného textového formátu sítě (vstupy, místa, přechody, hrany)Ukládání a načítání: Naimplementuje parsování a generování čitelného textového formátu sítě (vstupy, místa, přechody, hrany).
* **Protokol a komunikace:** Protokol a komunikace: Vyřeší spuštění generovaného interpretu a propojení s GUI (např. přes UDP sockety nebo sdílenou knihovnu).
* **Monitorování v reálném čase:** Monitorování v reálném čase: Zajistí, aby GUI asynchronně přijímalo data o stavu markingu, timeoutech a logovalo je, a naopak umělo injektovat vstupy do běžící sítě.
* **Infrastruktura:** Infrastruktura: Připraví Makefile (bez absolutních cest, cíle jako make run, make doxygen, make pack) a vygeneruje Doxygen dokumentaci do složky ./doc/Infrastruktura: Připraví Makefile (bez absolutních cest, cíle jako make run, make doxygen, make pack) a vygeneruje Doxygen dokumentaci do složky ./doc/`Makefile``make run``make doxygen``make pack``./doc/`.

### Expresní harmonogram (7 dní do odevzdání)

Vzhledem k nedostatku času musíte postupovat agilně:

* **Den 1 (Dnes): Architektura a sdílené rozhraní.** Den 1 (Dnes): Architektura a sdílené rozhraní. Společně vytvořte konceptuální UML návrh tříd (musíte ho odevzdat v PDF). Matěj založí repozitář, napíše základní Makefile a připraví adresářovou strukturu (src, examples, doc)`Makefile``src``examples``doc`. Dohodněte se na rozhraní, jak bude Janovo GUI číst data z Lukášova modelu.
* **Den 2 - 3: Izolovaný vývoj.** Den 2 - 3: Izolovaný vývoj. * Lukáš programuje struktury sítě a pravidla pro nezávislá odpálení *Lukáš* .
* *Jan*Jan si v Qt nakliká základní menu a kreslí na plátno "hloupé" objekty.
* *Matěj*Matěj píše parser pro textový formát, abyste měli jak načítat testovací data.
* **Den 4 - 5: Generování a Propojování.** Den 4 - 5: Generování a Propojování. Lukáš dokončuje generování interpretu. Matěj řeší UDP/IPC komunikaci mezi interpretovanou sítí a aplikací. Jan oživuje UI, napojuje ho na načtená data a přidává panel pro logováníJan oživuje UI, napojuje ho na načtená data a přidává panel pro logování.
* **Den 6: Testování monitoru a ladění.** Všichni tři testují běh sítě. Zkoušíte injektovat vstupy za běhu a sledujete, zda se správně updatují tokeny v Janově diagramu. Řešíte případná uváznutí programu.
* **Den 7: Kompletace a odevzdání.** Den 7: Kompletace a odevzdání. Matěj doladí README.txt, zkontroluje komentáře v kódu (chybějící autoři znamenají 0 bodů!)Den 7: Kompletace a odevzdání. Matěj doladí README.txt, zkontroluje komentáře v kódu (chybějící autoři znamenají 0 bodů!)`README.txt`. Otestujete příkaz make a make packOtestujete příkaz make a make pack `make``make pack`. Jeden z vás (vedoucí) odevzdá archiv pojmenovaný přesně podle zadání (např. xname01-xname02-xname03.zip)`xname01-xname02-xname03.zip`.

Máte už v týmu nějaké preference, kdo by se chtěl vrhnout na Qt grafiku a kdo spíše na C++ logiku interpretu, nebo to mám nechat takto navržené?
