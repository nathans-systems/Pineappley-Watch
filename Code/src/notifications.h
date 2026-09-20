#pragma once
#include <Arduino.h>

#define NOTIF_MAX   16   // link stays up in standby, so these accumulate
#define NOTIF_TITLE 26
#define NOTIF_BODY  62

struct Notif {
  uint8_t  cat;
  char     title[NOTIF_TITLE];
  char     body[NOTIF_BODY];
  uint32_t stamp;
};

namespace Notifs {
  void        begin();
  void        stop();
  void        loop();
  uint8_t     count();
  const Notif& get(uint8_t i);         // i = 0 is newest
  uint8_t     unread();
  void        markAllRead();
  void        clear();
  bool        connected();
  const char* categoryName(uint8_t cat);
  void        add(uint8_t cat, const char* title, const char* body);
}
