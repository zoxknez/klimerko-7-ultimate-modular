# Klimerko 6.6 Stable Professional

Najnovija, optimizovana verzija firmware-a za Klimerko uređaj. Ova verzija donosi maksimalnu stabilnost, preciznu dijagnostiku, detaljnu analizu čestica i potpunu daljinsku kontrolu.

## 🌟 Nove Funkcionalnosti (v6.0 - v6.6)

* **Brojanje čestica (Particle Counts):** Uređaj sada šalje tačan broj čestica u 6 dimenzija (0.3µm, 0.5µm, 1.0µm, 2.5µm, 5.0µm, 10µm). Ovo omogućava prepoznavanje izvora zagađenja (npr. dim vs prašina).
* **Detekcija kvara ventilatora (Fan Check):** Pametna dijagnostika koja prepoznaje ako se ventilator zaglavio (podaci se ne menjaju) ili ako senzor ne radi (šalje nule duže vreme). Status se šalje kroz `sensor-status`.
* **Kompenzacija vlažnosti (HumComp):** Napredni algoritam koji smanjuje lažna očitavanja PM čestica tokom visoke vlažnosti (magle). Šalju se i sirovi (`pm2-5`) i korigovani (`pm2-5-c`) podaci.
* **Stack Overflow Fix (v6.6):** Rešen problem restartovanja uređaja pri slanju velikih paketa podataka optimizacijom memorije.
* **Stabilnost na visokoj vlažnosti (Humidity Fix):** Rešen problem blokiranja senzora na 98-100% vlažnosti.
* **Daljinski restart:** Asset `restart-device` za softverski reset.
* **Precizna dijagnostika signala (dBm):** WiFi signal kao numerička vrednost.
* **Daljinsko ažuriranje (HTTP Update):** Nadogradnja firmware-a putem linka.

---

## ☁️ AllThingsTalk Podešavanja

Da biste koristili sve opcije verzije 6.6, potrebno je dodati **nove assete** na listu postojećih.

### Senzori (Podaci koje uređaj šalje vama)

| Name (Ime) | Title (Naslov) | Type | Profile | Opis |
| :--- | :--- | :--- | :--- | :--- |
| **NOVI ASSETI (v6.x)** | | | | |
| `count-0-3` | Count > 0.3um | Sensor | Integer | Čestice dima, izduvnih gasova |
| `count-0-5` | Count > 0.5um | Sensor | Integer | Bakterije, fina prašina |
| `count-1-0` | Count > 1.0um | Sensor | Integer | Dim cigarete, čađ |
| `count-2-5` | Count > 2.5um | Sensor | Integer | Fina prašina, spore |
| `count-5-0` | Count > 5.0um | Sensor | Integer | Krupna prašina, polen |
| `count-10-0` | Count > 10um | Sensor | Integer | Vidljiva prašina, pepeo |
| `pm1-c` | PM1 (Corrected) | Sensor | Integer | Korigovano za vlagu |
| `pm2-5-c` | PM2.5 (Corrected)| Sensor | Integer | Korigovano za vlagu |
| `pm10-c` | PM10 (Corrected) | Sensor | Integer | Korigovano za vlagu |
| `sensor-status`| Sensor Status | Sensor | **String** | Dijagnostika (OK, Error...) |
| **STANDARDNI ASSETI** | | | | |
| `wifi-signal` | WiFi Signal (dBm)| Sensor | Integer | Jačina signala |
| `altitude` | Altitude | Sensor | Number | Trenutna visina |
| `pressureSea` | Pressure (Sea Level)| Sensor| Number | Pritisak nivoa mora |
| `HeatIndex` | Heat Index | Sensor | Number | Subjektivni osećaj |
| `dewpoint` | Dew Point | Sensor | Number | Tačka rose |
| `humidityAbs` | Absolute Humidity| Sensor | Number | Apsolutna vlažnost |

*Napomena: Standardni asseti (pm1, pm2-5, pm10, temperature, humidity, pressure) ostaju nepromenjeni.*

### Aktuatori (Komande koje vi šaljete uređaju)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `restart-device` | Restart Device | Actuator | **Boolean** |
| `altitude-set` | Set Altitude | Actuator | Number (Integer) |
| `temperature-offset` | Set Temp Offset | Actuator | Number |
| `wifi-config` | Remote Config | Actuator | Boolean |
| `firmware-update` | Update Firmware | Actuator | String |

---

## 🔬 Kako tumačiti nove podatke (v6.x)

### 1. Broj čestica (Particle Counts)
Ovi podaci vam govore **šta** zagađuje vazduh:
* **Visok `count-0-3` (ostali niski):** Sagorevanje (dim, loženje, saobraćaj).
* **Visok `count-5-0` i `count-10-0`:** Mehaničko zagađenje (prašina sa puta, radovi, polen, vetar).

### 2. Status Senzora (`sensor-status`)
Uređaj sam proverava ispravnost PMS senzora:
* **"OK":** Sve radi kako treba.
* **"Fan Stuck / Error":** Podaci su identični duže od sat vremena. Ventilator je verovatno zaglavljen.
* **"Zero Data Error":** Senzor šalje nule (0,0,0) duže od sat vremena. Moguć prekid kabla ili kvar senzora.
* **"Sensor Offline":** Senzor ne odgovara na komande.

### 3. Korigovane vrednosti (`pm2-5-c`)
Kada je vlažnost vazduha preko 70% (magla), običan senzor može pokazivati prevelike vrednosti jer kapljice vode vidi kao prašinu.
* **`pm2-5` (Raw):** Sirovi podatak sa senzora (uključuje grešku zbog magle).
* **`pm2-5-c` (Corrected):** Matematički očišćen podatak, realnija slika zagađenja tokom vlažnih dana.

---

## ⚠️ VAŽNO: Ažuriranje Biblioteke (Library Update)

Da bi verzija 6.x radila i čitala broj čestica, neophodno je **ručno ažurirati** `PMS` biblioteku u vašem `src` folderu pre kompajliranja.
* Zamenite sadržaj fajlova `PMS.h` i `PMS.cpp` kodom koji podržava `PM_RAW` komande (32-bajtni protokol).

---

## 🔄 Kako restartovati uređaj na daljinu (restart-device)

1.  Na AllThingsTalk-u koristite asset **`restart-device`**.
2.  Postavite vrednost na `True` (ili pošaljite `1`).
3.  Uređaj će se restartovati za 1 sekundu. Podaci ostaju sačuvani.

---

## 🚀 Kako ažurirati Firmware na daljinu (firmware-update)

1.  U Arduino IDE-u: `Sketch` -> `Export Compiled Binary`.
2.  Postavite `.bin` fajl na GitHub.
3.  **VAŽNO:** Kopirajte **RAW** link do fajla.
    * *Ispravno:* `https://raw.githubusercontent.com/.../firmware.bin`
    * *Pogrešno:* `https://github.com/.../blob/...`
4.  Na AllThingsTalk-u, u asset **`firmware-update`** nalepite taj RAW link i pošaljite.
5.  Klimerko će preuzeti fajl i automatski se restartovati.


---

## ⚙️ Inicijalna Instalacija (v5.0+)

**Napomena:** Prelazak sa starijih verzija (4.x) na nove (5.x i 6.x) zahteva ponovnu konfiguraciju WiFi-a zbog promene strukture memorije.

1.  Flešujte kod preko USB kabla.
2.  Držite **FLASH dugme** 2 sekunde.
3.  Povežite se na WiFi `KLIMERKO-xxxx` -> `192.168.4.1`.
4.  Unesite podatke i sačuvajte.

---

## 🚀 Klimerko 7.0 Ultimate Modular

**Nova verzija dostupna!** Kompletno refaktorisana i nadograđena verzija sa modernom modularnom arhitekturom.

### ✨ Nove funkcionalnosti u v7.0:
- 📡 **mDNS Discovery** - pristup preko `klimerko-xxxxxx.local`
- 🌐 **Web Dashboard** - real-time prikaz podataka u browseru
- 📊 **Chart.js grafici** - vizualizacija PM i klimatskih podataka
- ⏰ **NTP sinhronizacija** - pravo vreme u logovima
- 🚨 **Alarm sistem** - upozorenja za PM2.5/PM10 pragove
- 📈 **Prometheus /metrics** - integracija sa Grafana
- 💾 **LittleFS logging** - lokalno čuvanje podataka
- 🔧 **Modularna arhitektura** - 8 header fajlova za lakše održavanje
- ⚡ **Deep Sleep mod** - baterijski rad

### 📦 Preuzimanje v7.0:
**GitHub:** https://github.com/zoxknez/klimerko-7-ultimate-modular

**Autor v7.0:** o0o0o0o
- GitHub: https://github.com/zoxknez
- Portfolio: https://mojportfolio.vercel.app/

Detaljne informacije: pogledajte `README_v7.md`

---

## ℹ️ O Projektu

Originalni projekat: [Klimerko GitHub](https://github.com/DesconBelgrade/Klimerko)  
Modifikacije v6.6: Particle Counts, Fan Diagnostic, Humidity Comp, Stack Fix, Remote Controls.
v7.0 Ultimate Modular: o0o0o0o (https://github.com/zoxknez)