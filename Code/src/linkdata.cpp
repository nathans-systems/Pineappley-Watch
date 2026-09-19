#include "linkdata.h"
#include "watch.h"

// Survives deep sleep, so the last reading is on screen the moment you wake the
// watch rather than blank until the phone reconnects.
// Packet layout, confirmed against real Gadgetbridge traffic:
//   [0]      message type, 0 = current conditions
//   [1]      version
//   [2..9]   unix timestamp, uint64 little endian
//   [10,11]  temperature, int16 LE, hundredths of a degree C
//   [12,13]  minimum      [14,15] maximum
//   [16..47] city name, 32 bytes, null padded
//   [48]     icon id
#define WX_OFF_TEMP  10
#define WX_OFF_MIN   12
#define WX_OFF_MAX   14
#define WX_OFF_PLACE 16
#define WX_PLACE_LEN 32
#define WX_OFF_ICON  48

RTC_DATA_ATTR static WeatherData s_wx;
RTC_DATA_ATTR static uint32_t    s_linkMagic = 0;
#define LINK_MAGIC 0x4C4E4B33UL   // bumped: WeatherData layout changed

void Link::begin() {
  if (s_linkMagic != LINK_MAGIC) {
    s_linkMagic = LINK_MAGIC;
    s_wx = WeatherData{};
  }
}

const WeatherData& Link::weather() { return s_wx; }

// Hundredths of a degree C, so 1700 = 17.0C. The Kelvin branch is a guard for
// companion apps that send absolute temperatures instead; it can only trigger
// on values that would otherwise mean 150-360 C, which is not weather.
static int16_t toTenthsC(int16_t raw) {
  float k = raw / 100.0f;
  if (k > 150.0f && k < 360.0f) return (int16_t)lroundf((k - 273.15f) * 10.0f);
  return (int16_t)(raw / 10);
}

const char* Link::iconName(uint8_t i) {
  switch (i) {
    // InfiniTime's SimpleWeatherService icon ids
    case 0:  return "Clear";
    case 1:  return "Sun + cloud";
    case 2:  return "Cloudy";
    case 3:  return "Overcast";
    case 4:  return "Heavy rain";
    case 5:  return "Showers";
    case 6:  return "Thunder";
    case 7:  return "Snow";
    case 8:  return "Smog";
    case 255:return "Unknown";
    default: break;
  }
  static char buf[12];
  snprintf(buf, sizeof(buf), "icon %u", (unsigned)i);   // so it can be mapped
  return buf;
}

// ---------------------------------------------------------------------------
// Parse an InfiniTime "simple weather" packet. Offsets are defined at the top
// of this file. The raw bytes are still logged, so if a future Gadgetbridge
// changes the format the evidence is right there in the serial output.
// ---------------------------------------------------------------------------
void Link::onWeatherPacket(const uint8_t* d, size_t len) {
  Serial.printf("[WX] raw (%u):", (unsigned)len);
  for (size_t i = 0; i < len && i < 40; i++) Serial.printf(" %02X", d[i]);
  Serial.println();

  if (len < 16 || d[0] != 0) return;      // not a current-conditions packet

  int16_t t   = (int16_t)(d[WX_OFF_TEMP] | (d[WX_OFF_TEMP + 1] << 8));
  int16_t tmn = (int16_t)(d[WX_OFF_MIN]  | (d[WX_OFF_MIN  + 1] << 8));
  int16_t tmx = (int16_t)(d[WX_OFF_MAX]  | (d[WX_OFF_MAX  + 1] << 8));

  s_wx.tempC10 = toTenthsC(t);
  s_wx.minC10  = toTenthsC(tmn);
  s_wx.maxC10  = toTenthsC(tmx);
  s_wx.icon    = (len > WX_OFF_ICON) ? d[WX_OFF_ICON] : 255;

  memset(s_wx.place, 0, sizeof(s_wx.place));
  for (uint8_t i = 0; i < WX_PLACE_LEN && i < sizeof(s_wx.place) - 1; i++) {
    size_t o = WX_OFF_PLACE + i;
    if (o >= len) break;
    char ch = (char)d[o];
    if (ch == 0) break;
    s_wx.place[i] = (ch >= 32 && ch < 127) ? ch : ' ';
  }

  s_wx.valid = true;
  s_wx.epoch = rtcOk ? rtc.now().unixtime() : 0;
  gDirty = true;
  Serial.printf("[WX] %.1fC (%.1f/%.1f) %s %s\n",
                s_wx.tempC10 / 10.0f, s_wx.minC10 / 10.0f, s_wx.maxC10 / 10.0f,
                iconName(s_wx.icon), s_wx.place);
}
