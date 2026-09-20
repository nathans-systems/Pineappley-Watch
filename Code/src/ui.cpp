#include "ui.h"

// ------------------------------------------------------------------- text ---
// U8g2 positions text by its baseline. Glyph box height is ascent - descent
// (descent is negative), so to centre that box on cy the baseline sits at
// cy + (ascent + descent) / 2.
int16_t uiBaseline(int16_t cy) {
  int a = display.getAscent();
  int d = display.getDescent();
  return (int16_t)(cy + (a + d) / 2);
}

void uiCentre(const char* s, int16_t cx, int16_t cy) {
  int16_t w = (int16_t)display.getStrWidth(s);
  display.drawStr(cx - w / 2, uiBaseline(cy), s);
}

void uiLeft(const char* s, int16_t lx, int16_t cy) {
  display.drawStr(lx, uiBaseline(cy), s);
}

void uiRight(const char* s, int16_t rx, int16_t cy) {
  int16_t w = (int16_t)display.getStrWidth(s);
  display.drawStr(rx - w, uiBaseline(cy), s);
}

// ----------------------------------------------------------------- chrome ---
static uint8_t* s_navPtr = nullptr;
static uint8_t  s_navVal = 0;

void navMark(uint8_t& v) { s_navPtr = &v; s_navVal = v; }
void navClear()          { s_navPtr = nullptr; }
void navUndo() {
  if (s_navPtr) { *s_navPtr = s_navVal; s_navPtr = nullptr; }
}

int16_t uiRowSelect(int16_t yt, bool selected) {
#if UI_SOLID_SELECTION
  if (selected) {
    display.setDrawColor(1);
    display.drawBox(0, yt, 128, 12);
    display.setDrawColor(0);
  } else {
    display.setDrawColor(1);
  }
  return 4;
#else
  display.setDrawColor(1);
  if (selected) {
    display.drawFrame(0, yt, 128, 12);
    display.drawStr(3, yt + 9, ">");
  }
  return 11;
#endif
}

void uiBell(int16_t x, int16_t y) {
  display.setDrawColor(1);
  display.drawVLine(x + 3, y, 2);
  display.drawHLine(x + 1, y + 2, 5);
  display.drawHLine(x,     y + 3, 7);
  display.drawHLine(x,     y + 4, 7);
  display.drawPixel(x + 3, y + 6);
}

void uiListFrame(const char* title) {
  display.clearBuffer();
  display.setDrawColor(1);
  display.setFont(F_TINY);
  display.drawStr(2, 7, title);
  display.drawHLine(0, 10, 128);
}

void uiScrollClamp(uint8_t n, uint8_t sel, uint8_t& top, uint8_t rows) {
  if (n <= rows) { top = 0; return; }
  if (sel < top) top = sel;
  if (sel >= top + rows) top = (uint8_t)(sel - rows + 1);
  if (top > n - rows) top = (uint8_t)(n - rows);
}

void uiList(const char* const* items, uint8_t n, uint8_t sel, uint8_t top, uint8_t rows) {
  display.setFont(F_BODY);
  for (uint8_t r = 0; r < rows; r++) {
    uint8_t i = (uint8_t)(top + r);
    if (i >= n) break;
    int16_t yt = (int16_t)(12 + r * 13);
    int16_t tx = uiRowSelect(yt, i == sel);
    display.drawStr(tx, yt + 9, items[i]);
    display.setDrawColor(1);
  }
  if (n > rows) {
    int16_t h = (int16_t)(52 * rows / n); if (h < 6) h = 6;
    int16_t y = (int16_t)(12 + (52 - h) * top / (n - rows));
    display.drawVLine(126, 12, 52);
    display.drawBox(125, y, 3, h);
  }
}

void uiWrapText(const char* s, int16_t x, int16_t yTop, int16_t w, int16_t lineH,
                uint8_t maxLines, uint8_t skipLines) {
  display.setFont(F_BODY);          // 6x10: exactly 6px per character
  display.setDrawColor(1);
  const uint8_t cpl = (uint8_t)(w / 6);
  char line[32];
  uint8_t drawn = 0, produced = 0, li = 0;
  size_t i = 0, len = strlen(s);

  while (i <= len && drawn < maxLines) {
    if (li >= cpl || i == len || s[i] == '\n') {
      line[li] = 0;
      if (produced++ >= skipLines) {
        display.drawStr(x, yTop + drawn * lineH + 8, line);
        drawn++;
      }
      li = 0;
      if (i == len) break;
      if (s[i] == '\n') i++;
      continue;
    }
    if (li < (uint8_t)(sizeof(line) - 1)) line[li++] = s[i];
    i++;
  }
}
