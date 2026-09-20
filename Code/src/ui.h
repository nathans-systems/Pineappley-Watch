#pragma once
#include "watch.h"
#include "fonts.h"

// --- text (U8g2 draws from the BASELINE; these take a centre point) ---
void     uiCentre(const char* s, int16_t cx, int16_t cy);
void     uiLeft  (const char* s, int16_t lx, int16_t cy);
void     uiRight (const char* s, int16_t rx, int16_t cy);
int16_t  uiBaseline(int16_t cy);

// --- chrome ---
void uiListFrame(const char* title);
void uiList(const char* const* items, uint8_t n, uint8_t sel, uint8_t top, uint8_t rows = 4);
void uiScrollClamp(uint8_t n, uint8_t sel, uint8_t& top, uint8_t rows = 4);
// Draws the selection indicator for a list row and returns the x offset the
// row text should start at. Leaves the draw colour set correctly for the text.
int16_t uiRowSelect(int16_t yt, bool selected);

// Cursor snapshot, used to make double-tap select exact. A screen calls
// navMark(sel) immediately BEFORE it changes sel on EV_DOWN; navUndo() puts the
// old value back. Snapshotting rather than reversing keeps it correct when the
// move was clamped at the end of a list and did not actually happen.
void navMark(uint8_t& v);
void navUndo();
void navClear();
void uiBell(int16_t x, int16_t y);
void uiWrapText(const char* s, int16_t x, int16_t yTop, int16_t w, int16_t lineH,
                uint8_t maxLines, uint8_t skipLines = 0);
