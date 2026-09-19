#include "screens.h"
#include "ui.h"
#include "faces.h"
#include "games.h"
#include "notifications.h"
#include "linkdata.h"
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

enum { M_BACK = 0, M_NOTIF, M_WEATHER, M_FACE, M_GAMES, M_JOKE, M_TT, M_TIME, M_INFO, M_COUNT };
static const char* MENU_ITEMS[M_COUNT] = {
  "< Watch face", "Notifications", "Weather",
  "Watch faces", "Games", "Jokes", "Timetable", "Set time", "Info"
};
static const char* GAME_ITEMS[4] = { "< Back", "Pong", "Snake", "Dino Run" };

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
  uiListFrame("Options");
  uiList(MENU_ITEMS, M_COUNT, menuSel, menuTop, 4);
  display.sendBuffer();
}

static void tickMenu(BtnEvent ev) {
  if (ev == EV_UP) {
    if (menuSel == 0) { goState(ST_FACE); return; }    // B0 on the top item exits
    menuSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
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
      case M_GAMES:   goState(ST_GAMEMENU);   return;
      case M_JOKE:    goState(ST_JOKES);      return;
      case M_TT:      goState(ST_TT_DAYS);    return;
      case M_TIME:    goState(ST_SETTIME);    return;
      case M_INFO:    goState(ST_INFO);       return;
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
  uiList(GAME_ITEMS, 4, gameSel, gameTop, 4);
  display.sendBuffer();
}

static void tickGameMenu(BtnEvent ev) {
  if (ev == EV_UP) {
    if (gameSel == 0) { goState(ST_MENU); return; }
    gameSel--; gDirty = true;
  } else if (ev == EV_DOWN) {
    if (gameSel + 1 < 4) gameSel++;
    gDirty = true;
  } else if (ev == EV_SELECT) {
    if (gameSel == 0) { goState(ST_MENU);  return; }
    if (gameSel == 1) { goState(ST_PONG);  return; }
    if (gameSel == 2) { goState(ST_SNAKE); return; }
    if (gameSel == 3) { goState(ST_DINO);  return; }
  } else if (ev == EV_EXIT) {
    goState(ST_MENU); return;
  }
  uiScrollClamp(4, gameSel, gameTop, 4);
  if (gDirty) { gDirty = false; drawGameMenu(); }
}

// ===========================================================================
// Dispatch
// ===========================================================================
void screenEnter(AppState s) {
  gDirty = true;
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

void screenTick(AppState s, BtnEvent ev) {
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
    case ST_INFO:       tickInfo(ev);      break;
    case ST_PONG:       pongTick(ev);      break;
    case ST_SNAKE:      snakeTick(ev);     break;
    case ST_DINO:       dinoTick(ev);      break;
    default: break;
  }
}
