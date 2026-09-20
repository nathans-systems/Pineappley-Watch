#include "screens.h"
#include "ui.h"
#include "faces.h"
#include "games.h"
#include "notifications.h"
#include "linkdata.h"
#include "calendar.h"
#include "settings.h"
#include <stdarg.h>

// ===========================================================================
// Timetable data
// ===========================================================================
static const char* TT_DAY_NAMES[5] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday" };

static const char* TT_MON[] = { "1 DCG", "2 Physics", "3 Comp Sci", "4 Lunch", "5 Irish", "6 English", "7 SCRE" };
static const char* TT_TUE[] = { "1 Maths", "2 DCG", "3 SCRE", "4 Lunch", "5 Irish", "6 Engineering", "7 Physics" };
static const char* TT_WED[] = { "1 English", "2 Engineering", "3 PE", "4 Lunch", "5 Irish", "6 Maths", "7 DCG" };
static const char* TT_THU[] = { "1 English", "2 Maths", "3 Lunch", "4 Comp Sci", "5 SPHE", "6 Engineering", "7 Physics" };
static const char* TT_FRI[] = { "1 Comp Sci", "2 Maths", "3 Lunch", "4 Irish", "5 English" };

static const char* const* TT_DATA[5] = { TT_MON, TT_TUE, TT_WED, TT_THU, TT_FRI };
static const uint8_t      TT_LEN [5] = { 7, 7, 7, 7, 5 };

// ===========================================================================
// Cursor state
// ===========================================================================
static uint8_t menuSel = 0,  menuTop = 0;
static uint8_t gameSel = 0,  gameTop = 0;
static uint8_t ttSel   = 0,  ttTop   = 0;
static uint8_t ttDay   = 0,  ttListTop = 0;
static uint8_t notifSel = 0, notifTop = 0;
static uint8_t faceSel  = 0, faceTop  = 0;
static uint8_t viewScroll = 0;
static uint8_t stField  = 0;
static int     stV[5];                        // hh mm dd mo yyyy
static uint32_t lastFaceDraw = 0;

enum { M_BACK = 0, M_NOTIF, M_WEATHER, M_CAL, M_FACE, M_SW, M_GAMES, M_JOKE,
       M_TT, M_TIME, M_DISP, M_INFO, M_FLASH, M_COUNT };
static const char* MENU_ITEMS[M_COUNT] = {
  "< Watch face", "Notifications", "Weather", "Calendar", "Watch faces",
  "Stopwatch", "Games", "Jokes", "Timetable", "Set time", "Display", "Info",
  "Flash mode"   // label is replaced at draw time with the current state
};
static const char* GAME_ITEMS[7] = { "< Back", "Pong", "Snake", "Dino Run",
                                    "Breakout", "Jet", "Simon" };

// ===========================================================================
// Watch face
// ===========================================================================
static void drawFaceNow() {
  DateTime n = nowSafe();
  display.clearBuffer();
  display.setDrawColor(1);
  FACES[gFace].draw(n);
  display.sendBuffer();
  lastFaceDraw = millis();
}

static void tickFace(BtnEvent ev) {
  if (ev == EV_UP) {                       // B0 scrolls up through faces
    gFace = (uint8_t)((gFace + 1) % FACE_COUNT);
    saveSettings();
    gDirty = true;
  } else if (ev == EV_DOWN) {              // B1 scrolls down into the options menu
    goState(ST_MENU);
    return;
  } else if (ev == EV_SELECT) {
    gDirty = true;
  }

  if (gDirty || (millis() - lastFaceDraw) >= FACES[gFace].frameMs) {
    gDirty = false;
    drawFaceNow();
  }
}

// ===========================================================================
// Notifications
// ===========================================================================
static void drawNotifList() {
  char title[24];
  snprintf(title, sizeof(title), "Notifications %d", Notifs::count());
  uiListFrame(title);

  if (Notifs::count() == 0) {
    display.setFont(F_BODY);
    display.setDrawColor(1);
    display.drawStr(6, 26, Notifs::connected() ? "Phone linked." : "No phone link.");
    display.drawStr(6, 38, "Nothing new.");
    display.setFont(F_TINY);
    display.drawStr(6, 56, "B0 or B1 to go back");
    display.sendBuffer();
    return;
  }

  display.setFont(F_BODY);
  for (uint8_t r = 0; r < 4; r++) {
    uint8_t i = (uint8_t)(notifTop + r);
    if (i >= Notifs::count()) break;
    const Notif& nf = Notifs::get(i);
    int16_t yt = (int16_t)(12 + r * 13);
    int16_t tx = uiRowSelect(yt, i == notifSel);
    char line[26];
    snprintf(line, sizeof(line), "%-4.4s %.12s", Notifs::categoryName(nf.cat), nf.title);
    display.drawStr(tx, yt + 9, line);
    display.setDrawColor(1);
  }
  display.sendBuffer();
}

static void tickNotifList(BtnEvent ev) {
  uint8_t n = Notifs::count();
  if (ev == EV_UP) {
    if (n == 0 || notifSel == 0) { goState(ST_MENU); return; }
    notifSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    if (n == 0 || notifSel + 1 >= n) { goState(ST_MENU); return; }
    navMark(notifSel);
    notifSel++; gDirty = true;
  } else if (ev == EV_SELECT) {
    if (n > 0) { goState(ST_NOTIF_VIEW); return; }
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(n ? n : 1, notifSel, notifTop, 4);
  if (gDirty) { gDirty = false; drawNotifList(); }
}

static void drawNotifView() {
  const Notif& nf = Notifs::get(notifSel);
  uiListFrame(Notifs::categoryName(nf.cat));
  display.setFont(F_BODY);
  display.setDrawColor(1);
  display.drawStr(2, 21, nf.title);
  display.drawHLine(0, 24, 128);
  uiWrapText(nf.body, 2, 26, 124, 9, 4, viewScroll);
  display.sendBuffer();
}

static void tickNotifView(BtnEvent ev) {
  if (ev == EV_UP) {
    if (viewScroll == 0) { goState(ST_NOTIF_LIST); return; }
    viewScroll--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(viewScroll);
    if (viewScroll < 8) viewScroll++;
    gDirty = true;
  } else if (ev == EV_SELECT || ev == EV_EXIT) {
    goState(ST_NOTIF_LIST); return;
  }
  if (gDirty) { gDirty = false; drawNotifView(); }
}

// ===========================================================================
// Options menu
// ===========================================================================
static void drawMenu() {
  // one row shows live state, so the labels are assembled per draw
  static char flashLbl[26];
  snprintf(flashLbl, sizeof(flashLbl), "Flash mode  %s",
           Settings::get().flashMode ? "ON" : "off");
  const char* items[M_COUNT];
  for (uint8_t i = 0; i < M_COUNT; i++) items[i] = MENU_ITEMS[i];
  items[M_FLASH] = flashLbl;

  uiListFrame("Options");
  uiList(items, M_COUNT, menuSel, menuTop, 4);
  display.sendBuffer();
}

static void tickMenu(BtnEvent ev) {
  if (ev == EV_UP) {
    if (menuSel == 0) { goState(ST_FACE); return; }    // B0 on the top item exits
    menuSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(menuSel);
    if (menuSel + 1 < M_COUNT) menuSel++;
    gDirty = true;
  } else if (ev == EV_EXIT) {
    goState(ST_FACE); return;
  } else if (ev == EV_SELECT) {
    switch (menuSel) {
      case M_BACK:    goState(ST_FACE);       return;
      case M_NOTIF:   goState(ST_NOTIF_LIST); return;
      case M_WEATHER: goState(ST_WEATHER);    return;
      case M_FACE:    goState(ST_FACEPICK);   return;
      case M_CAL:     goState(ST_CAL_LIST);   return;
      case M_DISP:    goState(ST_DISPLAY);    return;
      case M_SW:      goState(ST_STOPWATCH);  return;
      case M_GAMES:   goState(ST_GAMEMENU);   return;
      case M_JOKE:    goState(ST_JOKES);      return;
      case M_TT:      goState(ST_TT_DAYS);    return;
      case M_TIME:    goState(ST_SETTIME);    return;
      case M_INFO:    goState(ST_INFO);       return;
      case M_FLASH:
        // Deep sleep drops the USB CDC port, which is what makes uploading
        // awkward. This just stops it happening. Stays in the menu so you can
        // see it flip.
        Settings::get().flashMode = !Settings::get().flashMode;
        Settings::save();
        gDirty = true;
        return;
    }
  }
  uiScrollClamp(M_COUNT, menuSel, menuTop, 4);
  if (gDirty) { gDirty = false; drawMenu(); }
}

// ===========================================================================
// Face picker
// ===========================================================================
static void drawFacePick() {
  uiListFrame("Watch faces");
  display.setFont(F_BODY);
  for (uint8_t r = 0; r < 4; r++) {
    uint8_t i = (uint8_t)(faceTop + r);
    if (i >= FACE_COUNT) break;
    int16_t yt = (int16_t)(12 + r * 13);
    int16_t tx = uiRowSelect(yt, i == faceSel);
    display.drawStr(tx, yt + 9, FACES[i].name);
    if (i == gFace) display.drawStr(104, yt + 9, "*");
    display.setDrawColor(1);
  }
  display.sendBuffer();
}

static void tickFacePick(BtnEvent ev) {
  if (ev == EV_UP) {
    if (faceSel == 0) { goState(ST_MENU); return; }
    faceSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(faceSel);
    if (faceSel + 1 < FACE_COUNT) faceSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    gFace = faceSel;
    saveSettings();
    goState(ST_FACE);
    return;
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(FACE_COUNT, faceSel, faceTop, 4);
  if (gDirty) { gDirty = false; drawFacePick(); }
}

// ===========================================================================
// Set time
// ===========================================================================
static const char* ST_LABEL[5] = { "Hour", "Min", "Day", "Month", "Year" };
static const int   ST_MIN[5]   = { 0, 0, 1, 1, 2024 };
static const int   ST_MAX[5]   = { 23, 59, 31, 12, 2099 };

static void drawSetTime() {
  uiListFrame("Set time");
  char buf[28];
  display.setDrawColor(1);

  snprintf(buf, sizeof(buf), "%02d:%02d", stV[0], stV[1]);
  display.setFont(F_LARGE);
  uiCentre(buf, 64, 26);

  snprintf(buf, sizeof(buf), "%02d/%02d/%04d", stV[2], stV[3], stV[4]);
  display.setFont(F_BODY);
  uiCentre(buf, 64, 44);

  display.drawHLine(0, 52, 128);
  snprintf(buf, sizeof(buf), "%s  B0 +  B1 next", ST_LABEL[stField]);
  display.setFont(F_TINY);
  display.drawStr(2, 61, buf);
  display.sendBuffer();
}

static void tickSetTime(BtnEvent ev) {
  if (ev == EV_UP) {
    stV[stField]++;
    if (stV[stField] > ST_MAX[stField]) stV[stField] = ST_MIN[stField];
    gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(stField);
    stField = (uint8_t)((stField + 1) % 5);
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (rtcOk) rtc.adjust(DateTime((uint16_t)stV[4], (uint8_t)stV[3], (uint8_t)stV[2],
                                   (uint8_t)stV[0], (uint8_t)stV[1], 0));
    goState(ST_FACE);
    return;
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  if (gDirty) { gDirty = false; drawSetTime(); }
}

// ===========================================================================
// Timetable
// ===========================================================================
static void drawTtDays() {
  uiListFrame("Timetable");
  const char* list[6] = { "< Back", TT_DAY_NAMES[0], TT_DAY_NAMES[1], TT_DAY_NAMES[2],
                          TT_DAY_NAMES[3], TT_DAY_NAMES[4] };
  uiList(list, 6, ttSel, ttTop, 4);
  display.sendBuffer();
}

static void tickTtDays(BtnEvent ev) {
  if (ev == EV_UP) {
    if (ttSel == 0) { goState(ST_MENU); return; }
    ttSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(ttSel);
    if (ttSel + 1 < 6) ttSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (ttSel == 0) { goState(ST_MENU); return; }
    ttDay = (uint8_t)(ttSel - 1);
    goState(ST_TT_LIST);
    return;
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(6, ttSel, ttTop, 4);
  if (gDirty) { gDirty = false; drawTtDays(); }
}

static void drawTtList() {
  uiListFrame(TT_DAY_NAMES[ttDay]);
  display.setFont(F_BODY);
  display.setDrawColor(1);
  uint8_t n = TT_LEN[ttDay];
  for (uint8_t r = 0; r < 5; r++) {
    uint8_t i = (uint8_t)(ttListTop + r);
    if (i >= n) break;
    display.drawStr(4, 21 + r * 10, TT_DATA[ttDay][i]);
  }
  if (n > 5) {
    display.setFont(F_TINY);
    display.drawStr(100, 62, "B1 v");
  }
  display.sendBuffer();
}

static void tickTtList(BtnEvent ev) {
  uint8_t n = TT_LEN[ttDay];
  if (ev == EV_UP) {
    if (ttListTop == 0) { goState(ST_TT_DAYS); return; }
    ttListTop--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(ttListTop);
    if (n > 5 && ttListTop + 5 < n) ttListTop++;
    gDirty = true;
  } else if (ev == EV_SELECT || ev == EV_EXIT) {
    goState(ST_TT_DAYS); return;
  }
  if (gDirty) { gDirty = false; drawTtList(); }
}

// ===========================================================================
// Weather
// ===========================================================================
// Age from the RTC, so it stays correct across a deep sleep (millis() resets).
static void ageString(uint32_t epoch, char* out, size_t n) {
  if (!epoch || !rtcOk) { snprintf(out, n, "--"); return; }
  uint32_t now = nowSafe().unixtime();
  if (now < epoch) { snprintf(out, n, "just now"); return; }
  uint32_t secs = now - epoch;
  if      (secs < 90)    snprintf(out, n, "%lus ago", (unsigned long)secs);
  else if (secs < 5400)  snprintf(out, n, "%lum ago", (unsigned long)(secs / 60));
  else if (secs < 172800)snprintf(out, n, "%luh ago", (unsigned long)(secs / 3600));
  else                   snprintf(out, n, "stale");
}

static void drawWeather() {
  uiListFrame("Weather");
  const WeatherData& w = Link::weather();
  display.setDrawColor(1);

  if (!w.valid) {
    display.setFont(F_BODY);
    display.drawStr(4, 26, "No weather yet.");
    display.setFont(F_TINY);
    display.drawStr(4, 42, "Set a weather provider in");
    display.drawStr(4, 51, "Gadgetbridge. It sends on");
    display.drawStr(4, 60, "connect and on update.");
    display.sendBuffer();
    return;
  }

  char t[12];
  snprintf(t, sizeof(t), "%.1f", w.tempC10 / 10.0f);
  display.setFont(F_LARGE);
  uiLeft(t, 4, 26);
  display.setFont(F_BODY);
  display.drawStr(4 + display.getStrWidth(t) + 2, 20, "C");

  char l[28];
  display.setFont(F_TINY);
  snprintf(l, sizeof(l), "%s", Link::iconName(w.icon));
  uiRight(l, 124, 20);
  snprintf(l, sizeof(l), "%.0f / %.0f C", w.minC10 / 10.0f, w.maxC10 / 10.0f);
  uiRight(l, 124, 32);

  display.drawHLine(0, 42, 128);
  display.setFont(F_TINY);
  char place[26];
  snprintf(place, sizeof(place), "%.25s", w.place[0] ? w.place : "(no place)");
  display.drawStr(4, 52, place);
  ageString(w.epoch, l, sizeof(l));
  uiRight(l, 124, 58);
  display.sendBuffer();
}

static void tickWeather(BtnEvent ev) {
  if (ev != EV_NONE) { goState(ST_MENU); return; }
  static uint32_t last = 0;
  if (gDirty || millis() - last > 2000) { gDirty = false; last = millis(); drawWeather(); }
}

// ===========================================================================
// Calendar
// ===========================================================================
static uint8_t calSel = 0, calTop = 0, calEditIdx = 0, calField = 0;
static const char* CAL_MONTHS[12] = { "Jan","Feb","Mar","Apr","May","Jun",
                                      "Jul","Aug","Sep","Oct","Nov","Dec" };

static void drawCalList() {
  uiListFrame("Calendar");
  display.setFont(F_BODY);
  uint8_t n = Cal::count();
  uint8_t rows = (uint8_t)(n + 2);          // back + events + add

  for (uint8_t r = 0; r < 4; r++) {
    uint8_t i = (uint8_t)(calTop + r);
    if (i >= rows) break;
    int16_t yt = (int16_t)(12 + r * 13);
    int16_t tx = uiRowSelect(yt, i == calSel);
    char line[26];
    if (i == 0) {
      snprintf(line, sizeof(line), "< Back");
    } else if (i == rows - 1) {
      snprintf(line, sizeof(line), n < CAL_MAX ? "+ Add event" : "(full)");
    } else {
      CalEvent* e = Cal::at((uint8_t)(i - 1));
      int16_t d = Cal::daysUntil(*e);
      if (d == 0) snprintf(line, sizeof(line), "%.9s TODAY", Cal::name(e->nameIdx));
      else        snprintf(line, sizeof(line), "%.9s %dd", Cal::name(e->nameIdx), d);
    }
    display.drawStr(tx, yt + 9, line);
    display.setDrawColor(1);
  }
  display.sendBuffer();
}

static void tickCalList(BtnEvent ev) {
  uint8_t n = Cal::count();
  uint8_t rows = (uint8_t)(n + 2);
  if (ev == EV_UP) {
    if (calSel == 0) { goState(ST_MENU); return; }
    calSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(calSel);
    if (calSel + 1 < rows) calSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (calSel == 0) { goState(ST_MENU); return; }
    if (calSel == rows - 1) {
      if (Cal::add()) { calEditIdx = (uint8_t)(Cal::count() - 1); calField = 0;
                        goState(ST_CAL_EDIT); }
      gDirty = true;
      return;
    }
    calEditIdx = (uint8_t)(calSel - 1);
    calField = 0;
    goState(ST_CAL_EDIT);
    return;
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(rows, calSel, calTop, 4);
  if (gDirty) { gDirty = false; drawCalList(); }
}

static void drawCalEdit() {
  CalEvent* e = Cal::at(calEditIdx);
  if (!e) { goState(ST_CAL_LIST); return; }
  uiListFrame("Edit event");
  display.setDrawColor(1);

  char l[28];
  display.setFont(F_MED);
  uiCentre(Cal::name(e->nameIdx), 64, 22);

  snprintf(l, sizeof(l), "%02d %s", e->day, CAL_MONTHS[(e->month - 1) % 12]);
  display.setFont(F_BODY);
  uiCentre(l, 64, 38);

  int16_t d = Cal::daysUntil(*e);
  display.setFont(F_TINY);
  if (d == 0) uiCentre("today", 64, 48);
  else { snprintf(l, sizeof(l), "in %d days", d); uiCentre(l, 64, 48); }

  static const char* F[3] = { "name", "day", "month" };
  snprintf(l, sizeof(l), "%s  B0+  B1 next", F[calField]);
  display.drawHLine(0, 53, 128);
  display.drawStr(2, 61, l);
  display.sendBuffer();
}

static void tickCalEdit(BtnEvent ev) {
  CalEvent* e = Cal::at(calEditIdx);
  if (!e) { goState(ST_CAL_LIST); return; }

  if (ev == EV_UP) {
    if (calField == 0)      e->nameIdx = (uint8_t)((e->nameIdx + 1) % Cal::nameCount());
    else if (calField == 1) e->day     = (uint8_t)(e->day % 31 + 1);
    else                    e->month   = (uint8_t)(e->month % 12 + 1);
    gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(calField);
    calField = (uint8_t)((calField + 1) % 3);
    gDirty = true;
  } else if (ev == EV_SELECT) {
    Cal::save();
    goState(ST_CAL_LIST);
    return;
  } else if (ev == EV_EXIT) {
    Cal::remove(calEditIdx);          // hold both to delete
    calSel = 0;
    goState(ST_CAL_LIST);
    return;
  }
  if (gDirty) { gDirty = false; drawCalEdit(); }
}

// ===========================================================================
// Display settings
// ===========================================================================
static uint8_t dispSel = 0, dispTop = 0;
enum { D_BACK = 0, D_CONTRAST, D_INVERT, D_FLIP, D_STANDBY, D_SLEEP, D_COUNT };

static void dispLine(uint8_t i, char* out, size_t n) {
  WatchSettings& w = Settings::get();
  switch (i) {
    case D_BACK:     snprintf(out, n, "< Back"); break;
    case D_CONTRAST: snprintf(out, n, "Contrast   %d", w.contrast); break;
    case D_INVERT:   snprintf(out, n, "Invert     %s", w.invert ? "on" : "off"); break;
    case D_FLIP:     snprintf(out, n, "Rotate 180 %s", w.flip ? "on" : "off"); break;
    case D_STANDBY:  snprintf(out, n, "Dim after  %ds", w.standbyS); break;
    case D_SLEEP:    if (w.sleepS) snprintf(out, n, "Sleep at   %ds", w.sleepS);
                     else          snprintf(out, n, "Sleep at   never");
                     break;
  }
}

static void drawDisplaySettings() {
  uiListFrame("Display");
  display.setFont(F_BODY);
  for (uint8_t r = 0; r < 4; r++) {
    uint8_t i = (uint8_t)(dispTop + r);
    if (i >= D_COUNT) break;
    int16_t yt = (int16_t)(12 + r * 13);
    int16_t tx = uiRowSelect(yt, i == dispSel);
    char line[26];
    dispLine(i, line, sizeof(line));
    display.drawStr(tx, yt + 9, line);
    display.setDrawColor(1);
  }
  display.sendBuffer();
}

static void tickDisplaySettings(BtnEvent ev) {
  WatchSettings& w = Settings::get();
  if (ev == EV_UP) {
    if (dispSel == 0) { Settings::save(); goState(ST_MENU); return; }
    dispSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(dispSel);
    if (dispSel + 1 < D_COUNT) dispSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    switch (dispSel) {
      case D_BACK:     Settings::save(); goState(ST_MENU); return;
      case D_CONTRAST: w.contrast = (uint8_t)(w.contrast >= 240 ? 40 : w.contrast + 40); break;
      case D_INVERT:   w.invert = !w.invert; break;
      case D_FLIP:     w.flip   = !w.flip;   break;
      case D_STANDBY:  w.standbyS = (uint16_t)(w.standbyS >= 300 ? 15 : w.standbyS * 2); break;
      case D_SLEEP:    w.sleepS = (uint16_t)(w.sleepS == 0 ? w.standbyS
                                  : (w.sleepS >= 600 ? 0 : w.sleepS * 2)); break;
    }
    Settings::applyDisplay();
    gDirty = true;
  } else if (ev == EV_EXIT) {
    Settings::save(); goState(ST_MENU); return;
  }
  uiScrollClamp(D_COUNT, dispSel, dispTop, 4);
  if (gDirty) { gDirty = false; drawDisplaySettings(); }
}

// ===========================================================================
// Stopwatch
// ===========================================================================
#define SW_MAX_LAPS 6

static bool     swRun    = false;
static uint32_t swStart  = 0;      // millis() at the last start
static uint32_t swAccum  = 0;      // ms banked from previous runs
static uint32_t swLaps[SW_MAX_LAPS];
static uint8_t  swLapN   = 0;
static uint32_t swDrawAt = 0;

bool stopwatchActive() { return swRun; }

static uint32_t swElapsed() {
  return swAccum + (swRun ? (millis() - swStart) : 0);
}

static void swFormat(uint32_t ms, char* out, size_t n) {
  uint32_t cs = (ms / 10) % 100;
  uint32_t sec = (ms / 1000) % 60;
  uint32_t min = (ms / 60000UL) % 60;
  uint32_t hr  = ms / 3600000UL;
  if (hr) snprintf(out, n, "%lu:%02lu:%02lu", (unsigned long)hr,
                   (unsigned long)min, (unsigned long)sec);
  else    snprintf(out, n, "%02lu:%02lu.%02lu", (unsigned long)min,
                   (unsigned long)sec, (unsigned long)cs);
}

static void drawStopwatch() {
  uiListFrame(swRun ? "Stopwatch  RUN" : "Stopwatch");
  display.setDrawColor(1);

  char t[16];
  swFormat(swElapsed(), t, sizeof(t));
  display.setFont(F_LARGE);
  uiCentre(t, 64, 26);

  display.setFont(F_TINY);
  if (swLapN) {
    display.drawHLine(0, 38, 128);
    // newest laps first, two per row
    for (uint8_t i = 0; i < swLapN && i < 4; i++) {
      uint8_t idx = (uint8_t)(swLapN - 1 - i);
      char l[22];
      swFormat(swLaps[idx], t, sizeof(t));
      snprintf(l, sizeof(l), "%d %s", idx + 1, t);
      display.drawStr((i & 1) ? 66 : 4, 48 + (i / 2) * 9, l);
    }
  } else {
    display.drawStr(4, 50, swRun ? "B1 lap" : "both = start");
    display.drawStr(4, 59, swRun ? "both = stop" : "B1 reset   B0 back");
  }
  display.sendBuffer();
}

static void tickStopwatch(BtnEvent ev) {
  if (ev == EV_SELECT) {
    if (swRun) { swAccum += millis() - swStart; swRun = false; }
    else       { swStart = millis(); swRun = true; }
    gDirty = true;
  } else if (ev == EV_DOWN) {
    if (swRun) {
      if (swLapN < SW_MAX_LAPS) swLaps[swLapN++] = swElapsed();
    } else {
      swAccum = 0; swLapN = 0;          // reset when stopped
    }
    gDirty = true;
  } else if (ev == EV_UP) {
    goState(ST_MENU);                   // keeps counting in the background
    return;
  } else if (ev == EV_EXIT) {
    swRun = false; swAccum = 0; swLapN = 0;
    goState(ST_MENU);
    return;
  }

  // centiseconds need a fast redraw, but only while actually running
  if (gDirty || (swRun && millis() - swDrawAt >= 60)) {
    gDirty = false;
    swDrawAt = millis();
    drawStopwatch();
  }
}

// ===========================================================================
// Jokes
// ===========================================================================
struct Joke { const char* setup; const char* punch; };
static const Joke JOKES[] = {
  { "What does a clock do\nwhen it's hungry?", "It goes back\nfour seconds." },
  { "What time is it?",                         "Time for you\nto get a watch." },
};
static const uint8_t JOKE_N = sizeof(JOKES) / sizeof(JOKES[0]);
static uint8_t jokeIdx = 0;
static bool    jokeShown = false;

static void drawJokes() {
  char title[16];
  snprintf(title, sizeof(title), "Joke %d/%d", jokeIdx + 1, JOKE_N);
  uiListFrame(title);
  display.setDrawColor(1);
  display.setFont(F_BODY);

  uiWrapText(JOKES[jokeIdx].setup, 4, 14, 120, 10, 3);

  if (jokeShown) {
    display.drawHLine(0, 38, 128);
    uiWrapText(JOKES[jokeIdx].punch, 4, 40, 120, 10, 2);
  } else {
    display.setFont(F_TINY);
    display.drawStr(4, 60, "both = punchline");
  }
  display.sendBuffer();
}

static void tickJokes(BtnEvent ev) {
  if (ev == EV_UP) {
    goState(ST_MENU);
    return;
  } else if (ev == EV_DOWN) {
    navMark(jokeIdx);
    jokeIdx = (uint8_t)((jokeIdx + 1) % JOKE_N);
    jokeShown = false;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (!jokeShown) jokeShown = true;
    else { jokeIdx = (uint8_t)((jokeIdx + 1) % JOKE_N); jokeShown = false; }
    gDirty = true;
  } else if (ev == EV_EXIT) {
    goState(ST_MENU);
    return;
  }
  if (gDirty) { gDirty = false; drawJokes(); }
}

// ===========================================================================
// Info
// ===========================================================================
static uint8_t infoTop = 0;

// Built fresh each draw so the live values stay current.
static uint8_t infoLines(char out[][26], uint8_t max) {
  uint8_t n = 0;
  auto add = [&](const char* fmt, ...) {
    if (n >= max) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(out[n], 26, fmt, ap);
    va_end(ap);
    n++;
  };
  add("Pineappley Watch");
  add("v%s", FW_VERSION);
  add("");
  add("Designed and built");
  add("by Nathan S");
  add("");
  add("pineappley.ie");
  add("github/nathans-systems");
  add("");
  add("BLE   %s", Notifs::connected() ? "connected" : "advertising");
  add("RTC   %s", rtcOk ? "ok" : "MISSING");
  add("Face  %s", FACES[gFace].name);
  add("Reset %s", gResetReason);
  if (Settings::get().flashMode) add("FLASH MODE ON");
  return n;
}

static void drawInfo() {
  char lines[16][26];
  uint8_t n = infoLines(lines, 16);

  uiListFrame("Info");
  display.setFont(F_BODY);
  display.setDrawColor(1);
  for (uint8_t r = 0; r < 5; r++) {
    uint8_t i = (uint8_t)(infoTop + r);
    if (i >= n) break;
    display.drawStr(3, 21 + r * 10, lines[i]);
  }
  if (n > 5) {
    int16_t h = (int16_t)(50 * 5 / n); if (h < 6) h = 6;
    int16_t y = (int16_t)(13 + (50 - h) * infoTop / (n - 5));
    display.drawVLine(126, 13, 50);
    display.drawBox(125, y, 3, h);
  }
  display.sendBuffer();
}

static void tickInfo(BtnEvent ev) {
  char lines[16][26];
  uint8_t n = infoLines(lines, 16);

  if (ev == EV_UP) {
    if (infoTop == 0) { goState(ST_MENU); return; }
    infoTop--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(infoTop);
    if (n > 5 && infoTop + 5 < n) infoTop++;
    gDirty = true;
  } else if (ev == EV_SELECT || ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  static uint32_t last = 0;
  if (gDirty || millis() - last > 2000) { gDirty = false; last = millis(); drawInfo(); }
}

// ===========================================================================
// Game menu
// ===========================================================================
static void drawGameMenu() {
  uiListFrame("Games");
  uiList(GAME_ITEMS, 7, gameSel, gameTop, 4);
  display.sendBuffer();
}

static void tickGameMenu(BtnEvent ev) {
  if (ev == EV_UP) {
    if (gameSel == 0) { goState(ST_MENU); return; }
    gameSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    navMark(gameSel);
    if (gameSel + 1 < 7) gameSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (gameSel == 0) { goState(ST_MENU);     return; }
    if (gameSel == 1) { goState(ST_PONG);     return; }
    if (gameSel == 2) { goState(ST_SNAKE);    return; }
    if (gameSel == 3) { goState(ST_DINO);     return; }
    if (gameSel == 4) { goState(ST_BREAKOUT); return; }
    if (gameSel == 5) { goState(ST_JET);      return; }
    if (gameSel == 6) { goState(ST_SIMON);    return; }
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(7, gameSel, gameTop, 4);
  if (gDirty) { gDirty = false; drawGameMenu(); }
}

// ===========================================================================
// Dispatch
// ===========================================================================
void screenEnter(AppState s) {
  gDirty = true;
  navClear();                 // no stale cursor pointer across a screen change
  switch (s) {
    case ST_FACE:       lastFaceDraw = 0; break;
    case ST_NOTIF_LIST: notifSel = 0; notifTop = 0; Notifs::markAllRead(); break;
    case ST_NOTIF_VIEW: viewScroll = 0; break;
    case ST_MENU:       menuSel = 0; menuTop = 0; break;
    case ST_FACEPICK:   faceSel = gFace; faceTop = 0;
                        uiScrollClamp(FACE_COUNT, faceSel, faceTop, 4); break;
    case ST_GAMEMENU:   gameSel = 1; gameTop = 0; break;
    case ST_TT_DAYS:    ttSel = 1; ttTop = 0; break;
    case ST_TT_LIST:    ttListTop = 0; break;
    case ST_WEATHER:    break;
    case ST_JOKES:      jokeIdx = 0; jokeShown = false; break;
    case ST_STOPWATCH:  swDrawAt = 0; break;
    case ST_CAL_LIST:   calSel = 0; calTop = 0; break;
    case ST_CAL_EDIT:   calField = 0; break;
    case ST_DISPLAY:    dispSel = 0; dispTop = 0; break;
    case ST_BREAKOUT:   breakoutEnter(); break;
    case ST_JET:        jetEnter();      break;
    case ST_SIMON:      simonEnter();    break;
    case ST_INFO:       infoTop = 0; break;
    case ST_SETTIME: {
      DateTime n = nowSafe();
      stV[0] = n.hour(); stV[1] = n.minute(); stV[2] = n.day();
      stV[3] = n.month(); stV[4] = n.year();
      stField = 0;
      break;
    }
    case ST_PONG:  pongEnter();  break;
    case ST_SNAKE: snakeEnter(); break;
    case ST_DINO:  dinoEnter();  break;
    default: break;
  }
}

static bool isGameState(AppState s) {
  return s == ST_PONG || s == ST_SNAKE || s == ST_DINO ||
         s == ST_BREAKOUT || s == ST_JET || s == ST_SIMON ||
         s == ST_STOPWATCH;
}

void screenTick(AppState s, BtnEvent ev) {
  if (ev == EV_SELECT2) {
    if (isGameState(s)) {
      // Games and the stopwatch need rapid repeated B1 taps (turn, lap, flap),
      // so a quick second tap stays a plain press here.
      ev = EV_DOWN;
    } else {
      navUndo();          // put the cursor back where the first tap found it
      ev = EV_SELECT;
    }
  }

  switch (s) {
    case ST_FACE:       tickFace(ev);      break;
    case ST_NOTIF_LIST: tickNotifList(ev); break;
    case ST_NOTIF_VIEW: tickNotifView(ev); break;
    case ST_MENU:       tickMenu(ev);      break;
    case ST_FACEPICK:   tickFacePick(ev);  break;
    case ST_SETTIME:    tickSetTime(ev);   break;
    case ST_GAMEMENU:   tickGameMenu(ev);  break;
    case ST_TT_DAYS:    tickTtDays(ev);    break;
    case ST_TT_LIST:    tickTtList(ev);    break;
    case ST_WEATHER:    tickWeather(ev);   break;
    case ST_JOKES:      tickJokes(ev);     break;
    case ST_STOPWATCH:  tickStopwatch(ev); break;
    case ST_CAL_LIST:   tickCalList(ev);   break;
    case ST_CAL_EDIT:   tickCalEdit(ev);   break;
    case ST_DISPLAY:    tickDisplaySettings(ev); break;
    case ST_BREAKOUT:   breakoutTick(ev);  break;
    case ST_JET:        jetTick(ev);       break;
    case ST_SIMON:      simonTick(ev);     break;
    case ST_INFO:       tickInfo(ev);      break;
    case ST_PONG:       pongTick(ev);      break;
    case ST_SNAKE:      snakeTick(ev);     break;
    case ST_DINO:       dinoTick(ev);      break;
    default: break;
  }
}
