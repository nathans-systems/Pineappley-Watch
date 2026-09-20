#include "buttons.h"
#include "config.h"

namespace {
  const uint8_t PINS[2] = { BTN_0_PIN, BTN_1_PIN };

  bool     rawState[2]    = { false, false };
  bool     stableState[2] = { false, false };
  uint32_t lastEdge[2]    = { 0, 0 };
  uint32_t pressAt[2]     = { 0, 0 };
  bool     pending[2]     = { false, false };
  bool     repeating[2]   = { false, false };
  uint32_t lastEmit[2]    = { 0, 0 };
  uint16_t repeatCnt[2]   = { 0, 0 };

  uint32_t lastTapB1      = 0;       // when B1 last fired a fresh (non-repeat) tap

  bool     comboActive    = false;
  bool     comboExitFired = false;
  bool     suppress       = false;
  uint32_t comboAt        = 0;

  BtnEvent q[8];
  uint8_t  qh = 0, qt = 0;
  uint32_t activityAt = 0;

  void push(BtnEvent e) {
    uint8_t n = (uint8_t)((qt + 1) & 7);
    if (n == qh) return;
    q[qt] = e;
    qt = n;
  }
}

void Btn::begin() {
  pinMode(BTN_0_PIN, INPUT_PULLUP);
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  // Seed from the real pin state so the button still held from the wake press
  // doesn't immediately fire an event.
  for (uint8_t i = 0; i < 2; i++) {
    bool r = (digitalRead(PINS[i]) == LOW);
    rawState[i] = stableState[i] = r;
    lastEdge[i] = millis();
    pending[i] = false;
  }
  suppress   = (stableState[0] || stableState[1]);
  lastTapB1  = 0;
  qh = qt = 0;
  activityAt = millis();
}

void Btn::markActivity() { activityAt = millis(); }

void Btn::update() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < 2; i++) {
    bool r = (digitalRead(PINS[i]) == LOW);
    if (r != rawState[i]) { rawState[i] = r; lastEdge[i] = now; }

    if ((now - lastEdge[i]) >= DEBOUNCE_MS && stableState[i] != rawState[i]) {
      stableState[i] = rawState[i];
      activityAt = now;
      if (stableState[i]) {
        pressAt[i] = now;
        pending[i] = true;
      } else {
        pending[i]   = false;
        repeating[i] = false;
        repeatCnt[i] = 0;
      }
    }
  }

  if (stableState[0] || stableState[1]) activityAt = now;

  // both buttons
  if (stableState[0] && stableState[1]) {
    if (!comboActive) {
      comboActive    = true;
      comboExitFired = false;
      suppress       = true;
      comboAt        = now;
      pending[0] = pending[1] = false;
      repeating[0] = repeating[1] = false;
    } else if (!comboExitFired && (now - comboAt) >= HOLD_EXIT_MS) {
      push(EV_EXIT);
      comboExitFired = true;
    }
  } else if (comboActive && !stableState[0] && !stableState[1]) {
    if (!comboExitFired) push(EV_SELECT);
    comboActive = false;
  }

  if (!stableState[0] && !stableState[1] && !comboActive) suppress = false;

  // Single presses. Both fire as soon as the combo window has passed, which is
  // the only delay in the path - just long enough to tell a single press from a
  // deliberate both-button press.
  for (uint8_t i = 0; i < 2; i++) {
    if (!(pending[i] && !comboActive && !suppress && (now - pressAt[i]) >= COMBO_WINDOW_MS))
      continue;

    BtnEvent ev = (i == 0) ? EV_UP : EV_DOWN;

#if DOUBLE_TAP_MS
    // Second B1 tap inside the window: the first one already went out as
    // EV_DOWN, so send EV_SELECT2 and let the screen rewind itself. Nothing is
    // held back waiting to find out, which is why single taps stay instant.
    if (i == 1) {
      if (lastTapB1 && (now - lastTapB1) <= DOUBLE_TAP_MS) {
        ev = EV_SELECT2;
        lastTapB1 = 0;
      } else {
        lastTapB1 = now;
      }
    }
#endif

    push(ev);
    pending[i]   = false;
    repeating[i] = true;
    repeatCnt[i] = 0;
    lastEmit[i]  = now;
  }

  // ---- auto-repeat while held, both buttons ----
  for (uint8_t i = 0; i < 2; i++) {
    if (!repeating[i] || !stableState[i] || comboActive) continue;
    BtnEvent ev = (i == 0) ? EV_UP : EV_DOWN;
    uint32_t interval = (repeatCnt[i] == 0) ? REPEAT_DELAY_MS : REPEAT_RATE_MS;
    if ((now - lastEmit[i]) >= interval) {
      push(ev);
      lastEmit[i] = now;
      if (i == 1) lastTapB1 = 0;        // a held repeat is not half a double tap
      if (repeatCnt[i] < 1000) repeatCnt[i]++;
    }
  }
}

BtnEvent Btn::get() {
  if (qh == qt) return EV_NONE;
  BtnEvent e = q[qh];
  qh = (uint8_t)((qh + 1) & 7);
  return e;
}

bool     Btn::down(uint8_t i)  { return (i < 2) ? stableState[i] : false; }
bool     Btn::anyDown()        { return stableState[0] || stableState[1]; }
uint32_t Btn::lastActivity()   { return activityAt; }
void     Btn::flush()          { qh = qt = 0; }
