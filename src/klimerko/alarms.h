/**
 * @file alarms.h
 * @brief Klimerko Alarm System - Threshold monitoring and notifications
 * @version 7.0 Ultimate
 * 
 * Monitors air quality thresholds and triggers visual/MQTT alarms.
 */

#ifndef KLIMERKO_ALARMS_H
#define KLIMERKO_ALARMS_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "../ArduinoJson-v6.18.5.h"

// ============================================================================
// ALARM STATE
// ============================================================================

extern AlarmState alarmState;
extern bool alarmEnabled;
extern bool alarmTriggered;

// External references
extern SensorData sensorData;

// ============================================================================
// ALARM CONFIGURATION
// ============================================================================

/**
 * @brief Enable or disable alarm system
 * @param enabled true to enable
 */
inline void setAlarmEnabled(bool enabled) {
  alarmEnabled = enabled;
  if (!enabled) {
    alarmTriggered = false;
    alarmState.triggered = false;
  }
  DEBUG_PRINT(F("[ALARM] System ")); 
  DEBUG_PRINTLN(enabled ? F("enabled") : F("disabled"));
}

/**
 * @brief Set PM2.5 alarm threshold
 * @param threshold Threshold in µg/m³
 */
inline void setPM25Threshold(int threshold) {
  alarmState.pm25Threshold = threshold;
  DEBUG_PRINT(F("[ALARM] PM2.5 threshold: ")); DEBUG_PRINTLN(threshold);
}

/**
 * @brief Set PM10 alarm threshold
 * @param threshold Threshold in µg/m³
 */
inline void setPM10Threshold(int threshold) {
  alarmState.pm10Threshold = threshold;
  DEBUG_PRINT(F("[ALARM] PM10 threshold: ")); DEBUG_PRINTLN(threshold);
}

/**
 * @brief Set alarm cooldown period
 * @param cooldownMs Cooldown in milliseconds
 */
inline void setAlarmCooldown(unsigned long cooldownMs) {
  alarmState.cooldownMs = cooldownMs;
  DEBUG_PRINT(F("[ALARM] Cooldown: ")); DEBUG_PRINT(cooldownMs / 1000); 
  DEBUG_PRINTLN(F("s"));
}

// ============================================================================
// ALARM CHECKING
// ============================================================================

/**
 * @brief Visual alarm indication using LED
 * @param blinks Number of fast blinks
 */
inline void visualAlarm(uint8_t blinks = 10) {
  for (uint8_t i = 0; i < blinks; i++) {
    digitalWrite(LED_BUILTIN, LOW);   // LED ON
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);  // LED OFF
    delay(100);
  }
}

/**
 * @brief Check sensor values against alarm thresholds
 * @param publishCallback Callback to publish alarm to MQTT
 * @return true if alarm was triggered
 * 
 * This function checks PM2.5 and PM10 values against configured
 * thresholds and triggers alarms with cooldown protection.
 */
template<typename PublishFunc>
inline bool checkAlarms(PublishFunc publishCallback) {
  if (!alarmEnabled) {
    return false;
  }
  
  unsigned long now = millis();
  
  // Check cooldown
  if (alarmState.triggered && 
      (now - alarmState.lastTriggerTime < alarmState.cooldownMs)) {
    return false;
  }
  
  bool shouldAlarm = false;
  String alarmReason = "";
  
  // Check PM2.5
  if (sensorData.pm25 > alarmState.pm25Threshold) {
    shouldAlarm = true;
    alarmReason = "PM2.5 HIGH: " + String(sensorData.pm25) + " µg/m³";
    DEBUG_PRINT(F("[ALARM] ")); DEBUG_PRINTLN(alarmReason);
  }
  
  // Check PM10
  if (sensorData.pm10 > alarmState.pm10Threshold) {
    if (shouldAlarm) {
      alarmReason += ", ";
    }
    shouldAlarm = true;
    alarmReason += "PM10 HIGH: " + String(sensorData.pm10) + " µg/m³";
    DEBUG_PRINT(F("[ALARM] PM10 HIGH: ")); DEBUG_PRINTLN(sensorData.pm10);
  }
  
  if (shouldAlarm) {
    alarmState.triggered = true;
    alarmState.lastTriggerTime = now;
    alarmTriggered = true;
    
    // Visual alarm
    visualAlarm(10);
    
    // Publish alarm via MQTT
    StaticJsonDocument<256> doc;
    doc.createNestedObject("alarm")["value"] = alarmReason;
    
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    publishCallback(jsonBuffer);
    
    return true;
  } else {
    // Clear alarm if values return to normal
    if (alarmState.triggered) {
      alarmState.triggered = false;
      alarmTriggered = false;
      DEBUG_PRINTLN(F("[ALARM] Cleared - values normal"));
    }
  }
  
  return false;
}

/**
 * @brief Simple alarm check without MQTT publishing
 * @return true if alarm conditions are met
 */
inline bool checkAlarmsSimple() {
  if (!alarmEnabled) {
    return false;
  }
  
  unsigned long now = millis();
  
  // Check cooldown
  if (alarmState.triggered && 
      (now - alarmState.lastTriggerTime < alarmState.cooldownMs)) {
    return false;
  }
  
  bool shouldAlarm = false;
  
  if (sensorData.pm25 > alarmState.pm25Threshold) {
    shouldAlarm = true;
  }
  
  if (sensorData.pm10 > alarmState.pm10Threshold) {
    shouldAlarm = true;
  }
  
  if (shouldAlarm) {
    alarmState.triggered = true;
    alarmState.lastTriggerTime = now;
    alarmTriggered = true;
    visualAlarm(10);
    return true;
  } else if (alarmState.triggered) {
    alarmState.triggered = false;
    alarmTriggered = false;
  }
  
  return false;
}

/**
 * @brief Check if alarm is currently triggered
 * @return true if alarm is active
 */
inline bool isAlarmTriggered() {
  return alarmTriggered;
}

/**
 * @brief Get alarm state description
 * @return Human readable alarm status
 */
inline String getAlarmStatus() {
  if (!alarmEnabled) {
    return F("Disabled");
  }
  if (alarmTriggered) {
    return F("TRIGGERED");
  }
  return F("OK");
}

/**
 * @brief Get alarm configuration as JSON
 * @return JSON string with alarm settings
 */
inline String getAlarmConfigJson() {
  StaticJsonDocument<192> doc;
  doc["enabled"] = alarmEnabled;
  doc["triggered"] = alarmTriggered;
  doc["pm25Threshold"] = alarmState.pm25Threshold;
  doc["pm10Threshold"] = alarmState.pm10Threshold;
  doc["cooldownSec"] = alarmState.cooldownMs / 1000;
  
  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * @brief Initialize alarm system with default values
 */
inline void initAlarms() {
  alarmState.triggered = false;
  alarmState.lastTriggerTime = 0;
  alarmState.pm25Threshold = ALARM_PM25_THRESHOLD;
  alarmState.pm10Threshold = ALARM_PM10_THRESHOLD;
  alarmState.cooldownMs = ALARM_COOLDOWN;
  alarmEnabled = true;
  alarmTriggered = false;
  
  DEBUG_PRINTLN(F("[ALARM] System initialized"));
  DEBUG_PRINT(F("[ALARM] PM2.5 threshold: ")); DEBUG_PRINTLN(alarmState.pm25Threshold);
  DEBUG_PRINT(F("[ALARM] PM10 threshold: ")); DEBUG_PRINTLN(alarmState.pm10Threshold);
}

// ============================================================================
// AIR QUALITY ALERTS
// ============================================================================

/**
 * @brief Get air quality warning message
 * @param quality Current air quality level
 * @return Warning message or empty string
 */
inline String getAirQualityWarning(AirQuality quality) {
  switch (quality) {
    case AirQuality::POLLUTED:
      return F("⚠️ Air quality is poor. Consider staying indoors.");
    case AirQuality::VERY_POLLUTED:
      return F("🚨 Air quality is very poor! Avoid outdoor activities.");
    default:
      return "";
  }
}

/**
 * @brief Check if air quality requires health warning
 * @param quality Current air quality level
 * @return true if warning should be shown
 */
inline bool needsHealthWarning(AirQuality quality) {
  return quality == AirQuality::POLLUTED || quality == AirQuality::VERY_POLLUTED;
}

// ============================================================================
// THRESHOLD VALIDATION
// ============================================================================

/**
 * @brief Validate PM threshold value
 * @param threshold Threshold to validate
 * @return true if valid
 */
inline bool isValidPMThreshold(int threshold) {
  return threshold >= 1 && threshold <= 500;
}

/**
 * @brief Validate cooldown value
 * @param cooldownSec Cooldown in seconds
 * @return true if valid
 */
inline bool isValidCooldown(unsigned long cooldownSec) {
  return cooldownSec >= 60 && cooldownSec <= 86400;  // 1 minute to 24 hours
}

#endif // KLIMERKO_ALARMS_H
