/**
 * @file sensors.h
 * @brief Klimerko Sensor Management - PMS7003 and BME280
 * @version 7.0 Ultimate
 * 
 * Handles all sensor operations including reading, initialization,
 * and data processing for particle and environmental sensors.
 */

#ifndef KLIMERKO_SENSORS_H
#define KLIMERKO_SENSORS_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "../pmsLibrary/PMS.h"
#include "../AdafruitBME280/Adafruit_BME280.h"
#include "../movingAvg/movingAvg.h"

// ============================================================================
// GLOBAL SENSOR OBJECTS
// ============================================================================

extern SoftwareSerial pmsSerial;
extern PMS pms;
extern PMS::DATA pmsData;
extern Adafruit_BME280 bme;

extern movingAvg pm1Avg;
extern movingAvg pm25Avg;
extern movingAvg pm10Avg;
extern movingAvg tempAvg;
extern movingAvg humAvg;
extern movingAvg presAvg;

// ============================================================================
// SENSOR DATA STORAGE
// ============================================================================

extern SensorData sensorData;
extern Calibration calibration;
extern SensorStatus pmsStatus;
extern SensorStatus bmeStatus;

// Sensor state tracking
extern bool pmsSensorOnline;
extern bool bmeSensorOnline;
extern int pmsSensorRetry;
extern int bmeSensorRetry;
extern bool pmsNoSleep;
extern bool pmsWoken;

// Fan stuck detection
extern int prevPm1, prevPm25, prevPm10;
extern int stuckCounter, zeroCounter;
extern String sensorStatusText;

// ============================================================================
// SENSOR INITIALIZATION
// ============================================================================

/**
 * @brief Initialize PMS7003 particle sensor
 * 
 * Configures software serial and wakes sensor.
 */
inline void initPMS() {
  pmsSerial.begin(PMS_BAUD_RATE);
  // Wake up and set passive mode
  pms.wakeUp();
  pms.passiveMode();
  pmsWoken = true;
  DEBUG_PRINTLN(F("[PMS] Initialized"));
}

/**
 * @brief Initialize BME280 environmental sensor
 * 
 * Tries both I2C addresses (0x76 and 0x77).
 * @return true if sensor found
 */
inline bool initBME() {
  if (!bme.begin(BME_I2C_ADDR_PRIMARY)) {
    DEBUG_PRINTLN(F("[BME] Not found at 0x76, trying 0x77..."));
    if (!bme.begin(BME_I2C_ADDR_SECONDARY)) {
      DEBUG_PRINTLN(F("[BME] FATAL: Sensor not found!"));
      bmeSensorOnline = false;
      bmeStatus = SensorStatus::OFFLINE;
      return false;
    }
  }
  bmeSensorOnline = true;
  bmeStatus = SensorStatus::OK;
  DEBUG_PRINTLN(F("[BME] Initialized"));
  return true;
}

/**
 * @brief Initialize moving average filters
 */
inline void initAverages() {
  pm1Avg.begin();
  pm25Avg.begin();
  pm10Avg.begin();
  tempAvg.begin();
  humAvg.begin();
  presAvg.begin();
  DEBUG_PRINTLN(F("[AVG] Filters initialized"));
}

/**
 * @brief Initialize all sensors
 */
inline void initSensors() {
  initAverages();
  initPMS();
  initBME();
}

// ============================================================================
// PMS POWER CONTROL
// ============================================================================

/**
 * @brief Set PMS7003 power state
 * @param state true = wake, false = sleep
 */
inline void setPMSPower(bool state) {
  if (state) {
    pms.wakeUp();
    pms.passiveMode();
    pmsWoken = true;
    DEBUG_PRINTLN(F("[PMS] Woken up"));
  } else {
    pmsSerial.flush();
    delay(100);
    pmsWoken = false;
    pms.sleep();
    DEBUG_PRINTLN(F("[PMS] Sleeping"));
  }
}

// ============================================================================
// SENSOR READING
// ============================================================================

/**
 * @brief Read PMS7003 particle sensor data
 * 
 * Clears serial buffer, requests data, and updates averages.
 * Handles offline detection and recovery.
 */
inline void readPMSSensor() {
  // Clear serial buffer
  while (pmsSerial.available()) {
    pmsSerial.read();
  }
  
  pms.requestRead();
  
  if (pms.readUntil(pmsData)) {
    // Update averages with raw values
    sensorData.pm1 = pm1Avg.reading(pmsData.PM_AE_UG_1_0);
    sensorData.pm25 = pm25Avg.reading(pmsData.PM_AE_UG_2_5);
    sensorData.pm10 = pm10Avg.reading(pmsData.PM_AE_UG_10_0);
    
    // Store particle counts
    sensorData.count_0_3 = pmsData.PM_RAW_0_3;
    sensorData.count_0_5 = pmsData.PM_RAW_0_5;
    sensorData.count_1_0 = pmsData.PM_RAW_1_0;
    sensorData.count_2_5 = pmsData.PM_RAW_2_5;
    sensorData.count_5_0 = pmsData.PM_RAW_5_0;
    sensorData.count_10_0 = pmsData.PM_RAW_10_0;
    
    // Apply calibration factors
    if (calibration.pm25Factor != 1.0f) {
      sensorData.pm25 = (int)(sensorData.pm25 * calibration.pm25Factor);
    }
    if (calibration.pm10Factor != 1.0f) {
      sensorData.pm10 = (int)(sensorData.pm10 * calibration.pm10Factor);
    }
    
    // Determine air quality
    sensorData.airQuality = pmToAirQuality(sensorData.pm10);
    
    DEBUG_PRINTF("[PMS] PM1=%d PM2.5=%d PM10=%d AQ=%s\n", 
                 sensorData.pm1, sensorData.pm25, sensorData.pm10,
                 airQualityToString(sensorData.airQuality));
    
    pmsSensorRetry = 0;
    if (!pmsSensorOnline) {
      pmsSensorOnline = true;
      pmsStatus = SensorStatus::OK;
      DEBUG_PRINTLN(F("[PMS] Online!"));
    }
  } else {
    // No data received
    if (pmsSensorOnline) {
      DEBUG_PRINTLN(F("[PMS] No Data"));
      pmsSensorRetry++;
      if (pmsSensorRetry > SENSOR_RETRIES_OFFLINE) {
        pmsSensorOnline = false;
        pmsStatus = SensorStatus::OFFLINE;
        DEBUG_PRINTLN(F("[PMS] Offline!"));
        pm1Avg.reset();
        pm25Avg.reset();
        pm10Avg.reset();
        initPMS();
      }
    } else {
      initPMS();
    }
  }
}

/**
 * @brief Read BME280 environmental sensor data
 * 
 * Reads temperature, humidity, pressure with calibration offsets.
 * Calculates derived values: dewpoint, absolute humidity, heat index.
 */
inline void readBMESensor() {
  float temperatureRaw = bme.readTemperature();
  float temperature = temperatureRaw + calibration.tempOffset;
  float humidityRaw = bme.readHumidity();
  
  // Compensate humidity for temperature offset
  float humidity = humidityRaw * exp(MAGNUS_GAMMA * MAGNUS_BETA * 
                   (temperatureRaw - temperature) / 
                   (MAGNUS_GAMMA + temperatureRaw) / 
                   (MAGNUS_GAMMA + temperature));
  
  // Apply humidity calibration
  humidity += calibration.humOffset;
  
  float pressure = bme.readPressure() / 100.0f;  // Convert Pa to hPa
  
  DEBUG_PRINTF("[BME] Temp=%.1f Hum=%.1f Pres=%.1f\n", 
               temperature, humidity, pressure);
  
  // Validate readings
  if (temperatureRaw > TEMP_MIN_VALID && temperatureRaw < TEMP_MAX_VALID && 
      humidity >= HUM_MIN_VALID && humidity <= HUM_MAX_VALID) {
    
    // Clamp humidity to valid range
    humidity = clamp(humidity, 0.0f, 100.0f);
    
    // Update averages (multiply by 100 to preserve 2 decimal places)
    sensorData.temperature = tempAvg.reading((int)(temperature * 100)) / 100.0f;
    sensorData.humidity = humAvg.reading((int)(humidity * 100)) / 100.0f;
    sensorData.pressure = presAvg.reading((int)(pressure * 100)) / 100.0f;
    sensorData.altitude = bme.readAltitude(SEA_LEVEL_PRESSURE_HPA);
    
    // Calculate derived values
    sensorData.dewpoint = calculateDewpoint(sensorData.temperature, sensorData.humidity);
    sensorData.humidityAbs = calculateAbsoluteHumidity(sensorData.temperature, sensorData.humidity);
    sensorData.heatIndex = calculateHeatIndex(sensorData.temperature, sensorData.humidity);
    
    // Calculate sea level pressure if altitude is set
    if (sensorData.userAltitude > 0) {
      sensorData.pressureSea = calculateSeaLevelPressure(sensorData.pressure, 
                                                          sensorData.userAltitude);
    } else {
      sensorData.pressureSea = sensorData.pressure;
    }
    
    // Apply humidity correction to PM values
    sensorData.pm1_corrected = (int)applyEPAHumidityCorrection(
                                (float)sensorData.pm1, sensorData.humidity);
    sensorData.pm25_corrected = (int)applyEPAHumidityCorrection(
                                 (float)sensorData.pm25, sensorData.humidity);
    sensorData.pm10_corrected = (int)applyEPAHumidityCorrection(
                                 (float)sensorData.pm10, sensorData.humidity);
    
    bmeSensorRetry = 0;
    if (!bmeSensorOnline) {
      bmeSensorOnline = true;
      bmeStatus = SensorStatus::OK;
      DEBUG_PRINTLN(F("[BME] Online!"));
    }
  } else {
    // Invalid data
    if (bmeSensorOnline) {
      DEBUG_PRINTLN(F("[BME] Invalid Data"));
      bmeSensorRetry++;
      if (bmeSensorRetry > SENSOR_RETRIES_OFFLINE) {
        bmeSensorOnline = false;
        bmeStatus = SensorStatus::OFFLINE;
        DEBUG_PRINTLN(F("[BME] Offline!"));
        tempAvg.reset();
        humAvg.reset();
        presAvg.reset();
        initBME();
      }
    } else {
      initBME();
    }
  }
}

// ============================================================================
// FAN STUCK DETECTION
// ============================================================================

/**
 * @brief Check for PMS7003 fan stuck condition
 * 
 * Detects when PM values remain unchanged or zero for multiple readings.
 * @return Current sensor status
 */
inline SensorStatus checkFanStatus() {
  // Check for stuck values (same readings multiple times)
  if (sensorData.pm1 == prevPm1 && 
      sensorData.pm25 == prevPm25 && 
      sensorData.pm10 == prevPm10) {
    stuckCounter++;
  } else {
    stuckCounter = 0;
  }
  
  // Check for all-zero readings
  if (sensorData.pm1 == 0 && 
      sensorData.pm25 == 0 && 
      sensorData.pm10 == 0) {
    zeroCounter++;
  } else {
    zeroCounter = 0;
  }
  
  // Update previous values
  prevPm1 = sensorData.pm1;
  prevPm25 = sensorData.pm25;
  prevPm10 = sensorData.pm10;
  
  // Determine status
  if (stuckCounter >= FAN_STUCK_THRESHOLD) {
    sensorStatusText = F("Fan Stuck / Error");
    pmsStatus = SensorStatus::FAN_STUCK;
    return SensorStatus::FAN_STUCK;
  } else if (zeroCounter >= ZERO_DATA_THRESHOLD) {
    sensorStatusText = F("Zero Data Error");
    pmsStatus = SensorStatus::ZERO_DATA;
    return SensorStatus::ZERO_DATA;
  }
  
  sensorStatusText = F("OK");
  pmsStatus = SensorStatus::OK;
  return SensorStatus::OK;
}

// ============================================================================
// SENSOR LOOP
// ============================================================================

/**
 * @brief Calculate sensor read interval in milliseconds
 * @param publishIntervalMinutes Data publish interval in minutes
 * @return Interval in milliseconds between sensor reads
 */
inline unsigned long getReadIntervalMillis(uint8_t publishIntervalMinutes) {
  return ((unsigned long)publishIntervalMinutes * 60000UL) / SENSOR_AVERAGE_SAMPLES;
}

/**
 * @brief Main sensor reading loop
 * 
 * Handles sensor wake-up timing and read scheduling.
 * Should be called from main loop().
 * 
 * @param lastReadTime Reference to last read timestamp
 * @param publishInterval Data publish interval in minutes
 */
inline void sensorLoop(unsigned long& lastReadTime, uint8_t publishInterval) {
  unsigned long now = millis();
  unsigned long readInterval = getReadIntervalMillis(publishInterval);
  
  // Wake PMS sensor before reading
  if (now - lastReadTime >= readInterval - (PMS_WAKE_BEFORE_SEC * 1000) && 
      !pmsWoken && pmsSensorOnline && !pmsNoSleep) {
    DEBUG_PRINTLN(F("[PMS] Waking up before read"));
    setPMSPower(true);
  }
  
  // Read sensors
  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;
    
    DEBUG_PRINTLN(F("=== SENSOR READ ==="));
    readPMSSensor();
    readBMESensor();
    checkFanStatus();
    DEBUG_PRINTLN(F("=================="));
    
    // Sleep PMS if allowed
    if (!pmsNoSleep && pmsSensorOnline) {
      DEBUG_PRINTF("[PMS] Sleeping until %ds before next read\n", PMS_WAKE_BEFORE_SEC);
      setPMSPower(false);
    }
  }
}

/**
 * @brief Get current sensor data structure
 * @return Pointer to current sensor data
 */
inline SensorData* getSensorData() {
  return &sensorData;
}

/**
 * @brief Check if both sensors are online
 * @return true if all sensors operational
 */
inline bool allSensorsOnline() {
  return pmsSensorOnline && bmeSensorOnline;
}

/**
 * @brief Get combined sensor status string
 * @return Human readable status
 */
inline String getSensorStatusString() {
  if (!pmsSensorOnline && !bmeSensorOnline) {
    return F("All Sensors Offline");
  } else if (!pmsSensorOnline) {
    return F("PMS Offline");
  } else if (!bmeSensorOnline) {
    return F("BME Offline");
  } else if (pmsStatus == SensorStatus::FAN_STUCK) {
    return F("Fan Stuck");
  } else if (pmsStatus == SensorStatus::ZERO_DATA) {
    return F("Zero Data");
  }
  return F("OK");
}

#endif // KLIMERKO_SENSORS_H
