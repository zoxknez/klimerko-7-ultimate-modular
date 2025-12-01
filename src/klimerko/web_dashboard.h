/**
 * @file web_dashboard.h
 * @brief Klimerko Web Dashboard - HTTP Server and Prometheus Metrics
 * @version 7.0 Ultimate
 * 
 * Provides local web dashboard with real-time data visualization,
 * Chart.js graphs, API endpoints, and Prometheus metrics export.
 */

#ifndef KLIMERKO_WEB_DASHBOARD_H
#define KLIMERKO_WEB_DASHBOARD_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "../ArduinoJson-v6.18.5.h"

// ============================================================================
// GLOBAL WEB SERVER
// ============================================================================

extern ESP8266WebServer webServer;

// External references for data access
extern SensorData sensorData;
extern Statistics stats;
extern char klimerkoID[32];
extern bool ntpSynced;
extern bool alarmTriggered;
extern unsigned long bootTime;

// ============================================================================
// DASHBOARD HTML (PROGMEM)
// ============================================================================

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Klimerko Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root { --bg: #0f0f1a; --card: #1a1a2e; --card2: #16213e; --text: #eee; --accent: #0f3460; --good: #4ade80; --warn: #fbbf24; --bad: #f87171; --blue: #60a5fa; }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: var(--bg); color: var(--text); min-height: 100vh; padding: 15px; }
    .container { max-width: 1400px; margin: 0 auto; }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; flex-wrap: wrap; gap: 10px; }
    h1 { color: #fff; font-size: 1.6rem; }
    .time { color: #888; font-size: 0.9rem; }
    .alarm-badge { background: var(--bad); color: #fff; padding: 5px 12px; border-radius: 20px; font-size: 0.8rem; animation: pulse 1s infinite; }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 12px; margin-bottom: 20px; }
    .card { background: var(--card); border-radius: 12px; padding: 16px; }
    .card.wide { grid-column: span 2; }
    .card.full { grid-column: 1 / -1; }
    .card h2 { font-size: 0.75rem; color: #888; margin-bottom: 8px; text-transform: uppercase; letter-spacing: 1px; }
    .value { font-size: 2rem; font-weight: bold; }
    .unit { font-size: 0.85rem; color: #888; }
    .status { display: inline-block; padding: 3px 10px; border-radius: 12px; font-size: 0.75rem; margin-top: 8px; }
    .good { background: var(--good); color: #000; }
    .warn { background: var(--warn); color: #000; }
    .bad { background: var(--bad); color: #000; }
    .chart-container { height: 200px; margin-top: 10px; }
    .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(100px, 1fr)); gap: 8px; }
    .stat { background: var(--card2); padding: 12px; border-radius: 8px; text-align: center; }
    .stat-value { font-size: 1.2rem; font-weight: bold; color: var(--blue); }
    .stat-label { font-size: 0.7rem; color: #666; margin-top: 4px; }
    .tabs { display: flex; gap: 10px; margin-bottom: 15px; flex-wrap: wrap; }
    .tab { background: var(--card); padding: 8px 16px; border-radius: 8px; cursor: pointer; font-size: 0.85rem; border: none; color: #888; }
    .tab.active { background: var(--accent); color: #fff; }
    .panel { display: none; }
    .panel.active { display: block; }
    .footer { text-align: center; color: #444; font-size: 0.75rem; margin-top: 20px; }
    @media (max-width: 600px) { .value { font-size: 1.6rem; } .card.wide { grid-column: span 1; } }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🌡️ Klimerko</h1>
      <div style="display:flex;align-items:center;gap:15px;">
        <span id="alarm-badge" class="alarm-badge" style="display:none;">⚠️ ALARM</span>
        <span class="time" id="time">--:--:--</span>
      </div>
    </header>
    
    <div class="tabs">
      <button class="tab active" onclick="showPanel('live')">Live Data</button>
      <button class="tab" onclick="showPanel('charts')">Charts</button>
      <button class="tab" onclick="showPanel('stats')">Statistics</button>
    </div>
    
    <div id="live" class="panel active">
      <div class="grid">
        <div class="card">
          <h2>PM2.5</h2>
          <span class="value" id="pm25">--</span><span class="unit">µg/m³</span>
          <div><span class="status" id="pm25-status">--</span></div>
        </div>
        <div class="card">
          <h2>PM10</h2>
          <span class="value" id="pm10">--</span><span class="unit">µg/m³</span>
          <div><span class="status" id="pm10-status">--</span></div>
        </div>
        <div class="card">
          <h2>PM1</h2>
          <span class="value" id="pm1">--</span><span class="unit">µg/m³</span>
        </div>
        <div class="card">
          <h2>Temperature</h2>
          <span class="value" id="temp">--</span><span class="unit">°C</span>
        </div>
        <div class="card">
          <h2>Humidity</h2>
          <span class="value" id="hum">--</span><span class="unit">%</span>
        </div>
        <div class="card">
          <h2>Pressure</h2>
          <span class="value" id="pres">--</span><span class="unit">hPa</span>
        </div>
        <div class="card wide">
          <h2>Air Quality Index</h2>
          <span class="value" id="aq">--</span>
          <div><span class="status" id="aq-status">--</span></div>
        </div>
      </div>
    </div>
    
    <div id="charts" class="panel">
      <div class="grid">
        <div class="card full">
          <h2>PM History (Last 20 readings)</h2>
          <div class="chart-container"><canvas id="pmChart"></canvas></div>
        </div>
        <div class="card full">
          <h2>Temperature & Humidity</h2>
          <div class="chart-container"><canvas id="envChart"></canvas></div>
        </div>
      </div>
    </div>
    
    <div id="stats" class="panel">
      <div class="card">
        <h2>System Statistics</h2>
        <div class="stats-grid">
          <div class="stat"><div class="stat-value" id="uptime">--</div><div class="stat-label">Uptime</div></div>
          <div class="stat"><div class="stat-value" id="boots">--</div><div class="stat-label">Boots</div></div>
          <div class="stat"><div class="stat-value" id="heap">--</div><div class="stat-label">Free RAM</div></div>
          <div class="stat"><div class="stat-value" id="wifi">--</div><div class="stat-label">WiFi dBm</div></div>
          <div class="stat"><div class="stat-value" id="publishes">--</div><div class="stat-label">Publishes</div></div>
          <div class="stat"><div class="stat-value" id="ntp">--</div><div class="stat-label">NTP Sync</div></div>
        </div>
      </div>
    </div>
    
    <div class="footer">Klimerko 7.0 Ultimate • Auto-refresh 5s • <a href="/metrics" style="color:#666;">Prometheus</a></div>
  </div>
  
  <script>
    let pmChart, envChart;
    const pmHistory = {labels:[], pm1:[], pm25:[], pm10:[]};
    const envHistory = {labels:[], temp:[], hum:[]};
    const maxPoints = 20;
    
    function initCharts() {
      const ctx1 = document.getElementById('pmChart').getContext('2d');
      pmChart = new Chart(ctx1, {
        type: 'line',
        data: {
          labels: pmHistory.labels,
          datasets: [
            {label: 'PM1', data: pmHistory.pm1, borderColor: '#60a5fa', tension: 0.3, fill: false},
            {label: 'PM2.5', data: pmHistory.pm25, borderColor: '#fbbf24', tension: 0.3, fill: false},
            {label: 'PM10', data: pmHistory.pm10, borderColor: '#f87171', tension: 0.3, fill: false}
          ]
        },
        options: {responsive: true, maintainAspectRatio: false, plugins: {legend: {labels: {color: '#888'}}}, scales: {x: {ticks: {color: '#666'}}, y: {ticks: {color: '#666'}, beginAtZero: true}}}
      });
      
      const ctx2 = document.getElementById('envChart').getContext('2d');
      envChart = new Chart(ctx2, {
        type: 'line',
        data: {
          labels: envHistory.labels,
          datasets: [
            {label: 'Temp °C', data: envHistory.temp, borderColor: '#f87171', tension: 0.3, yAxisID: 'y'},
            {label: 'Humidity %', data: envHistory.hum, borderColor: '#60a5fa', tension: 0.3, yAxisID: 'y1'}
          ]
        },
        options: {responsive: true, maintainAspectRatio: false, plugins: {legend: {labels: {color: '#888'}}}, scales: {x: {ticks: {color: '#666'}}, y: {type: 'linear', position: 'left', ticks: {color: '#f87171'}}, y1: {type: 'linear', position: 'right', ticks: {color: '#60a5fa'}, grid: {drawOnChartArea: false}}}}
      });
    }
    
    function showPanel(id) {
      document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      document.getElementById(id).classList.add('active');
      event.target.classList.add('active');
    }
    
    function getStatus(pm, type) {
      const limits = type === 'pm25' ? [10, 25, 50] : [20, 40, 100];
      if (pm <= limits[0]) return {cls: 'good', txt: 'Excellent'};
      if (pm <= limits[1]) return {cls: 'warn', txt: 'Moderate'};
      return {cls: 'bad', txt: 'Poor'};
    }
    
    function updateTime() {
      document.getElementById('time').textContent = new Date().toLocaleTimeString();
    }
    
    function fetchData() {
      fetch('/api/data').then(r => r.json()).then(d => {
        document.getElementById('pm1').textContent = d.pm1 ?? '--';
        document.getElementById('pm25').textContent = d.pm25 ?? '--';
        document.getElementById('pm10').textContent = d.pm10 ?? '--';
        document.getElementById('temp').textContent = d.temp?.toFixed(1) ?? '--';
        document.getElementById('hum').textContent = d.hum?.toFixed(1) ?? '--';
        document.getElementById('pres').textContent = d.pres?.toFixed(1) ?? '--';
        document.getElementById('aq').textContent = d.aq ?? '--';
        document.getElementById('uptime').textContent = d.uptime ?? '--';
        document.getElementById('boots').textContent = d.boots ?? '--';
        document.getElementById('heap').textContent = ((d.heap||0)/1024).toFixed(0) + 'K';
        document.getElementById('wifi').textContent = d.wifi ?? '--';
        document.getElementById('publishes').textContent = d.publishes ?? '--';
        document.getElementById('ntp').textContent = d.ntp ? 'Yes' : 'No';
        
        const pm25s = getStatus(d.pm25, 'pm25');
        const pm10s = getStatus(d.pm10, 'pm10');
        document.getElementById('pm25-status').className = 'status ' + pm25s.cls;
        document.getElementById('pm25-status').textContent = pm25s.txt;
        document.getElementById('pm10-status').className = 'status ' + pm10s.cls;
        document.getElementById('pm10-status').textContent = pm10s.txt;
        document.getElementById('aq-status').className = 'status ' + pm10s.cls;
        document.getElementById('aq-status').textContent = d.aq;
        
        document.getElementById('alarm-badge').style.display = d.alarm ? 'block' : 'none';
        
        // Update charts
        const now = new Date().toLocaleTimeString().slice(0,5);
        pmHistory.labels.push(now); pmHistory.pm1.push(d.pm1); pmHistory.pm25.push(d.pm25); pmHistory.pm10.push(d.pm10);
        envHistory.labels.push(now); envHistory.temp.push(d.temp); envHistory.hum.push(d.hum);
        
        if (pmHistory.labels.length > maxPoints) {
          pmHistory.labels.shift(); pmHistory.pm1.shift(); pmHistory.pm25.shift(); pmHistory.pm10.shift();
          envHistory.labels.shift(); envHistory.temp.shift(); envHistory.hum.shift();
        }
        
        if (pmChart) pmChart.update();
        if (envChart) envChart.update();
      }).catch(e => console.error(e));
    }
    
    initCharts();
    updateTime();
    fetchData();
    setInterval(updateTime, 1000);
    setInterval(fetchData, 5000);
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// HTTP HANDLERS
// ============================================================================

/**
 * @brief Serve main dashboard page
 */
inline void handleRoot() {
  webServer.send(200, "text/html", DASHBOARD_HTML);
}

/**
 * @brief Serve current sensor data as JSON
 */
inline void handleApiData() {
  StaticJsonDocument<512> doc;
  
  doc["pm1"] = sensorData.pm1;
  doc["pm25"] = sensorData.pm25;
  doc["pm10"] = sensorData.pm10;
  doc["temp"] = sensorData.temperature;
  doc["hum"] = sensorData.humidity;
  doc["pres"] = sensorData.pressure;
  doc["aq"] = airQualityToString(sensorData.airQuality);
  doc["uptime"] = formatUptime(getUptimeSeconds(bootTime));
  doc["heap"] = ESP.getFreeHeap();
  doc["wifi"] = WiFi.isConnected() ? WiFi.RSSI() : 0;
  doc["publishes"] = stats.successfulPublishes;
  doc["boots"] = stats.bootCount;
  doc["ntp"] = ntpSynced;
  doc["alarm"] = alarmTriggered;
  
  String response;
  serializeJson(doc, response);
  webServer.send(200, "application/json", response);
}

/**
 * @brief Serve system statistics as JSON
 */
inline void handleApiStats() {
  StaticJsonDocument<384> doc;
  
  doc["bootCount"] = stats.bootCount;
  doc["wifiReconnects"] = stats.wifiReconnects;
  doc["mqttReconnects"] = stats.mqttReconnects;
  doc["successfulPublishes"] = stats.successfulPublishes;
  doc["failedPublishes"] = stats.failedPublishes;
  doc["uptimeSeconds"] = getUptimeSeconds(bootTime);
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["chipId"] = ESP.getChipId();
  doc["flashSize"] = ESP.getFlashChipRealSize();
  doc["sketchSize"] = ESP.getSketchSize();
  doc["freeSketch"] = ESP.getFreeSketchSpace();
  
  String response;
  serializeJson(doc, response);
  webServer.send(200, "application/json", response);
}

/**
 * @brief Stream log file as JSON
 */
inline void handleApiLog() {
  if (LittleFS.exists(LOG_FILE_PATH)) {
    File logFile = LittleFS.open(LOG_FILE_PATH, "r");
    if (logFile) {
      webServer.streamFile(logFile, "application/json");
      logFile.close();
      return;
    }
  }
  webServer.send(200, "application/json", "[]");
}

/**
 * @brief Handle 404 not found
 */
inline void handleNotFound() {
  webServer.send(404, "text/plain", "Not Found");
}

// ============================================================================
// PROMETHEUS METRICS
// ============================================================================

/**
 * @brief Serve Prometheus metrics endpoint
 */
inline void handlePrometheusMetrics() {
  String metrics = "";
  String device = String(klimerkoID);
  
  // Sensor metrics
  metrics += "# HELP klimerko_pm1 PM1.0 concentration in µg/m³\n";
  metrics += "# TYPE klimerko_pm1 gauge\n";
  metrics += "klimerko_pm1{device=\"" + device + "\"} " + String(sensorData.pm1) + "\n";
  
  metrics += "# HELP klimerko_pm25 PM2.5 concentration in µg/m³\n";
  metrics += "# TYPE klimerko_pm25 gauge\n";
  metrics += "klimerko_pm25{device=\"" + device + "\"} " + String(sensorData.pm25) + "\n";
  
  metrics += "# HELP klimerko_pm10 PM10 concentration in µg/m³\n";
  metrics += "# TYPE klimerko_pm10 gauge\n";
  metrics += "klimerko_pm10{device=\"" + device + "\"} " + String(sensorData.pm10) + "\n";
  
  metrics += "# HELP klimerko_pm25_corrected Humidity-corrected PM2.5 in µg/m³\n";
  metrics += "# TYPE klimerko_pm25_corrected gauge\n";
  metrics += "klimerko_pm25_corrected{device=\"" + device + "\"} " + String(sensorData.pm25_corrected) + "\n";
  
  metrics += "# HELP klimerko_pm10_corrected Humidity-corrected PM10 in µg/m³\n";
  metrics += "# TYPE klimerko_pm10_corrected gauge\n";
  metrics += "klimerko_pm10_corrected{device=\"" + device + "\"} " + String(sensorData.pm10_corrected) + "\n";
  
  metrics += "# HELP klimerko_temperature Temperature in Celsius\n";
  metrics += "# TYPE klimerko_temperature gauge\n";
  metrics += "klimerko_temperature{device=\"" + device + "\"} " + String(sensorData.temperature, 2) + "\n";
  
  metrics += "# HELP klimerko_humidity Relative humidity in percent\n";
  metrics += "# TYPE klimerko_humidity gauge\n";
  metrics += "klimerko_humidity{device=\"" + device + "\"} " + String(sensorData.humidity, 2) + "\n";
  
  metrics += "# HELP klimerko_pressure Atmospheric pressure in hPa\n";
  metrics += "# TYPE klimerko_pressure gauge\n";
  metrics += "klimerko_pressure{device=\"" + device + "\"} " + String(sensorData.pressure, 2) + "\n";
  
  metrics += "# HELP klimerko_heat_index Heat index in Celsius\n";
  metrics += "# TYPE klimerko_heat_index gauge\n";
  metrics += "klimerko_heat_index{device=\"" + device + "\"} " + String(sensorData.heatIndex, 2) + "\n";
  
  metrics += "# HELP klimerko_dewpoint Dewpoint temperature in Celsius\n";
  metrics += "# TYPE klimerko_dewpoint gauge\n";
  metrics += "klimerko_dewpoint{device=\"" + device + "\"} " + String(sensorData.dewpoint, 2) + "\n";
  
  // System metrics
  metrics += "# HELP klimerko_wifi_rssi WiFi signal strength in dBm\n";
  metrics += "# TYPE klimerko_wifi_rssi gauge\n";
  metrics += "klimerko_wifi_rssi{device=\"" + device + "\"} " + String(WiFi.isConnected() ? WiFi.RSSI() : 0) + "\n";
  
  metrics += "# HELP klimerko_uptime_seconds Device uptime in seconds\n";
  metrics += "# TYPE klimerko_uptime_seconds counter\n";
  metrics += "klimerko_uptime_seconds{device=\"" + device + "\"} " + String(getUptimeSeconds(bootTime)) + "\n";
  
  metrics += "# HELP klimerko_boot_count Number of device boots\n";
  metrics += "# TYPE klimerko_boot_count counter\n";
  metrics += "klimerko_boot_count{device=\"" + device + "\"} " + String(stats.bootCount) + "\n";
  
  metrics += "# HELP klimerko_heap_free Free heap memory in bytes\n";
  metrics += "# TYPE klimerko_heap_free gauge\n";
  metrics += "klimerko_heap_free{device=\"" + device + "\"} " + String(ESP.getFreeHeap()) + "\n";
  
  metrics += "# HELP klimerko_publishes_total Total successful MQTT publishes\n";
  metrics += "# TYPE klimerko_publishes_total counter\n";
  metrics += "klimerko_publishes_total{device=\"" + device + "\"} " + String(stats.successfulPublishes) + "\n";
  
  metrics += "# HELP klimerko_publishes_failed Total failed MQTT publishes\n";
  metrics += "# TYPE klimerko_publishes_failed counter\n";
  metrics += "klimerko_publishes_failed{device=\"" + device + "\"} " + String(stats.failedPublishes) + "\n";
  
  metrics += "# HELP klimerko_wifi_reconnects Total WiFi reconnection attempts\n";
  metrics += "# TYPE klimerko_wifi_reconnects counter\n";
  metrics += "klimerko_wifi_reconnects{device=\"" + device + "\"} " + String(stats.wifiReconnects) + "\n";
  
  metrics += "# HELP klimerko_mqtt_reconnects Total MQTT reconnection attempts\n";
  metrics += "# TYPE klimerko_mqtt_reconnects counter\n";
  metrics += "klimerko_mqtt_reconnects{device=\"" + device + "\"} " + String(stats.mqttReconnects) + "\n";
  
  metrics += "# HELP klimerko_alarm_triggered Alarm currently triggered (1=yes, 0=no)\n";
  metrics += "# TYPE klimerko_alarm_triggered gauge\n";
  metrics += "klimerko_alarm_triggered{device=\"" + device + "\"} " + String(alarmTriggered ? 1 : 0) + "\n";
  
  metrics += "# HELP klimerko_ntp_synced NTP time synchronized (1=yes, 0=no)\n";
  metrics += "# TYPE klimerko_ntp_synced gauge\n";
  metrics += "klimerko_ntp_synced{device=\"" + device + "\"} " + String(ntpSynced ? 1 : 0) + "\n";
  
  // Particle counts
  metrics += "# HELP klimerko_particle_count_0_3 Particle count >0.3µm per 0.1L\n";
  metrics += "# TYPE klimerko_particle_count_0_3 gauge\n";
  metrics += "klimerko_particle_count_0_3{device=\"" + device + "\"} " + String(sensorData.count_0_3) + "\n";
  
  metrics += "# HELP klimerko_particle_count_2_5 Particle count >2.5µm per 0.1L\n";
  metrics += "# TYPE klimerko_particle_count_2_5 gauge\n";
  metrics += "klimerko_particle_count_2_5{device=\"" + device + "\"} " + String(sensorData.count_2_5) + "\n";
  
  webServer.send(200, "text/plain; version=0.0.4; charset=utf-8", metrics);
}

// ============================================================================
// WEB SERVER INITIALIZATION
// ============================================================================

/**
 * @brief Initialize and start web server
 */
inline void initWebServer() {
  webServer.on("/", handleRoot);
  webServer.on("/api/data", handleApiData);
  webServer.on("/api/stats", handleApiStats);
  webServer.on("/api/log", handleApiLog);
  webServer.on("/metrics", handlePrometheusMetrics);
  webServer.onNotFound(handleNotFound);
  
  webServer.begin();
  
  DEBUG_PRINTLN(F("[WEB] Server started on port 80"));
  DEBUG_PRINTLN(F("[WEB] Dashboard: http://<ip>/"));
  DEBUG_PRINTLN(F("[WEB] API: http://<ip>/api/data"));
  DEBUG_PRINTLN(F("[WEB] Prometheus: http://<ip>/metrics"));
}

/**
 * @brief Handle web server requests (call in loop)
 */
inline void handleWebServer() {
  webServer.handleClient();
}

#endif // KLIMERKO_WEB_DASHBOARD_H
