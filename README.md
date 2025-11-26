# Klimerko 5.4 Professional

Najnovija, optimizovana verzija firmware-a za Klimerko uređaj. Ova verzija donosi maksimalnu stabilnost u ekstremnim vremenskim uslovima, preciznu dijagnostiku i potpunu daljinsku kontrolu bez potrebe za fizičkim pristupom uređaju.

## 🌟 Nove Funkcionalnosti (v5.4)

* **Stabilnost na visokoj vlažnosti (Humidity Fix):** Rešen problem blokiranja senzora na 98-100% vlažnosti (magla). Uređaj sada pametno obrađuje zasićenje i nastavlja rad.
* **Daljinski restart:** Dodat asset `restart-device` koji omogućava ponovno pokretanje (reboot) uređaja na klik, bez fizičkog isključivanja iz struje.
* **Precizna dijagnostika signala (dBm):** WiFi signal se prikazuje kao numerička vrednost (npr. `-65` dBm) umesto opisnih ocena, za preciznije praćenje kvaliteta veze.
* **Daljinsko ažuriranje (HTTP Update):** Mogućnost nadogradnje firmware-a slanjem linka do `.bin` fajla direktno preko AllThingsTalk-a.
* **Razdvojena kontrola visine:** Asset `altitude-set` služi za bezbedno menjanje nadmorske visine na daljinu, dok `altitude` prikazuje trenutnu vrednost.
* **Daljinska kalibracija temperature:** Precizno podešavanje (offset) temperature slanjem broja preko `temperature-offset`.
* **Napredni proračun pritiska:** Korišćenje unete visine za tačan proračun pritiska na nivou mora (`pressureSea`).
* **Daljinska konfiguracija:** Pokretanje WiFi podešavanja komandom `wifi-config`.
* **Optimizacija memorije:** Korišćenje `struct` strukture za EEPROM i `F()` makroa za tekstove drastično smanjuje upotrebu RAM-a.

---

## ☁️ AllThingsTalk Podešavanja

Da biste koristili sve opcije, potrebno je kreirati (ili ažurirati) sledeće assete na AllThingsTalk platformi:

### Senzori (Podaci koje uređaj šalje vama)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `wifi-signal` | WiFi Signal (dBm) | Sensor | **Number (Integer)** |
| `altitude` | Altitude | Sensor | Number |
| `pressureSea` | Pressure (Sea Level) | Sensor | Number |
| `HeatIndex` | Heat Index | Sensor | Number |
| `dewpoint` | Dew Point | Sensor | Number |
| `humidityAbs` | Absolute Humidity | Sensor | Number |

*Napomena: Ako je stari `wifi-signal` bio tipa String, obrišite ga i napravite novi kao Number.*

### Aktuatori (Komande koje vi šaljete uređaju)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `restart-device` | Restart Device | Actuator | **Boolean** |
| `altitude-set` | Set Altitude | Actuator | Number (Integer) |
| `temperature-offset` | Set Temp Offset | Actuator | Number |
| `wifi-config` | Remote Config | Actuator | Boolean |
| `firmware-update` | Update Firmware | Actuator | String |

*Napomena: Standardni asseti (pm1, pm10, temperature, humidity...) ostaju nepromenjeni.*

---

## 🔄 Kako restartovati uređaj na daljinu (restart-device)

Ako uređaj prestane da šalje podatke ili želite da osvežite konekciju:

1.  Na AllThingsTalk-u koristite asset **`restart-device`**.
2.  Postavite vrednost na `True` (ili pošaljite `1`).
3.  Uređaj će primiti komandu, sačekati 1 sekundu i uraditi softverski restart.
4.  Svi podaci (visina, offset, wifi) ostaju sačuvani.

---

## 🚀 Kako ažurirati Firmware na daljinu (firmware-update)

Najlakši način za nadogradnju uređaja koji su već montirani:

1.  U Arduino IDE-u idite na `Sketch` -> `Export Compiled Binary` (ovo pravi `.bin` fajl).
2.  Postavite taj `.bin` fajl na GitHub (ili drugi javni server).
3.  **VAŽNO:** Morate koristiti **RAW** link do fajla.
    * *Primer dobrog linka:* `https://raw.githubusercontent.com/korisnik/repo/main/firmware.bin`
    * *Loš link (neće raditi):* `https://github.com/korisnik/repo/blob/main/firmware.bin`
4.  Na AllThingsTalk-u, u asset **`firmware-update`** nalepite taj RAW link i pošaljite.
5.  Klimerko će preuzeti fajl, instalirati ga i automatski se restartovati.

---

## 🏔️ Promena nadmorske visine (altitude-set)

1.  U asset **`altitude-set`** upišite tačnu visinu u metrima (npr. `380`).
2.  Pošaljite komandu.
3.  Uređaj pamti visinu u trajnoj memoriji, a asset **`altitude`** (senzor) se ažurira pri sledećem slanju kao potvrda.

---

## 🌡️ Kalibracija temperature (temperature-offset)

1.  U asset **`temperature-offset`** upišite korekciju (npr. `-2.5` ako senzor pokazuje previše).
2.  Pošaljite komandu.
3.  Uređaj resetuje proseke i momentalno primenjuje korekciju.

---

## 📶 Tumačenje WiFi Signala (dBm)

* **-50 do -60 dBm:** Odličan signal.
* **-60 do -70 dBm:** Dobar signal (stabilan rad).
* **-70 do -80 dBm:** Slabiji signal.
* **Ispod -85 dBm:** Kritično (mogući prekidi).

---

## ⚙️ Inicijalna Instalacija (Prvi put - v5.x)

**Napomena:** Zbog unapređenja načina čuvanja podataka (prelazak na Struct u v5.0), pri prvom učitavanju ove verzije stara podešavanja će biti resetovana.

1.  Flešujte `.ino` fajl preko USB kabla.
2.  Držite **FLASH dugme** 2 sekunde nakon paljenja.
3.  Povežite se na WiFi mrežu `KLIMERKO-xxxx`.
4.  Idite na adresu `192.168.4.1`.
5.  Unesite WiFi podatke, Tokene i početnu nadmorsku visinu.
6.  Sačuvajte.

---

## ℹ️ O Projektu

Originalni projekat: [Klimerko GitHub](https://github.com/DesconBelgrade/Klimerko)  
Razvoj: Vanja Stanić & Descon  
Modifikacije v5.4: Humidity Fix, Remote Restart, Struct EEPROM, HTTP Update, Watchdog, dBm Signal.
