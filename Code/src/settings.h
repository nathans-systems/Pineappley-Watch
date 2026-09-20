#pragma once
#include <Arduino.h>

// Display and power options the user can change on the watch. Persisted in NVS
// so they survive sleep and reflashing keeps them too.

struct WatchSettings {
  uint8_t  contrast;     // 0-255
  uint8_t  invert;       // 0/1
  uint8_t  flip;         // 0/1, rotate 180 for wearing it the other way round
  uint16_t standbyS;     // seconds idle before the screen goes dark
  uint16_t sleepS;       // seconds idle before deep sleep, 0 = never
  uint8_t  flashMode;    // 1 = never deep sleep, so the USB port stays up
  uint8_t  magic;
};

namespace Settings {
  void begin();                 // load from NVS, fall back to config.h defaults
  void save();
  void applyDisplay();          // push contrast / invert / flip to the panel
  WatchSettings& get();
  uint32_t standbyMs();
  uint32_t sleepMs();
}
