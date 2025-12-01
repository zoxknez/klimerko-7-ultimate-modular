#include "Arduino.h"
#include "PMS.h"

PMS::PMS(Stream& stream)
{
  this->_stream = &stream;
}

void PMS::sleep()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
  _stream->write(command, sizeof(command));
}

void PMS::wakeUp()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
  _stream->write(command, sizeof(command));
}

void PMS::activeMode()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
  _stream->write(command, sizeof(command));
  _mode = MODE_ACTIVE;
}

void PMS::passiveMode()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
  _stream->write(command, sizeof(command));
  _mode = MODE_PASSIVE;
}

void PMS::requestRead()
{
  if (_mode == MODE_PASSIVE)
  {
    uint8_t command[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
    _stream->write(command, sizeof(command));
  }
}

bool PMS::read(DATA& data)
{
  _data = &data;
  _status = STATUS_WAIT;
  loop();
  return _status == STATUS_OK;
}

bool PMS::readUntil(DATA& data, uint16_t timeout)
{
  _data = &data;
  _status = STATUS_WAIT;
  uint32_t start = millis();
  do
  {
    loop();
    if (_status == STATUS_OK) break;
  } while (millis() - start < timeout);

  return _status == STATUS_OK;
}

void PMS::loop()
{
  // Timeout mehanizam za nekompletan frame
  // Ako je proslo vise od FRAME_TIMEOUT_MS od poslednjeg bajta, resetuj parser
  if (_index > 0 && (millis() - _lastByteTime > FRAME_TIMEOUT_MS))
  {
    _index = 0;
  }
  
  if (_stream->available())
  {
    uint8_t ch = _stream->read();
    _lastByteTime = millis();  // Azuriraj vreme poslednjeg bajta

    switch (_index)
    {
    case 0:
      if (ch != 0x42) { return; }
      _calculatedChecksum = ch;
      break;
    case 1:
      if (ch != 0x4D) { _index = 0; return; }
      _calculatedChecksum += ch;
      break;
    case 2:
      _calculatedChecksum += ch;
      _frameLen = ch << 8;
      break;
    case 3:
      _frameLen |= ch;
      _calculatedChecksum += ch;
      // Validacija duzine frame-a
      if (_frameLen != 28) { _index = 0; return; }
      break;
    default:
      // _frameLen je 28. Header je 4. Ukupno 32 bajta.
      // Index ide od 0 do 31.
      
      if (_index == _frameLen + 2 + 1) // Index 31 (poslednji bajt)
      {
        _checksum = (_payload[26] << 8) | ch; // ch je ovde Low byte checksuma

        if (_calculatedChecksum == _checksum)
        {
          _status = STATUS_OK; // Tek sad je uspeh!

          // Mapiranje podataka
          _data->PM_SP_UG_1_0 = makeWord(_payload[0], _payload[1]);
          _data->PM_SP_UG_2_5 = makeWord(_payload[2], _payload[3]);
          _data->PM_SP_UG_10_0 = makeWord(_payload[4], _payload[5]);

          _data->PM_AE_UG_1_0 = makeWord(_payload[6], _payload[7]);
          _data->PM_AE_UG_2_5 = makeWord(_payload[8], _payload[9]);
          _data->PM_AE_UG_10_0 = makeWord(_payload[10], _payload[11]);

          _data->PM_RAW_0_3 = makeWord(_payload[12], _payload[13]);
          _data->PM_RAW_0_5 = makeWord(_payload[14], _payload[15]);
          _data->PM_RAW_1_0 = makeWord(_payload[16], _payload[17]);
          _data->PM_RAW_2_5 = makeWord(_payload[18], _payload[19]);
          _data->PM_RAW_5_0 = makeWord(_payload[20], _payload[21]);
          _data->PM_RAW_10_0 = makeWord(_payload[22], _payload[23]);
        }
        _index = 0;
        return;
      }
      else
      {
        // Racunaj checksum za sve osim poslednja 2 bajta (koji su sam checksum)
        // Indexi checksuma su 30 i 31.
        if (_index < _frameLen + 2) {
           _calculatedChecksum += ch;
        }
        
        // Punimo payload buffer (od indexa 4 do 29)
        // Ali buffer mora da primi i prvu polovinu checksuma da bi logika gore radila
        // (linija: _checksum = (_payload[26] << 8)...)
        // Payload ima 28 mesta (0..27).
        // Index 4 -> Payload 0
        // Index 30 -> Payload 26.
        if ((_index - 4) < 28)
        {
          _payload[_index - 4] = ch;
        }
      }
      break;
    }
    _index++;
  }
}