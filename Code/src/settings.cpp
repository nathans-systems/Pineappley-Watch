#include "settings.h"
#include "watch.h"

#define SETTINGS_MAGIC 0x5B   // bumped: flashMode added

static WatchSettings s;

WatchSettings& Settings::get() { return s; }

void Settings::begin() {
  size_t got = prefs.getBytes("disp", &s, sizeof(s));
  if (got != sizeof(s) || s.magic != SETTINGS_MAGIC) {
    s.contrast = OLED_CONTRAST;
    s.invert   = 0;
    s.flip     = 0;
    s.standbyS = (uint16_t)(IDLE_SCREEN_OFF_MS / 1000UL);
    s.sleepS   = (uint16_t)(DEEP_SLEEP_AFTER_MS / 1000UL);
    s.flashMode = 0;
    s.magic    = SETTINGS_MAGIC;
  }
  if (s.standbyS < 5)   s.standbyS = 5;
  if (s.sleepS && s.sleepS < s.standbyS) s.sleepS = s.standbyS;
}

void Settings::save() { prefs.putBytes("disp", &s, sizeof(s)); }

void Settings::applyDisplay() {
  display.setContrast(s.contrast);
  display.sendF("c", s.invert ? 0xA7 : 0xA6);        // SSD1309 inverse on/off
  display.setDisplayRotation(s.flip ? U8G2_R2 : U8G2_R0);
}

uint32_t Settings::standbyMs() { return (uint32_t)s.standbyS * 1000UL; }
uint32_t Settings::sleepMs()   { return (uint32_t)s.sleepS   * 1000UL; }
