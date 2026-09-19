#include "faces.h"
#include "ui.h"
#include "notifications.h"

static const char* DOW[7]  = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
static const char* MON[12] = { "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC" };

static void unreadMark(int16_t x, int16_t y) {
  if (Notifs::unread()) uiBell(x, y);
}

// Deterministic pseudo-random. Same inputs always give the same byte, so stars
// and glyphs stay put between frames without storing anything.
static inline uint8_t hash8(uint16_t a, uint16_t b) {
  uint32_t h = a * 73856093u ^ b * 19349663u;
  h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
  return (uint8_t)h;
}
static inline float ease(float t) { return t * t * (3.0f - 2.0f * t); }
static inline float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }

// ---------------------------------------------------------------- face 1 ---
// Bold: big digits, blinking colon, seconds creeping along the bottom.
static void faceBold(const DateTime& n) {
  char t[8], d[20];
  snprintf(t, sizeof(t), "%02d%c%02d", n.hour(), (n.second() & 1) ? ' ' : ':', n.minute());
  snprintf(d, sizeof(d), "%s %02d %s", DOW[n.dayOfTheWeek()], n.day(), MON[n.month() - 1]);

  display.setFont(F_HUGE);
  uiCentre(t, 64, 28);

  display.setFont(F_BODY);
  uiCentre(d, 64, 52);

  // seconds as a thin line growing left to right
  int w = (int)(n.second() * 126 / 59);
  display.drawHLine(1, 62, w > 0 ? w : 1);

  unreadMark(4, 1);
}

// ---------------------------------------------------------------- face 2 ---
static void faceSeconds(const DateTime& n) {
  char hm[8], ss[4], d[16];
  snprintf(hm, sizeof(hm), "%02d:%02d", n.hour(), n.minute());
  snprintf(ss, sizeof(ss), "%02d", n.second());
  snprintf(d,  sizeof(d),  "%s %02d/%02d", DOW[n.dayOfTheWeek()], n.day(), n.month());

  display.setFont(F_HUGE);
  uiLeft(hm, 2, 26);

  display.setFont(F_MED);
  uiRight(ss, 126, 36);

  display.setFont(F_BODY);
  uiLeft(d, 2, 54);

  int w = (int)(n.second() * 120 / 59);
  display.drawFrame(2, 59, 124, 4);
  if (w > 0) display.drawBox(3, 60, w, 2);

  unreadMark(4, 1);
}

// ---------------------------------------------------------------- face 3 ---
// Round analog dial with a date panel.
static void faceAnalog(const DateTime& n) {
  const int cx = 40, cy = 34, r = 28;
  display.drawCircle(cx, cy, r);
  display.drawCircle(cx, cy, r - 1);

  for (uint8_t i = 0; i < 12; i++) {
    float a = i * PI / 6.0f;
    int r1 = (i % 3 == 0) ? r - 6 : r - 3;
    display.drawLine(cx + (int)(r1 * sinf(a)), cy - (int)(r1 * cosf(a)),
                     cx + (int)((r - 2) * sinf(a)), cy - (int)((r - 2) * cosf(a)));
  }

  float sa = n.second() * PI / 30.0f;
  float ma = (n.minute() + n.second() / 60.0f) * PI / 30.0f;
  float ha = ((n.hour() % 12) + n.minute() / 60.0f) * PI / 6.0f;

  display.drawLine(cx,     cy, cx     + (int)(13 * sinf(ha)), cy - (int)(13 * cosf(ha)));
  display.drawLine(cx + 1, cy, cx + 1 + (int)(13 * sinf(ha)), cy - (int)(13 * cosf(ha)));
  display.drawLine(cx, cy, cx + (int)(21 * sinf(ma)), cy - (int)(21 * cosf(ma)));
  display.drawLine(cx, cy, cx + (int)(24 * sinf(sa)), cy - (int)(24 * cosf(sa)));
  // counterweight on the second hand
  display.drawLine(cx, cy, cx - (int)(6 * sinf(sa)), cy + (int)(6 * cosf(sa)));
  display.drawDisc(cx, cy, 2);

  char l1[6], l2[6], l3[6];
  snprintf(l1, sizeof(l1), "%s", DOW[n.dayOfTheWeek()]);
  snprintf(l2, sizeof(l2), "%02d", n.day());
  snprintf(l3, sizeof(l3), "%s", MON[n.month() - 1]);

  display.setFont(F_TINY);
  uiCentre(l1, 102, 22);
  display.setFont(F_MED);
  uiCentre(l2, 102, 38);
  display.setFont(F_TINY);
  uiCentre(l3, 102, 52);

  unreadMark(2, 2);
}

// ---------------------------------------------------------------- face 4 ---
// Square analog: the dial is the whole screen. Hands reach right out to the
// edge of the panel, so their length changes with angle.
static void faceSquare(const DateTime& n) {
  const float cx = 63.5f, cy = 31.5f, hw = 62.0f, hh = 30.0f;

  // distance from centre to the rectangle edge along angle a (0 = 12 o'clock)
  auto edge = [&](float a) -> float {
    float s = fabsf(sinf(a)), c = fabsf(cosf(a));
    float k = 1e6f;
    if (s > 0.001f) k = fminf(k, hw / s);
    if (c > 0.001f) k = fminf(k, hh / c);
    return k;
  };
  auto hand = [&](float a, float frac, float back) {
    float k = edge(a) * frac;
    display.drawLine((int)(cx - sinf(a) * back), (int)(cy + cosf(a) * back),
                     (int)(cx + sinf(a) * k),    (int)(cy - cosf(a) * k));
  };

  display.drawFrame(0, 0, 128, 64);

  // hour markers sitting on the perimeter
  for (uint8_t i = 0; i < 12; i++) {
    float a = i * PI / 6.0f;
    float k = edge(a);
    float inner = k - ((i % 3 == 0) ? 7.0f : 4.0f);
    display.drawLine((int)(cx + sinf(a) * inner), (int)(cy - cosf(a) * inner),
                     (int)(cx + sinf(a) * (k - 1)), (int)(cy - cosf(a) * (k - 1)));
  }

  float sa = n.second() * PI / 30.0f;
  float ma = (n.minute() + n.second() / 60.0f) * PI / 30.0f;
  float ha = ((n.hour() % 12) + n.minute() / 60.0f) * PI / 6.0f;

  hand(ha, 0.52f, 0);        // hour, drawn twice for weight
  hand(ha + 0.012f, 0.52f, 0);
  hand(ma, 0.80f, 0);
  hand(sa, 0.92f, 7);        // second, with a tail past the centre

  display.drawDisc((int)cx, (int)cy, 2);

  char d[14];
  snprintf(d, sizeof(d), "%s %02d", DOW[n.dayOfTheWeek()], n.day());
  display.setFont(F_TINY);
  display.setDrawColor(0);
  display.drawBox(46, 44, display.getStrWidth(d) + 4, 9);   // punch a hole for it
  display.setDrawColor(1);
  display.drawStr(48, 51, d);

  unreadMark(4, 4);
}

// ---------------------------------------------------------------- face 5 ---
// Space. Cycles through three scenes: Earth with its moon in orbit, the moon
// spiralling in and hitting, and a pull back to the whole galaxy.
// Wireframe globe. The key detail is which axis the meridian's width follows:
// a meridian at longitude a projects to a half-ellipse of horizontal radius
// r*sin(a), and is on the near side when cos(a) > 0. So as the phase advances a
// meridian enters at one limb, sweeps across the face through a vertical line
// at the centre, and leaves at the other limb. No culling threshold, so nothing
// pops - they just flow one way.
#define GLOBE_MERIDIANS 8

static void drawStars(uint8_t n, int16_t exclX, int16_t exclY, int16_t exclR) {
  uint16_t tw = (uint16_t)(millis() / 320);
  for (uint8_t i = 0; i < n; i++) {
    int16_t x = 2 + (hash8(i, 1) % 124);
    int16_t y = 1 + (hash8(i, 2) % 62);
    int16_t dx = x - exclX, dy = y - exclY;
    if (exclR && (dx * dx + dy * dy) < exclR * exclR) continue;
    if ((hash8(i, tw) & 7) == 0) continue;            // twinkle out now and then
    display.drawPixel(x, y);
  }
}

static void drawEarth(float cx, float cy, float r, float phase) {
  if (r < 3.0f) { display.drawDisc((int)cx, (int)cy, r < 1 ? 1 : (int)r); return; }
  display.drawCircle((int)cx, (int)cy, (int)r);

  // latitude circles: viewed from the equator these project to straight chords
  for (int k = -2; k <= 2; k++) {
    if (k == 0) continue;
    float lat = k * 27.0f * DEG_TO_RAD;
    int yy = (int)(cy - r * sinf(lat));
    int xr = (int)(r * cosf(lat));
    if (xr > 1) display.drawHLine((int)cx - xr, yy, 2 * xr);
  }
  display.drawHLine((int)cx - (int)r, (int)cy, 2 * (int)r);     // equator

  // meridians, near side only, sweeping smoothly limb to limb
  for (uint8_t m = 0; m < GLOBE_MERIDIANS; m++) {
    float a = phase + m * (TWO_PI / GLOBE_MERIDIANS);
    if (cosf(a) <= 0.0f) continue;                    // far side of the globe
    float rx = r * sinf(a);
    int px = 0, py = 0; bool first = true;
    for (uint8_t i = 0; i <= 18; i++) {
      float t = -HALF_PI + PI * (float)i / 18.0f;
      int x = (int)(cx + rx * cosf(t));
      int y = (int)(cy - r  * sinf(t));
      if (!first) display.drawLine(px, py, x, y);
      px = x; py = y; first = false;
    }
  }
}

static void drawGalaxy(float cx, float cy, float amount) {
  float rot  = millis() / 5000.0f;
  float maxR = 46.0f * amount;
  display.drawDisc((int)cx, (int)cy, 2);
  for (uint8_t arm = 0; arm < 3; arm++) {
    for (uint8_t i = 3; i <= 34; i++) {
      float t   = i / 34.0f;
      float ang = rot + arm * (TWO_PI / 3.0f) + t * 3.1f;
      float rr  = t * maxR;
      int x = (int)(cx + rr * cosf(ang) * 1.35f);
      int y = (int)(cy + rr * sinf(ang) * 0.72f);
      if (x < 0 || x > 127 || y < 0 || y > 63) continue;
      display.drawPixel(x, y);
      if ((hash8(arm, i) & 3) == 0 && x > 0) display.drawPixel(x - 1, y);
    }
  }
}

static void faceSpace(const DateTime& n) {
  // scene timeline, derived from millis() so nothing needs storing
  const uint32_t T_ORBIT1 = 18000, T_FALL = 7000, T_ORBIT2 = 14000, T_GALAXY = 12000;
  const uint32_t TOTAL = T_ORBIT1 + T_FALL + T_ORBIT2 + T_GALAXY;
  uint32_t t = millis() % TOTAL;

  float ex = 30, ey = 34, er = 19;
  float phase = (millis() % 9000) / 9000.0f * TWO_PI;

  if (t < T_ORBIT1 || (t >= T_ORBIT1 + T_FALL && t < T_ORBIT1 + T_FALL + T_ORBIT2)) {
    // ---- scene 1: ordinary orbit ----
    drawStars(14, (int16_t)ex, (int16_t)ey, (int16_t)er + 2);
    drawEarth(ex, ey, er, phase);
    float ma = (millis() % 11000) / 11000.0f * TWO_PI;
    display.drawDisc((int)(ex + 30 * cosf(ma)), (int)(ey + 15 * sinf(ma)), 2);

  } else if (t < T_ORBIT1 + T_FALL) {
    // ---- scene 2: the moon loses it ----
    float p = (float)(t - T_ORBIT1) / (float)T_FALL;
    drawStars(14, (int16_t)ex, (int16_t)ey, (int16_t)er + 2);

    if (p < 0.78f) {
      float q  = p / 0.78f;
      float mr = 30.0f * (1.0f - ease(q));
      float ma = q * TWO_PI * 3.2f;                    // winds up as it falls
      drawEarth(ex, ey, er, phase);
      display.drawDisc((int)(ex + mr * cosf(ma) * 1.6f),
                       (int)(ey + mr * sinf(ma)), mr > 8 ? 2 : 1);
      // motion trail
      for (uint8_t k = 1; k <= 3; k++) {
        float qq = q - k * 0.02f; if (qq < 0) break;
        float rr = 30.0f * (1.0f - ease(qq));
        float aa = qq * TWO_PI * 3.2f;
        display.drawPixel((int)(ex + rr * cosf(aa) * 1.6f), (int)(ey + rr * sinf(aa)));
      }
    } else {
      // impact: shake the planet and ring out a shockwave
      float q = (p - 0.78f) / 0.22f;
      float shake = (1.0f - q) * 2.0f;
      float sx = ex + ((hash8((uint16_t)(millis() / 40), 7) & 1) ? shake : -shake);
      drawEarth(sx, ey, er, phase);
      int ring = (int)(er + q * 26.0f);
      display.drawCircle((int)ex, (int)ey, ring);
      if (q < 0.6f) display.drawCircle((int)ex, (int)ey, ring / 2 + (int)er / 2);
    }

  } else {
    // ---- scene 3: pull back to the galaxy ----
    float p = (float)(t - T_ORBIT1 - T_FALL - T_ORBIT2) / (float)T_GALAXY;
    float out = (p < 0.25f) ? ease(p / 0.25f)
              : (p > 0.75f) ? ease((1.0f - p) / 0.25f) : 1.0f;
    float r = er - (er - 1.0f) * out;
    drawStars(20, 0, 0, 0);
    if (out > 0.15f) drawGalaxy(64, 32, clamp01(out));
    drawEarth(ex + (64 - ex) * out, ey + (32 - ey) * out, r, phase);
  }

  // time panel, punched out of whatever is behind it
  char tm[8], d[14];
  snprintf(tm, sizeof(tm), "%02d:%02d", n.hour(), n.minute());
  snprintf(d,  sizeof(d),  "%s %02d.%02d", DOW[n.dayOfTheWeek()], n.day(), n.month());
  display.setDrawColor(0);
  display.drawBox(72, 18, 56, 30);
  display.setDrawColor(1);
  display.setFont(F_LARGE);
  uiRight(tm, 126, 30);
  display.setFont(F_TINY);
  uiRight(d, 126, 44);

  unreadMark(54, 2);
}

// ---------------------------------------------------------------- face 6 ---
// A solid tumbling beside a mono clock. Swaps between a cube and an octahedron
// every so often, with a spin-up through the change.
static const int8_t CUBE_V[8][3] = {
  {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
  {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}
};
static const uint8_t CUBE_E[12][2] = {
  {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
};
static const int8_t OCT_V[6][3] = {
  { 0, 0,-1},{ 0, 0, 1},{-1, 0, 0},{ 1, 0, 0},{ 0,-1, 0},{ 0, 1, 0}
};
static const uint8_t OCT_E[12][2] = {
  {0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{4,3},{3,5},{5,2}
};

static void faceCube(const DateTime& n) {
  const uint32_t PERIOD = 12000;
  uint32_t t   = millis() % (PERIOD * 2);
  bool     oct = (t >= PERIOD);
  float    loc = (float)(t % PERIOD) / (float)PERIOD;
  // spin faster either side of a swap
  float boost = 1.0f + 2.2f * (loc > 0.94f ? (loc - 0.94f) / 0.06f
                             : loc < 0.06f ? (0.06f - loc) / 0.06f : 0.0f);

  float ax = (millis() % 7000) / 7000.0f * TWO_PI * boost;
  float ay = (millis() % 4300) / 4300.0f * TWO_PI * boost;
  const int cx = 96, cy = 32;
  const float scale = 22.0f, dist = 3.2f;

  int px[8], py[8];
  uint8_t nv = oct ? 6 : 8;
  for (uint8_t i = 0; i < nv; i++) {
    float x = oct ? OCT_V[i][0] : CUBE_V[i][0];
    float y = oct ? OCT_V[i][1] : CUBE_V[i][1];
    float z = oct ? OCT_V[i][2] : CUBE_V[i][2];
    if (oct) { x *= 1.35f; y *= 1.35f; z *= 1.35f; }
    float y2 = y * cosf(ax) - z * sinf(ax);
    float z2 = y * sinf(ax) + z * cosf(ax);
    float x2 = x * cosf(ay) + z2 * sinf(ay);
    float z3 = -x * sinf(ay) + z2 * cosf(ay);
    float f  = scale / (dist + z3 * 0.55f) * 1.6f;
    px[i] = cx + (int)(x2 * f);
    py[i] = cy + (int)(y2 * f);
  }
  for (uint8_t e = 0; e < 12; e++) {
    uint8_t a = oct ? OCT_E[e][0] : CUBE_E[e][0];
    uint8_t b = oct ? OCT_E[e][1] : CUBE_E[e][1];
    display.drawLine(px[a], py[a], px[b], py[b]);
  }
  // shadow puddle, breathing with the spin
  int sw = 10 + (int)(4 * sinf(ax * 2.0f));
  display.drawHLine(cx - sw, 58, sw * 2);

  char hh[4], mm[4], d[10];
  snprintf(hh, sizeof(hh), "%02d", n.hour());
  snprintf(mm, sizeof(mm), "%02d", n.minute());
  snprintf(d,  sizeof(d),  "%02d/%02d", n.day(), n.month());

  display.setFont(F_MONOB);
  uiLeft(hh, 6, 22);
  uiLeft(mm, 6, 42);
  display.setFont(F_TINY);
  uiLeft(d, 6, 57);
  display.drawVLine(56, 8, 48);
  if (n.second() & 1) display.drawDisc(50, 32, 1);

  unreadMark(20, 3);
}

// ---------------------------------------------------------------- face 7 ---
static const uint8_t SEGMAP[10] = {
  0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
  0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
};

static void drawSeg(int x, int y, int w, int h, int t, uint8_t v) {
  uint8_t m = SEGMAP[v % 10];
  int half = (h - 3 * t) / 2;
  if (m & 0b0000001) display.drawBox(x + t,     y,                w - 2 * t, t);
  if (m & 0b0000010) display.drawBox(x + w - t, y + t,            t,         half);
  if (m & 0b0000100) display.drawBox(x + w - t, y + 2 * t + half, t,         half);
  if (m & 0b0001000) display.drawBox(x + t,     y + h - t,        w - 2 * t, t);
  if (m & 0b0010000) display.drawBox(x,         y + 2 * t + half, t,         half);
  if (m & 0b0100000) display.drawBox(x,         y + t,            t,         half);
  if (m & 0b1000000) display.drawBox(x + t,     y + t + half,     w - 2 * t, t);
}

static void faceSevenSeg(const DateTime& n) {
  const int w = 22, h = 38, t = 4, y = 14;
  int x = 6;
  drawSeg(x,              y, w, h, t, n.hour() / 10);
  drawSeg(x + w + 4,      y, w, h, t, n.hour() % 10);
  if (n.second() % 2 == 0) {
    display.drawBox(x + 2 * w + 11, y + 10, 4, 4);
    display.drawBox(x + 2 * w + 11, y + 24, 4, 4);
  }
  drawSeg(x + 2 * w + 20, y, w, h, t, n.minute() / 10);
  drawSeg(x + 3 * w + 24, y, w, h, t, n.minute() % 10);

  // scanline drifting down the panel, like a tired VFD
  int sl = 12 + (int)((millis() / 55) % 40);
  display.setDrawColor(2);                 // XOR, so it cuts through the digits
  display.drawHLine(4, sl, 120);
  display.setDrawColor(1);

  char d[24];
  snprintf(d, sizeof(d), "%s %02d %s %02d", DOW[n.dayOfTheWeek()], n.day(),
           MON[n.month() - 1], n.second());
  display.setFont(F_TINY);
  uiCentre(d, 64, 59);

  unreadMark(4, 1);
}

// ---------------------------------------------------------------- face 8 ---
static void faceTerminal(const DateTime& n) {
  display.setFont(F_TINY);
  display.drawStr(0, 7, "ESP32C3://watch");
  display.drawHLine(0, 9, 128);

  char t[12];
  snprintf(t, sizeof(t), "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
  display.setFont(F_MONOB);
  uiCentre(t, 64, 26);

  char l[28];
  display.setFont(F_TINY);
  snprintf(l, sizeof(l), "> %s %02d.%02d.%04d", DOW[n.dayOfTheWeek()], n.day(),
           n.month(), n.year());
  display.drawStr(0, 45, l);
  snprintf(l, sizeof(l), "> msg %d  ble %s", Notifs::unread(),
           Notifs::connected() ? "up" : "--");
  display.drawStr(0, 54, l);
  if ((millis() / 500) & 1) display.drawBox(display.getStrWidth(l) + 2, 48, 4, 7);

  long secs = (long)n.hour() * 3600L + n.minute() * 60L + n.second();
  int w = (int)(secs * 124L / 86400L);
  display.drawFrame(0, 58, 128, 5);
  if (w > 0) display.drawBox(2, 60, w, 1);
}

// ---------------------------------------------------------------- face 9 ---
// Hack: glyph rain down the screen with the clock cut out of the middle.
static const char RAIN[] = "01@#$%&*+=<>/\\|{}[]ABCDEFXYZ";
#define RAIN_N (sizeof(RAIN) - 1)
#define RAIN_COLS 21

static void faceMatrix(const DateTime& n) {
  uint32_t tick = millis() / 90;

  display.setFont(F_TINY);
  for (uint8_t c = 0; c < RAIN_COLS; c++) {
    uint8_t speed = 1 + (hash8(c, 11) % 3);
    uint8_t len   = 3 + (hash8(c, 12) % 4);
    int8_t  head  = (int8_t)(((tick * speed) / 2 + hash8(c, 13)) % 20) - 2;

    for (uint8_t k = 0; k < len; k++) {
      int8_t row = head - k;
      if (row < 0 || row > 7) continue;
      // the trail thins out behind the head
      if (k > 1 && (hash8(c, row * 7 + k) & 1)) continue;
      char ch[2] = { RAIN[hash8(c, row + (uint16_t)(tick / (4 + k))) % RAIN_N], 0 };
      display.drawStr(c * 6 + 1, row * 8 + 7, ch);
    }
  }

  // clock knocked out of the rain
  display.setDrawColor(0);
  display.drawBox(10, 18, 108, 28);
  display.setDrawColor(1);
  display.drawFrame(10, 18, 108, 28);

  char t[12];
  snprintf(t, sizeof(t), "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
  display.setFont(F_MED);
  uiCentre(t, 64, 30);

  char l[26];
  snprintf(l, sizeof(l), "root@watch:~# %s", (millis() / 400) & 1 ? "_" : "");
  display.setFont(F_TINY);
  display.setDrawColor(0);
  display.drawBox(10, 38, 108, 8);
  display.setDrawColor(1);
  display.drawStr(14, 45, l);

  unreadMark(120, 2);
}

// --------------------------------------------------------------- table -----
const Face FACES[] = {
  { "Bold",      faceBold,       500 },
  { "Seconds",   faceSeconds,    500 },
  { "Analog",    faceAnalog,     500 },
  { "Square",    faceSquare,     250 },
  { "Space",     faceSpace,       70 },
  { "Cube 3D",   faceCube,        90 },
  { "Seven Seg", faceSevenSeg,    60 },
  { "Terminal",  faceTerminal,   250 },
  { "Hack",      faceMatrix,      90 },
};
const uint8_t FACE_COUNT = sizeof(FACES) / sizeof(FACES[0]);
