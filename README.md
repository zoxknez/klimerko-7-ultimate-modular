# Klimerko 5.2 Professional

Najnovija, optimizovana verzija firmware-a za Klimerko uređaj. Ova verzija donosi maksimalnu stabilnost, preciznu dijagnostiku signala i potpunu daljinsku kontrolu.

## 🌟 Nove Funkcionalnosti (v5.2)

* **Precizna dijagnostika signala (dBm):** Asset `wifi-signal` sada šalje tačnu numeričku vrednost jačine signala (npr. `-65` dBm) umesto opisnih ocena. Ovo omogućava preciznije praćenje kvaliteta veze.
* **Stabilnost ažuriranja:** Rešen problem sa memorijom prilikom HTTPS konekcije. Daljinsko ažuriranje (`firmware-update`) je sada izmešteno u glavnu petlju radi veće pouzdanosti.
* **Optimizacija memorije:** Korišćenje `struct` strukture za EEPROM i `F()` makroa za tekstove drastično smanjuje upotrebu RAM-a.
* **Daljinsko ažuriranje (HTTP Update):** Nadogradnja firmware-a slanjem linka do `.bin` fajla.
* **Razdvojena kontrola visine:** Asset `altitude-set` služi za menjanje visine, dok `altitude` prikazuje trenutnu vrednost.
* **Daljinska kalibracija:** Podešavanje `temperature-offset` na daljinu.
* **Sigurnost:** Watchdog Timer i zaštićen JSON parser.

---

## ☁️ AllThingsTalk Podešavanja

**⚠️ VAŽNO:** Zbog promene formata WiFi signala, potrebno je ažurirati taj asset na platformi.

Idite na vaš uređaj na AllThingsTalk, kliknite na **+ NEW ASSET** (ili izmenite postojeće) prema ovoj tabeli:

### Senzori (Podaci koje uređaj šalje vama)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `wifi-signal` | WiFi Signal (dBm) | Sensor | **Number (Integer)** |
| `altitude` | Altitude | Sensor | Number |
| `pressureSea` | Pressure (Sea Level) | Sensor | Number |
| `HeatIndex` | Heat Index | Sensor | Number |
| `dewpoint` | Dew Point | Sensor | Number |
| `humidityAbs` | Absolute Humidity | Sensor | Number |

*Napomena: Ako je vaš stari `wifi-signal` asset bio tipa String, obrišite ga i napravite novi kao Number.*

### Aktuatori (Komande koje vi šaljete uređaju)
| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `altitude-set` | Set Altitude | Actuator | Number (Integer) |
| `temperature-offset` | Set Temp Offset | Actuator | Number |
| `wifi-config` | Remote Config | Actuator | Boolean |
| `firmware-update` | Update Firmware | Actuator | String |

---

## 🚀 Kako ažurirati Firmware na daljinu (firmware-update)

Najlakši način za nadogradnju uređaja bez kablova:

1.  U Arduino IDE-u: `Sketch` -> `Export Compiled Binary` (kreira `.bin` fajl).
2.  Postavite `.bin` fajl na GitHub.
3.  **VAŽNO:** Kopirajte **RAW** link do fajla.
    * *Ispravno:* `https://raw.githubusercontent.com/.../firmware.bin`
    * *Pogrešno:* `https://github.com/.../blob/...`
4.  Na AllThingsTalk-u, u asset **`firmware-update`** nalepite taj RAW link i pošaljite.
5.  Klimerko će preuzeti fajl i automatski se restartovati.

---

## 🏔️ Promena nadmorske visine (altitude-set)

1.  U asset **`altitude-set`** upišite tačnu visinu u metrima (npr. `380`).
2.  Pošaljite komandu.
3.  Uređaj pamti visinu u trajnoj memoriji, a asset **`altitude`** (senzor) se ažurira kao potvrda.

---

## 📶 Tumačenje WiFi Signala (dBm)

Sada kada dobijate brojeve, evo kako da znate da li je signal dobar:
* **-50 dBm do -60 dBm:** Odličan signal.
* **-60 dBm do -70 dBm:** Dobar signal (stabilan rad).
* **-70 dBm do -80 dBm:** Slabiji signal (moguća kašnjenja).
* **Ispod -85 dBm:** Kritično (česta diskonekcija).

---

## ⚙️ Inicijalna Instalacija (Prelazak na v5.x)

**Napomena:** Zbog unapređenja načina čuvanja podataka (prelazak na Struct), pri prvom učitavanju verzije 5.x, stara podešavanja će biti obrisana.

1.  Flešujte kod preko USB kabla.
2.  Držite **FLASH dugme** 2 sekunde za konfiguracioni mod.
3.  Povežite se na WiFi `KLIMERKO-xxxx` i idite na `192.168.4.1`.
4.  Ponovo unesite WiFi podatke, Tokene i visinu.
5.  Sačuvajte.

---

## ℹ️ O Projektu

Originalni projekat: [Klimerko GitHub](https://github.com/DesconBelgrade/Klimerko)  
Razvoj: Vanja Stanić & Descon  
Modifikacije v5.2: dBm signal, Struct EEPROM, HTTP Update Fix, Watchdog, Memory Optimization.
