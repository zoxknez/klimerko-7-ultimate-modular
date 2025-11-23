# Klimerko 5.1 Professional

Najnovija, optimizovana verzija firmware-a za Klimerko uređaj. Ova verzija donosi maksimalnu stabilnost, uštedu memorije i potpunu daljinsku kontrolu bez potrebe za fizičkim pristupom uređaju.

## 🌟 Nove Funkcionalnosti (v5.1)

* **Daljinsko ažuriranje (HTTP Update):** Mogućnost nadogradnje firmware-a slanjem linka do `.bin` fajla direktno preko AllThingsTalk-a. Rešen problem sa memorijom pri HTTPS konekciji.
* **Optimizacija memorije:** Prelazak na `struct` za čuvanje podataka i korišćenje `F()` makroa drastično smanjuje upotrebu RAM-a i povećava stabilnost.
* **Razdvojena kontrola visine:** Asset `altitude-set` služi za bezbedno menjanje nadmorske visine na daljinu, dok `altitude` prikazuje trenutnu vrednost.
* **UI Fix:** Ispravljen prikaz u `altitude-set` polju (sada pamti unetu vrednost).
* **Daljinska kalibracija temperature:** Precizno podešavanje (offset) temperature slanjem broja preko `temperature-offset`.
* **Napredni proračun pritiska:** Korišćenje unete visine za tačan proračun pritiska na nivou mora (`pressureSea`).
* **Daljinska konfiguracija:** Pokretanje WiFi podešavanja komandom `wifi-config`.
* **Sigurnost:** Watchdog Timer štiti uređaj od blokiranja, a `JSON` parser je zaštićen od preopterećenja buffer-a.

---

## ☁️ AllThingsTalk Podešavanja

Da biste koristili sve opcije, potrebno je kreirati sledeće assete na AllThingsTalk platformi:

### Senzori (Podaci koje uređaj šalje vama)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `altitude` | Altitude | Sensor | Number |
| `pressureSea` | Pressure (Sea Level) | Sensor | Number |
| `HeatIndex` | Heat Index | Sensor | Number |
| `dewpoint` | Dew Point | Sensor | Number |
| `humidityAbs` | Absolute Humidity | Sensor | Number |

### Aktuatori (Komande koje vi šaljete uređaju)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `altitude-set` | Set Altitude | Actuator | Number (Integer) |
| `temperature-offset` | Set Temp Offset | Actuator | Number |
| `wifi-config` | Remote Config | Actuator | Boolean |
| `firmware-update` | Update Firmware | Actuator | String |

*Napomena: Standardni asseti (pm1, pm10, temperature, humidity...) ostaju nepromenjeni.*

---

## 🚀 Kako ažurirati Firmware na daljinu (firmware-update)

Ovo je najlakši način za nadogradnju uređaja koji su već montirani:

1.  U Arduino IDE-u idite na `Sketch` -> `Export Compiled Binary` (ovo pravi `.bin` fajl u folderu skice).
2.  Postavite taj `.bin` fajl na GitHub (ili drugi javni server).
3.  **VAŽNO:** Morate koristiti **RAW** link do fajla (na GitHub-u kliknite na "Raw" dugme ili "Download" pa kopirajte link).
    * *Primer dobrog linka:* `https://raw.githubusercontent.com/korisnik/repo/main/firmware.bin`
    * *Loš link (neće raditi):* `https://github.com/korisnik/repo/blob/main/firmware.bin`
4.  Na AllThingsTalk-u, u asset **`firmware-update`** nalepite taj RAW link i pošaljite.
5.  Klimerko će preuzeti fajl, instalirati ga i automatski se restartovati sa novim softverom.

---

## 🏔️ Promena nadmorske visine (altitude-set)

1.  U asset **`altitude-set`** upišite tačnu visinu u metrima (npr. `380`).
2.  Pošaljite komandu.
3.  Uređaj pamti visinu u trajnoj memoriji (EEPROM), a asset **`altitude`** (senzor) će se ažurirati pri sledećem slanju podataka kao potvrda da je promenu prihvaćena.

---

## 🌡️ Kalibracija temperature (temperature-offset)

1.  U asset **`temperature-offset`** upišite korekciju (npr. `-2.5` ako senzor pokazuje previše).
2.  Pošaljite komandu.
3.  Uređaj resetuje proseke merenja i momentalno primenjuje korekciju.

---

## ⚙️ Inicijalna Instalacija (Prvi put - v5.0+)

**Važno:** Zbog promena u strukturi memorije, prelazak sa verzije 4.x na 5.x zahteva ponovno unošenje podešavanja.

1.  Flešujte `.ino` fajl preko USB kabla.
2.  Nakon paljenja, držite **FLASH dugme** 2 sekunde da uđete u konfiguracioni mod.
3.  Povežite se telefonom/laptopom na WiFi mrežu `KLIMERKO-xxxx`.
4.  Idite na adresu `192.168.4.1`.
5.  Unesite WiFi podatke, Device ID, Token i početnu nadmorsku visinu.
6.  Sačuvajte.

---

## ℹ️ O Projektu

Originalni projekat: [Klimerko GitHub](https://github.com/DesconBelgrade/Klimerko)  
Razvoj: Vanja Stanić & Descon  
Modifikacije v5.1: Kompletna optimizacija koda, Struct EEPROM, HTTP Update Fix, Watchdog.