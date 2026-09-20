#pragma once
#include <Arduino.h>

// Dated events kept in NVS. Names come from a fixed list rather than free text,
// because entering letters with two buttons is miserable. Add your own to
// CAL_NAMES in calendar.cpp and they appear in the picker.

#define CAL_MAX 8

struct CalEvent {
  uint8_t month;     // 1-12
  uint8_t day;       // 1-31
  uint8_t nameIdx;
  uint8_t used;      // 0 = free slot
};

namespace Cal {
  void        begin();
  void        save();
  uint8_t     count();                    // used slots
  CalEvent*   at(uint8_t i);              // i < count()
  bool        add();                      // seeded with today's date
  void        remove(uint8_t i);
  const char* name(uint8_t idx);
  uint8_t     nameCount();
  int16_t     daysUntil(const CalEvent& e);   // 0 = today
  const char* todayName();                    // name if an event is today, else nullptr
}
