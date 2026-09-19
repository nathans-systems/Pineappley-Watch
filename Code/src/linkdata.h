#pragma once
#include <Arduino.h>

// Weather pushed to the watch by Gadgetbridge over InfiniTime's weather
// service. Kept in RTC memory so the last reading survives deep sleep.

struct WeatherData {
  bool     valid;
  int16_t  tempC10;      // tenths of a degree C
  int16_t  minC10;
  int16_t  maxC10;
  uint8_t  icon;         // 0..8, see Link::iconName()
  char     place[33];     // the packet carries 32 bytes
  uint32_t epoch;        // RTC time when received, 0 if the RTC was unset
};

namespace Link {
  void begin();
  const WeatherData& weather();
  const char* iconName(uint8_t i);
  void onWeatherPacket(const uint8_t* d, size_t len);
}
