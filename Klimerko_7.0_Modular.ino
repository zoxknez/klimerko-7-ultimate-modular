/**
 * @file Klimerko_7.0_Modular.ino
 * @brief Klimerko Air Quality Monitor - Modular Edition
 * @version 7.0 Ultimate Modular
 * @date December 2025
 * 
 * @author Original Klimerko: Vanja Stanic (https://descon.me/klimerko)
 * @author v7.0 Modular Refactor: o0o0o0o
 *         GitHub: https://github.com/zoxknez
 *         Portfolio: https://mojportfolio.vercel.app/
 * 
 * ╔══════════════════════════════════════════════════════════════════════════╗
 * ║                    KLIMERKO 7.0 ULTIMATE - MODULAR                       ║
 * ║              Citizen Air Quality Monitor with Cloud Connectivity         ║
 * ╠══════════════════════════════════════════════════════════════════════════╣
 * ║  Hardware: ESP8266 NodeMCU v2 + PMS7003 + BME280                         ║
 * ║  Cloud: AllThingsTalk MQTT Platform (configurable)                       ║
 * ║  Project: https://descon.me/klimerko                                     ║
 * ╚══════════════════════════════════════════════════════════════════════════╝
 * 
 * FEATURES:
 * - PMS7003 particle sensor (PM1, PM2.5, PM10 + particle counts)
 * - BME280 environmental sensor (temperature, humidity, pressure)
 * - EPA humidity correction for PM values
 * - MQTT to AllThingsTalk (configurable broker)
 * - Local Web Dashboard with Chart.js graphs
 * - Prometheus metrics endpoint (/metrics)
 * - mDNS discovery (klimerko-xxxxxx.local)
 * - NTP time synchronization
 * - Air quality alarm system
 * - LittleFS data logging
 * - OTA updates with password protection
 * - Deep sleep mode for battery operation
 * - WiFiManager captive portal configuration
 * - CRC32-protected EEPROM settings
 * 
 * SECURITY:
 * - Unique AP password per device (ChipID-based)
 * - OTA password protection
 * - Buffer overflow protection in MQTT callback
 * - CRC32 validation for stored settings
 * 
 * MODULE STRUCTURE:
 * - config.h      - All configuration constants
 * - types.h       - Data structures and enums
 * - utils.h       - Utility functions (CRC32, calculations)
 * - sensors.h     - PMS7003 and BME280 management
 * - network.h     - WiFi, MQTT, mDNS, NTP, OTA
 * - storage.h     - EEPROM and LittleFS persistence
 * - web_dashboard.h - HTTP server and Prometheus
 * - alarms.h      - Threshold monitoring and alerts
 */

// ============================================================================
// MODULE INCLUDES
// ============================================================================

#include "src/klimerko/config.h"
#include "src/klimerko/types.h"
#include "src/klimerko/utils.h"
#include "src/klimerko/sensors.h"
#include "src/klimerko/network.h"
#include "src/klimerko/storage.h"
#include "src/klimerko/web_dashboard.h"
#include "src/klimerko/alarms.h"

// Additional required includes
#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

// ============================================================================
// GLOBAL OBJECT INSTANTIATION
// ============================================================================

// Sensor objects
SoftwareSerial pmsSerial(PMS_TX_PIN, PMS_RX_PIN);
PMS pms(pmsSerial);
PMS::DATA pmsData;
Adafruit_BME280 bme;

// Moving averages
movingAvg pm1Avg(SENSOR_AVERAGE_SAMPLES);
movingAvg pm25Avg(SENSOR_AVERAGE_SAMPLES);
movingAvg pm10Avg(SENSOR_AVERAGE_SAMPLES);
movingAvg tempAvg(SENSOR_AVERAGE_SAMPLES);
movingAvg humAvg(SENSOR_AVERAGE_SAMPLES);
movingAvg presAvg(SENSOR_AVERAGE_SAMPLES);

// Network objects
WiFiClient networkClient;
PubSubClient mqtt(networkClient);
WiFiManager wm;

// Web server
ESP8266WebServer webServer(WEB_SERVER_PORT);

// WiFi portal parameters
WiFiManagerParameter portalDeviceID("device_id", "AllThingsTalk Device ID", "", 32);
WiFiManagerParameter portalDeviceToken("device_token", "AllThingsTalk Device Token", "", 64);
WiFiManagerParameter portalTemperatureOffset("temperature_offset", "Temp Offset", DEFAULT_TEMP_OFFSET_STR, 8);
WiFiManagerParameter portalAltitude("altitude", "Altitude (m)", "0", 6);
WiFiManagerParameter portalDisplayFirmwareVersion("<p>Firmware: 7.0 Ultimate Modular</p>");
WiFiManagerParameter portalDisplayCredits("<p>Original: Vanja Stanic | v7.0: o0o0o0o</p>");

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

// Device identification
char klimerkoID[32];
char apPassword[16];
char otaPassword[16];
char mdnsHostname[32];
char deviceId[32];
char deviceToken[64];

// Settings
char altitudeChar[6] = "0";
char bmeTemperatureOffsetChar[8] = DEFAULT_TEMP_OFFSET_STR;
float bmeTemperatureOffset = DEFAULT_TEMP_OFFSET;
int userAltitude = 0;

// MQTT configuration
char mqttServer[64] = DEFAULT_MQTT_SERVER;
uint16_t mqttPort = DEFAULT_MQTT_PORT;

// State structures
Settings klimerkoSettings;
Statistics stats = {0, 0, 0, 0, 0, 0};
SensorData sensorData;
Calibration calibration = {1.0f, 1.0f, 0.0f, 0.0f};
AlarmState alarmState;
WifiState wifiState;
MqttState mqttState;
ButtonState buttonState;

// Sensor status
bool pmsSensorOnline = true;
bool bmeSensorOnline = true;
int pmsSensorRetry = 0;
int bmeSensorRetry = 0;
bool pmsNoSleep = false;
bool pmsWoken = false;
SensorStatus pmsStatus = SensorStatus::OK;
SensorStatus bmeStatus = SensorStatus::OK;

// Fan stuck detection
int prevPm1 = -1, prevPm25 = -1, prevPm10 = -1;
int stuckCounter = 0, zeroCounter = 0;
String sensorStatusText = "Init";

// Timing
unsigned long bootTime = 0;
unsigned long sensorReadTime = 0;
unsigned long dataPublishTime = 0;

// Control flags
bool ntpSynced = false;
bool alarmEnabled = true;
bool alarmTriggered = false;
bool deepSleepEnabled = false;
bool shouldStartConfig = false;
String pendingUpdateUrl = "";

// Data publishing
uint8_t dataPublishInterval = 5;  // minutes
bool dataPublishFailed = false;

// LED state
bool ledState = false;
bool ledSuccessBlink = false;
unsigned long ledLastUpdate = 0;

// ============================================================================
// ASSET DEFINITIONS (for MQTT)
// ============================================================================

const char* PM1_ASSET = "pm1";
const char* PM2_5_ASSET = "pm2-5";
const char* PM10_ASSET = "pm10";
const char* PM1_CORR_ASSET = "pm1-c";
const char* PM2_5_CORR_ASSET = "pm2-5-c";
const char* PM10_CORR_ASSET = "pm10-c";
const char* COUNT_0_3_ASSET = "count-0-3";
const char* COUNT_0_5_ASSET = "count-0-5";
const char* COUNT_1_0_ASSET = "count-1-0";
const char* COUNT_2_5_ASSET = "count-2-5";
const char* COUNT_5_0_ASSET = "count-5-0";
const char* COUNT_10_0_ASSET = "count-10-0";
const char* AQ_ASSET = "air-quality";
const char* TEMPERATURE_ASSET = "temperature";
const char* TEMP_OFFSET_ASSET = "temperature-offset";
const char* HUMIDITY_ASSET = "humidity";
const char* PRESSURE_ASSET = "pressure";
const char* INTERVAL_ASSET = "interval";
const char* FIRMWARE_ASSET = "firmware";
const char* WIFI_SIGNAL_ASSET = "wifi-signal";
const char* ALTITUDE_ASSET = "altitude";
const char* ALTITUDE_SET_ASSET = "altitude-set";
const char* DEWPOINT_ASSET = "dewpoint";
const char* HUMIDITYABS_ASSET = "humidityAbs";
const char* PRESSURESEA_ASSET = "pressureSea";
const char* HEATINDEX_ASSET = "HeatIndex";
const char* WIFI_CONFIG_ASSET = "wifi-config";
const char* FIRMWARE_UPDATE_ASSET = "firmware-update";
const char* RESTART_DEVICE_ASSET = "restart-device";
const char* SENSOR_STATUS_ASSET = "sensor-status";

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void publishDiagnosticData();
void savePortalData();

// ============================================================================
// INTERVAL CONTROL
// ============================================================================

void changeInterval(int interval) {
  if (interval > 5 && interval <= 60) {
    dataPublishInterval = interval;
    pmsNoSleep = false;
  } else if (interval <= 5) {
    dataPublishInterval = (interval <= 1) ? 1 : interval;
    pmsNoSleep = true;
    setPMSPower(true);
  } else {
    dataPublishInterval = 60;
    pmsNoSleep = false;
  }
  publishDiagnosticData();
}

// ============================================================================
// DATA PUBLISHING
// ============================================================================

void publishSensorData() {
  static char jsonBuffer[2048];
  StaticJsonDocument<2048> doc;
  
  if (pmsSensorOnline) {
    doc.createNestedObject(AQ_ASSET)["value"] = airQualityToString(sensorData.airQuality);
    doc.createNestedObject(PM1_ASSET)["value"] = sensorData.pm1;
    doc.createNestedObject(PM2_5_ASSET)["value"] = sensorData.pm25;
    doc.createNestedObject(PM10_ASSET)["value"] = sensorData.pm10;
    
    doc.createNestedObject(COUNT_0_3_ASSET)["value"] = sensorData.count_0_3;
    doc.createNestedObject(COUNT_0_5_ASSET)["value"] = sensorData.count_0_5;
    doc.createNestedObject(COUNT_1_0_ASSET)["value"] = sensorData.count_1_0;
    doc.createNestedObject(COUNT_2_5_ASSET)["value"] = sensorData.count_2_5;
    doc.createNestedObject(COUNT_5_0_ASSET)["value"] = sensorData.count_5_0;
    doc.createNestedObject(COUNT_10_0_ASSET)["value"] = sensorData.count_10_0;
    
    // Check fan status
    checkFanStatus();
    doc.createNestedObject(SENSOR_STATUS_ASSET)["value"] = sensorStatusText;
    
    // Humidity-corrected values
    doc.createNestedObject(PM1_CORR_ASSET)["value"] = sensorData.pm1_corrected;
    doc.createNestedObject(PM2_5_CORR_ASSET)["value"] = sensorData.pm25_corrected;
    doc.createNestedObject(PM10_CORR_ASSET)["value"] = sensorData.pm10_corrected;
  } else {
    doc.createNestedObject(SENSOR_STATUS_ASSET)["value"] = "Sensor Offline";
  }
  
  if (bmeSensorOnline) {
    doc.createNestedObject(TEMPERATURE_ASSET)["value"] = sensorData.temperature;
    doc.createNestedObject(HUMIDITY_ASSET)["value"] = sensorData.humidity;
    doc.createNestedObject(PRESSURE_ASSET)["value"] = sensorData.pressure;
    doc.createNestedObject(ALTITUDE_ASSET)["value"] = sensorData.altitude;
    doc.createNestedObject(DEWPOINT_ASSET)["value"] = sensorData.dewpoint;
    doc.createNestedObject(HUMIDITYABS_ASSET)["value"] = sensorData.humidityAbs;
    doc.createNestedObject(PRESSURESEA_ASSET)["value"] = sensorData.pressureSea;
    doc.createNestedObject(HEATINDEX_ASSET)["value"] = sensorData.heatIndex;
  }
  
  doc.createNestedObject(FIRMWARE_ASSET)["value"] = FIRMWARE_VERSION;
  doc.createNestedObject(WIFI_SIGNAL_ASSET)["value"] = getWifiSignal();
  
  serializeJson(doc, jsonBuffer);
  
  if (publishToState(jsonBuffer)) {
    recordSuccessfulPublish();
    DEBUG_PRINTLN(F("[DATA] Published successfully"));
  } else {
    recordFailedPublish();
    DEBUG_PRINTLN(F("[DATA] Publish failed"));
  }
  
  // Log data locally
  logSensorDataToFS(sensorData, getUptimeSeconds(bootTime));
}

void publishDiagnosticData() {
  if (wifiState.connectionLost || mqttState.connectionLost) return;
  
  char jsonBuffer[512];
  StaticJsonDocument<512> doc;
  
  doc.createNestedObject(INTERVAL_ASSET)["value"] = dataPublishInterval;
  doc.createNestedObject(FIRMWARE_ASSET)["value"] = FIRMWARE_VERSION;
  doc.createNestedObject(WIFI_SIGNAL_ASSET)["value"] = getWifiSignal();
  doc.createNestedObject(TEMP_OFFSET_ASSET)["value"] = bmeTemperatureOffset;
  doc.createNestedObject(ALTITUDE_ASSET)["value"] = userAltitude;
  doc.createNestedObject(ALTITUDE_SET_ASSET)["value"] = userAltitude;
  doc.createNestedObject(SENSOR_STATUS_ASSET)["value"] = getSensorStatusString();
  
  serializeJson(doc, jsonBuffer);
  publishToState(jsonBuffer);
  
  DEBUG_PRINTLN(F("[DATA] Diagnostics published"));
}

// ============================================================================
// MQTT CALLBACK
// ============================================================================

void mqttCallback(char* p_topic, byte* p_payload, unsigned int p_length) {
  DEBUG_PRINTLN(F("[MQTT] Message received"));
  
  // Security: Prevent buffer overflow
  if (p_length >= MQTT_CALLBACK_BUFFER_SIZE) {
    DEBUG_PRINTLN(F("[MQTT] Payload too large, ignoring"));
    return;
  }
  
  char json[MQTT_CALLBACK_BUFFER_SIZE + 1];
  memcpy(json, p_payload, p_length);
  json[p_length] = '\0';
  
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    DEBUG_PRINT(F("[MQTT] JSON parse error: "));
    DEBUG_PRINTLN(error.c_str());
    return;
  }
  
  String asset = extractAssetFromTopic(String(p_topic));
  DEBUG_PRINT(F("[MQTT] Asset: ")); DEBUG_PRINTLN(asset);
  
  // Handle different assets
  if (asset == INTERVAL_ASSET) {
    changeInterval(doc["value"]);
  }
  else if (asset == WIFI_CONFIG_ASSET) {
    String v = doc["value"].as<String>();
    if (v == "true" || v == "1") {
      shouldStartConfig = true;
    }
  }
  else if (asset == TEMP_OFFSET_ASSET) {
    String v = doc["value"].as<String>();
    if (isValidNumber(v.c_str())) {
      bmeTemperatureOffset = v.toFloat();
      calibration.tempOffset = bmeTemperatureOffset;
      snprintf(bmeTemperatureOffsetChar, sizeof(bmeTemperatureOffsetChar), "%.2f", bmeTemperatureOffset);
      updateSetting("tempOffset", bmeTemperatureOffsetChar);
      tempAvg.reset();
      humAvg.reset();
      publishDiagnosticData();
    }
  }
  else if (asset == ALTITUDE_SET_ASSET) {
    String v = doc["value"].as<String>();
    if (isValidNumber(v.c_str())) {
      userAltitude = v.toInt();
      sensorData.userAltitude = userAltitude;
      snprintf(altitudeChar, sizeof(altitudeChar), "%d", userAltitude);
      updateSetting("altitude", altitudeChar);
      publishDiagnosticData();
    }
  }
  else if (asset == FIRMWARE_UPDATE_ASSET) {
    String url = doc["value"].as<String>();
    if (url.length() > 10) {
      pendingUpdateUrl = url;
    }
  }
  else if (asset == RESTART_DEVICE_ASSET) {
    String v = doc["value"].as<String>();
    if (v == "true" || v == "1") {
      DEBUG_PRINTLN(F("[SYSTEM] Remote restart requested..."));
      saveStatistics(getUptimeSeconds(bootTime));
      delay(1000);
      ESP.restart();
    }
  }
  else if (asset == "deep-sleep") {
    String v = doc["value"].as<String>();
    deepSleepEnabled = (v == "true" || v == "1");
    updateBoolSetting("deepSleep", deepSleepEnabled);
    DEBUG_PRINT(F("[SLEEP] Deep sleep ")); 
    DEBUG_PRINTLN(deepSleepEnabled ? F("enabled") : F("disabled"));
  }
  else if (asset == "alarm-enable") {
    String v = doc["value"].as<String>();
    alarmEnabled = (v == "true" || v == "1");
    updateBoolSetting("alarmEnabled", alarmEnabled);
    setAlarmEnabled(alarmEnabled);
  }
  else if (asset == "calibration") {
    if (doc.containsKey("pm25")) {
      calibration.pm25Factor = doc["pm25"].as<float>();
    }
    if (doc.containsKey("pm10")) {
      calibration.pm10Factor = doc["pm10"].as<float>();
    }
    if (doc.containsKey("temp")) {
      calibration.tempOffset = doc["temp"].as<float>();
    }
    if (doc.containsKey("hum")) {
      calibration.humOffset = doc["hum"].as<float>();
    }
    updateCalibration(calibration);
    DEBUG_PRINTLN(F("[CAL] Calibration updated"));
  }
  else if (asset == "mqtt-broker") {
    if (doc.containsKey("server")) {
      String server = doc["server"].as<String>();
      server.toCharArray(mqttServer, sizeof(mqttServer));
    }
    if (doc.containsKey("port")) {
      mqttPort = doc["port"].as<uint16_t>();
    }
    updateMqttBroker(mqttServer, mqttPort);
  }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void buttonLoop() {
  int currentState = digitalRead(BUTTON_PIN);
  
  if (buttonState.lastState == HIGH && currentState == LOW) {
    buttonState.pressedTime = millis();
    buttonState.pressed = true;
    buttonState.longPressDetected = false;
  } else if (buttonState.lastState == LOW && currentState == HIGH) {
    buttonState.releasedTime = millis();
    buttonState.pressed = false;
    unsigned long duration = buttonState.releasedTime - buttonState.pressedTime;
    
    if (duration > BUTTON_SHORT_PRESS_MS && duration < BUTTON_MEDIUM_PRESS_MS) {
      wifiConfigStop();
    } else if (duration > BUTTON_MEDIUM_PRESS_MS && duration < BUTTON_LONG_PRESS_MS) {
      wifiConfigStart();
    }
  }
  
  if (buttonState.pressed && !buttonState.longPressDetected) {
    if (millis() - buttonState.pressedTime > BUTTON_LONG_PRESS_MS) {
      buttonState.longPressDetected = true;
      factoryReset(wm);
    }
  }
  
  buttonState.lastState = currentState;
}

// ============================================================================
// LED HANDLING
// ============================================================================

void ledLoop() {
  if (ledSuccessBlink) {
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
    ledSuccessBlink = false;
  }
  
  if (isConfigPortalActive()) {
    ledState = true;
  } else {
    if (wifiState.connectionLost || mqttState.connectionLost) {
      if (millis() - ledLastUpdate >= LED_BLINK_INTERVAL) {
        ledState = !ledState;
        ledLastUpdate = millis();
      }
    } else {
      ledState = false;
    }
  }
  
  digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
}

// ============================================================================
// DEEP SLEEP
// ============================================================================

void enterDeepSleep() {
  DEBUG_PRINTLN(F("[SLEEP] Entering deep sleep..."));
  
  saveStatistics(getUptimeSeconds(bootTime));
  
  if (pmsSensorOnline) {
    pms.sleep();
    delay(100);
  }
  
  uint32_t sleepDuration = DEEP_SLEEP_DURATION_US;
  if (dataPublishInterval > 5) {
    sleepDuration = (uint32_t)dataPublishInterval * 60 * 1000000;
  }
  
  DEBUG_PRINT(F("[SLEEP] Duration: ")); DEBUG_PRINT(sleepDuration / 1000000);
  DEBUG_PRINTLN(F("s"));
  
  ESP.deepSleep(sleepDuration, WAKE_RF_DEFAULT);
}

// ============================================================================
// WIFI MANAGER CALLBACKS
// ============================================================================

void savePortalData() {
  // Save portal values
  strncpy(deviceId, portalDeviceID.getValue(), sizeof(deviceId) - 1);
  strncpy(deviceToken, portalDeviceToken.getValue(), sizeof(deviceToken) - 1);
  strncpy(bmeTemperatureOffsetChar, portalTemperatureOffset.getValue(), sizeof(bmeTemperatureOffsetChar) - 1);
  bmeTemperatureOffset = atof(bmeTemperatureOffsetChar);
  strncpy(altitudeChar, portalAltitude.getValue(), sizeof(altitudeChar) - 1);
  userAltitude = atoi(altitudeChar);
  sensorData.userAltitude = userAltitude;
  calibration.tempOffset = bmeTemperatureOffset;
  
  saveSettings(deviceId, deviceToken, bmeTemperatureOffsetChar, altitudeChar,
               deepSleepEnabled, alarmEnabled, mqttServer, mqttPort, calibration);
}

void wifiConfigWebServerStarted() {
  wm.server->on("/exit", wifiConfigStop);
}

void wifiConfigStarted(WiFiManager* /*wmPtr*/) {
  wifiState.configActiveSince = millis();
}

void setupWiFiManager() {
  wm.setDebugOutput(false);
  
  wm.addParameter(&portalDeviceID);
  wm.addParameter(&portalDeviceToken);
  wm.addParameter(&portalTemperatureOffset);
  wm.addParameter(&portalAltitude);
  wm.addParameter(&portalDisplayFirmwareVersion);
  wm.addParameter(&portalDisplayCredits);
  
  wm.setSaveParamsCallback(savePortalData);
  wm.setSaveConfigCallback([]() { connectMQTT(); });
  wm.setWebServerCallback(wifiConfigWebServerStarted);
  wm.setAPCallback(wifiConfigStarted);
  
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT / 1000);
  wm.setConnectRetries(3);
  wm.setConnectTimeout(10);
  wm.setDarkMode(true);
  wm.setTitle("Klimerko");
  wm.setHostname(klimerkoID);
  wm.setCountry("RS");
  wm.setEnableConfigPortal(false);
  wm.setParamsPage(false);
  wm.setSaveConnect(true);
  wm.setBreakAfterConfig(true);
  wm.setWiFiAutoReconnect(true);
}

// ============================================================================
// MAIN SENSOR/DATA LOOP
// ============================================================================

void mainSensorLoop() {
  sensorLoop(sensorReadTime, dataPublishInterval);
  
  // Check alarms after sensor read
  checkAlarms([](const char* payload) {
    publishToState(payload);
  });
  
  // Publish data on interval
  unsigned long now = millis();
  if (now - dataPublishTime >= (unsigned long)dataPublishInterval * 60000UL) {
    if (!wifiState.connectionLost && !mqttState.connectionLost) {
      dataPublishFailed = false;
      dataPublishTime = now;
      publishSensorData();
    } else {
      if (!dataPublishFailed) {
        DEBUG_PRINTLN(wifiState.connectionLost ? 
                      F("[DATA] Error: No WiFi") : F("[DATA] Error: No MQTT"));
        dataPublishFailed = true;
      }
    }
  }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void initPins() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  bootTime = millis();
  Serial.begin(115200);
  
  DEBUG_PRINTLN(F("\n"));
  DEBUG_PRINTLN(F("╔══════════════════════════════════════════════════════════════╗"));
  DEBUG_PRINTLN(F("║        KLIMERKO 7.0 ULTIMATE - MODULAR EDITION               ║"));
  DEBUG_PRINTLN(F("║    Air Quality Monitor - ESP8266 NodeMCU v2                  ║"));
  DEBUG_PRINTLN(F("║  mDNS + Dashboard + Charts + NTP + Alarms + Prometheus       ║"));
  DEBUG_PRINTLN(F("╚══════════════════════════════════════════════════════════════╝"));
  
  ESP.wdtEnable(5000);
  
  // Initialize hardware
  initPins();
  
  // Generate unique IDs
  generateDeviceID(klimerkoID, sizeof(klimerkoID));
  generateUniquePasswords(apPassword, otaPassword, mdnsHostname);
  
  // Initialize storage
  initLittleFS();
  loadStatistics();
  
  // Restore settings
  restoreSettings(deviceId, deviceToken, bmeTemperatureOffsetChar, bmeTemperatureOffset,
                  altitudeChar, userAltitude, deepSleepEnabled, alarmEnabled,
                  mqttServer, mqttPort, calibration);
  
  sensorData.userAltitude = userAltitude;
  
  // Update portal values with restored settings
  portalDeviceID.setValue(deviceId, 32);
  portalDeviceToken.setValue(deviceToken, 64);
  portalTemperatureOffset.setValue(bmeTemperatureOffsetChar, 8);
  portalAltitude.setValue(altitudeChar, 6);
  
  // Initialize sensors
  initSensors();
  
  // Setup WiFiManager
  setupWiFiManager();
  
  // Connect WiFi
  WiFi.mode(WIFI_STA);
  connectWiFi();
  
  if (!wifiState.connectionLost) {
    // Initialize network services
    initNTP();
    initMDNS();
    initWebServer();
    initOTA();
    initMQTT(mqttCallback);
  }
  
  // Initialize alarms
  initAlarms();
  
  DEBUG_PRINTLN(F("[SYSTEM] Initialization complete!"));
  DEBUG_PRINT(F("[SYSTEM] Free heap: ")); DEBUG_PRINTLN(ESP.getFreeHeap());
  DEBUG_PRINT(F("[SYSTEM] Dashboard: http://")); 
  DEBUG_PRINT(mdnsHostname); DEBUG_PRINTLN(F(".local"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  ESP.wdtFeed();
  
  // Network services
  handleOTA();
  updateMDNS();
  handleWebServer();
  
  // Configuration portal request
  if (shouldStartConfig) {
    shouldStartConfig = false;
    wifiConfigStart();
  }
  
  // Pending firmware update
  if (pendingUpdateUrl != "") {
    String url = pendingUpdateUrl;
    pendingUpdateUrl = "";
    if (pmsSensorOnline) pms.sleep();
    performHttpUpdate(url);
  }
  
  // Deep sleep mode
  if (deepSleepEnabled && !isConfigPortalActive()) {
    static bool deepSleepMeasurementDone = false;
    if (!deepSleepMeasurementDone) {
      setPMSPower(true);
      delay(30000);  // Wait for PMS to stabilize
      readPMSSensor();
      readBMESensor();
      if (!wifiState.connectionLost && !mqttState.connectionLost) {
        publishSensorData();
        delay(1000);
      }
      deepSleepMeasurementDone = true;
      enterDeepSleep();
    }
  }
  
  // Normal operation
  mainSensorLoop();
  maintainWiFi();
  maintainMQTT();
  wifiConfigLoop();
  buttonLoop();
  ledLoop();
}
