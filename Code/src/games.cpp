#include "games.h"
#include "ui.h"

// ---------------------------------------------------------------- sprites --
// XBM bitmaps, LSB first, drawn with u8g2 drawXBM().
// spDino1: 16x16
#define SPDINO1_W 16
#define SPDINO1_H 16
static const uint8_t spDino1[] = {
  0x00, 0x3C, 0x00, 0x7E, 0x00, 0x76, 0x00, 0x7E, 0x00, 0xFE, 0x00, 0x3E, 0x03, 0x3E,
  0x03, 0x3F, 0x87, 0x3F, 0xCF, 0x3F, 0xFE, 0x3F, 0xFC, 0x3F, 0xF8, 0x3F, 0xF0, 0x0E,
  0x70, 0x06, 0x30, 0x1C
};

// spDino2: 16x16
#define SPDINO2_W 16
#define SPDINO2_H 16
static const uint8_t spDino2[] = {
  0x00, 0x3C, 0x00, 0x7E, 0x00, 0x76, 0x00, 0x7E, 0x00, 0xFE, 0x00, 0x3E, 0x03, 0x3E,
  0x03, 0x3F, 0x87, 0x3F, 0xCF, 0x3F, 0xFE, 0x3F, 0xFC, 0x3F, 0xF8, 0x3F, 0x70, 0x0E,
  0x38, 0x0C, 0x1C, 0x18
};

// spDinoDuck: 16x16
#define SPDINODUCK_W 16
#define SPDINODUCK_H 16
static const uint8_t spDinoDuck[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x78,
  0x03, 0xFC, 0x07, 0xF6, 0xFF, 0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0x38, 0x0E, 0x1C, 0x0C,
  0x00, 0x00, 0x00, 0x00
};

// spCactusS: 6x16
#define SPCACTUSS_W 6
#define SPCACTUSS_H 16
static const uint8_t spCactusS[] = {
  0x0C, 0x0C, 0x0D, 0x2D, 0x2D, 0x2F, 0x2E, 0x3C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
  0x0C, 0x0C
};

// spCactusL: 10x16
#define SPCACTUSL_W 10
#define SPCACTUSL_H 16
static const uint8_t spCactusL[] = {
  0x18, 0x00, 0x18, 0x00, 0x99, 0x01, 0x99, 0x01, 0x99, 0x01, 0x99, 0x01, 0xCF, 0x01,
  0xCE, 0x00, 0x7C, 0x00, 0x38, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,
  0x18, 0x00, 0x18, 0x00
};

// spBird1: 16x8
#define SPBIRD1_W 16
#define SPBIRD1_H 8
static const uint8_t spBird1[] = {
  0x0C, 0x00, 0x1E, 0x00, 0xFC, 0x00, 0xF8, 0x1F, 0xF0, 0xDF, 0xE0, 0x1F, 0xC0, 0x03,
  0x80, 0x01
};

// spBird2: 16x8
#define SPBIRD2_W 16
#define SPBIRD2_H 8
static const uint8_t spBird2[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF8, 0x1F, 0xFC, 0x6F, 0xFE, 0x01, 0x0F, 0x00,
  0x03, 0x00
};


// Hold both buttons ~1s to quit any game. Tap both to restart after a game over.

static void gameOverBanner(const char* line1, const char* line2) {
  display.setDrawColor(0);
  display.drawBox(10, 18, 108, 30);
  display.setDrawColor(1);
  display.drawFrame(10, 18, 108, 30);
  display.setFont(F_BODY);
  uiCentre(line1, 64, 28);
  display.setFont(F_TINY);
  uiCentre(line2, 64, 40);
}

// ===========================================================================
// PONG
// ===========================================================================
namespace {
  struct PongState {
    float bx, by, vx, vy;
    float py, ay;
    float aiAim;          // where the AI *thinks* it should be, with error
    uint8_t ps, as;
    bool over;
    uint32_t last;
  } P;
  const int PAD_H = 14, TOP = 11, BOT = 63;
  // Paddle speed cap. The ball's vertical speed is capped at 2.2, so anything
  // below that leaves shots the AI physically cannot reach.
  const float AI_SPEED = 1.15f;
}

static void pongReset(bool toPlayer) {
  P.bx = 64; P.by = 36;
  P.vx = toPlayer ? -1.25f : 1.25f;
  P.vy = (random(0, 2) ? 0.65f : -0.65f);
  P.aiAim = P.ay;
}

void pongEnter() {
  P.py = P.ay = 30;
  P.ps = P.as = 0;
  P.over = false;
  P.last = millis();
  pongReset(random(0, 2));
}

static void pongDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);

  char s[8];
  snprintf(s, sizeof(s), "%d", P.ps);
  display.drawStr(40, 7, s);
  snprintf(s, sizeof(s), "%d", P.as);
  display.drawStr(84, 7, s);
  display.drawStr(56, 7, "CPU");
  display.drawHLine(0, 10, 128);

  for (int y = TOP + 2; y < BOT; y += 6) display.drawVLine(64, y, 3);

  display.drawBox(2, (int)P.py, 3, PAD_H);
  display.drawBox(123, (int)P.ay, 3, PAD_H);
  display.drawBox((int)P.bx, (int)P.by, 3, 3);

  if (P.over) gameOverBanner(P.ps > P.as ? "YOU WIN" : "CPU WINS", "both=again hold=exit");
  display.sendBuffer();
}

void pongTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (P.over) {
    if (ev == EV_SELECT) pongEnter();
    pongDraw();
    return;
  }

  uint32_t now = millis();
  if (now - P.last < 28) return;
  P.last = now;

  if (Btn::down(0)) P.py -= 2.4f;
  if (Btn::down(1)) P.py += 2.4f;
  if (P.py < TOP) P.py = TOP;
  if (P.py > BOT - PAD_H) P.py = BOT - PAD_H;

  // AI. Three things make it beatable rather than a wall:
  //  1. it only reacts once the ball is heading its way and past midcourt,
  //  2. it aims at a point with a random offset, re-rolled on each return, so
  //     it does not sit dead centre on the ball,
  //  3. its paddle is slower than the ball can travel vertically, so a steep
  //     angle off the edge of your paddle will beat it.
  bool incoming = (P.vx > 0.0f) && (P.bx > 46.0f);
  if (incoming) {
    float want = P.by - PAD_H / 2.0f + P.aiAim;
    float d    = want - P.ay;
    float lim  = AI_SPEED;
    float step = (d >  lim) ?  lim : (d < -lim ? -lim : d);
    P.ay += step;
  } else {
    // drift back toward the middle instead of shadowing the ball
    float d = (BOT - PAD_H) / 2.0f - P.ay;
    P.ay += (d > 0 ? 0.35f : -0.35f);
  }
  if (P.ay < TOP) P.ay = TOP;
  if (P.ay > BOT - PAD_H) P.ay = BOT - PAD_H;

  P.bx += P.vx;
  P.by += P.vy;
  if (P.by <= TOP)     { P.by = TOP;     P.vy = -P.vy; }
  if (P.by >= BOT - 3) { P.by = BOT - 3; P.vy = -P.vy; }

  if (P.bx <= 5 && P.vx < 0) {
    if (P.by + 3 >= P.py && P.by <= P.py + PAD_H) {
      P.bx = 5; P.vx = -P.vx * 1.04f;
      P.vy += (P.by - (P.py + PAD_H / 2.0f)) * 0.09f;
      P.aiAim = (float)random(-7, 8);      // fresh error for this return
    }
  }
  if (P.bx >= 120 && P.vx > 0) {
    if (P.by + 3 >= P.ay && P.by <= P.ay + PAD_H) {
      P.bx = 120; P.vx = -P.vx * 1.04f;
      P.vy += (P.by - (P.ay + PAD_H / 2.0f)) * 0.09f;
    }
  }
  if (P.vy >  2.2f) P.vy =  2.2f;
  if (P.vy < -2.2f) P.vy = -2.2f;
  if (P.vx >  3.0f) P.vx =  3.0f;
  if (P.vx < -3.0f) P.vx = -3.0f;

  if (P.bx <  -4) { P.as++; pongReset(false); }
  if (P.bx > 130) { P.ps++; pongReset(true); }
  if (P.ps >= 7 || P.as >= 7) P.over = true;

  pongDraw();
}

// ===========================================================================
// SNAKE  (B0 turns left, B1 turns right)
// ===========================================================================
namespace {
  const uint8_t CELL = 4, COLS = 32, ROWS = 13, YOFF = 12;
  struct SnakeState {
    uint16_t body[160];
    uint8_t  len;
    uint8_t  dir;          // 0 up 1 right 2 down 3 left
    uint16_t food;
    uint16_t score;
    bool     over;
    uint32_t last;
    uint16_t interval;
  } S;
  inline uint16_t cellOf(uint8_t c, uint8_t r) { return (uint16_t)r * COLS + c; }
  inline uint8_t  colOf(uint16_t p) { return (uint8_t)(p % COLS); }
  inline uint8_t  rowOf(uint16_t p) { return (uint8_t)(p / COLS); }
}

static bool snakeOccupied(uint16_t p) {
  for (uint8_t i = 0; i < S.len; i++) if (S.body[i] == p) return true;
  return false;
}

static void snakeFood() {
  for (uint8_t tries = 0; tries < 200; tries++) {
    uint16_t p = cellOf((uint8_t)random(0, COLS), (uint8_t)random(0, ROWS));
    if (!snakeOccupied(p)) { S.food = p; return; }
  }
}

void snakeEnter() {
  S.len = 4;
  for (uint8_t i = 0; i < S.len; i++) S.body[i] = cellOf(10 - i, 6);
  S.dir = 1;
  S.score = 0;
  S.over = false;
  S.interval = 170;
  S.last = millis();
  snakeFood();
}

static void snakeDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  char s[20];
  snprintf(s, sizeof(s), "SNAKE  %d", S.score);
  display.drawStr(2, 7, s);
  display.drawHLine(0, 10, 128);

  display.drawBox(colOf(S.food) * CELL + 1, YOFF + rowOf(S.food) * CELL + 1, 2, 2);
  for (uint8_t i = 0; i < S.len; i++) {
    int x = colOf(S.body[i]) * CELL;
    int y = YOFF + rowOf(S.body[i]) * CELL;
    if (i == 0) display.drawBox(x, y, CELL, CELL);
    else        display.drawBox(x, y, CELL - 1, CELL - 1);
  }
  if (S.over) gameOverBanner("GAME OVER", "both=again hold=exit");
  display.sendBuffer();
}

void snakeTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (S.over) {
    if (ev == EV_SELECT) snakeEnter();
    snakeDraw();
    return;
  }
  if (ev == EV_UP)   S.dir = (uint8_t)((S.dir + 3) & 3);   // left
  if (ev == EV_DOWN) S.dir = (uint8_t)((S.dir + 1) & 3);   // right

  uint32_t now = millis();
  if (now - S.last < S.interval) { snakeDraw(); return; }
  S.last = now;

  int c = colOf(S.body[0]), r = rowOf(S.body[0]);
  if (S.dir == 0) r--; else if (S.dir == 1) c++; else if (S.dir == 2) r++; else c--;

  if (c < 0 || c >= COLS || r < 0 || r >= ROWS) { S.over = true; snakeDraw(); return; }
  uint16_t head = cellOf((uint8_t)c, (uint8_t)r);
  for (uint8_t i = 0; i < S.len - 1; i++)
    if (S.body[i] == head) { S.over = true; snakeDraw(); return; }

  for (int i = S.len; i > 0; i--) S.body[i] = S.body[i - 1];
  S.body[0] = head;

  if (head == S.food) {
    if (S.len < 159) S.len++;
    S.score += 10;
    if (S.interval > 70) S.interval -= 4;
    snakeFood();
  }
  snakeDraw();
}

// ===========================================================================
// BREAKOUT  (B0 left, B1 right, both = launch)
// ===========================================================================
namespace {
  const uint8_t BK_COLS = 10, BK_ROWS = 4;
  const uint8_t BK_BW = 12, BK_BH = 5;
  struct BreakState {
    bool     brick[BK_ROWS][BK_COLS];
    float    px;               // paddle left edge
    float    bx, by, vx, vy;
    bool     stuck;            // sitting on the paddle waiting to launch
    uint8_t  lives;
    uint16_t score;
    bool     over, won;
    uint32_t last;
  } B;
  const int BK_PADW = 22, BK_PADY = 60, BK_TOP = 12;
}

static void breakoutServe() {
  B.stuck = true;
  B.bx = B.px + BK_PADW / 2.0f;
  B.by = BK_PADY - 3;
  B.vx = 0; B.vy = 0;
}

void breakoutEnter() {
  for (uint8_t r = 0; r < BK_ROWS; r++)
    for (uint8_t c = 0; c < BK_COLS; c++) B.brick[r][c] = true;
  B.px = 53;
  B.lives = 3;
  B.score = 0;
  B.over = B.won = false;
  B.last = millis();
  breakoutServe();
}

static void breakoutDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  char s[20];
  snprintf(s, sizeof(s), "BREAK %d", B.score);
  display.drawStr(2, 7, s);
  for (uint8_t i = 0; i < B.lives; i++) display.drawBox(112 + i * 5, 2, 3, 5);
  display.drawHLine(0, 9, 128);

  for (uint8_t r = 0; r < BK_ROWS; r++)
    for (uint8_t c = 0; c < BK_COLS; c++)
      if (B.brick[r][c]) {
        int x = 4 + c * (BK_BW + 1), y = BK_TOP + r * (BK_BH + 2);
        if (r & 1) display.drawFrame(x, y, BK_BW, BK_BH);
        else       display.drawBox(x, y, BK_BW, BK_BH);
      }

  display.drawBox((int)B.px, BK_PADY, BK_PADW, 3);
  display.drawBox((int)B.bx, (int)B.by, 3, 3);

  if (B.over) gameOverBanner(B.won ? "CLEARED" : "GAME OVER", "both=again hold=exit");
  display.sendBuffer();
}

void breakoutTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (B.over) {
    if (ev == EV_SELECT) breakoutEnter();
    breakoutDraw();
    return;
  }
  if (ev == EV_SELECT && B.stuck) {
    B.stuck = false;
    B.vx = (random(0, 2) ? 1.1f : -1.1f);
    B.vy = -1.35f;
  }

  uint32_t now = millis();
  if (now - B.last < 26) return;
  B.last = now;

  if (Btn::down(0)) B.px -= 2.8f;
  if (Btn::down(1)) B.px += 2.8f;
  if (B.px < 1) B.px = 1;
  if (B.px > 127 - BK_PADW) B.px = 127 - BK_PADW;

  if (B.stuck) { B.bx = B.px + BK_PADW / 2.0f; breakoutDraw(); return; }

  B.bx += B.vx;
  B.by += B.vy;
  if (B.bx <= 0)   { B.bx = 0;   B.vx = -B.vx; }
  if (B.bx >= 125) { B.bx = 125; B.vx = -B.vx; }
  if (B.by <= 10)  { B.by = 10;  B.vy = -B.vy; }

  // paddle
  if (B.vy > 0 && B.by + 3 >= BK_PADY && B.by < BK_PADY + 4 &&
      B.bx + 3 >= B.px && B.bx <= B.px + BK_PADW) {
    B.by = BK_PADY - 3;
    B.vy = -B.vy;
    // where it lands on the paddle sets the angle, so you can aim
    B.vx += ((B.bx + 1.5f) - (B.px + BK_PADW / 2.0f)) * 0.10f;
    if (B.vx >  2.0f) B.vx =  2.0f;
    if (B.vx < -2.0f) B.vx = -2.0f;
  }

  // bricks
  for (uint8_t r = 0; r < BK_ROWS && !B.over; r++) {
    for (uint8_t c = 0; c < BK_COLS; c++) {
      if (!B.brick[r][c]) continue;
      int x = 4 + c * (BK_BW + 1), y = BK_TOP + r * (BK_BH + 2);
      if (B.bx + 3 > x && B.bx < x + BK_BW && B.by + 3 > y && B.by < y + BK_BH) {
        B.brick[r][c] = false;
        B.vy = -B.vy;
        B.score += (BK_ROWS - r) * 5;
        goto hit;
      }
    }
  }
hit:

  if (B.by > 63) {
    if (--B.lives == 0) { B.over = true; B.won = false; }
    else breakoutServe();
  }

  bool any = false;
  for (uint8_t r = 0; r < BK_ROWS; r++)
    for (uint8_t c = 0; c < BK_COLS; c++) if (B.brick[r][c]) any = true;
  if (!any) { B.over = true; B.won = true; }

  breakoutDraw();
}

// ===========================================================================
// JET  (flap with either button, thread the gaps)
// ===========================================================================
namespace {
  struct Pipe { int16_t x; uint8_t gapY; bool active; };
  struct JetState {
    float    y, vy;
    Pipe     pipe[3];
    float    speed;
    uint16_t score;
    bool     over, started;
    uint32_t last, lastSpawn;
  } J;
  const int JET_X = 24, JET_GAP = 26;
}

void jetEnter() {
  J.y = 30; J.vy = 0;
  for (uint8_t i = 0; i < 3; i++) J.pipe[i].active = false;
  J.speed = 1.6f;
  J.score = 0;
  J.over = false;
  J.started = false;
  J.last = millis();
  J.lastSpawn = millis();
}

static void jetDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  char s[16];
  snprintf(s, sizeof(s), "JET %d", J.score);
  display.drawStr(2, 7, s);
  display.drawHLine(0, 9, 128);
  display.drawHLine(0, 63, 128);

  for (uint8_t i = 0; i < 3; i++) {
    if (!J.pipe[i].active) continue;
    Pipe& p = J.pipe[i];
    display.drawBox(p.x, 10, 8, p.gapY - 10);
    display.drawBox(p.x, p.gapY + JET_GAP, 8, 63 - (p.gapY + JET_GAP));
  }

  // little craft with a thruster flare when climbing
  display.drawBox(JET_X, (int)J.y, 7, 4);
  display.drawPixel(JET_X + 7, (int)J.y + 1);
  if (J.vy < 0) display.drawHLine(JET_X - 3, (int)J.y + 2, 3);

  if (!J.started) { display.setFont(F_TINY); uiCentre("press to fly", 64, 46); }
  if (J.over) gameOverBanner("CRASHED", "both=again hold=exit");
  display.sendBuffer();
}

void jetTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (J.over) {
    if (ev == EV_SELECT) jetEnter();
    jetDraw();
    return;
  }
  if (ev == EV_UP || ev == EV_DOWN || ev == EV_SELECT) {
    J.started = true;
    J.vy = -1.55f;
  }
  if (!J.started) { jetDraw(); return; }

  uint32_t now = millis();
  if (now - J.last < 30) return;
  J.last = now;

  J.vy += 0.18f;
  if (J.vy > 2.6f) J.vy = 2.6f;
  J.y += J.vy;
  if (J.y < 10) { J.y = 10; J.vy = 0; }
  if (J.y > 59) { J.over = true; }

  J.speed = 1.6f + J.score * 0.04f;
  if (J.speed > 3.4f) J.speed = 3.4f;

  if (now - J.lastSpawn > (uint32_t)(1500 / J.speed)) {
    for (uint8_t i = 0; i < 3; i++) {
      if (J.pipe[i].active) continue;
      J.pipe[i].active = true;
      J.pipe[i].x = 128;
      J.pipe[i].gapY = (uint8_t)random(14, 63 - JET_GAP - 2);
      break;
    }
    J.lastSpawn = now;
  }

  for (uint8_t i = 0; i < 3; i++) {
    if (!J.pipe[i].active) continue;
    Pipe& p = J.pipe[i];
    int16_t prev = p.x;
    p.x -= (int16_t)J.speed;
    if (p.x + 8 < 0) { p.active = false; continue; }
    if (prev > JET_X && p.x <= JET_X) J.score++;
    bool overlapX = (JET_X + 7 > p.x) && (JET_X < p.x + 8);
    if (overlapX && ((int)J.y < p.gapY || (int)J.y + 4 > p.gapY + JET_GAP)) J.over = true;
  }

  jetDraw();
}

// ===========================================================================
// SIMON  (watch the sequence, repeat it: B0 = left, B1 = right)
// ===========================================================================
namespace {
  struct SimonState {
    uint8_t  seq[32];
    uint8_t  len;
    uint8_t  pos;         // how far through the player is
    bool     showing;
    uint8_t  showIdx;
    uint32_t showAt;
    int8_t   flash;       // -1 none, else which side is lit
    bool     over;
    uint16_t best;
  } M;
}

static void simonNext() {
  if (M.len < 32) M.seq[M.len++] = (uint8_t)random(0, 2);
  M.showing = true;
  M.showIdx = 0;
  M.showAt = millis();
  M.pos = 0;
  M.flash = -1;
}

void simonEnter() {
  M.len = 0;
  M.over = false;
  simonNext();
}

static void simonDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  char s[24];
  snprintf(s, sizeof(s), "SIMON  round %d", M.len);
  display.drawStr(2, 7, s);
  display.drawHLine(0, 9, 128);

  for (uint8_t side = 0; side < 2; side++) {
    int x = side ? 66 : 4;
    if (M.flash == (int8_t)side) display.drawBox(x, 14, 58, 36);
    else                         display.drawFrame(x, 14, 58, 36);
  }
  display.setDrawColor(1);
  display.setFont(F_TINY);
  display.drawStr(22, 58, M.showing ? "watch..." : "your turn");
  display.drawStr(4, 58, "B0");
  display.drawStr(116, 58, "B1");

  if (M.over) gameOverBanner("WRONG", "both=again hold=exit");
  display.sendBuffer();
}

void simonTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (M.over) {
    if (ev == EV_SELECT) simonEnter();
    simonDraw();
    return;
  }

  if (M.showing) {
    uint32_t dt = millis() - M.showAt;
    if (M.flash < 0 && dt > 250) { M.flash = (int8_t)M.seq[M.showIdx]; M.showAt = millis(); }
    else if (M.flash >= 0 && dt > 420) {
      M.flash = -1;
      M.showAt = millis();
      if (++M.showIdx >= M.len) { M.showing = false; M.showIdx = 0; }
    }
    simonDraw();
    return;
  }

  if (ev == EV_UP || ev == EV_DOWN) {
    uint8_t pressed = (ev == EV_DOWN) ? 1 : 0;
    M.flash = (int8_t)pressed;
    if (M.seq[M.pos] != pressed) { M.over = true; }
    else if (++M.pos >= M.len)   { simonNext(); }
    simonDraw();
    return;
  }

  if (M.flash >= 0 && millis() - M.showAt > 160) { M.flash = -1; }
  simonDraw();
}

// ===========================================================================
// DINO RUN  (B0 jump, B1 duck)
// ===========================================================================
namespace {
  const int GROUND = 56;     // y of the ground line
  const int DINO_X  = 10;
  const int BIRD_Y  = 30;    // high enough that you must duck
  struct Obs { int16_t x; uint8_t w, h; uint8_t type; bool active; };  // type 0/1 cactus, 2 bird
  struct DinoState {
    float y, vy;
    bool  ducking;
    Obs   obs[4];
    float speed;
    float scroll;
    uint32_t score;
    bool  over;
    uint32_t last, lastSpawn;
  } D;
}

void dinoEnter() {
  D.y = 0; D.vy = 0; D.ducking = false;
  for (uint8_t i = 0; i < 4; i++) D.obs[i].active = false;
  D.speed = 2.0f;
  D.scroll = 0;
  D.score = 0;
  D.over = false;
  D.last = millis();
  D.lastSpawn = millis();
}

static void dinoSpawn() {
  for (uint8_t i = 0; i < 4; i++) {
    if (D.obs[i].active) continue;
    D.obs[i].active = true;
    D.obs[i].x = 132;
    if (D.score > 250 && random(0, 4) == 0) {
      D.obs[i].type = 2;                                  // bird, duck under it
      D.obs[i].w = SPBIRD1_W; D.obs[i].h = SPBIRD1_H;
    } else if (random(0, 3) == 0) {
      D.obs[i].type = 1;                                  // wide cactus cluster
      D.obs[i].w = SPCACTUSL_W; D.obs[i].h = SPCACTUSL_H;
    } else {
      D.obs[i].type = 0;                                  // single cactus
      D.obs[i].w = SPCACTUSS_W; D.obs[i].h = SPCACTUSS_H;
    }
    return;
  }
}

static void dinoDraw() {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  char s[12];
  snprintf(s, sizeof(s), "%05lu", (unsigned long)D.score);
  display.drawStr(96, 7, s);
  display.drawStr(2, 7, "DINO");

  // ground line plus scrolling grit, so the speed reads visually
  display.drawHLine(0, GROUND + 1, 128);
  int16_t shift = (int16_t)(D.scroll) % 24;
  for (int16_t x = -shift; x < 128; x += 24) {
    display.drawPixel(x + 4,  GROUND + 4);
    display.drawPixel(x + 13, GROUND + 3);
    display.drawPixel(x + 18, GROUND + 5);
  }

  // dino: two-frame run cycle, separate ducking sprite
  int dh = D.ducking ? SPDINODUCK_H : SPDINO1_H;
  int dy = GROUND - dh - (int)D.y + (D.ducking ? 0 : 0);
  if (D.ducking) {
    display.drawXBM(DINO_X, GROUND - SPDINODUCK_H + 1, SPDINODUCK_W, SPDINODUCK_H, spDinoDuck);
  } else if (D.y > 0.1f) {
    display.drawXBM(DINO_X, dy, SPDINO1_W, SPDINO1_H, spDino1);       // legs together mid-air
  } else {
    bool frame = ((millis() / 110) & 1);
    display.drawXBM(DINO_X, dy, SPDINO1_W, SPDINO1_H, frame ? spDino1 : spDino2);
  }

  for (uint8_t i = 0; i < 4; i++) {
    if (!D.obs[i].active) continue;
    Obs& o = D.obs[i];
    if (o.type == 2) {
      bool flap = ((millis() / 160) & 1);
      display.drawXBM(o.x, BIRD_Y, SPBIRD1_W, SPBIRD1_H, flap ? spBird1 : spBird2);
    } else if (o.type == 1) {
      display.drawXBM(o.x, GROUND - SPCACTUSL_H + 1, SPCACTUSL_W, SPCACTUSL_H, spCactusL);
    } else {
      display.drawXBM(o.x, GROUND - SPCACTUSS_H + 1, SPCACTUSS_W, SPCACTUSS_H, spCactusS);
    }
  }

  if (D.over) gameOverBanner("GAME OVER", "both=again hold=exit");
  display.sendBuffer();
}

void dinoTick(BtnEvent ev) {
  if (ev == EV_EXIT) { goState(ST_GAMEMENU); return; }
  if (D.over) {
    if (ev == EV_SELECT) dinoEnter();
    dinoDraw();
    return;
  }

  uint32_t now = millis();
  if (now - D.last < 30) return;
  D.last = now;

  if (Btn::down(0) && D.y == 0 && D.vy == 0) D.vy = 4.9f;
  D.ducking = Btn::down(1) && D.y == 0;

  D.y += D.vy;
  D.vy -= 0.42f;
  if (D.y <= 0) { D.y = 0; D.vy = 0; }

  D.speed = 2.0f + D.score / 500.0f;
  if (D.speed > 5.5f) D.speed = 5.5f;

  uint32_t gap = (uint32_t)(900 - D.speed * 90);
  if (now - D.lastSpawn > gap + (uint32_t)random(0, 500)) { dinoSpawn(); D.lastSpawn = now; }

  D.scroll += D.speed;

  // Hit box is inset from the sprite so near-misses feel fair rather than cheap.
  int dh = D.ducking ? SPDINODUCK_H : SPDINO1_H;
  int dw = D.ducking ? 15 : 11;
  int dy = D.ducking ? (GROUND - SPDINODUCK_H + 1) : (GROUND - dh - (int)D.y);
  if (D.ducking) { dy += 6; dh -= 6; }          // the duck sprite is mostly empty up top

  for (uint8_t i = 0; i < 4; i++) {
    if (!D.obs[i].active) continue;
    Obs& o = D.obs[i];
    o.x -= (int16_t)D.speed;
    if (o.x + o.w < -4) { o.active = false; continue; }

    int oy = (o.type == 2) ? BIRD_Y : (GROUND - o.h + 1);
    int ox = o.x + 1, ow = o.w - 2;             // 1px forgiveness each side
    bool hit = (DINO_X + 2 < ox + ow) && (DINO_X + dw > ox) &&
               (dy < oy + o.h) && (dy + dh > oy);
    if (hit) { D.over = true; break; }
  }

  D.score += 1;
  dinoDraw();
}
