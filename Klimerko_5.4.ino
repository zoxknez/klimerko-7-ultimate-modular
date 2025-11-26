/* ------------------------------------------- Project "KLIMERKO" ---------------------------------------------
 * Citizen Air Quality measuring device with cloud monitoring, built at https://descon.me for the whole world.
 * Programmed, built and maintained by Vanja Stanic // www.vanjastanic.com
 * ------------------------------------------------------------------------------------------------------------
 * Version 5.4 Humidity Fix: 
 * - Solved sensor "blocking" at >98% humidity by allowing calculation tolerance up to 110%.
 * - Clamps humidity to 100% if calculation exceeds it.
 * - Includes all previous features (Remote Restart, Update, etc.)
 */

#include "src/AdafruitBME280/Adafruit_Sensor.h"
#include "src/AdafruitBME280/Adafruit_BME280.h"
#include "src/pmsLibrary/PMS.h"
#include "src/movingAvg/movingAvg.h"
#include "src/WiFiManager/WiFiManager.h"
#include "src/PubSubClient/PubSubClient.h"
#include "src/ArduinoJson-v6.18.5.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ArduinoOTA.h> 
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h> 

#define BUTTON_PIN     0
#define pmsTX          D5
#define pmsRX          D6
#define SEA_LEVEL_PRESSURE_CAL 1.0

struct Settings {
  char header[4];       
  char deviceId[32];
  char deviceToken[64];
  char tempOffset[8];
  char altitude[6];
};
Settings klimerkoSettings;

float b = 17.62;
float c = 243.12;
float gamma_var, dewpoint, humidityAbs, pressureSea;
float pressureSea_cal = SEA_LEVEL_PRESSURE_CAL;
float HeatIndex;

int altitude = 0; 
char altitudeChar[6] = "0"; 
float bmeTemperatureOffset = -2;
char bmeTemperatureOffsetChar[8] = "-2";

String pendingUpdateUrl = "";
bool shouldStartConfig = false; 

String firmwareVersion = "5.4 Humidity Fix";
const char* firmwareVersionPortal = "<p>Firmware Version: 5.4 (Humidity Fix)</p>";
char klimerkoID[32];

const int wifiReconnectInterval = 60;
bool wifiConnectionLost = true;
unsigned long wifiReconnectLastAttempt;

char const *wifiConfigPortalPassword = "ConfigMode";
const int wifiConfigTimeout = 1800;
unsigned long wifiConfigActiveSince;

const char* MQTT_SERVER = "api.allthingstalk.io";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_PASSWORD = "arbitrary";
uint16_t MQTT_MAX_MESSAGE_SIZE = 2048;
char deviceId[32], deviceToken[64]; 
const int mqttReconnectInterval = 30;
bool mqttConnectionLost = true;
unsigned long mqttReconnectLastAttempt;

const char* PM1_ASSET = "pm1";
const char* PM2_5_ASSET = "pm2-5";
const char* PM10_ASSET = "pm10";
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

const int buttonLongPressTime = 15000;
const int buttonMediumPressTime = 1000;
const int buttonShortPressTime = 50;
unsigned long buttonPressedTime = 0;
unsigned long buttonReleasedTime = 0;
bool buttonPressed = false;
bool buttonLongPressDetected = false;
int buttonLastState = HIGH;
int buttonCurrentState;

bool ledState = false;
bool ledSuccessBlink = false;
const int ledBlinkInterval = 1000;
unsigned long ledLastUpdate;

uint8_t dataPublishInterval = 5;
const uint8_t sensorAverageSamples = 10;
const int sensorRetriesUntilConsideredOffline = 3;
bool dataPublishFailed = false;
unsigned long sensorReadTime, dataPublishTime;

const uint8_t pmsWakeBefore = 30;
bool pmsSensorOnline = true;
int pmsSensorRetry = 0;
bool pmsNoSleep = false;
bool pmsWoken = false;
const char *airQuality, *airQualityRaw;
int avgPM1, avgPM25, avgPM10;

bool bmeSensorOnline = true;
int bmeSensorRetry = 0;
float avgTemperature, avgHumidity, avgPressure, avgaltitude;

WiFiManager wm;
WiFiManagerParameter portalDeviceID("device_id", "AllThingsTalk Device ID", deviceId, 32);
WiFiManagerParameter portalDeviceToken("device_token", "AllThingsTalk Device Token", deviceToken, 64);
WiFiManagerParameter portalTemperatureOffset("temperature_offset", "Temp Offset", bmeTemperatureOffsetChar, 8);
WiFiManagerParameter portalAltitude("altitude", "Altitude (m)", altitudeChar, 6);
WiFiManagerParameter portalDisplayFirmwareVersion(firmwareVersionPortal);
WiFiManagerParameter portalDisplayCredits("Firmware Designed and Developed by Vanja Stanic");

WiFiClient networkClient;
PubSubClient mqtt(networkClient);
SoftwareSerial pmsSerial(pmsTX, pmsRX);
PMS pms(pmsSerial);
PMS::DATA data;
Adafruit_BME280 bme;
movingAvg pm1(sensorAverageSamples);
movingAvg pm25(sensorAverageSamples);
movingAvg pm10(sensorAverageSamples);
movingAvg temp(sensorAverageSamples);
movingAvg hum(sensorAverageSamples);
movingAvg pres(sensorAverageSamples);

void performFirmwareUpdate(String url) {
  Serial.println(F("[UPDATE] Starting Remote Firmware Update..."));
  Serial.print(F("[UPDATE] URL: ")); Serial.println(url);
  if(pmsSensorOnline) pms.sleep(); 
  ESP.wdtDisable(); 
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[UPDATE] FAILED. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      ESP.wdtEnable(5000); break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[UPDATE] No Updates")); ESP.wdtEnable(5000); break;
    case HTTP_UPDATE_OK:
      Serial.println(F("[UPDATE] OK! Rebooting...")); ESP.restart(); break;
  }
}

void sensorLoop() { 
  if (millis() - sensorReadTime >= readIntervalMillis() - (pmsWakeBefore * 1000) && !pmsWoken && pmsSensorOnline) {
    Serial.println(F("[PMS] Waking up Sensor"));
    pmsPower(true);
  }
  if (millis() - sensorReadTime >= readIntervalMillis()) {
    sensorReadTime = millis();
    readSensorData();
  }
  if (millis() - dataPublishTime >= dataPublishInterval * 60000) {
    if (!wifiConnectionLost) {
      if (!mqttConnectionLost) {
        dataPublishFailed = false;
        dataPublishTime = millis();
        publishSensorData();
      } else {
        if (!dataPublishFailed) {
          Serial.println(F("[DATA] Error: No ATT Connection"));
          dataPublishFailed = true;
        }
      }
    } else {
      if (!dataPublishFailed) {
        Serial.println(F("[DATA] Error: No WiFi"));
        dataPublishFailed = true;
      }
    }
  }
}

void readSensorData() {
  Serial.println(F("------------------------------DATA------------------------------"));
  readPMS();
  readBME();
  Serial.println(F("----------------------------------------------------------------"));
  if (!pmsNoSleep && pmsSensorOnline) {
    Serial.print(F("[PMS] Sleeping until "));
    Serial.print(pmsWakeBefore);
    Serial.println(F("s before next read."));
    pmsPower(false);
  }
}

void publishSensorData() {
  char JSONmessageBuffer[512];
  DynamicJsonDocument doc(512);
  
  if (pmsSensorOnline) {
    doc.createNestedObject(AQ_ASSET)["value"] = airQuality;
    doc.createNestedObject(PM1_ASSET)["value"] = avgPM1;
    doc.createNestedObject(PM2_5_ASSET)["value"] = avgPM25;
    doc.createNestedObject(PM10_ASSET)["value"] = avgPM10;
  } else {
    Serial.println(F("[DATA] PMS Offline, skipping."));
  }
  
  if (bmeSensorOnline) {
    doc.createNestedObject(TEMPERATURE_ASSET)["value"] = avgTemperature;
    doc.createNestedObject(HUMIDITY_ASSET)["value"] = avgHumidity;
    doc.createNestedObject(PRESSURE_ASSET)["value"] = avgPressure;
    doc.createNestedObject(ALTITUDE_ASSET)["value"] = avgaltitude;
    doc.createNestedObject(DEWPOINT_ASSET)["value"] = dewpoint;
    doc.createNestedObject(HUMIDITYABS_ASSET)["value"] = humidityAbs;
    doc.createNestedObject(PRESSURESEA_ASSET)["value"] = pressureSea;
    doc.createNestedObject(HEATINDEX_ASSET)["value"] = HeatIndex;
  } else {
    Serial.println(F("[DATA] BME Offline, skipping."));
  }
  
  doc.createNestedObject(FIRMWARE_ASSET)["value"] = firmwareVersion;
  doc.createNestedObject(WIFI_SIGNAL_ASSET)["value"] = wifiSignal();
  serializeJson(doc, JSONmessageBuffer);
  char topic[128];
  snprintf(topic, sizeof topic, "%s%s%s", "device/", deviceId, "/state");
  mqtt.publish(topic, JSONmessageBuffer, false);
  Serial.print(F("[DATA] Sent: ")); Serial.println(JSONmessageBuffer);
}

void readPMS() { 
  while (pmsSerial.available()) { pmsSerial.read(); }
  pms.requestRead();
  if (pms.readUntil(data)) {
    avgPM1 = pm1.reading(data.PM_AE_UG_1_0);
    avgPM25 = pm25.reading(data.PM_AE_UG_2_5);
    avgPM10 = pm10.reading(data.PM_AE_UG_10_0);
    
    int PM10 = data.PM_AE_UG_10_0;
    if (PM10 <= 20) airQualityRaw = "Excellent"; 
    else if (PM10 <= 40) airQualityRaw = "Good"; 
    else if (PM10 <= 50) airQualityRaw = "Acceptable"; 
    else if (PM10 <= 100) airQualityRaw = "Polluted"; 
    else airQualityRaw = "Very Polluted";

    if (avgPM10 <= 20) airQuality = "Excellent"; 
    else if (avgPM10 <= 40) airQuality = "Good"; 
    else if (avgPM10 <= 50) airQuality = "Acceptable"; 
    else if (avgPM10 <= 100) airQuality = "Polluted"; 
    else airQuality = "Very Polluted";

    Serial.print(F("Air Quality: ")); Serial.println(airQuality);
    pmsSensorRetry = 0;
    if (!pmsSensorOnline) { pmsSensorOnline = true; Serial.println(F("[PMS] Online!")); }
  } else {
    if (pmsSensorOnline) {
      Serial.println(F("[PMS] No Data"));
      pmsSensorRetry++;
      if (pmsSensorRetry > sensorRetriesUntilConsideredOffline) {
        pmsSensorOnline = false;
        Serial.println(F("[PMS] Offline!"));
        pm1.reset(); pm25.reset(); pm10.reset();
        initPMS();
      }
    } else { initPMS(); }
  }
}

// --- KLJUCNA IZMENA U OVOJ FUNKCIJI (readBME) ---
void readBME() { 
  float temperatureRaw = bme.readTemperature();
  float temperature = temperatureRaw + bmeTemperatureOffset;
  float humidityRaw = bme.readHumidity();
  // Kompenzacija
  float humidity = humidityRaw * exp(243.12 * 17.62 * (temperatureRaw - temperature) / (243.12 + temperatureRaw) / (243.12 + temperature));
  
  float pressure = bme.readPressure() / 100.0F;
  pressureSea = (((pressure)/pow((1-((float)(altitude))/44330), 5.255))) - pressureSea_cal;
  gamma_var = (b*temperature) / (c + temperature) + log(humidity/100.0);
  dewpoint = (c * gamma_var) / (b - gamma_var);
  humidityAbs = (6.112*exp((b*temperature)/(c + temperature))*humidity*2.1674) / (273.15 + temperature);
  
  if (temperature > 26.7) {
    double c1 = -8.78469475566, c2 = 1.61139411, c3 = 2.33854883889, c4 = -0.14611605, c5= -0.012308094, c6= -0.0164248277778, c7= 0.002211732, c8= 0.00072546, c9= -0.000003582;
    double T = temperature; double R = humidity;
    double A = ((c5 * T) + c2) * T + c1; double B = ((c7 * T) + c4) * T + c3; double C = ((c9 * T) + c8) * T + c6;
    HeatIndex = ((C * R + B) * R + A);
  } else { HeatIndex = temperature; }
  
  Serial.print(F("Temp: ")); Serial.println(temperature);
  Serial.print(F("Hum: ")); Serial.println(humidity);

  // --- FIX ZA VLAGU ---
  // Dozvoljavamo do 110% pre nego sto proglasimo gresku
  if (temperatureRaw > -100 && temperatureRaw < 150 && humidity >= 0 && humidity <= 110) {
    
    // Ako je vlaga preko 100% (matematicki), limitiramo je na 100%
    if (humidity > 100.0) humidity = 100.0;

    avgTemperature = temp.reading(temperature*100) / 100.0;
    avgHumidity = hum.reading(humidity*100) / 100.0;
    avgPressure = pres.reading(pressure*100) / 100.0;
    avgaltitude = bme.readAltitude(pressureSea); 

    bmeSensorRetry = 0;
    if (!bmeSensorOnline) { bmeSensorOnline = true; Serial.println(F("[BME] Online!")); }
  } else {
    if (bmeSensorOnline) {
      Serial.println(F("[BME] Invalid Data (Out of range)"));
      bmeSensorRetry++;
      if (bmeSensorRetry > sensorRetriesUntilConsideredOffline) {
        bmeSensorOnline = false;
        Serial.println(F("[BME] Offline!"));
        temp.reset(); hum.reset(); pres.reset();
        initBME();
      }
    } else { initBME(); }
  }
}
// ---------------------------------------------

void pmsPower(bool state) { 
  if (state) { pms.wakeUp(); pms.passiveMode(); pmsWoken = true; } 
  else { pmsSerial.flush(); delay(100); pmsWoken = false; pms.sleep(); }
}
 
void changeInterval(int interval) {
  if (interval > 5 && interval <= 60) {
    dataPublishInterval = interval; pmsNoSleep = false;
  } else if (interval <= 5) {
    dataPublishInterval = (interval <= 1) ? 1 : interval;
    pmsNoSleep = true; pmsPower(true);
  } else if (interval >= 60) {
    dataPublishInterval = 60; pmsNoSleep = false;
  }
  publishDiagnosticData();
}

void publishDiagnosticData() { 
  if (!wifiConnectionLost && !mqttConnectionLost) {
    char JSONmessageBuffer[512];
    DynamicJsonDocument doc(512);
    doc.createNestedObject(INTERVAL_ASSET)["value"] = dataPublishInterval;
    doc.createNestedObject(FIRMWARE_ASSET)["value"] = firmwareVersion;
    doc.createNestedObject(WIFI_SIGNAL_ASSET)["value"] = wifiSignal();
    doc.createNestedObject(TEMP_OFFSET_ASSET)["value"] = bmeTemperatureOffset;
    doc.createNestedObject(ALTITUDE_ASSET)["value"] = altitude;
    doc.createNestedObject(ALTITUDE_SET_ASSET)["value"] = altitude;
    serializeJson(doc, JSONmessageBuffer);
    char topic[256];
    snprintf(topic, sizeof topic, "%s%s%s", "device/", deviceId, "/state");
    mqtt.publish(topic, JSONmessageBuffer, false);
    Serial.print(F("[DATA] Diag: ")); Serial.println(JSONmessageBuffer);
  }
}

unsigned long readIntervalMillis() { return (dataPublishInterval * 60000) / sensorAverageSamples; }
int readIntervalSeconds() { return (dataPublishInterval * 60) / sensorAverageSamples; }

void restoreData() {
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, klimerkoSettings);
  EEPROM.end();
  if (String(klimerkoSettings.header) == "KLI") {
    strcpy(deviceId, klimerkoSettings.deviceId);
    strcpy(deviceToken, klimerkoSettings.deviceToken);
    strcpy(bmeTemperatureOffsetChar, klimerkoSettings.tempOffset);
    bmeTemperatureOffset = atof(bmeTemperatureOffsetChar);
    strcpy(altitudeChar, klimerkoSettings.altitude);
    altitude = atoi(altitudeChar);
    Serial.println(F("[MEMORY] Settings Loaded OK"));
  } else {
    Serial.println(F("[MEMORY] Defaults."));
    deviceId[0] = 0; deviceToken[0] = 0;
    strcpy(bmeTemperatureOffsetChar, "-2"); bmeTemperatureOffset = -2;
    strcpy(altitudeChar, "0"); altitude = 0;
  }
  portalTemperatureOffset.setValue(bmeTemperatureOffsetChar, 8);
  portalAltitude.setValue(altitudeChar, 6);
  portalDeviceID.setValue(deviceId, 32);
  portalDeviceToken.setValue(deviceToken, 64);
}

void saveData() {
  Serial.println(F("[MEMORY] Saving..."));
  bool changed = false;
  if (String(portalDeviceID.getValue()) != "" && String(portalDeviceID.getValue()) != String(deviceId)) {
    strcpy(deviceId, portalDeviceID.getValue()); changed = true;
  }
  if (String(portalDeviceToken.getValue()) != "" && String(portalDeviceToken.getValue()) != String(deviceToken)) {
    strcpy(deviceToken, portalDeviceToken.getValue()); changed = true;
  }
  if (String(portalTemperatureOffset.getValue()) != "" && isNumber(portalTemperatureOffset.getValue()) && String(portalTemperatureOffset.getValue()) != String(bmeTemperatureOffsetChar)) {
     strcpy(bmeTemperatureOffsetChar, portalTemperatureOffset.getValue());
     bmeTemperatureOffset = atof(bmeTemperatureOffsetChar);
     temp.reset(); hum.reset(); changed = true;
  }
  if (String(portalAltitude.getValue()) != "" && isNumber(portalAltitude.getValue()) && String(portalAltitude.getValue()) != String(altitudeChar)) {
     strcpy(altitudeChar, portalAltitude.getValue());
     altitude = atoi(altitudeChar); changed = true;
  }
  if (changed || String(klimerkoSettings.header) != "KLI") {
    strcpy(klimerkoSettings.header, "KLI");
    strcpy(klimerkoSettings.deviceId, deviceId);
    strcpy(klimerkoSettings.deviceToken, deviceToken);
    strcpy(klimerkoSettings.tempOffset, bmeTemperatureOffsetChar);
    strcpy(klimerkoSettings.altitude, altitudeChar);
    EEPROM.begin(sizeof(Settings)); EEPROM.put(0, klimerkoSettings); 
    if (EEPROM.commit()) Serial.println(F("[MEMORY] Saved."));
    else Serial.println(F("[MEMORY] Fail!"));
    EEPROM.end();
  }
}

bool isNumber(const char* value) {
  if (value[0] != '-' && value[0] != '+' && !isDigit(value[0])) return false;
  if (value[1] != '\0') {
    int i = 1;
    do { if (!isDigit(value[i]) && value[i] != '.') return false; i++; } while(value[i] != '\0');
  }
  return true;
}

void connectAfterSavingData() { connectMQTT(); }

void factoryReset() { 
  for (int i=0;i<40;i++) { digitalWrite(LED_BUILTIN, HIGH); delay(50); digitalWrite(LED_BUILTIN, LOW); delay(50); }
  wm.resetSettings(); ESP.eraseConfig();
  EEPROM.begin(sizeof(Settings));
  for (int i=0; i < sizeof(Settings); i++) EEPROM.write(i, 0);
  EEPROM.commit(); EEPROM.end();
  ESP.restart();
}

void wifiConfigWebServerStarted() { wm.server->on("/exit", wifiConfigStop); }
void wifiConfigStarted(WiFiManager *wm) { wifiConfigActiveSince = millis(); }
void wifiConfigStart() { 
  if (!wm.getConfigPortalActive()) {
    Serial.println(F("[WIFICONFIG] Start")); WiFi.mode(WIFI_AP_STA); wm.startConfigPortal(klimerkoID, wifiConfigPortalPassword);
  }
}
void wifiConfigStop() { if (wm.getConfigPortalActive()) wm.stopConfigPortal(); }
void wifiConfigLoop() { 
  if (wm.getConfigPortalActive()) { wm.process(); if (millis() - wifiConfigActiveSince >= wifiConfigTimeout * 1000) wifiConfigStop(); }
}

void buttonLoop() { 
  buttonCurrentState = digitalRead(BUTTON_PIN);
  if (buttonLastState == HIGH && buttonCurrentState == LOW) { buttonPressedTime = millis(); buttonPressed = true; buttonLongPressDetected = false; } 
  else if (buttonLastState == LOW && buttonCurrentState == HIGH) {
    buttonReleasedTime = millis(); buttonPressed = false;
    long duration = buttonReleasedTime - buttonPressedTime;
    if (duration > buttonShortPressTime && duration < buttonMediumPressTime) wifiConfigStop();
    else if (duration > buttonMediumPressTime && duration < buttonLongPressTime) wifiConfigStart();
  }
  if (buttonPressed && !buttonLongPressDetected) {
    if (millis() - buttonPressedTime > buttonLongPressTime) { buttonLongPressDetected = true; factoryReset(); }
  }
  buttonLastState = buttonCurrentState;
}

void ledLoop() { 
  if (ledSuccessBlink) { for (int i=0;i<6;i++) { digitalWrite(LED_BUILTIN, LOW); delay(100); digitalWrite(LED_BUILTIN, HIGH); delay(100); } ledSuccessBlink = false; }
  if (wm.getConfigPortalActive()) ledState = true;
  else { if (wifiConnectionLost || mqttConnectionLost) { if (millis() - ledLastUpdate >= ledBlinkInterval) { ledState = !ledState; ledLastUpdate = millis(); } } else ledState = false; }
  digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
}

String extractAssetNameFromTopic(String topic) { return topic.substring(38, topic.length()-8); }

void mqttCallback(char* p_topic, byte* p_payload, unsigned int p_length) {
  Serial.println(F("[MQTT] Message"));
  if (p_length >= 512) return;
  DynamicJsonDocument doc(512); char json[512];
  memcpy(json, p_payload, p_length);
  json[p_length] = '\0';
  if (deserializeJson(doc, json)) return;

  String asset = extractAssetNameFromTopic(String(p_topic));

  if (asset == INTERVAL_ASSET) changeInterval(doc["value"]);
  else if (asset == WIFI_CONFIG_ASSET) {
    String v = doc["value"].as<String>(); if (v == "true" || v == "1") shouldStartConfig = true;
  }
  else if (asset == TEMP_OFFSET_ASSET) {
     String v = doc["value"].as<String>();
     if (isNumber(v.c_str())) {
        bmeTemperatureOffset = v.toFloat();
        snprintf(bmeTemperatureOffsetChar, 8, "%.2f", bmeTemperatureOffset);
        strcpy(klimerkoSettings.tempOffset, bmeTemperatureOffsetChar);
        EEPROM.begin(sizeof(Settings)); EEPROM.put(0, klimerkoSettings); EEPROM.commit(); EEPROM.end();
        portalTemperatureOffset.setValue(bmeTemperatureOffsetChar, 8);
        temp.reset(); hum.reset(); 
        publishDiagnosticData(); 
     }
  }
  else if (asset == ALTITUDE_SET_ASSET) {
     String v = doc["value"].as<String>();
     if (isNumber(v.c_str())) {
        altitude = v.toInt();
        snprintf(altitudeChar, 6, "%d", altitude);
        strcpy(klimerkoSettings.altitude, altitudeChar);
        EEPROM.begin(sizeof(Settings)); EEPROM.put(0, klimerkoSettings); EEPROM.commit(); EEPROM.end();
        portalAltitude.setValue(altitudeChar, 6);
        publishDiagnosticData();
     }
  }
  else if (asset == FIRMWARE_UPDATE_ASSET) {
      String url = doc["value"].as<String>();
      if (url.length() > 10) { pendingUpdateUrl = url; }
  }
  else if (asset == RESTART_DEVICE_ASSET) {
      String v = doc["value"].as<String>();
      if (v == "true" || v == "1") {
          Serial.println(F("[SYSTEM] Remote Restart..."));
          delay(1000);
          ESP.restart();
      }
  }
}

int wifiSignal() {
  if (!wifiConnectionLost) { return WiFi.RSSI(); }
  return 0; 
}

void initPMS() { pmsSerial.begin(9600); pmsPower(true); }
void initBME() { 
  // Inicijalizacija sa default podesavanjima (Adafruit)
  // Ovo je dovoljno dobro, problem je bio u logici koda
  bme.begin(0x76); 
}

void generateID() { snprintf(klimerkoID, sizeof(klimerkoID), "%s%i", "KLIMERKO-", ESP.getChipId()); Serial.println(klimerkoID); }
void mqttSubscribeTopics() { 
    char t[256]; snprintf(t, sizeof t, "%s%s%s", "device/", deviceId, "/asset/+/command"); mqtt.subscribe(t); 
}

bool connectMQTT() {
  if (!wifiConnectionLost) {
    if (mqtt.connect(klimerkoID, deviceToken, MQTT_PASSWORD)) {
      if (mqttConnectionLost) { mqttConnectionLost = false; ledSuccessBlink = true; }
      mqttSubscribeTopics(); publishDiagnosticData(); return true;
    } else { mqttConnectionLost = true; return false; }
  } return false;
}

void maintainMQTT() {
  mqtt.loop();
  if (mqtt.connected()) { if (mqttConnectionLost) { mqttConnectionLost = false; publishDiagnosticData(); } } 
  else {
    if (!mqttConnectionLost) mqttConnectionLost = true;
    if (millis() - mqttReconnectLastAttempt >= mqttReconnectInterval * 1000 && !wifiConnectionLost) { connectMQTT(); mqttReconnectLastAttempt = millis(); }
  }
}

bool initMQTT() { mqtt.setBufferSize(MQTT_MAX_MESSAGE_SIZE); mqtt.setServer(MQTT_SERVER, MQTT_PORT); mqtt.setKeepAlive(30); mqtt.setCallback(mqttCallback); return connectMQTT(); }

bool connectWiFi() {
  if(!wm.autoConnect(klimerkoID, wifiConfigPortalPassword)) { wifiConnectionLost = true; return false; } 
  else { wifiConnectionLost = false; ledSuccessBlink = true; return true; }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) { if (wifiConnectionLost) { wifiConnectionLost = false; ledSuccessBlink = true; } } 
  else { if (!wifiConnectionLost) wifiConnectionLost = true; if (millis() - wifiReconnectLastAttempt >= wifiReconnectInterval * 1000 && !wm.getConfigPortalActive()) { connectWiFi(); wifiReconnectLastAttempt = millis(); } }
}

void initOTA() {
  ArduinoOTA.setHostname(klimerkoID);
  ArduinoOTA.onStart([]() { Serial.println(F("OTA Start")); });
  ArduinoOTA.onEnd([]() { Serial.println(F("\nOTA End")); });
  ArduinoOTA.begin();
}

void initWiFi() {
  wm.setDebugOutput(false);
  wm.addParameter(&portalDeviceID); wm.addParameter(&portalDeviceToken); wm.addParameter(&portalTemperatureOffset); wm.addParameter(&portalAltitude);
  wm.addParameter(&portalDisplayFirmwareVersion); wm.addParameter(&portalDisplayCredits);
  wm.setSaveParamsCallback(saveData); wm.setSaveConfigCallback(connectAfterSavingData); wm.setWebServerCallback(wifiConfigWebServerStarted); wm.setAPCallback(wifiConfigStarted);
  wm.setConfigPortalBlocking(false); wm.setConfigPortalTimeout(wifiConfigTimeout); wm.setConnectRetries(2); wm.setConnectTimeout(5); wm.setDarkMode(true);
  wm.setTitle("Klimerko"); wm.setHostname(klimerkoID); wm.setCountry("RS"); wm.setEnableConfigPortal(false); wm.setParamsPage(false); wm.setSaveConnect(true); wm.setBreakAfterConfig(true); wm.setWiFiAutoReconnect(true);
  WiFi.mode(WIFI_STA); connectWiFi();
}

void initAvg() { pm1.begin(); pm25.begin(); pm10.begin(); temp.begin(); hum.begin(); pres.begin(); }
void initPins() { pinMode(BUTTON_PIN, INPUT); pinMode(LED_BUILTIN, OUTPUT); digitalWrite(LED_BUILTIN, HIGH); }

void setup() {
  Serial.begin(115200);
  Serial.println(F("\n --- KLIMERKO 5.4 Humidity Fix ---"));
  ESP.wdtEnable(5000);
  initAvg(); initPins(); initPMS(); initBME(); generateID(); restoreData(); initWiFi(); initOTA(); initMQTT();
}

void loop() {
  ESP.wdtFeed(); 
  ArduinoOTA.handle();
  
  if (shouldStartConfig) { shouldStartConfig = false; wifiConfigStart(); }
  
  if (pendingUpdateUrl != "") {
     String url = pendingUpdateUrl;
     pendingUpdateUrl = ""; 
     performFirmwareUpdate(url);
  }
  
  sensorLoop(); maintainWiFi(); maintainMQTT(); wifiConfigLoop(); buttonLoop(); ledLoop();  
}
