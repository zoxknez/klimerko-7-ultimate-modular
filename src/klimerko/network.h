/**
 * @file network.h
 * @brief Klimerko Network Management - WiFi, MQTT, mDNS, NTP, OTA
 * @version 7.0 Ultimate
 * 
 * Handles all network operations including WiFi connection management,
 * MQTT messaging, mDNS discovery, NTP time sync, and OTA updates.
 */

#ifndef KLIMERKO_NETWORK_H
#define KLIMERKO_NETWORK_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "config.h"
#include "types.h"
#include "../WiFiManager/WiFiManager.h"
#include "../PubSubClient/PubSubClient.h"

// ============================================================================
// GLOBAL NETWORK OBJECTS
// ============================================================================

extern WiFiManager wm;
extern WiFiClient networkClient;
extern PubSubClient mqtt;

// WiFi portal parameters
extern WiFiManagerParameter portalDeviceID;
extern WiFiManagerParameter portalDeviceToken;
extern WiFiManagerParameter portalTemperatureOffset;
extern WiFiManagerParameter portalAltitude;
extern WiFiManagerParameter portalDisplayFirmwareVersion;
extern WiFiManagerParameter portalDisplayCredits;

// ============================================================================
// NETWORK STATE
// ============================================================================

extern WifiState wifiState;
extern MqttState mqttState;
extern char klimerkoID[32];
extern char apPassword[16];
extern char otaPassword[16];
extern char mdnsHostname[32];
extern char deviceId[32];
extern char deviceToken[64];

// MQTT configuration
extern char mqttServer[64];
extern uint16_t mqttPort;

// NTP state
extern bool ntpSynced;

// ============================================================================
// IDENTITY GENERATION
// ============================================================================

/**
 * @brief Generate unique device ID from ESP ChipID
 * @param buffer Output buffer for device ID
 * @param bufferSize Buffer size
 */
inline void generateDeviceID(char* buffer, size_t bufferSize) {
  snprintf(buffer, bufferSize, "KLIMERKO-%u", ESP.getChipId());
  DEBUG_PRINT(F("[ID] Device: ")); DEBUG_PRINTLN(buffer);
}

/**
 * @brief Generate unique passwords and hostname from ChipID
 * 
 * Creates:
 * - AP Password: K + 8-char hex chip ID
 * - OTA Password: O + 8-char hex chip ID
 * - mDNS hostname: klimerko-xxxxxx
 */
inline void generateUniquePasswords(char* apPass, char* otaPass, char* mdnsHost) {
  uint32_t chipId = ESP.getChipId();
  snprintf(apPass, 16, "K%08X", chipId);
  snprintf(otaPass, 16, "O%08X", chipId);
  snprintf(mdnsHost, 32, "klimerko-%06x", chipId & 0xFFFFFF);
  
  DEBUG_PRINT(F("[SEC] AP Password: ")); DEBUG_PRINTLN(apPass);
  DEBUG_PRINT(F("[SEC] OTA Password: ")); DEBUG_PRINTLN(otaPass);
  DEBUG_PRINT(F("[SEC] mDNS: ")); DEBUG_PRINT(mdnsHost); DEBUG_PRINTLN(F(".local"));
}

// ============================================================================
// WIFI MANAGEMENT
// ============================================================================

/**
 * @brief Get current WiFi signal strength
 * @return RSSI in dBm, or 0 if not connected
 */
inline int getWifiSignal() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return 0;
}

/**
 * @brief Check if WiFi is connected
 * @return true if connected
 */
inline bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Connect to WiFi using WiFiManager
 * @return true if connected
 */
inline bool connectWiFi() {
  if (!wm.autoConnect(klimerkoID, apPassword)) {
    wifiState.connectionLost = true;
    wifiState.reconnectFailCount++;
    
    // Exponential backoff
    wifiState.reconnectInterval = min(WIFI_RECONNECT_MAX_INTERVAL, 
                                      WIFI_RECONNECT_BASE_INTERVAL * 
                                      (1UL << min(wifiState.reconnectFailCount, (uint8_t)5)));
    DEBUG_PRINTLN(F("[WIFI] Connection failed"));
    return false;
  }
  
  wifiState.connectionLost = false;
  wifiState.reconnectFailCount = 0;
  wifiState.reconnectInterval = WIFI_RECONNECT_BASE_INTERVAL;
  DEBUG_PRINT(F("[WIFI] Connected! IP: "));
  DEBUG_PRINTLN(WiFi.localIP());
  return true;
}

/**
 * @brief Maintain WiFi connection with exponential backoff
 * @return true if connected
 */
inline bool maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiState.connectionLost) {
      wifiState.connectionLost = false;
      wifiState.reconnectFailCount = 0;
      wifiState.reconnectInterval = WIFI_RECONNECT_BASE_INTERVAL;
      return true;
    }
    return true;
  }
  
  // Connection lost
  if (!wifiState.connectionLost) {
    wifiState.connectionLost = true;
    DEBUG_PRINTLN(F("[WIFI] Connection lost"));
  }
  
  // Try to reconnect with backoff
  if (!wm.getConfigPortalActive() && 
      millis() - wifiState.lastReconnectAttempt >= wifiState.reconnectInterval) {
    DEBUG_PRINTLN(F("[WIFI] Attempting reconnect..."));
    connectWiFi();
    wifiState.lastReconnectAttempt = millis();
  }
  
  return false;
}

// ============================================================================
// WIFI CONFIG PORTAL
// ============================================================================

/**
 * @brief Start WiFi configuration portal
 */
inline void wifiConfigStart() {
  if (!wm.getConfigPortalActive()) {
    DEBUG_PRINTLN(F("[WIFICONFIG] Starting portal"));
    DEBUG_PRINT(F("[WIFICONFIG] AP: ")); DEBUG_PRINTLN(klimerkoID);
    DEBUG_PRINT(F("[WIFICONFIG] Password: ")); DEBUG_PRINTLN(apPassword);
    WiFi.mode(WIFI_AP_STA);
    wm.startConfigPortal(klimerkoID, apPassword);
    wifiState.configActiveSince = millis();
  }
}

/**
 * @brief Stop WiFi configuration portal
 */
inline void wifiConfigStop() {
  if (wm.getConfigPortalActive()) {
    wm.stopConfigPortal();
    DEBUG_PRINTLN(F("[WIFICONFIG] Portal stopped"));
  }
}

/**
 * @brief Process WiFi configuration portal with timeout
 */
inline void wifiConfigLoop() {
  if (wm.getConfigPortalActive()) {
    wm.process();
    if (millis() - wifiState.configActiveSince >= WIFI_CONFIG_TIMEOUT) {
      DEBUG_PRINTLN(F("[WIFICONFIG] Timeout, stopping"));
      wifiConfigStop();
    }
  }
}

/**
 * @brief Check if config portal is active
 * @return true if portal is running
 */
inline bool isConfigPortalActive() {
  return wm.getConfigPortalActive();
}

// ============================================================================
// MQTT FUNCTIONS
// ============================================================================

// Forward declaration for callback
typedef void (*MqttCallbackFunc)(char*, byte*, unsigned int);

/**
 * @brief Build MQTT topic string
 * @param buffer Output buffer
 * @param bufferSize Buffer size
 * @param suffix Topic suffix (e.g., "state", "asset/+/command")
 */
inline void buildMqttTopicStr(char* buffer, size_t bufferSize, const char* suffix) {
  snprintf(buffer, bufferSize, "device/%s/%s", deviceId, suffix);
}

/**
 * @brief Subscribe to MQTT command topics
 */
inline void mqttSubscribeTopics() {
  char topic[128];
  buildMqttTopicStr(topic, sizeof(topic), "asset/+/command");
  mqtt.subscribe(topic);
  DEBUG_PRINT(F("[MQTT] Subscribed: ")); DEBUG_PRINTLN(topic);
}

/**
 * @brief Connect to MQTT broker
 * @return true if connected
 */
inline bool connectMQTT() {
  if (wifiState.connectionLost) return false;
  
  DEBUG_PRINTF("[MQTT] Connecting to %s:%d\n", mqttServer, mqttPort);
  
  if (mqtt.connect(klimerkoID, deviceToken, MQTT_PASSWORD)) {
    mqttState.connectionLost = false;
    mqttSubscribeTopics();
    DEBUG_PRINTLN(F("[MQTT] Connected!"));
    return true;
  }
  
  mqttState.connectionLost = true;
  mqttState.reconnectCount++;
  DEBUG_PRINTLN(F("[MQTT] Connection failed"));
  return false;
}

/**
 * @brief Maintain MQTT connection
 * @return true if connected
 */
inline bool maintainMQTT() {
  mqtt.loop();
  
  if (mqtt.connected()) {
    if (mqttState.connectionLost) {
      mqttState.connectionLost = false;
    }
    return true;
  }
  
  // Connection lost
  if (!mqttState.connectionLost) {
    mqttState.connectionLost = true;
    DEBUG_PRINTLN(F("[MQTT] Connection lost"));
  }
  
  // Try to reconnect
  if (!wifiState.connectionLost && 
      millis() - mqttState.lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
    connectMQTT();
    mqttState.lastReconnectAttempt = millis();
  }
  
  return false;
}

/**
 * @brief Check if MQTT is connected
 * @return true if connected
 */
inline bool isMqttConnected() {
  return mqtt.connected();
}

/**
 * @brief Publish JSON data to MQTT
 * @param topic Topic to publish to
 * @param payload JSON payload string
 * @param retained Retain message flag
 * @return true if published successfully
 */
inline bool mqttPublish(const char* topic, const char* payload, bool retained = false) {
  if (!mqtt.connected()) {
    DEBUG_PRINTLN(F("[MQTT] Cannot publish - not connected"));
    return false;
  }
  
  bool result = mqtt.publish(topic, payload, retained);
  if (result) {
    DEBUG_PRINT(F("[MQTT] Published to ")); DEBUG_PRINTLN(topic);
  } else {
    DEBUG_PRINTLN(F("[MQTT] Publish failed!"));
  }
  return result;
}

/**
 * @brief Publish to device state topic
 * @param payload JSON payload string
 * @return true if published successfully
 */
inline bool publishToState(const char* payload) {
  char topic[128];
  buildMqttTopicStr(topic, sizeof(topic), "state");
  return mqttPublish(topic, payload);
}

/**
 * @brief Initialize MQTT client
 * @param callback MQTT message callback function
 */
inline void initMQTT(MqttCallbackFunc callback) {
  mqtt.setBufferSize(MQTT_MAX_MESSAGE_SIZE);
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setKeepAlive(30);
  mqtt.setCallback(callback);
  DEBUG_PRINTF("[MQTT] Configured for %s:%d\n", mqttServer, mqttPort);
  connectMQTT();
}

/**
 * @brief Update MQTT broker configuration
 * @param server New broker hostname
 * @param port New broker port
 */
inline void updateMqttBroker(const char* server, uint16_t port) {
  strncpy(mqttServer, server, sizeof(mqttServer) - 1);
  mqttServer[sizeof(mqttServer) - 1] = '\0';
  mqttPort = port;
  
  mqtt.disconnect();
  mqtt.setServer(mqttServer, mqttPort);
  connectMQTT();
  
  DEBUG_PRINTF("[MQTT] Broker updated: %s:%d\n", mqttServer, mqttPort);
}

// ============================================================================
// mDNS FUNCTIONS
// ============================================================================

/**
 * @brief Initialize mDNS responder
 * @return true if started successfully
 */
inline bool initMDNS() {
  if (MDNS.begin(mdnsHostname)) {
    MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    MDNS.addService("prometheus", "tcp", WEB_SERVER_PORT);
    DEBUG_PRINT(F("[mDNS] Started: ")); DEBUG_PRINT(mdnsHostname);
    DEBUG_PRINTLN(F(".local"));
    return true;
  }
  DEBUG_PRINTLN(F("[mDNS] Failed to start"));
  return false;
}

/**
 * @brief Update mDNS responder (call in loop)
 */
inline void updateMDNS() {
  MDNS.update();
}

// ============================================================================
// NTP TIME FUNCTIONS
// ============================================================================

/**
 * @brief Initialize NTP time synchronization
 * @param gmtOffsetSec GMT offset in seconds
 * @param dstOffsetSec DST offset in seconds
 * @return true if time synced
 */
inline bool initNTP(long gmtOffsetSec = GMT_OFFSET_SEC, 
                    int dstOffsetSec = DAYLIGHT_OFFSET_SEC) {
  configTime(gmtOffsetSec, dstOffsetSec, NTP_SERVER_1, NTP_SERVER_2);
  DEBUG_PRINTLN(F("[NTP] Configuring time..."));
  
  // Wait up to 10 seconds for sync
  int timeout = 20;
  time_t now = time(nullptr);
  while (now < 1000000000 && timeout > 0) {
    delay(500);
    now = time(nullptr);
    timeout--;
  }
  
  if (now > 1000000000) {
    ntpSynced = true;
    struct tm* timeinfo = localtime(&now);
    DEBUG_PRINTF("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return true;
  }
  
  ntpSynced = false;
  DEBUG_PRINTLN(F("[NTP] Sync failed"));
  return false;
}

/**
 * @brief Get current time as ISO timestamp
 * @return ISO format timestamp or uptime string
 */
inline String getISOTimestamp() {
  if (!ntpSynced) {
    return String(millis() / 1000);
  }
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
           timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(buffer);
}

/**
 * @brief Get current time formatted as HH:MM:SS
 * @return Time string
 */
inline String getFormattedTime() {
  if (!ntpSynced) {
    return formatUptime(millis() / 1000);
  }
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(buffer);
}

/**
 * @brief Check if NTP is synchronized
 * @return true if time is synced
 */
inline bool isNtpSynced() {
  return ntpSynced;
}

// ============================================================================
// OTA UPDATE FUNCTIONS
// ============================================================================

/**
 * @brief Initialize ArduinoOTA with password protection
 */
inline void initOTA() {
  ArduinoOTA.setHostname(klimerkoID);
  ArduinoOTA.setPassword(otaPassword);
  
  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN(F("[OTA] Starting update..."));
  });
  
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN(F("\n[OTA] Update complete!"));
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("[OTA] Error[%u]: ", error);
    switch (error) {
      case OTA_AUTH_ERROR: DEBUG_PRINTLN(F("Auth Failed")); break;
      case OTA_BEGIN_ERROR: DEBUG_PRINTLN(F("Begin Failed")); break;
      case OTA_CONNECT_ERROR: DEBUG_PRINTLN(F("Connect Failed")); break;
      case OTA_RECEIVE_ERROR: DEBUG_PRINTLN(F("Receive Failed")); break;
      case OTA_END_ERROR: DEBUG_PRINTLN(F("End Failed")); break;
    }
  });
  
  ArduinoOTA.begin();
  DEBUG_PRINTLN(F("[OTA] Initialized"));
}

/**
 * @brief Handle OTA updates (call in loop)
 */
inline void handleOTA() {
  ArduinoOTA.handle();
}

// ============================================================================
// HTTP FIRMWARE UPDATE
// ============================================================================

/**
 * @brief Perform HTTP firmware update from URL
 * @param url Firmware binary URL
 * @return true if update started (device will reboot)
 */
inline bool performHttpUpdate(const String& url) {
  DEBUG_PRINTLN(F("[UPDATE] Starting HTTP firmware update..."));
  DEBUG_PRINT(F("[UPDATE] URL: ")); DEBUG_PRINTLN(url);
  
  ESP.wdtDisable();
  
  WiFiClientSecure client;
  client.setInsecure();  // TODO: Add certificate pinning
  client.setTimeout(15000);
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
  
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      DEBUG_PRINTF("[UPDATE] FAILED (%d): %s\n", 
                   ESPhttpUpdate.getLastError(), 
                   ESPhttpUpdate.getLastErrorString().c_str());
      ESP.wdtEnable(5000);
      return false;
    case HTTP_UPDATE_NO_UPDATES:
      DEBUG_PRINTLN(F("[UPDATE] No updates available"));
      ESP.wdtEnable(5000);
      return false;
    case HTTP_UPDATE_OK:
      DEBUG_PRINTLN(F("[UPDATE] Success! Rebooting..."));
      ESP.restart();
      return true;
  }
  return false;
}

// ============================================================================
// NETWORK INITIALIZATION
// ============================================================================

/**
 * @brief Complete network initialization
 * 
 * Initializes WiFi, mDNS, NTP, and OTA.
 * Call after WiFiManager parameters are set.
 * 
 * @param mqttCallback MQTT message callback function
 */
inline void initNetwork(MqttCallbackFunc mqttCallback) {
  // Generate unique identifiers
  generateDeviceID(klimerkoID, sizeof(klimerkoID));
  generateUniquePasswords(apPassword, otaPassword, mdnsHostname);
  
  // Connect WiFi
  connectWiFi();
  
  if (!wifiState.connectionLost) {
    // Initialize network services
    initNTP();
    initMDNS();
    initOTA();
    initMQTT(mqttCallback);
  }
}

/**
 * @brief Main network loop - call in main loop()
 */
inline void networkLoop() {
  handleOTA();
  updateMDNS();
  maintainWiFi();
  maintainMQTT();
  wifiConfigLoop();
}

#endif // KLIMERKO_NETWORK_H
