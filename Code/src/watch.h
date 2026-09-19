#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <Preferences.h>
#include "config.h"

// Exactly the constructor from the known-good sketch: SSD1309, 128x64,
// hardware I2C, with OLED_RST_PIN handed to U8g2 so it drives RES# itself.
extern U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display;

extern RTC_DS3231 rtc;
extern Preferences prefs;
extern bool        rtcOk;

enum AppState : uint8_t {
  ST_FACE = 0,
  ST_NOTIF_LIST,
  ST_NOTIF_VIEW,
  ST_MENU,
  ST_FACEPICK,
  ST_SETTIME,
  ST_GAMEMENU,
  ST_PONG,
  ST_SNAKE,
  ST_DINO,
  ST_TT_DAYS,
  ST_TT_LIST,
  ST_WEATHER,
  ST_JOKES,
  ST_INFO
};

extern AppState gState;
extern uint8_t  gFace;
extern bool     gDirty;
extern const char* gResetReason;   // why the chip last restarted, shown in Info

void goState(AppState s);
void saveSettings();
DateTime nowSafe();
