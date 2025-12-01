/**
 * @file types.h
 * @brief Klimerko Type Definitions - All structs, enums, and type aliases
 * @version 7.0 Ultimate
 * 
 * Centralized type definitions for consistent data structures across modules.
 */

#ifndef KLIMERKO_TYPES_H
#define KLIMERKO_TYPES_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * @brief Air Quality Index categories
 */
enum class AirQuality : uint8_t {
  EXCELLENT = 0,
  GOOD = 1,
  ACCEPTABLE = 2,
  POLLUTED = 3,
  VERY_POLLUTED = 4,
  UNKNOWN = 255
};

/**
 * @brief Sensor status codes
 */
enum class SensorStatus : uint8_t {
  OK = 0,
  INITIALIZING = 1,
  OFFLINE = 2,
  FAN_STUCK = 3,
  ZERO_DATA = 4,
  ERROR = 255
};

/**
 * @brief Device operating modes
 */
enum class DeviceMode : uint8_t {
  NORMAL = 0,
  CONFIG_PORTAL = 1,
  DEEP_SLEEP = 2,
  OTA_UPDATE = 3,
  FACTORY_RESET = 4
};

/**
 * @brief MQTT Asset identifiers
 */
enum class MqttAsset : uint8_t {
  // Particle measurements
  PM1,
  PM2_5,
  PM10,
  PM1_CORRECTED,
  PM2_5_CORRECTED,
  PM10_CORRECTED,
  
  // Particle counts
  COUNT_0_3,
  COUNT_0_5,
  COUNT_1_0,
  COUNT_2_5,
  COUNT_5_0,
  COUNT_10_0,
  
  // Environmental
  TEMPERATURE,
  HUMIDITY,
  PRESSURE,
  DEWPOINT,
  HUMIDITY_ABS,
  PRESSURE_SEA,
  HEAT_INDEX,
  ALTITUDE,
  
  // Device status
  AIR_QUALITY,
  SENSOR_STATUS,
  WIFI_SIGNAL,
  FIRMWARE,
  INTERVAL,
  
  // Configuration
  TEMP_OFFSET,
  ALTITUDE_SET,
  WIFI_CONFIG,
  RESTART_DEVICE,
  FIRMWARE_UPDATE,
  DEEP_SLEEP,
  ALARM_ENABLE,
  CALIBRATION,
  MQTT_BROKER,
  
  UNKNOWN
};

// ============================================================================
// STRUCTURES
// ============================================================================

/**
 * @brief Persistent device settings stored in EEPROM
 * @note CRC32 must be last field for validation
 */
struct Settings {
  char header[4];                         // "KLI" magic header
  char deviceId[DEVICE_ID_SIZE];          // AllThingsTalk Device ID
  char deviceToken[DEVICE_TOKEN_SIZE];    // AllThingsTalk Token
  char tempOffset[8];                     // Temperature offset as string
  char altitude[6];                       // Altitude in meters
  bool deepSleepEnabled;                  // Deep sleep mode flag
  char mqttBroker[MQTT_SERVER_SIZE];      // Custom MQTT broker
  uint16_t mqttBrokerPort;                // Custom MQTT port
  bool alarmEnabled;                      // Alarm system enabled
  int8_t gmtOffset;                       // GMT offset in hours
  float pm25CalFactor;                    // PM2.5 calibration factor
  float pm10CalFactor;                    // PM10 calibration factor
  uint32_t crc32;                         // CRC32 checksum (MUST be last)
};

/**
 * @brief Runtime statistics (persisted separately)
 */
struct Statistics {
  uint32_t bootCount;           // Number of device boots
  uint32_t wifiReconnects;      // WiFi reconnection attempts
  uint32_t mqttReconnects;      // MQTT reconnection attempts
  uint32_t successfulPublishes; // Successful MQTT publishes
  uint32_t failedPublishes;     // Failed MQTT publishes
  uint32_t uptimeSeconds;       // Total uptime (saved periodically)
};

/**
 * @brief Current sensor readings
 */
struct SensorData {
  // Particle measurements (raw)
  int pm1;
  int pm25;
  int pm10;
  
  // Particle measurements (averaged)
  int pm1Avg;
  int pm25Avg;
  int pm10Avg;
  
  // Particle measurements (humidity corrected)
  int pm1_corrected;
  int pm25_corrected;
  int pm10_corrected;
  
  // Particle counts (per 0.1L) - underscore naming for MQTT compatibility
  int count_0_3;
  int count_0_5;
  int count_1_0;
  int count_2_5;
  int count_5_0;
  int count_10_0;
  
  // Environmental
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  
  // Calculated values
  float dewpoint;
  float humidityAbs;
  float pressureSea;
  float heatIndex;
  
  // User configuration
  int userAltitude;
  
  // Status
  AirQuality airQuality;
  SensorStatus pmsStatus;
  SensorStatus bmeStatus;
};

/**
 * @brief Calibration factors
 */
struct Calibration {
  float pm25Factor;
  float pm10Factor;
  float tempOffset;
  float humOffset;
};

/**
 * @brief WiFi connection state
 */
struct WifiState {
  bool connected;
  bool connectionLost;
  unsigned long lastReconnectAttempt;
  unsigned long reconnectInterval;
  unsigned long configActiveSince;
  uint8_t failCount;
  uint8_t reconnectFailCount;
  int8_t rssi;
};

/**
 * @brief MQTT connection state
 */
struct MqttState {
  bool connected;
  bool connectionLost;
  unsigned long lastReconnectAttempt;
  uint32_t reconnectCount;
  char server[MQTT_SERVER_SIZE];
  uint16_t port;
};

/**
 * @brief Button state tracking
 */
struct ButtonState {
  unsigned long pressedTime;
  unsigned long releasedTime;
  bool pressed;
  bool longPressDetected;
  int lastState;
};

/**
 * @brief Alarm system state
 */
struct AlarmState {
  bool enabled;
  bool triggered;
  unsigned long lastTriggerTime;
  unsigned long cooldownMs;
  int pm25Threshold;
  int pm10Threshold;
  String reason;
};

// ============================================================================
// HELPER FUNCTIONS FOR ENUMS
// ============================================================================

/**
 * @brief Convert PM10 value to AirQuality enum
 */
inline AirQuality pmToAirQuality(int pm10) {
  if (pm10 <= AQI_EXCELLENT_MAX) return AirQuality::EXCELLENT;
  if (pm10 <= AQI_GOOD_MAX) return AirQuality::GOOD;
  if (pm10 <= AQI_ACCEPTABLE_MAX) return AirQuality::ACCEPTABLE;
  if (pm10 <= AQI_POLLUTED_MAX) return AirQuality::POLLUTED;
  return AirQuality::VERY_POLLUTED;
}

/**
 * @brief Convert AirQuality enum to string
 */
inline const char* airQualityToString(AirQuality aq) {
  switch (aq) {
    case AirQuality::EXCELLENT:    return "Excellent";
    case AirQuality::GOOD:         return "Good";
    case AirQuality::ACCEPTABLE:   return "Acceptable";
    case AirQuality::POLLUTED:     return "Polluted";
    case AirQuality::VERY_POLLUTED: return "Very Polluted";
    default:                       return "Unknown";
  }
}

/**
 * @brief Convert SensorStatus enum to string
 */
inline const char* sensorStatusToString(SensorStatus status) {
  switch (status) {
    case SensorStatus::OK:          return "OK";
    case SensorStatus::INITIALIZING: return "Initializing";
    case SensorStatus::OFFLINE:     return "Offline";
    case SensorStatus::FAN_STUCK:   return "Fan Stuck";
    case SensorStatus::ZERO_DATA:   return "Zero Data";
    default:                        return "Error";
  }
}

/**
 * @brief Get MQTT asset name string
 */
inline const char* assetToString(MqttAsset asset) {
  switch (asset) {
    case MqttAsset::PM1:            return "pm1";
    case MqttAsset::PM2_5:          return "pm2-5";
    case MqttAsset::PM10:           return "pm10";
    case MqttAsset::PM1_CORRECTED:  return "pm1-c";
    case MqttAsset::PM2_5_CORRECTED: return "pm2-5-c";
    case MqttAsset::PM10_CORRECTED: return "pm10-c";
    case MqttAsset::COUNT_0_3:      return "count-0-3";
    case MqttAsset::COUNT_0_5:      return "count-0-5";
    case MqttAsset::COUNT_1_0:      return "count-1-0";
    case MqttAsset::COUNT_2_5:      return "count-2-5";
    case MqttAsset::COUNT_5_0:      return "count-5-0";
    case MqttAsset::COUNT_10_0:     return "count-10-0";
    case MqttAsset::TEMPERATURE:    return "temperature";
    case MqttAsset::HUMIDITY:       return "humidity";
    case MqttAsset::PRESSURE:       return "pressure";
    case MqttAsset::DEWPOINT:       return "dewpoint";
    case MqttAsset::HUMIDITY_ABS:   return "humidityAbs";
    case MqttAsset::PRESSURE_SEA:   return "pressureSea";
    case MqttAsset::HEAT_INDEX:     return "HeatIndex";
    case MqttAsset::ALTITUDE:       return "altitude";
    case MqttAsset::AIR_QUALITY:    return "air-quality";
    case MqttAsset::SENSOR_STATUS:  return "sensor-status";
    case MqttAsset::WIFI_SIGNAL:    return "wifi-signal";
    case MqttAsset::FIRMWARE:       return "firmware";
    case MqttAsset::INTERVAL:       return "interval";
    case MqttAsset::TEMP_OFFSET:    return "temperature-offset";
    case MqttAsset::ALTITUDE_SET:   return "altitude-set";
    case MqttAsset::WIFI_CONFIG:    return "wifi-config";
    case MqttAsset::RESTART_DEVICE: return "restart-device";
    case MqttAsset::FIRMWARE_UPDATE: return "firmware-update";
    case MqttAsset::DEEP_SLEEP:     return "deep-sleep";
    case MqttAsset::ALARM_ENABLE:   return "alarm-enable";
    case MqttAsset::CALIBRATION:    return "calibration";
    case MqttAsset::MQTT_BROKER:    return "mqtt-broker";
    default:                        return "unknown";
  }
}

/**
 * @brief Parse asset name to enum
 */
inline MqttAsset stringToAsset(const String& name) {
  if (name == "pm1") return MqttAsset::PM1;
  if (name == "pm2-5") return MqttAsset::PM2_5;
  if (name == "pm10") return MqttAsset::PM10;
  if (name == "interval") return MqttAsset::INTERVAL;
  if (name == "temperature-offset") return MqttAsset::TEMP_OFFSET;
  if (name == "altitude-set") return MqttAsset::ALTITUDE_SET;
  if (name == "wifi-config") return MqttAsset::WIFI_CONFIG;
  if (name == "restart-device") return MqttAsset::RESTART_DEVICE;
  if (name == "firmware-update") return MqttAsset::FIRMWARE_UPDATE;
  if (name == "deep-sleep") return MqttAsset::DEEP_SLEEP;
  if (name == "alarm-enable") return MqttAsset::ALARM_ENABLE;
  if (name == "calibration") return MqttAsset::CALIBRATION;
  if (name == "mqtt-broker") return MqttAsset::MQTT_BROKER;
  return MqttAsset::UNKNOWN;
}

#endif // KLIMERKO_TYPES_H
