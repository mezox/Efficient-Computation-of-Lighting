Efektivní výpočet osvětlení
Dátum:	15/5/2014
Verzia:	1.0
----------------------------

Obsah tohoto CD vznikol ako praktická časť bakalárskej práce s názvom Efektivní výpočet osvětlení
na fakulte Informačních technologií (FIT), Vysoké Učení Technické (VUT) v Brne.

Ústav počítačové grafiky a multimédií

Autor práce:		Tomáš Kubovčík
Fakultný E-mail:	xkubov02@stud.fit.vutbr.cz
Osobný E-mail:		t.kubovcik@gmail.com

Vedúci práce:		Milet Tomáš, Ing., UPGM FIT VUT
Oponent práce:		Pečiva Jan, Ing., Ph.D., UPGM FIT VUT
----------------------------------------------------------

OBSAH CD:
---------
bin/			-	binárne súbory aplikácie
data/			-	model grafickej scény, model guľe reprezentujúcej bodové svetlo
experiments/	-	obsahuje hodnoty namerané pri experimentoch s tiled shadingom na NVIDIA GTX660
guide/			-	návod pre kompiláciu, potrebné knižnice, parametrizácia programu
include/		-	hlavičkové súbory modulov, tried, externých knižníc
lib/			-	statické knižnice
logs/			-	log z kompilácie shaderov
obj/			-	objektové súbory vytvorené pri kompilácii, logy z kompilácie
release-libs/	-	knižnice potrebné pre spustenie aplikácie
report/			-	text práce , zdrojové súbory textu práce pre latex
shaders/		-	zdrojové kódy shaderov pre generovanie
src/			-	zdrojové kódy aplikačnej logiky

ECL.cpp			-	zdrojový súbor obsahujúci triedu main
ECL.sln			-	solution projektu pre Microsoft Visual Studio
README.txt
run.cmd			-	spúštací skript aplikácie

súbory vytvorené Visual Studiom pre solution.

POPIS APLIKÁCIE:
----------------
Vytvorená aplikácia rieši problematiku výpočtu osvetlenia v scénach s veľkým počtom svetiel. Obsahuje
implementáciu techník deferred shading, tiled deferred shading a tiled forward shading v jazykoch C/C++
s využitím grafickej knižnice OpenGL. Pre demonštráciu výpočtov bol použitý model scény Crytek Sponza. 
Pre výpočet osvetlenia sa v práci využíva Phongov osvetľovací model s využitím techniky bump mapping a
Fresnelovými rovnicami pre výpočet speculárnej zložky odrazu svetla. 

VLASTNOSTI APLIKÁCIE:
---------------------
-	výpočet osvetlenia v real-time pre stovky až tisícky svetiel
-	jednoduchý prechod medzi implementovanými technikami
-	využitie spoločných prostriedkov pre výpočet osvetlenia medzi jednotlivými technikami
-	zobrazenie obsahu textúrových bufferov využitých na výpočet osvetlenia
-	optimalizácia výpočtov prácou s hĺbkovým bufferom

KOMPILÁCIA, EXPERIMENTOVANIE, PRÁCA S APLIKÁCIOU:
--------------------------------------------------
viď príručka k aplikácii v adresári guide/

OBMEDZENIA APLIKÁCIE:
----------------------
Deferred shading je funkčný len na grafických kartách AMD, prípadne intel. Na GPU od spoločnosti
NVIDIA spôsobuje komplikácie z neznámych dôvodov (je možné, že je to spôsobené drivermi). Z dôvodu
obmedzeného prístupu ku grafickej karte NVIDIA tento problém nie je vyriešený a štandardne je 
deferred shading pre tento typ grafických kariet zakázaný. Pre vynútenie použitia tejto techniky
zmeňte hodnotu definície preprocesoru #define FORCE_AMD_CONFIG na hodnotu 1 (v include/configuration/Config.h).
