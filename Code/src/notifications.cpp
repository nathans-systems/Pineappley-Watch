#include "notifications.h"
#include "watch.h"
#include "linkdata.h"

#if ENABLE_BLE
#include <NimBLEDevice.h>
#endif

// The store lives in RTC slow memory so notifications received while awake
// survive deep sleep and are still there on the next wake.
RTC_DATA_ATTR static uint32_t s_magic = 0;
RTC_DATA_ATTR static Notif    s_buf[NOTIF_MAX];
RTC_DATA_ATTR static uint8_t  s_count  = 0;
RTC_DATA_ATTR static uint8_t  s_head   = 0;
RTC_DATA_ATTR static uint8_t  s_unread = 0;

#define STORE_MAGIC 0x57415431UL   // "WAT1"

static bool s_connected = false;

// ---------------------------------------------------------------- store ----
void Notifs::add(uint8_t cat, const char* title, const char* body) {
  Notif& n = s_buf[s_head];
  n.cat = cat;
  strncpy(n.title, title ? title : "", NOTIF_TITLE - 1); n.title[NOTIF_TITLE - 1] = 0;
  strncpy(n.body,  body  ? body  : "", NOTIF_BODY  - 1); n.body [NOTIF_BODY  - 1] = 0;
  n.stamp = rtcOk ? rtc.now().unixtime() : 0;

  s_head = (uint8_t)((s_head + 1) % NOTIF_MAX);
  if (s_count  < NOTIF_MAX) s_count++;
  if (s_unread < NOTIF_MAX) s_unread++;
  gDirty = true;
  Serial.printf("[NOTIF] %s | %s\n", n.title, n.body);
}

uint8_t Notifs::count()  { return s_count; }
uint8_t Notifs::unread() { return s_unread; }
void    Notifs::markAllRead() { s_unread = 0; }
void    Notifs::clear()  { s_count = 0; s_head = 0; s_unread = 0; }
bool    Notifs::connected() { return s_connected; }

const Notif& Notifs::get(uint8_t i) {
  if (i >= s_count) i = 0;
  int8_t idx = (int8_t)s_head - 1 - (int8_t)i;
  while (idx < 0) idx += NOTIF_MAX;
  return s_buf[idx];
}

const char* Notifs::categoryName(uint8_t cat) {
  switch (cat) {
    case 0:  return "General";
    case 1:  return "Call";
    case 2:  return "Missed";
    case 3:  return "SMS";
    case 4:  return "Voicemail";
    case 5:  return "Schedule";
    case 6:  return "Alert";
    case 7:  return "Alarm";
    case 8:  return "Battery";
    case 9:  return "Email";
    case 10: return "News";
    default: return "Notify";
  }
}

// ------------------------------------------------------------------ BLE ----
#if ENABLE_BLE

class SrvCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    s_connected = true; gDirty = true;
    Serial.println("[BLE] phone connected");
  }
  void onDisconnect(NimBLEServer*) override {
    s_connected = false; gDirty = true;
    Serial.println("[BLE] disconnected, re-advertising");
    NimBLEDevice::startAdvertising();
  }
};

// Alert Notification Service "New Alert" (0x2A46). Gadgetbridge/InfiniTime send:
//   [0] category, [1] count, [2] 0x00, [3..] title 0x00 body
// Parsed loosely because senders differ by a byte or two.
class AlertCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    const uint8_t* d = (const uint8_t*)v.data();
    size_t len = v.size();
    if (len == 0) return;

    Serial.print("[BLE] raw:");
    for (size_t i = 0; i < len && i < 48; i++) Serial.printf(" %02X", d[i]);
    Serial.println();

    uint8_t cat = d[0];
    size_t  p   = 1;
    if (len > 1) p = 2;
    if (len > 2 && d[2] == 0x00) p = 3;

    char title[NOTIF_TITLE] = {0};
    char body [NOTIF_BODY ] = {0};

    size_t ti = 0;
    while (p < len && d[p] != 0x00 && ti < NOTIF_TITLE - 1) title[ti++] = (char)d[p++];
    title[ti] = 0;
    if (p < len && d[p] == 0x00) p++;

    size_t bi = 0;
    while (p < len && bi < NOTIF_BODY - 1) {
      char ch = (char)d[p++];
      body[bi++] = (ch == '\n' || ch == '\r') ? ' ' : ch;
    }
    body[bi] = 0;

    if (ti == 0 && bi == 0) return;
    if (ti == 0) strncpy(title, Notifs::categoryName(cat), NOTIF_TITLE - 1);
    Notifs::add(cat, title, body);
  }
};

// InfiniTime weather service, written by Gadgetbridge when a weather provider
// is configured on the phone.
static const char* WX_SVC  = "00050000-78fc-48fe-8e23-433b3a1942d0";
static const char* WX_CHR  = "00050001-78fc-48fe-8e23-433b3a1942d0";

class WeatherCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    Link::onWeatherPacket((const uint8_t*)v.data(), v.size());
  }
};

static void startBle() {
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setMTU(185);
#if BLE_BONDING
  // Store pairing keys so the phone can come back without re-pairing.
  NimBLEDevice::setSecurityAuth(true, false, true);
#endif
#if BLE_LOW_TX_POWER
  // Cuts the transmit current spike at the cost of range. Delete this line if
  // your NimBLE version doesn't accept it.
  NimBLEDevice::setPower(BLE_TX_LEVEL);
#endif

  NimBLEServer* srv = NimBLEDevice::createServer();
  srv->setCallbacks(new SrvCB());

  NimBLEService* ans = srv->createService(NimBLEUUID((uint16_t)0x1811));
  NimBLECharacteristic* newAlert = ans->createCharacteristic(
      NimBLEUUID((uint16_t)0x2A46),
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  newAlert->setCallbacks(new AlertCB());

  uint8_t all[2] = { 0xFF, 0xFF };
  ans->createCharacteristic(NimBLEUUID((uint16_t)0x2A47), NIMBLE_PROPERTY::READ)
     ->setValue(all, 2);
  ans->createCharacteristic(NimBLEUUID((uint16_t)0x2A48), NIMBLE_PROPERTY::READ)
     ->setValue(all, 2);
  ans->createCharacteristic(NimBLEUUID((uint16_t)0x2A44),
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  ans->start();

  // Weather (InfiniTime custom service)
  NimBLEService* wx = srv->createService(NimBLEUUID::fromString(WX_SVC));
  wx->createCharacteristic(NimBLEUUID::fromString(WX_CHR),
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR)
    ->setCallbacks(new WeatherCB());
  wx->start();

  NimBLEService* dis = srv->createService(NimBLEUUID((uint16_t)0x180A));
  dis->createCharacteristic(NimBLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ)
     ->setValue(std::string("DIY"));
  dis->createCharacteristic(NimBLEUUID((uint16_t)0x2A24), NIMBLE_PROPERTY::READ)
     ->setValue(std::string("ESP32C3-Watch"));
  dis->createCharacteristic(NimBLEUUID((uint16_t)0x2A26), NIMBLE_PROPERTY::READ)
     ->setValue(std::string(BLE_FW_REV));
  dis->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NimBLEUUID((uint16_t)0x1811));
  adv->setScanResponse(true);
  // Connection interval. Was 7.5-22.5ms, which woke the radio ~130x/sec for no
  // good reason on a device that only carries notifications.
  adv->setMinPreferred(BLE_CONN_MIN);
  adv->setMaxPreferred(BLE_CONN_MAX);
  // Advertising interval. Runs continuously while disconnected, so this is the
  // idle radio cost. Slower = cheaper, at the price of reconnect latency.
  adv->setMinInterval(BLE_ADV_MIN);
  adv->setMaxInterval(BLE_ADV_MAX);
  NimBLEDevice::startAdvertising();
  Serial.println("[BLE] advertising as " BLE_DEVICE_NAME);
}
#endif // ENABLE_BLE

void Notifs::begin() {
  if (s_magic != STORE_MAGIC) { s_magic = STORE_MAGIC; clear(); }
  Link::begin();
#if ENABLE_BLE
  startBle();
#endif
}

void Notifs::stop() {
#if ENABLE_BLE
  NimBLEDevice::deinit(true);
  s_connected = false;
#endif
}

void Notifs::loop() { /* NimBLE runs on its own task */ }
