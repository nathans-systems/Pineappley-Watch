#pragma once
#include <Arduino.h>

enum BtnEvent : uint8_t {
  EV_NONE = 0,
  EV_UP,      // B0 short press (auto-repeats when held)
  EV_DOWN,    // B1 short press (auto-repeats when held)
  EV_SELECT,  // both pressed and released together
  EV_EXIT     // both held for HOLD_EXIT_MS
};

namespace Btn {
  void      begin();
  void      update();
  BtnEvent  get();
  bool      down(uint8_t i);
  bool      anyDown();
  uint32_t  lastActivity();
  void      flush();
  void      markActivity();
}
