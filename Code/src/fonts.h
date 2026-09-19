#pragma once
#include <U8g2lib.h>

// Every font used anywhere in the firmware is named here. If one of these
// identifiers doesn't exist in your U8g2 version the compiler will say so,
// and you fix it in one place rather than hunting through the faces.
//
// Naming: _tr = ASCII, _tf = full latin, _tn = DIGITS AND PUNCTUATION ONLY.
// Do not pass letters to an _tn font, you'll get blanks.

#define F_HUGE    u8g2_font_logisoso30_tn   // big clock digits, "12:34"
#define F_LARGE   u8g2_font_logisoso20_tn   // medium clock digits
#define F_BIG     u8g2_font_helvB18_tr
#define F_MED     u8g2_font_helvB12_tr
#define F_SMALLB  u8g2_font_helvB08_tr
#define F_BODY    u8g2_font_6x10_tf         // 6px per char, list/menu text
#define F_TINY    u8g2_font_5x7_tf
#define F_MONO    u8g2_font_profont12_tf
#define F_MONOB   u8g2_font_courB14_tr
