/**
 * @file utils.h
 * @brief Klimerko Utility Functions - CRC32, validation, formatting
 * @version 7.0 Ultimate
 * 
 * Common utility functions used across all modules.
 */

#ifndef KLIMERKO_UTILS_H
#define KLIMERKO_UTILS_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// ============================================================================
// CRC32 CALCULATION
// ============================================================================

/**
 * @brief Calculate CRC32 checksum for data integrity
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return CRC32 checksum value
 */
inline uint32_t calculateCRC32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  while (length--) {
    uint8_t c = *data++;
    for (uint8_t i = 0; i < 8; i++) {
      if ((crc ^ c) & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
      c >>= 1;
    }
  }
  return ~crc;
}

/**
 * @brief Calculate CRC32 for Settings struct (excluding CRC field)
 * @param settings Settings struct reference
 * @return CRC32 checksum
 */
inline uint32_t calculateSettingsCRC(const Settings& settings) {
  return calculateCRC32((const uint8_t*)&settings, sizeof(Settings) - sizeof(uint32_t));
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

/**
 * @brief Validate if string is a valid number
 * @param value String to validate
 * @return true if valid number
 */
inline bool isValidNumber(const char* value) {
  if (value == nullptr || value[0] == '\0') return false;
  
  size_t i = 0;
  // Allow leading sign
  if (value[0] == '-' || value[0] == '+') i++;
  
  bool hasDigit = false;
  bool hasDecimal = false;
  
  for (; value[i] != '\0'; i++) {
    if (isDigit(value[i])) {
      hasDigit = true;
    } else if (value[i] == '.' && !hasDecimal) {
      hasDecimal = true;
    } else {
      return false;
    }
  }
  
  return hasDigit;
}

/**
 * @brief Validate calibration factor is in acceptable range
 * @param factor Calibration factor to validate
 * @return true if valid
 */
inline bool isValidCalibrationFactor(float factor) {
  return factor >= MIN_CAL_FACTOR && factor <= MAX_CAL_FACTOR;
}

/**
 * @brief Clamp value to range
 * @param value Value to clamp
 * @param minVal Minimum allowed value
 * @param maxVal Maximum allowed value
 * @return Clamped value
 */
template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

// ============================================================================
// TIME FORMATTING
// ============================================================================

/**
 * @brief Format seconds to human readable uptime string
 * @param seconds Total seconds
 * @return Formatted string like "5d 12:34:56"
 */
inline String formatUptime(unsigned long seconds) {
  unsigned long days = seconds / 86400;
  seconds %= 86400;
  unsigned long hours = seconds / 3600;
  seconds %= 3600;
  unsigned long minutes = seconds / 60;
  seconds %= 60;
  
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%lud %02lu:%02lu:%02lu", 
           days, hours, minutes, seconds);
  return String(buffer);
}

/**
 * @brief Get uptime in seconds since boot
 * @param bootTime Timestamp when device booted (millis)
 * @return Uptime in seconds
 */
inline unsigned long getUptimeSeconds(unsigned long bootTime) {
  return (millis() - bootTime) / 1000;
}

// ============================================================================
// PHYSICAL CALCULATIONS
// ============================================================================

/**
 * @brief Calculate dewpoint temperature
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percent
 * @return Dewpoint temperature in Celsius
 */
inline float calculateDewpoint(float temperature, float humidity) {
  float gamma = (MAGNUS_BETA * temperature) / (MAGNUS_GAMMA + temperature) + log(humidity / 100.0f);
  return (MAGNUS_GAMMA * gamma) / (MAGNUS_BETA - gamma);
}

/**
 * @brief Calculate absolute humidity
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percent
 * @return Absolute humidity in g/m³
 */
inline float calculateAbsoluteHumidity(float temperature, float humidity) {
  return (6.112f * exp((MAGNUS_BETA * temperature) / (MAGNUS_GAMMA + temperature)) 
          * humidity * 2.1674f) / (273.15f + temperature);
}

/**
 * @brief Calculate sea level pressure from local pressure
 * @param pressure Local pressure in hPa
 * @param altitude Altitude in meters
 * @return Sea level pressure in hPa
 */
inline float calculateSeaLevelPressure(float pressure, int altitude) {
  return pressure / pow((1 - ((float)altitude) / 44330.0f), 5.255f);
}

/**
 * @brief Apply EPA humidity correction to PM values
 * @param pmValue Raw PM value
 * @param humidity Relative humidity in percent
 * @return Corrected PM value
 * 
 * Reference: https://www.epa.gov/air-sensor-toolbox
 */
inline float applyEPAHumidityCorrection(float pmValue, float humidity) {
  float factor;
  
  if (humidity <= 30.0f) {
    factor = 1.0f;  // No correction
  } else if (humidity <= 50.0f) {
    factor = 1.0f + 0.005f * (humidity - 30.0f);
  } else if (humidity <= 70.0f) {
    factor = 1.1f + 0.01f * (humidity - 50.0f);
  } else if (humidity <= 90.0f) {
    factor = 1.3f + 0.02f * (humidity - 70.0f);
  } else {
    factor = 1.7f + 0.03f * (humidity - 90.0f);
  }
  
  return pmValue / factor;
}

/**
 * @brief Calculate Heat Index with smooth transition
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity in percent
 * @return Heat Index in Celsius
 * 
 * Uses Rothfusz regression with smooth transition around 26.7°C
 */
inline float calculateHeatIndex(float temperature, float humidity) {
  const float THRESHOLD = 26.7f;
  const float TRANSITION_START = 20.0f;
  
  if (temperature < TRANSITION_START) {
    return temperature;
  }
  
  // Rothfusz regression coefficients
  const double c1 = -8.78469475556;
  const double c2 = 1.61139411;
  const double c3 = 2.33854883889;
  const double c4 = -0.14611605;
  const double c5 = -0.012308094;
  const double c6 = -0.0164248277778;
  const double c7 = 0.002211732;
  const double c8 = 0.00072546;
  const double c9 = -0.000003582;
  
  double T = (double)temperature;
  double R = (double)humidity;
  double A = ((c5 * T) + c2) * T + c1;
  double B = ((c7 * T) + c4) * T + c3;
  double C = ((c9 * T) + c8) * T + c6;
  float fullHeatIndex = (float)((C * R + B) * R + A);
  
  if (temperature >= THRESHOLD) {
    return fullHeatIndex;
  }
  
  // Smooth transition in 20-26.7°C range
  float blend = (temperature - TRANSITION_START) / (THRESHOLD - TRANSITION_START);
  return temperature * (1.0f - blend) + fullHeatIndex * blend;
}

// ============================================================================
// STRING UTILITIES
// ============================================================================

/**
 * @brief Safe string copy with null termination
 * @param dest Destination buffer
 * @param src Source string
 * @param maxLen Maximum length including null terminator
 */
inline void safeStrCopy(char* dest, const char* src, size_t maxLen) {
  strncpy(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}

/**
 * @brief Generate unique identifier from ChipID
 * @param buffer Destination buffer
 * @param bufferSize Buffer size
 * @param prefix Prefix string (e.g., "KLIMERKO-", "K", "O")
 */
inline void generateChipIdString(char* buffer, size_t bufferSize, const char* prefix) {
  snprintf(buffer, bufferSize, "%s%08X", prefix, ESP.getChipId());
}

/**
 * @brief Build MQTT topic string
 * @param buffer Destination buffer
 * @param bufferSize Buffer size
 * @param deviceId Device ID
 * @param suffix Topic suffix (e.g., "state", "asset/+/command")
 */
inline void buildMqttTopic(char* buffer, size_t bufferSize, 
                           const char* deviceId, const char* suffix) {
  snprintf(buffer, bufferSize, "device/%s/%s", deviceId, suffix);
}

/**
 * @brief Extract asset name from MQTT topic
 * @param topic Full topic string
 * @return Asset name or empty string
 * 
 * Topic format: device/{deviceId}/asset/{assetName}/command
 */
inline String extractAssetFromTopic(const String& topic) {
  int assetStart = topic.indexOf("/asset/") + 7;
  int assetEnd = topic.lastIndexOf("/command");
  
  if (assetStart > 6 && assetEnd > assetStart) {
    return topic.substring(assetStart, assetEnd);
  }
  return "";
}

// ============================================================================
// MEDIAN FILTER CLASS
// ============================================================================

/**
 * @brief Maximum filter size for stack allocation
 * 
 * ESP8266 has limited stack (~4KB), so we limit median filter size.
 * This avoids heap fragmentation from repeated new/delete.
 */
#define MEDIAN_FILTER_MAX_SIZE 16

/**
 * @brief Median filter for robust outlier rejection
 * 
 * More robust than moving average against sensor spikes.
 * Uses fixed-size arrays to avoid heap fragmentation on ESP8266.
 */
class MedianFilter {
private:
  int _values[MEDIAN_FILTER_MAX_SIZE];
  int _sortBuffer[MEDIAN_FILTER_MAX_SIZE];
  int _size;
  int _index;
  bool _filled;
  
public:
  /**
   * @brief Construct median filter with specified window size
   * @param filterSize Number of samples in filter window (max 16)
   */
  MedianFilter(int filterSize) : _size(min(filterSize, MEDIAN_FILTER_MAX_SIZE)), _index(0), _filled(false) {
    for (int i = 0; i < MEDIAN_FILTER_MAX_SIZE; i++) {
      _values[i] = 0;
      _sortBuffer[i] = 0;
    }
  }
  
  /**
   * @brief Add reading and get filtered value
   * @param value New reading
   * @return Median of current window
   */
  int reading(int value) {
    _values[_index] = value;
    _index = (_index + 1) % _size;
    if (_index == 0) _filled = true;
    
    int count = _filled ? _size : _index;
    if (count == 0) return value;
    
    // Copy to sort buffer (no heap allocation!)
    for (int i = 0; i < count; i++) {
      _sortBuffer[i] = _values[i];
    }
    
    // Simple bubble sort (efficient for small arrays)
    for (int i = 0; i < count - 1; i++) {
      for (int j = 0; j < count - i - 1; j++) {
        if (_sortBuffer[j] > _sortBuffer[j + 1]) {
          int temp = _sortBuffer[j];
          _sortBuffer[j] = _sortBuffer[j + 1];
          _sortBuffer[j + 1] = temp;
        }
      }
    }
    
    return _sortBuffer[count / 2];
  }
  
  /**
   * @brief Reset filter state
   */
  void reset() {
    _index = 0;
    _filled = false;
    for (int i = 0; i < _size; i++) {
      _values[i] = 0;
    }
  }
};

#endif // KLIMERKO_UTILS_H
