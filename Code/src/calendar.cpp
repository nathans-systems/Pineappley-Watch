#include "calendar.h"
#include "watch.h"

static const char* CAL_NAMES[] = {
  "Birthday", "Exam", "Holiday", "Party", "Trip", "Deadline",
  "Match", "Anniversary", "Appointment", "Event", "Christmas", "New Year"
};
static const uint8_t CAL_NAME_N = sizeof(CAL_NAMES) / sizeof(CAL_NAMES[0]);

static CalEvent s_ev[CAL_MAX];

const char* Cal::name(uint8_t idx) { return CAL_NAMES[idx % CAL_NAME_N]; }
uint8_t     Cal::nameCount()       { return CAL_NAME_N; }

void Cal::begin() {
  memset(s_ev, 0, sizeof(s_ev));
  size_t got = prefs.getBytes("cal", s_ev, sizeof(s_ev));
  if (got != sizeof(s_ev)) memset(s_ev, 0, sizeof(s_ev));
  for (uint8_t i = 0; i < CAL_MAX; i++) {
    if (!s_ev[i].used) continue;
    if (s_ev[i].month < 1 || s_ev[i].month > 12) s_ev[i].used = 0;
    if (s_ev[i].day   < 1 || s_ev[i].day   > 31) s_ev[i].used = 0;
  }
}

void Cal::save() { prefs.putBytes("cal", s_ev, sizeof(s_ev)); }

uint8_t Cal::count() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < CAL_MAX; i++) if (s_ev[i].used) n++;
  return n;
}

CalEvent* Cal::at(uint8_t i) {
  uint8_t n = 0;
  for (uint8_t k = 0; k < CAL_MAX; k++) {
    if (!s_ev[k].used) continue;
    if (n == i) return &s_ev[k];
    n++;
  }
  return nullptr;
}

bool Cal::add() {
  for (uint8_t k = 0; k < CAL_MAX; k++) {
    if (s_ev[k].used) continue;
    DateTime now = nowSafe();
    s_ev[k].month   = now.month();
    s_ev[k].day     = now.day();
    s_ev[k].nameIdx = 0;
    s_ev[k].used    = 1;
    save();
    return true;
  }
  return false;
}

void Cal::remove(uint8_t i) {
  CalEvent* e = at(i);
  if (e) { e->used = 0; save(); }
}

// Days to the next occurrence, treating every event as yearly.
int16_t Cal::daysUntil(const CalEvent& e) {
  DateTime now = nowSafe();
  if (e.month == now.month() && e.day == now.day()) return 0;

  DateTime today(now.year(), now.month(), now.day(), 0, 0, 0);
  DateTime target(now.year(), e.month, e.day, 0, 0, 0);
  if (target.unixtime() < today.unixtime())
    target = DateTime((uint16_t)(now.year() + 1), e.month, e.day, 0, 0, 0);

  return (int16_t)((target.unixtime() - today.unixtime()) / 86400UL);
}

const char* Cal::todayName() {
  DateTime now = nowSafe();
  for (uint8_t k = 0; k < CAL_MAX; k++) {
    if (!s_ev[k].used) continue;
    if (s_ev[k].month == now.month() && s_ev[k].day == now.day())
      return name(s_ev[k].nameIdx);
  }
  return nullptr;
}
