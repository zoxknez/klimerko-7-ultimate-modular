#ifndef PMS_H
#define PMS_H

#include <Stream.h>

class PMS
{
public:
  static const uint16_t SINGLE_RESPONSE_TIME = 1000;
  static const uint16_t TOTAL_RESPONSE_TIME = 1000 * 10;
  static const uint16_t STEADY_RESPONSE_TIME = 1000 * 30;

  static const uint16_t BAUD_RATE = 9600;

  struct DATA {
    // Standard Particles, CF=1
    uint16_t PM_SP_UG_1_0;
    uint16_t PM_SP_UG_2_5;
    uint16_t PM_SP_UG_10_0;

    // Atmospheric Environment
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;

    // Particle Counts
    uint16_t PM_RAW_0_3;
    uint16_t PM_RAW_0_5;
    uint16_t PM_RAW_1_0;
    uint16_t PM_RAW_2_5;
    uint16_t PM_RAW_5_0;
    uint16_t PM_RAW_10_0;
  };

  PMS(Stream& stream);
  void sleep();
  void wakeUp();
  void activeMode();
  void passiveMode();

  void requestRead();
  bool read(DATA& data);
  bool readUntil(DATA& data, uint16_t timeout = SINGLE_RESPONSE_TIME);

private:
  // Dodat STATUS_OK da znamo kad je uspesno
  enum STATUS { STATUS_WAIT, STATUS_OK };
  enum MODE { MODE_ACTIVE, MODE_PASSIVE };
  
  static const uint16_t FRAME_TIMEOUT_MS = 100;  // Timeout za nekompletan frame
  
  Stream* _stream;
  DATA* _data;
  STATUS _status;
  MODE _mode = MODE_ACTIVE;
  
  uint8_t _index = 0;
  uint16_t _frameLen;
  uint16_t _checksum;
  uint16_t _calculatedChecksum;
  uint8_t _payload[28]; // Buffer za podatke
  uint32_t _lastByteTime = 0;  // Vreme poslednjeg primljenog bajta

  void loop();
};

#endif