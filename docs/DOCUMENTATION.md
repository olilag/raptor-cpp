# Vyhľadávač spojení

## Obsah

1. [Algoritmus](#algoritmus)
2. [Inštalácia](#inštalácia)
3. [Formát vstupných dát](#formát-vstupných-dát)
4. [Popis programu](#popis-programu)
5. [Záver](#záver)

## Algoritmus

Rozhodol som sa implementovať algoritmus [RAPTOR](https://www.microsoft.com/en-us/research/wp-content/uploads/2012/01/raptor_alenex.pdf). Tento algoritmus iteratívne počíta najskoršie príchody z počiatočnej zastávky na všetky ostatné zastávky v sieti. Jedna iterácia vlastne 'povolí' použiť o jeden prestup viac pri hľadaní. Na hľadanie pre iteráciu $k$ využije hodnoty spočítané v iterácii $k-1$.

Pre každú zastávku si pamätáme najskoršie príchody pre každú iteráciu v $\tau_i$ a najskoršie príchody celkovo v $\tau^*$.

Iterácia $k$ začína tým, že pre každú zastávku $p$ zlepšenú v predchádzajúcej iterácii pridáme do množiny dvojíc $(linka, zastávka) \in Q$ dvojicu $(r, p)$, kde $r$ je linka, ktorá obsluhuje $p$ pokiaľ $(r, p') \not \in Q$ pre inú zastávku $p'$. Ak takáto zastávka $p'$ existuje, tak v $Q$ ostane tá dvojica, ktorej zastávka je v smere jazdy $r$ na trase skôr. Po pridaní všetkých takýchto dvojíc pre $p$, odstránime $p$ zo zoznamu zlepšených zastávok.

Následne prejdeme všetky dvojice $(r, p) \in Q$ a ideme skúsiť pomocou nejakého výjazdu, ktorý ide po linke $r$ cez zastávku $p$ zlepšiť príchody na ostatné zastávky. Najprv začíname s nedefinovaným výjazdom $t$. Pre každú ďalšiu zastávku $p_i$ na linke $r$, ktoré nasledujú po $p$ spravíme nasledovné. Ak je $t$ definované a príchod na $p_i$ pomocou $t$ je skorší ako minimum z najskoršieho príchodu na $p_i$ a najskoršieho príchodu na cieľovú zastávku, tak zmeníme príchod v tejto iterácii na $p_i$ na čas príchodu $t$ na $p_i$ a zmeníme najskorší príchod na $p_i$ na rovnaký čas. Následne pridáme $p_i$ do zoznamu zlepšených zastávok. Ak sme v minulej iterácii vedeli prísť na $p_i$ skôr ako čas odchodu $t$ z $p_i$, tak sa môže stať, že vieme nastúpiť na skorší výjazd linky $r$ z $p_i$, takže zmeníme $t$ na práve tento výjazd.

Po spočítaní príchodov pre túto iteráciu prejdeme zoznam zlepšených zastávok. Pre každú zastávku $p$ z nich prejdeme zoznam jej peších presunov. Ak sa vieme na konečnú zastávku presunu dostať z $p$ pomocou pešieho presunu skôr ako bez použitia toho presunu, tak príchod na danú zastávku v tejto iterácii upravíme na ten čas a pridáme danú zastávku do zoznamu zlepšených zastávok.

Algoritmus končí, keď sme v danej iterácii nezlepšili ani jeden príchod, čiže zoznam zlepšených zastávok je prázdny.

Na efektívny beh tohto algoritmu je treba vstupné dáta s odchodmi a príchodmi dať d špeciálneho tvaru. Tieto dátové štruktúry sú bližšie popísané na konci PDF linknutom vyššie.

## Inštalácia

```bash
git clone --recursive https://gitlab.mff.cuni.cz/teaching/nprg041/2023-24/svoboda-1040/lagoo.git
cd lagoo/project
cmake -S. -Bbuild
cmake --build build
```

### Windows

Ak `cmake` zvolí Visual Studio generator, treba ešte špecifikovať, ktorý preset má použiť. To sa spraví nasledujúcim príkazom

```bash
cmake --build build --config Release
```

Ak na Windowse program vypisuje na výstup nezrozumiteľné názvy zástavok, treba zmeniť terminál na `Command Propmt` a code page na $65001$.

```powershell
chcp 65001
```

## Formát vstupných dát

Vstupné dáta pre program sú vo formáte [GTFS Schedule](https://gtfs.org/schedule/). Tento formát som zvolil pre ľahkú dostupnosť dát pre MHD rôznych miest. V projekte sa nachádzajú 2 feedy, veľmi jednoduchý feed `example-data`, ktorý používajú testy a `BA-data` feed s dátami Dopravného podniku Bratislava ([zdroj](https://www.arcgis.com/sharing/rest/content/items/aba12fd2cbac4843bc7406151bc66106/data)).

Jednotlivé linky musia mať v celom feede konštantný počet zastávok, ak nemajú, program načíta iba ich najdlhší výjazd, ostatné s iným počtom zastávok bude ignorovať.

Na špecifikovanie `service_id` treba použiť súbor `calendar.txt` vo feede.

## Popis programu

Program sa skladá z dvoch častí, knižnice `raptor`, kde sa nachádza moja nie úplná implementácia algoritmu Raptor a spustiteľného súboru `ConnectionFinder`, ktorý slúži ako TUI k tejto knižnici.

Na čítanie vstupného feedu používam knižnicu [just_gtfs](https://github.com/mapsme/just_gtfs), ktoré je súčasťou ako git submodule.

V knižnici `raptor` sa nachádza trieda `raptor::RouteFinder`, ktorá slúži na hľadanie spojení. Ako vstupné dáta pre konštruktor berie pointer na `gtfs::Feed`, odkiaľ bude čerpať dáta pre následnú konštrukciu dátových štruktúr `raptor::RouteTraversal` a `raptor::Stops`. Dáta v správnom formáte pre tieto dve štruktúry pripraví funkcia `raptor::GTFSFeedParser::parseFeed`. Táto funkcia načíta a zoradí dáta do správneho poradia pre dátové štruktúry. Ešte predtým však pripraví triedu `raptor::IdTranslator`, ktorá slúži ako prekladový slovník medzi identifikátormi z feedu, čo sú stringy a identifikátormi, ktoré používam v algoritme `raptor::Id<size_t>` (typovo odlíšené size_t čísla).

Po skunštruovaní triedy `raptor::RouteFinder` vieme pomocou jej funkcie `findRoute` hľadať spojenia medzi zastávkami z feedu. Na vstupe chce funkcia zoznam začiatočných a konečných zastávok vo forme vectoru ich idčiek a čas odchodu ako počet sekúnd od polnoci. Zoznam vstupných a konečných zastávok treba preto, lebo vo feede má každé nástupište v rámci jednej zastávky svoje vlastné id, ale my vlastne chceme odchádzať a prichádzať na ľubovoľné nástupište. Návratová hodnota tejto funkcie je postupnosť zastávok s príchodmi a výjazdami, ktoré sme použili uložená do vectoru. Pre tento typ má aj operátor zápisu do streamu, ktorý sa používa na vypísanie nájdeného spojenia.

### Nedostatky programu

#### Obmedzenie na konštantný počet zastávok na linke

Nepodarilo sa mi vymyslieť a implementovať spôsob ako takéto linky správne načítať a postaviť pre ne dátové štruktúry. Nie je to nejaké veľké obmedzenie, väčšinou sa to týka iba ranných výjazdov z depa a večerných dojazdov do depa. Možno by sa to dalo implementovať rozlíšením týchto liniek a interne si pre ne vytvoriť nové id.

#### Pre nočné linky niekedy nespočíta správne spojenie

Toto plynie z toho, že vo feede pre Bratislavu majú večerné linky časy odchodov väčšie ako `24:00`, čo je pre algoritmus vhodné, ak človek odchádza pred polnocou. Ak odchádza po polnoci, tak algoritmus ráta s časom odchodu vo formáte na príklad `2:00`, ale vo feede je ten istý čas ako `26:00`, tým pádom nájde skôr odchod danej linky s časom napríklad `23:59`.

Tiež treba povoliť robenie prestupu z času odchodu `26:00` na `2:03`, lebo v realite sú medzi tými časmi iba 3 minúty. To isté platí pre opačné časy.

Možno by sa to dalo dorobiť naklonovaním dát pre linky, ktoré majú takéto veľké časy odchodov a ich následným uložím po zmodulení ich časov hodnotou jedného dňa.

#### Algoritmus nájde iba jednu linku a nie celý Pareto set ako v popise algoritmu v PDF vyššie

Túto feature som hlavne z časových dôvodov nestíhal implementovať. Tiež som nemal čas sa zamyslieť nad tým ako by sa dala implementovať.

#### Pre vstupné dáta sa ignorujú výnimky v cestovných poriadkoch pre konkrétne dni

Táto feature nie je implementovaná hlavne z dôvodu, že každý feed ich môže mať rôzne špecifikované a vyžadovalo by to pomerne veľké zmeny pri vytváraní dátových štruktúr. Bolo by treba tie dáta najprv roztriediť podľa `service_id`, tieto dáta následne uložiť. Tiež bude treba pravdepodobne upraviť formát akým algoritmus používa čas, iba zároveň mal informácie o aktuálnom dátume a s tým aj možné zmeny v cestovnom poriadku pre linky.

## Používanie `ConnectionFinder`

S programom sa komunikuje cez štandardný vstup napríklad cez terminál. Po spustení treba programu zadať relatívnu alebo absolútnu cestu k priečinku, ktorý obsahuje požadovaný GTFS Schedule. Program si potom načíta daný feed, overí jeho správnosť a následne postaví dátové štruktúry. Tento krok môže nejaký čas trvať. Po inicializácii je program pripravený zodpovedať na požiadavky.

### Zoznam príkazov

| `meno\|alias (argumenty)` | Vysvetlenie |
| --- | --- |
| `help\|h` | Zobrazí návod na ovládanie programu. |
| `liststops\|ls (optional: prefix)` | Vypíše názvy všetkých/začínajúcich na `prefix` zastávok vo feede. |
| `services\|ser` | Vypíše idčka všetkých services vo feede. |
| `set\|s (walking speed - 'Fast'\|'Normal'\|'Slow', service id)` | Nastaví walking speed a service, ktorý sa má používať. Ak je service prázdny string, tak sa nenastaví. Ak je ľubovoľný argument neplatný, tak nenastanú žiadne zmeny. |
| `findroute\|fr (start stop, end stop, departure time - hh:mm)` | Nájde spojenie medzi `start stop` a `end stop` s odchodom najskôr v čase `departure`. Toto spojenie následne vypíše na štandardný výstup. Argumenty musia byť oddelené `-`. |
| `quit\|q` | Ukončí program. |

## Záver

### Zhodnotenie

Práca na tomto programe ma celkom bavila. Vyskúšal som si robenie dynamickej alokácie, písanie vlastných iterátorov a veľmi som sa zlepšil pri debugovaní pomocou `gdb`. Naučil som ako tak používať CMake na buildenie. Získal som predstavu ako rozdeliť zdrojové súbory do priečinkov v rámci projektu.

Trochu som sa vykašľal na vytvorenie automatických testov, preto som niektoré bugy odhalil neskôr ako bolo ideálne. Nakoniec som napísal jednoduché testy, ktoré nájdu spojenie medzi každou dvojicou zastávok v `example-data` feede.

### Možné rozšírenia

Program by sa dal rozšíriť o implementáciu chýbajúcich featur. Tiež by sa hodilo spraviť popracovať na user experience pri používanie terminálu, napríklad pridať tab autocompletion na zastávky, ukladanie zadaných príkazov, lepšie zobrazovanie nájdeného spojenia (napríklad pridať informácie, n ktoré nástupište treba ísť, ofarbiť linky podľa ich farby vo feede),...

Takisto by bolo viac premyslieť spracovanie dát z feedu. Každý poskytovateľ dát má istú voľnosť v tom, ako ten feed vyzerá. Bolo by fajn vymyslieť nejaký všeobecnejší model spracovania dát, ktorý by vedel akceptovať nejaké užívateľom špecifikované funkcie na spracovanie nejakých konkrétnych detailov pre daný feed a potom pri vytváraní dátových štruktúr tieto funkcie použiť.

Tiež by sa dalo v budúcnosti pozrieť na spracovávanie realtime dát cez [GTFS Realtime](https://gtfs.org/realtime/).
