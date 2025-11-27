# Analiza broja čestica (Particle Counts) — Vizuelno poboljšan pregled

Ovaj dokument objašnjava šta znače različiti brojači čestica koje meri senzor i kako ih interpretirati. Broj čestica daje „otisak prsta“ zagađenja: masa (PM2.5 u µg/m³) govori koliko je zagađeno, dok broj čestica govori šta tipično zagađuje.

## Sažeti pregled asset-a

| Asset | Granica (µm) | Šta predstavlja | Tipični izvori | Napomena / dijagnostika |
|---|---:|---|---|---|
| count-0-3 | > 0.3 | Najsitnije i često najopasnije čestice | Izduvni gasovi, dim (loženje, cigarete), bio-aerosoli, fotohemijski smog | Uvek najveći broj; visoke vrednosti ukazuju na sagorevanje ili gust saobraćaj |
| count-0-5 | > 0.5 | Fino filtrira najsitnije nano-čestice | Bakterije, fina prašina, isparenja iz kuvanja | Skok pri kuvanju; daje dodatnu granulaciju u odnosu na 0.3 |
| count-1-0 | > 1.0 | Granica koja odgovara PM1 | Dim cigarete, fina čađ, sitne kapljice vode (pri visokoj vlažnosti) | Koristan za detekciju dima i sitne čađi |
| count-2-5 | > 2.5 | Granica za PM2.5 (fina prašina) | Spore buđi, građevinska prašina, ostaci insekata | Ako je ovaj visok, a count-0-3 nizak → verovatno mehaničko podizanje prašine (ne dim) |
| count-5-0 | > 5.0 | Krupna prašina vidljiva na svetlu | Polen, vlakna, perut, grinje, erozija tla | Skoči kod usisavanja, brisanja i polenskih dana |
| count-10-0 | > 10.0 | Veoma krupne čestice koje brzo padaju | Pesak, zemlja, krupan polen, pepeo | Indikator velikih mehaničkih izvora ili radova u blizini |

## Praktični primeri tumačenja (kako čitati grafikone)

| Scenarijo | Šta pokazuju podaci? | Tumačenje |
|---|---|---|
| Zima / Veče | count-0-3 ogroman, krupne čestice mirne | Najverovatnije loženje (dim) — male čestice su dominantne |
| Proleće / Vetar | count-5-0 i count-10-0 visoki, count-0-3 relativno nizak | Polen ili prašina koju diže vetar — mehanički izvori |
| Unutra / Kuhinja | count-0-5 naglo skoči dok spremate ručak | Prženje i kuvanje oslobađaju puno sitnih aerosola |
| Unutra / Čišćenje | count-5-0 i count-10-0 skoče | Usisavanje / brisanje podižu krupnu prašinu i grinje |

## Saveti za vizuelizaciju (preporučeni grafici i metrika)
- Koristi složene (stacked) area ili linijske grafikone za sve count-* serije kako bi se videlo relativno učešće sitnih vs krupnih čestica.
- Prikaži i odnos coarse/fine (npr. (count-5-0 + count-10-0) / (count-0-3)) kao indikator mehaničkog zagađenja.
- Dodaj anotacije za događaje (kuvanje, čišćenje, vredi proveriti komšija loži) — pomaže u korelaciji podataka sa aktivnostima.
- Primeni kratku glatku (rolling average, npr. 5–15 min) da redukuješ šum i istakneš trendove.
- Koristi logaritamsku skalu ako counts variraju po redovima veličine.
- Postavi pragove/alarme za nagle skokove u count-0-3 (zdravstveno relevantno) i za count-2-5 (PM2.5 relevantno).

## Dodatne napomene za dijagnostiku
- Visok count-0-3 ali nizak count-2-5: verovatno sagorevanje/dim ili veoma sitne čestice (loženje, izduvni gasovi).
- Suprotno (visok count-2-5 i count-5-0): verovatno prašina, gradilište, polen.
- Vlažnost može uticati: pri visokoj relativnoj vlažnosti kapljice vode mogu povećati broj čestica u nekim opsezima (PM1/PM2.5).

## Zaključak
Brojači čestica omogućavaju da budemo „detektivi vazduha“ — posmatranjem odnosa i obrazaca između različitih count-* vrednosti lako se razlikuju izvori zagađenja (dim vs prašina vs kuvanje). Dobro osmišljene vizualizacije i jednostavne metrike (ratios, rolling averages, anotacije) značajno ubrzavaju i olakšavaju interpretaciju.

---
