/**
 * @file storage.h
 * @brief Klimerko Storage Management - EEPROM and LittleFS
 * @version 7.0 Ultimate
 * 
 * Handles persistent storage operations including:
 * - EEPROM settings with CRC32 validation
 * - LittleFS data logging
 * - Statistics persistence
 */

#ifndef KLIMERKO_STORAGE_H
#define KLIMERKO_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "../ArduinoJson-v6.18.5.h"

// ============================================================================
// GLOBAL STORAGE STATE
// ============================================================================

extern Settings klimerkoSettings;
extern Statistics stats;

// ============================================================================
// LITTLEFS MANAGEMENT
// ============================================================================

/**
 * @brief Initialize LittleFS filesystem
 * @return true if mounted successfully
 */
inline bool initLittleFS() {
  if (!LittleFS.begin()) {
    DEBUG_PRINTLN(F("[FS] LittleFS mount failed, formatting..."));
    if (LittleFS.format()) {
      DEBUG_PRINTLN(F("[FS] Format successful"));
      if (LittleFS.begin()) {
        DEBUG_PRINTLN(F("[FS] LittleFS mounted after format"));
        return true;
      }
    }
    DEBUG_PRINTLN(F("[FS] Format failed!"));
    return false;
  }
  DEBUG_PRINTLN(F("[FS] LittleFS mounted"));
  return true;
}

/**
 * @brief Get filesystem info
 * @param totalBytes Output total bytes
 * @param usedBytes Output used bytes
 */
inline void getFilesystemInfo(size_t& totalBytes, size_t& usedBytes) {
  FSInfo fs_info;
  if (LittleFS.info(fs_info)) {
    totalBytes = fs_info.totalBytes;
    usedBytes = fs_info.usedBytes;
  } else {
    totalBytes = 0;
    usedBytes = 0;
  }
}

// ============================================================================
// DATA LOGGING (LittleFS)
// ============================================================================

/**
 * @brief Log sensor data to LittleFS JSON file
 * @param data Sensor data to log
 * @param uptimeSeconds Current uptime
 */
inline void logSensorDataToFS(const SensorData& data, unsigned long uptimeSeconds) {
  // Create file if doesn't exist
  if (!LittleFS.exists(LOG_FILE_PATH)) {
    File f = LittleFS.open(LOG_FILE_PATH, "w");
    if (f) {
      f.println("[]");
      f.close();
    } else {
      DEBUG_PRINTLN(F("[FS] Cannot create log file"));
      return;
    }
  }
  
  // Read existing log
  File logFile = LittleFS.open(LOG_FILE_PATH, "r");
  if (!logFile) {
    DEBUG_PRINTLN(F("[FS] Cannot open log file"));
    return;
  }
  
  StaticJsonDocument<8192> doc;
  DeserializationError err = deserializeJson(doc, logFile);
  logFile.close();
  
  if (err) {
    DEBUG_PRINT(F("[FS] JSON parse error: ")); DEBUG_PRINTLN(err.c_str());
    DEBUG_PRINTLN(F("[FS] Resetting log file"));
    LittleFS.remove(LOG_FILE_PATH);
    return;
  }
  
  JsonArray arr = doc.as<JsonArray>();
  
  // Remove oldest entries if exceeding max
  while (arr.size() >= MAX_LOG_ENTRIES) {
    arr.remove(0);
  }
  
  // Add new entry
  JsonObject entry = arr.createNestedObject();
  entry["ts"] = uptimeSeconds;
  entry["pm1"] = data.pm1;
  entry["pm25"] = data.pm25;
  entry["pm10"] = data.pm10;
  entry["temp"] = serialized(String(data.temperature, 1));
  entry["hum"] = serialized(String(data.humidity, 1));
  entry["pres"] = serialized(String(data.pressure, 1));
  
  // Write back
  logFile = LittleFS.open(LOG_FILE_PATH, "w");
  if (logFile) {
    serializeJson(doc, logFile);
    logFile.close();
    DEBUG_PRINTLN(F("[FS] Data logged"));
  } else {
    DEBUG_PRINTLN(F("[FS] Failed to write log"));
  }
}

/**
 * @brief Read log file contents
 * @return JSON string of log data
 */
inline String readLogFile() {
  if (!LittleFS.exists(LOG_FILE_PATH)) {
    return "[]";
  }
  
  File logFile = LittleFS.open(LOG_FILE_PATH, "r");
  if (!logFile) {
    return "[]";
  }
  
  String content = logFile.readString();
  logFile.close();
  return content;
}

/**
 * @brief Clear log file
 */
inline void clearLogFile() {
  if (LittleFS.exists(LOG_FILE_PATH)) {
    LittleFS.remove(LOG_FILE_PATH);
    DEBUG_PRINTLN(F("[FS] Log cleared"));
  }
}

/**
 * @brief Get log file size in bytes
 * @return File size or 0 if not exists
 */
inline size_t getLogFileSize() {
  if (!LittleFS.exists(LOG_FILE_PATH)) {
    return 0;
  }
  File f = LittleFS.open(LOG_FILE_PATH, "r");
  if (!f) return 0;
  size_t size = f.size();
  f.close();
  return size;
}

// ============================================================================
// EEPROM SETTINGS
// ============================================================================

/**
 * @brief Load default settings
 */
inline void loadDefaultSettings(char* devId, char* devToken, 
                                char* tempOffsetChar, float& tempOffset,
                                char* altitudeChar, int& userAltitude) {
  devId[0] = '\0';
  devToken[0] = '\0';
  strcpy(tempOffsetChar, DEFAULT_TEMP_OFFSET_STR);
  tempOffset = DEFAULT_TEMP_OFFSET;
  strcpy(altitudeChar, "0");
  userAltitude = 0;
  DEBUG_PRINTLN(F("[EEPROM] Default settings loaded"));
}

/**
 * @brief Restore settings from EEPROM with CRC32 validation
 * @return true if valid settings restored
 */
inline bool restoreSettings(char* devId, char* devToken,
                           char* tempOffsetChar, float& tempOffset,
                           char* altitudeChar, int& userAltitude,
                           bool& deepSleepEnabled, bool& alarmEnabled,
                           char* mqttBroker, uint16_t& mqttBrokerPort,
                           Calibration& cal) {
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(0, klimerkoSettings);
  EEPROM.end();
  
  // Verify header
  if (String(klimerkoSettings.header) != "KLI") {
    DEBUG_PRINTLN(F("[EEPROM] No valid header - using defaults"));
    loadDefaultSettings(devId, devToken, tempOffsetChar, tempOffset, 
                        altitudeChar, userAltitude);
    return false;
  }
  
  // Verify CRC32
  uint32_t calculatedCrc = calculateSettingsCRC(klimerkoSettings);
  if (calculatedCrc != klimerkoSettings.crc32) {
    DEBUG_PRINTLN(F("[EEPROM] CRC mismatch - using defaults"));
    loadDefaultSettings(devId, devToken, tempOffsetChar, tempOffset, 
                        altitudeChar, userAltitude);
    return false;
  }
  
  // Restore values
  safeStrCopy(devId, klimerkoSettings.deviceId, 32);
  safeStrCopy(devToken, klimerkoSettings.deviceToken, 64);
  safeStrCopy(tempOffsetChar, klimerkoSettings.tempOffset, 8);
  tempOffset = atof(tempOffsetChar);
  safeStrCopy(altitudeChar, klimerkoSettings.altitude, 6);
  userAltitude = atoi(altitudeChar);
  
  deepSleepEnabled = klimerkoSettings.deepSleepEnabled;
  alarmEnabled = klimerkoSettings.alarmEnabled;
  
  // Load custom MQTT broker
  if (strlen(klimerkoSettings.mqttBroker) > 0) {
    safeStrCopy(mqttBroker, klimerkoSettings.mqttBroker, 64);
    mqttBrokerPort = klimerkoSettings.mqttBrokerPort > 0 ? 
                     klimerkoSettings.mqttBrokerPort : DEFAULT_MQTT_PORT;
  }
  
  // Load calibration with validation
  if (isValidCalibrationFactor(klimerkoSettings.pm25CalFactor)) {
    cal.pm25Factor = klimerkoSettings.pm25CalFactor;
  }
  if (isValidCalibrationFactor(klimerkoSettings.pm10CalFactor)) {
    cal.pm10Factor = klimerkoSettings.pm10CalFactor;
  }
  
  DEBUG_PRINTLN(F("[EEPROM] Settings restored (CRC valid)"));
  DEBUG_PRINT(F("[EEPROM] Device ID: ")); DEBUG_PRINTLN(devId);
  DEBUG_PRINT(F("[EEPROM] Deep Sleep: ")); DEBUG_PRINTLN(deepSleepEnabled ? "On" : "Off");
  DEBUG_PRINT(F("[EEPROM] Alarms: ")); DEBUG_PRINTLN(alarmEnabled ? "On" : "Off");
  DEBUG_PRINT(F("[EEPROM] MQTT: ")); DEBUG_PRINT(mqttBroker);
  DEBUG_PRINT(":"); DEBUG_PRINTLN(mqttBrokerPort);
  
  return true;
}

/**
 * @brief Save settings to EEPROM with CRC32
 * @return true if saved successfully
 */
inline bool saveSettings(const char* devId, const char* devToken,
                        const char* tempOffsetChar, const char* altitudeChar,
                        bool deepSleepEnabled, bool alarmEnabled,
                        const char* mqttBroker, uint16_t mqttBrokerPort,
                        const Calibration& cal) {
  DEBUG_PRINTLN(F("[EEPROM] Saving settings..."));
  
  // Prepare settings struct
  strcpy(klimerkoSettings.header, "KLI");
  safeStrCopy(klimerkoSettings.deviceId, devId, sizeof(klimerkoSettings.deviceId));
  safeStrCopy(klimerkoSettings.deviceToken, devToken, sizeof(klimerkoSettings.deviceToken));
  safeStrCopy(klimerkoSettings.tempOffset, tempOffsetChar, sizeof(klimerkoSettings.tempOffset));
  safeStrCopy(klimerkoSettings.altitude, altitudeChar, sizeof(klimerkoSettings.altitude));
  klimerkoSettings.deepSleepEnabled = deepSleepEnabled;
  klimerkoSettings.alarmEnabled = alarmEnabled;
  safeStrCopy(klimerkoSettings.mqttBroker, mqttBroker, sizeof(klimerkoSettings.mqttBroker));
  klimerkoSettings.mqttBrokerPort = mqttBrokerPort;
  klimerkoSettings.pm25CalFactor = cal.pm25Factor;
  klimerkoSettings.pm10CalFactor = cal.pm10Factor;
  
  // Calculate CRC
  klimerkoSettings.crc32 = calculateSettingsCRC(klimerkoSettings);
  
  // Write to EEPROM
  EEPROM.begin(sizeof(Settings));
  EEPROM.put(0, klimerkoSettings);
  bool success = EEPROM.commit();
  EEPROM.end();
  
  if (success) {
    DEBUG_PRINTLN(F("[EEPROM] Settings saved with CRC"));
  } else {
    DEBUG_PRINTLN(F("[EEPROM] Save failed!"));
  }
  
  return success;
}

/**
 * @brief Update single setting field and save
 * @param field Setting field name
 * @param value New value
 */
inline void updateSetting(const char* field, const char* value) {
  bool changed = false;
  
  if (strcmp(field, "tempOffset") == 0) {
    safeStrCopy(klimerkoSettings.tempOffset, value, sizeof(klimerkoSettings.tempOffset));
    changed = true;
  } else if (strcmp(field, "altitude") == 0) {
    safeStrCopy(klimerkoSettings.altitude, value, sizeof(klimerkoSettings.altitude));
    changed = true;
  } else if (strcmp(field, "mqttBroker") == 0) {
    safeStrCopy(klimerkoSettings.mqttBroker, value, sizeof(klimerkoSettings.mqttBroker));
    changed = true;
  }
  
  if (changed) {
    klimerkoSettings.crc32 = calculateSettingsCRC(klimerkoSettings);
    EEPROM.begin(sizeof(Settings));
    EEPROM.put(0, klimerkoSettings);
    EEPROM.commit();
    EEPROM.end();
    DEBUG_PRINT(F("[EEPROM] Updated ")); DEBUG_PRINTLN(field);
  }
}

/**
 * @brief Update boolean setting
 * @param field Setting field name
 * @param value New value
 */
inline void updateBoolSetting(const char* field, bool value) {
  bool changed = false;
  
  if (strcmp(field, "deepSleep") == 0) {
    klimerkoSettings.deepSleepEnabled = value;
    changed = true;
  } else if (strcmp(field, "alarmEnabled") == 0) {
    klimerkoSettings.alarmEnabled = value;
    changed = true;
  }
  
  if (changed) {
    klimerkoSettings.crc32 = calculateSettingsCRC(klimerkoSettings);
    EEPROM.begin(sizeof(Settings));
    EEPROM.put(0, klimerkoSettings);
    EEPROM.commit();
    EEPROM.end();
    DEBUG_PRINT(F("[EEPROM] Updated ")); DEBUG_PRINT(field);
    DEBUG_PRINT(F(": ")); DEBUG_PRINTLN(value ? "true" : "false");
  }
}

/**
 * @brief Update calibration settings
 * @param cal New calibration values
 */
inline void updateCalibration(const Calibration& cal) {
  klimerkoSettings.pm25CalFactor = cal.pm25Factor;
  klimerkoSettings.pm10CalFactor = cal.pm10Factor;
  klimerkoSettings.crc32 = calculateSettingsCRC(klimerkoSettings);
  
  EEPROM.begin(sizeof(Settings));
  EEPROM.put(0, klimerkoSettings);
  EEPROM.commit();
  EEPROM.end();
  
  DEBUG_PRINTF("[EEPROM] Calibration updated - PM2.5: %.2f, PM10: %.2f\n",
               cal.pm25Factor, cal.pm10Factor);
}

// ============================================================================
// STATISTICS PERSISTENCE
// ============================================================================

/**
 * @brief Load statistics from EEPROM
 */
inline void loadStatistics() {
  size_t statsOffset = sizeof(Settings);
  EEPROM.begin(statsOffset + sizeof(Statistics));
  EEPROM.get(statsOffset, stats);
  EEPROM.end();
  
  // Validate (sanity check for garbage data)
  if (stats.bootCount > 100000 || stats.successfulPublishes > 10000000) {
    DEBUG_PRINTLN(F("[STATS] Invalid data, resetting"));
    memset(&stats, 0, sizeof(Statistics));
  }
  
  stats.bootCount++;
  DEBUG_PRINT(F("[STATS] Boot #")); DEBUG_PRINTLN(stats.bootCount);
}

/**
 * @brief Save statistics to EEPROM
 * @param uptimeSeconds Current uptime
 */
inline void saveStatistics(unsigned long uptimeSeconds) {
  size_t statsOffset = sizeof(Settings);
  stats.uptimeSeconds = uptimeSeconds;
  
  EEPROM.begin(statsOffset + sizeof(Statistics));
  EEPROM.put(statsOffset, stats);
  EEPROM.commit();
  EEPROM.end();
  
  DEBUG_PRINTLN(F("[STATS] Saved"));
}

/**
 * @brief Increment WiFi reconnect counter
 */
inline void incrementWifiReconnects() {
  stats.wifiReconnects++;
}

/**
 * @brief Increment MQTT reconnect counter
 */
inline void incrementMqttReconnects() {
  stats.mqttReconnects++;
}

/**
 * @brief Record successful publish
 */
inline void recordSuccessfulPublish() {
  stats.successfulPublishes++;
}

/**
 * @brief Record failed publish
 */
inline void recordFailedPublish() {
  stats.failedPublishes++;
}

/**
 * @brief Get current statistics
 * @return Pointer to statistics struct
 */
inline Statistics* getStatistics() {
  return &stats;
}

// ============================================================================
// FACTORY RESET
// ============================================================================

/**
 * @brief Perform complete factory reset
 * 
 * Erases WiFi credentials, EEPROM data, and restarts device.
 * @param wm Reference to WiFiManager instance
 */
template<typename WifiMgr>
inline void factoryReset(WifiMgr& wm) {
  DEBUG_PRINTLN(F("[SYSTEM] Factory Reset..."));
  
  // Visual indication (fast LED blink)
  for (int i = 0; i < 40; i++) {
    digitalWrite(LED_BUILTIN, (i % 2) ? HIGH : LOW);
    delay(50);
  }
  
  // Reset WiFi credentials
  wm.resetSettings();
  ESP.eraseConfig();
  
  // Clear EEPROM
  size_t totalSize = sizeof(Settings) + sizeof(Statistics);
  EEPROM.begin(totalSize);
  for (size_t i = 0; i < totalSize; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
  
  // Clear LittleFS log
  if (LittleFS.exists(LOG_FILE_PATH)) {
    LittleFS.remove(LOG_FILE_PATH);
  }
  
  DEBUG_PRINTLN(F("[SYSTEM] Reset complete, rebooting..."));
  delay(500);
  ESP.restart();
}

#endif // KLIMERKO_STORAGE_H
