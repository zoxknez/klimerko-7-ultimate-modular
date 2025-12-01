# Klimerko 7.0 Ultimate Edition

Najnovija, **kompletno nadograđena** verzija firmware-a za Klimerko uređaj. 
Ova verzija donosi **sve funkcionalnosti** koje jedan moderan IoT uređaj treba da ima.

## 🆕 Šta je novo u verziji 7.0 Ultimate

### ⏰ NTP Sinhronizacija Vremena
* **Pravo vreme**: Sinhronizacija sa pool.ntp.org i time.nist.gov
* **ISO Timestamp**: Logovi sa pravim datumom i vremenom
* **Timezone podrška**: Konfigurabilan GMT offset

### 🚨 Alarm Sistem
* **PM2.5 Alarm**: Aktivira se kada PM2.5 > 35 µg/m³ (WHO guideline)
* **PM10 Alarm**: Aktivira se kada PM10 > 45 µg/m³
* **Vizuelni alarm**: Brzo treptanje LED-a
* **MQTT notifikacija**: Šalje alarm na cloud
* **Cooldown**: 1 sat između uzastopnih alarma
* **Kontrola**: Uključi/isključi preko MQTT `alarm-enable` asset-a

### 📊 Grafici i Vizualizacija
* **Chart.js**: Real-time grafici u browseru
* **PM History**: Linijski grafik za PM1, PM2.5, PM10
* **Temp/Humidity**: Dual-axis grafik za klimatske podatke
* **Tab navigacija**: Live Data / Charts / Statistics paneli
* **20 tačaka**: Zadnjih 20 merenja u grafikonu

### 📈 Prometheus Metrics
* **Endpoint**: `/metrics` za Prometheus scraping
* **Podržane metrike**:
  - klimerko_pm1, pm25, pm10
  - klimerko_temperature, humidity, pressure
  - klimerko_wifi_rssi
  - klimerko_uptime_seconds
  - klimerko_boot_count
  - klimerko_heap_free
  - klimerko_publishes_total
  - klimerko_alarm_triggered
  - klimerko_heat_index, dewpoint
* **Grafana-ready**: Lako se integriše sa Grafana

### 🔧 Konfigurabilni MQTT Broker
* **Custom broker**: Promenite MQTT server bez rekompilacije
* **MQTT komanda**: 
  ```json
  {"server": "broker.example.com", "port": 1883}
  ```
* **Perzistentno**: Sačuvano u EEPROM-u

### 🎚️ Kalibracija Senzora
* **PM2.5 faktor**: Multiplikator za korekciju PM2.5
* **PM10 faktor**: Multiplikator za korekciju PM10
* **Temp offset**: Dodatna korekcija temperature
* **Humidity offset**: Dodatna korekcija vlažnosti
* **MQTT komanda**:
  ```json
  {"pm25": 1.1, "pm10": 0.95, "temp": -0.5, "hum": 2.0}
  ```

### 🌐 mDNS Podrška
* **Lokalni pristup**: `klimerko-xxxxxx.local`
* **Prometheus discovery**: mDNS servis za auto-discovery

### 📊 Lokalni Web Dashboard
* **Moderne UI**: Responzivan dizajn sa tamnom temom
* **Tabovi**: Live Data, Charts, Statistics
* **Real-time**: Auto-refresh svakih 5 sekundi
* **Alarm badge**: Vizuelni indikator alarma

### 💾 LittleFS Data Logging
* **Offline čuvanje**: Čuva do 100 poslednjih merenja
* **Perzistentno**: Podaci preživljavaju restart
* **API**: `/api/log` za pristup istoriji

### 😴 Deep Sleep Mode
* **Za baterijske instalacije**: Dramatična ušteda energije
* **MQTT kontrola**: `deep-sleep` asset
* **Hardverski zahtev**: D0 → RST veza

### 📈 Statistika i Uptime
* **Boot count, WiFi/MQTT reconnects**
* **Successful/Failed publishes**
* **Uptime tracking**
* **Perzistencija u EEPROM-u**

---

## 🌐 API Endpointi

| Endpoint | Opis |
|----------|------|
| `/` | Web Dashboard sa graficima |
| `/api/data` | JSON sa trenutnim podacima |
| `/api/stats` | JSON sa sistemskom statistikom |
| `/api/log` | JSON sa istorijom merenja |
| `/metrics` | Prometheus format metrike |

---

## 🎮 MQTT Komande

| Asset | Format | Opis |
|-------|--------|------|
| `interval` | `{"value": 5}` | Interval merenja (minuti) |
| `deep-sleep` | `{"value": "true"}` | Deep sleep on/off |
| `alarm-enable` | `{"value": "true"}` | Alarm sistem on/off |
| `calibration` | `{"pm25": 1.1, "pm10": 1.0}` | Kalibracija senzora |
| `mqtt-broker` | `{"server": "...", "port": 1883}` | Custom MQTT broker |
| `temperature-offset` | `{"value": "-2.5"}` | Temp offset |
| `altitude-set` | `{"value": "200"}` | Nadmorska visina |
| `wifi-config` | `{"value": "true"}` | Pokreni config portal |
| `restart-device` | `{"value": "true"}` | Restart uređaja |
| `firmware-update` | `{"value": "https://..."}` | OTA update URL |

---

## 📊 Prometheus + Grafana Setup

### prometheus.yml
```yaml
scrape_configs:
  - job_name: 'klimerko'
    static_configs:
      - targets: ['klimerko-xxxxxx.local:80']
    metrics_path: '/metrics'
    scrape_interval: 30s
```

### Grafana Dashboard
Importujte metrike:
- klimerko_pm25 → PM2.5 gauge
- klimerko_temperature → Temperature graph
- klimerko_wifi_rssi → Signal strength

---

## 🔒 Bezbednost

### AP Lozinka: `K` + ChipID (hex)
### OTA Lozinka: `O` + ChipID (hex)

---

## 📝 Changelog v7.0 Ultimate (Decembar 2025)

### Nove funkcionalnosti
- ✅ NTP sinhronizacija (pravi timestamp)
- ✅ Alarm sistem (PM2.5/PM10 threshold)
- ✅ Chart.js grafici u dashboardu
- ✅ Prometheus metrics endpoint
- ✅ Konfigurabilni MQTT broker
- ✅ Sensor calibration mode
- ✅ mDNS podrška
- ✅ LittleFS data logging
- ✅ Deep Sleep mode
- ✅ Uptime i statistika

### Bezbednost i stabilnost
- ✅ Jedinstvena AP/OTA lozinka
- ✅ Buffer overflow fix
- ✅ EEPROM CRC32 validacija
- ✅ Memory leak fix
- ✅ PMS parser timeout
- ✅ EPA humidity correction
- ✅ Glatki Heat Index
- ✅ Eksponencijalni WiFi backoff

---

## 💡 Memory Footprint

| Resurs | Korišćenje |
|--------|------------|
| Flash | ~500KB (od 1MB) |
| RAM | ~32KB slobodno |
| LittleFS | ~50KB za podatke |
| EEPROM | ~200 bytes |

---

## 👥 Autori

- **Originalni Klimerko**: Vanja Stanic (www.vanjastanic.com)
- **v7.0 Ultimate Modular**: o0o0o0o
  - GitHub: https://github.com/zoxknez
  - Portfolio: https://mojportfolio.vercel.app/
- **Organizacija**: Descon.me (Beograd)
