# Klimerko 7.0 Ultimate Modular 🚀

**Najnovija verzija!** Kompletno refaktorisana i nadograđena verzija firmware-a za Klimerko uređaj sa modernom modularnom arhitekturom.

**Autor v7.0:** o0o0o0o
- 🔗 GitHub: https://github.com/zoxknez
- 🌐 Portfolio: https://mojportfolio.vercel.app/

---

## ✨ Nove funkcionalnosti u v7.0

| Funkcionalnost | Opis |
|----------------|------|
| 📡 **mDNS Discovery** | Pristup preko `klimerko-xxxxxx.local` - bez IP adrese! |
| 🌐 **Web Dashboard** | Real-time prikaz svih podataka u browseru |
| 📊 **Chart.js grafici** | Interaktivna vizualizacija PM i klimatskih podataka |
| ⏰ **NTP sinhronizacija** | Pravo vreme u svim logovima |
| 🚨 **Alarm sistem** | Automatska upozorenja za PM2.5/PM10 pragove |
| 📈 **Prometheus /metrics** | Direktna integracija sa Grafana |
| 💾 **LittleFS logging** | Lokalno čuvanje podataka na uređaju |
| 🔧 **Modularna arhitektura** | 8 header fajlova za lakše održavanje |
| ⚡ **Deep Sleep mod** | Baterijski rad sa minimalnom potrošnjom |
| 🔐 **Poboljšana bezbednost** | CRC32, unique passwords, buffer overflow zaštita |

---

## 📦 Struktura Projekta

```
klimerko-7-ultimate-modular/
├── Klimerko_7.0_Modular.ino    # Glavni fajl
├── src/
│   └── klimerko/               # Modularni header fajlovi
│       ├── config.h            # Sve konfiguracije
│       ├── types.h             # Strukture i enumi
│       ├── utils.h             # CRC32, kalkulacije
│       ├── sensors.h           # PMS7003 + BME280
│       ├── network.h           # WiFi, MQTT, mDNS, NTP, OTA
│       ├── storage.h           # EEPROM + LittleFS
│       ├── web_dashboard.h     # HTTP server + Prometheus
│       └── alarms.h            # Alarm sistem
├── README.md
└── README_v7.md                # Detaljna dokumentacija
```

---

## 🖥️ Web Dashboard

Pristupite dashboardu na: `http://klimerko-xxxxxx.local`

- **Live Data** - trenutni podaci sa senzora
- **Charts** - PM i temperatura/vlažnost grafici
- **Statistics** - uptime, boot count, publish stats
- **Prometheus** - `/metrics` endpoint za Grafana

---

## 🚨 Alarm Sistem

```
PM2.5 > 35 µg/m³  →  ⚠️ Alarm + LED treptanje + MQTT notifikacija
PM10  > 45 µg/m³  →  ⚠️ Alarm + LED treptanje + MQTT notifikacija
```

Kontrola preko MQTT: `alarm-enable` asset (true/false)

---

## ☁️ AllThingsTalk Podešavanja

### Senzori (Podaci koje uređaj šalje)

| Name (Ime) | Title (Naslov) | Type | Profile | Opis |
| :--- | :--- | :--- | :--- | :--- |
| **PM SENZORI** | | | | |
| `pm1` | PM1 | Sensor | Integer | Čestice < 1µm |
| `pm2-5` | PM2.5 | Sensor | Integer | Čestice < 2.5µm |
| `pm10` | PM10 | Sensor | Integer | Čestice < 10µm |
| `pm1-c` | PM1 (Corrected) | Sensor | Integer | Korigovano za vlagu |
| `pm2-5-c` | PM2.5 (Corrected)| Sensor | Integer | Korigovano za vlagu |
| `pm10-c` | PM10 (Corrected) | Sensor | Integer | Korigovano za vlagu |
| **PARTICLE COUNTS** | | | | |
| `count-0-3` | Count > 0.3um | Sensor | Integer | Čestice dima |
| `count-0-5` | Count > 0.5um | Sensor | Integer | Bakterije, fina prašina |
| `count-1-0` | Count > 1.0um | Sensor | Integer | Dim cigarete |
| `count-2-5` | Count > 2.5um | Sensor | Integer | Fina prašina |
| `count-5-0` | Count > 5.0um | Sensor | Integer | Krupna prašina |
| `count-10-0` | Count > 10um | Sensor | Integer | Vidljiva prašina |
| **KLIMATSKI** | | | | |
| `temperature` | Temperature | Sensor | Number | Temperatura °C |
| `humidity` | Humidity | Sensor | Number | Vlažnost % |
| `pressure` | Pressure | Sensor | Number | Pritisak hPa |
| `dewpoint` | Dew Point | Sensor | Number | Tačka rose |
| `humidityAbs` | Absolute Humidity | Sensor | Number | Apsolutna vlažnost |
| `HeatIndex` | Heat Index | Sensor | Number | Subjektivni osećaj |
| `pressureSea` | Sea Level Pressure | Sensor | Number | Pritisak nivoa mora |
| **DIJAGNOSTIKA** | | | | |
| `sensor-status` | Sensor Status | Sensor | **String** | OK, Fan Stuck, Error... |
| `wifi-signal` | WiFi Signal | Sensor | Integer | RSSI u dBm |
| `air-quality` | Air Quality | Sensor | String | Excellent/Good/Polluted |

### Aktuatori (Komande koje šaljete uređaju)

| Name (Ime) | Title (Naslov) | Type | Profile |
| :--- | :--- | :--- | :--- |
| `restart-device` | Restart Device | Actuator | Boolean |
| `altitude-set` | Set Altitude | Actuator | Integer |
| `temperature-offset` | Set Temp Offset | Actuator | Number |
| `wifi-config` | Remote Config | Actuator | Boolean |
| `firmware-update` | Update Firmware | Actuator | String |
| `alarm-enable` | Enable Alarms | Actuator | Boolean |
| `deep-sleep` | Deep Sleep Mode | Actuator | Boolean |

---

## 🔬 Kako tumačiti podatke

### Broj čestica (Particle Counts)
* **Visok `count-0-3`:** Sagorevanje (dim, loženje, saobraćaj)
* **Visok `count-5-0` i `count-10-0`:** Mehaničko zagađenje (prašina, polen)

### Status Senzora
* **"OK"** - Sve radi
* **"Fan Stuck / Error"** - Ventilator zaglavljen
* **"Zero Data Error"** - Senzor šalje nule
* **"Sensor Offline"** - Senzor ne odgovara

### Korigovane vrednosti (`pm2-5-c`)
Pri vlažnosti > 70%, sirovi podaci mogu biti netačni. Korigovane vrednosti daju realniju sliku.

---

## ⚙️ Inicijalna Instalacija

1. Flešujte kod preko USB kabla
2. Držite **FLASH dugme** 2 sekunde
3. Povežite se na WiFi `KLIMERKO-xxxx` → `192.168.4.1`
4. Unesite AllThingsTalk podatke i sačuvajte

---

## 🔄 Daljinski Restart

1. Na AllThingsTalk koristite asset **`restart-device`**
2. Postavite vrednost na `True`
3. Uređaj se restartuje za 1 sekundu

---

## 🚀 OTA Firmware Update

1. Arduino IDE: `Sketch` → `Export Compiled Binary`
2. Upload `.bin` na GitHub
3. Kopirajte **RAW** link: `https://raw.githubusercontent.com/.../firmware.bin`
4. U asset **`firmware-update`** nalepite link

---

## 💡 Memory Footprint

| Resurs | Korišćenje |
|--------|------------|
| Flash | ~500KB (od 1MB) |
| RAM | ~32KB slobodno |
| LittleFS | ~50KB za podatke |
| EEPROM | ~200 bytes |

---

## ℹ️ O Projektu

| Verzija | Autor | Link |
|---------|-------|------|
| **v7.0 Ultimate Modular** | **o0o0o0o** | **https://github.com/zoxknez** |
| Originalni Klimerko | Vanja Stanic | https://descon.me/klimerko |
| v6.6 Stable | DesconBelgrade | https://github.com/DesconBelgrade/Klimerko |

---

*Decembar 2025*
