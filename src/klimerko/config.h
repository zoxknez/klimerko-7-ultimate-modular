/**
 * @file config.h
 * @brief Klimerko Configuration - All constants, pins, and compile-time settings
 * @version 7.0 Ultimate Modular
 * @date December 2025
 * 
 * @author Original Klimerko: Vanja Stanic
 * @author v7.0 Modular: o0o0o0o (https://github.com/zoxknez)
 * 
 * This file contains all configurable parameters for the Klimerko device.
 * Modify these values to customize behavior without changing logic code.
 */

#ifndef KLIMERKO_CONFIG_H
#define KLIMERKO_CONFIG_H

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
#define FIRMWARE_VERSION        "7.0 Ultimate"
#define FIRMWARE_VERSION_PORTAL "<p>Firmware: 7.0 Ultimate (mDNS+Dashboard+NTP+Alarms+Prometheus)</p>"

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#define DEBUG_ENABLED 1  // Set to 0 for production builds to save memory

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define PIN_BUTTON      0     // FLASH button (GPIO0)
#define PIN_PMS_TX      D5    // PMS7003 TX (to ESP RX)
#define PIN_PMS_RX      D6    // PMS7003 RX (from ESP TX)
#define PIN_I2C_SDA     D2    // BME280 SDA (default)
#define PIN_I2C_SCL     D1    // BME280 SCL (default)

// ============================================================================
// AIR QUALITY INDEX THRESHOLDS (EAQI Standard for PM10)
// ============================================================================
#define AQI_EXCELLENT_MAX   20    // 0-20 µg/m³
#define AQI_GOOD_MAX        40    // 21-40 µg/m³
#define AQI_ACCEPTABLE_MAX  50    // 41-50 µg/m³
#define AQI_POLLUTED_MAX    100   // 51-100 µg/m³
// Above 100 = Very Polluted

// ============================================================================
// ALARM THRESHOLDS (WHO 24-hour guidelines)
// ============================================================================
#define ALARM_PM25_THRESHOLD    35    // µg/m³
#define ALARM_PM10_THRESHOLD    45    // µg/m³
#define ALARM_COOLDOWN_MS       3600000UL  // 1 hour between alarms

// ============================================================================
// TIMING CONSTANTS (milliseconds unless noted)
// ============================================================================
// WiFi
#define WIFI_RECONNECT_BASE_MS      10000UL     // 10 seconds initial retry
#define WIFI_RECONNECT_MAX_MS       300000UL    // 5 minutes max backoff
#define WIFI_CONFIG_TIMEOUT_MS      1800000UL   // 30 minutes portal timeout

// MQTT
#define MQTT_RECONNECT_INTERVAL_MS  30000UL     // 30 seconds

// Button
#define BUTTON_SHORT_PRESS_MS       50UL
#define BUTTON_MEDIUM_PRESS_MS      1000UL      // 1 second
#define BUTTON_LONG_PRESS_MS        15000UL     // 15 seconds (factory reset)

// LED
#define LED_BLINK_INTERVAL_MS       1000UL

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
#define PMS_WAKE_BEFORE_SEC     30      // Wake PMS this many seconds before reading
#define SENSOR_AVG_SAMPLES      10      // Number of samples for moving average
#define SENSOR_RETRIES_OFFLINE  3       // Retries before marking sensor offline
#define FAN_STUCK_THRESHOLD     5       // Cycles with same value = stuck (5 * 5min = 25min)
#define ZERO_DATA_THRESHOLD     5       // Cycles with zero values

// BME280 I2C Addresses (try primary, then secondary)
#define BME280_ADDR_PRIMARY     0x76
#define BME280_ADDR_SECONDARY   0x77

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
#define MQTT_DEFAULT_SERVER     "api.allthingstalk.io"
#define MQTT_DEFAULT_PORT       1883
#define MQTT_PASSWORD           "arbitrary"
#define MQTT_MAX_MESSAGE_SIZE   4096
#define MQTT_CALLBACK_BUFFER    1023    // Leave room for null terminator
#define MQTT_KEEPALIVE_SEC      30

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================
#define WEB_SERVER_PORT         80
#define MAX_LOG_ENTRIES         100     // LittleFS log size limit
#define LOG_FILE_PATH           "/sensor_log.json"

// ============================================================================
// NTP CONFIGURATION
// ============================================================================
#define NTP_SERVER_1            "pool.ntp.org"
#define NTP_SERVER_2            "time.nist.gov"
#define NTP_GMT_OFFSET_SEC      3600    // UTC+1 (Central European Time)
#define NTP_DAYLIGHT_OFFSET     3600    // +1 hour for summer time

// ============================================================================
// DEEP SLEEP CONFIGURATION
// ============================================================================
#define DEEP_SLEEP_DEFAULT_US   300000000UL  // 5 minutes in microseconds

// ============================================================================
// PHYSICAL CONSTANTS
// ============================================================================
#define MAGNUS_BETA             17.62f
#define MAGNUS_GAMMA            243.12f
#define SEA_LEVEL_PRESSURE_HPA  1013.25f

// ============================================================================
// CALIBRATION DEFAULTS
// ============================================================================
#define DEFAULT_TEMP_OFFSET     -2.0f   // Default temperature correction
#define DEFAULT_PM_CAL_FACTOR   1.0f    // No correction by default
#define MIN_CAL_FACTOR          0.1f    // Minimum valid calibration factor
#define MAX_CAL_FACTOR          10.0f   // Maximum valid calibration factor

// ============================================================================
// BUFFER SIZES
// ============================================================================
#define DEVICE_ID_SIZE          32
#define DEVICE_TOKEN_SIZE       64
#define MQTT_SERVER_SIZE        64
#define TOPIC_BUFFER_SIZE       128
#define JSON_BUFFER_SMALL       256
#define JSON_BUFFER_MEDIUM      512
#define JSON_BUFFER_LARGE       2048

// ============================================================================
// COMPATIBILITY ALIASES (for modular code consistency)
// ============================================================================

// Pin aliases
#define PMS_TX_PIN              PIN_PMS_TX
#define PMS_RX_PIN              PIN_PMS_RX
#define BUTTON_PIN              PIN_BUTTON

// PMS7003 serial configuration
#define PMS_BAUD_RATE           9600

// BME280 I2C address aliases
#define BME_I2C_ADDR_PRIMARY    BME280_ADDR_PRIMARY
#define BME_I2C_ADDR_SECONDARY  BME280_ADDR_SECONDARY

// Timing aliases (remove _MS suffix for cleaner code)
#define WIFI_RECONNECT_BASE_INTERVAL    WIFI_RECONNECT_BASE_MS
#define WIFI_RECONNECT_MAX_INTERVAL     WIFI_RECONNECT_MAX_MS
#define WIFI_CONFIG_TIMEOUT             WIFI_CONFIG_TIMEOUT_MS
#define MQTT_RECONNECT_INTERVAL         MQTT_RECONNECT_INTERVAL_MS
#define LED_BLINK_INTERVAL              LED_BLINK_INTERVAL_MS
#define DEEP_SLEEP_DURATION_US          DEEP_SLEEP_DEFAULT_US
#define ALARM_COOLDOWN                  ALARM_COOLDOWN_MS

// NTP aliases
#define GMT_OFFSET_SEC                  NTP_GMT_OFFSET_SEC
#define DAYLIGHT_OFFSET_SEC             NTP_DAYLIGHT_OFFSET

// Sensor aliases
#define SENSOR_AVERAGE_SAMPLES          SENSOR_AVG_SAMPLES

// MQTT aliases
#define DEFAULT_MQTT_SERVER             MQTT_DEFAULT_SERVER
#define DEFAULT_MQTT_PORT               MQTT_DEFAULT_PORT
#define MQTT_CALLBACK_BUFFER_SIZE       MQTT_CALLBACK_BUFFER

// Temperature offset as string for WiFiManager
#define DEFAULT_TEMP_OFFSET_STR         "-2.00"

// Sensor validation ranges (for BME280)
#define TEMP_MIN_VALID                  -40.0f
#define TEMP_MAX_VALID                  85.0f
#define HUM_MIN_VALID                   0.0f
#define HUM_MAX_VALID                   100.0f

#endif // KLIMERKO_CONFIG_H
