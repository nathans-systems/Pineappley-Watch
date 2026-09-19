#pragma once
#include "watch.h"

struct Face {
  const char* name;
  void (*draw)(const DateTime& now);
  uint16_t frameMs;      // redraw interval: ~1000 static, ~80 animated
};

extern const Face    FACES[];
extern const uint8_t FACE_COUNT;
