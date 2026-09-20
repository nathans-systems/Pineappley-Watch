#include "watch.h"
#include "fonts.h"
#include "buttons.h"
#include "ui.h"
#include "screens.h"
#include "faces.h"
#include "notifications.h"
#include "calendar.h"
#include "settings.h"

#include <esp_system.h>
#include <esp_sleep.h>
#include <esp_idf_version.h>
#if __has_include(<esp_pm.h>)
  #include <esp_pm.h>
  #define WATCH_HAS_PM 1
  // IDF 4.x used a chip-specific name; IDF 5.x renamed it to the generic one.
  #if ESP_IDF_VERSION_MAJOR >= 5
    typedef esp_pm_config_t         watch_pm_config_t;
  #else
    typedef esp_pm_config_esp32c3_t watch_pm_config_t;
  #endif
#else
  #define WATCH_HAS_PM 0
#endif
#include <driver/gpio.h>

// ---------------------------------------------------------------- globals --
// Same constructor as the known-good sketch: U8g2 owns OLED_RST_PIN and pulses
// RES# itself inside begin(). Do not drive that pin by hand anywhere else.
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(U8G2_R0, OLED_RST_PIN);

RTC_DS3231  rtc;
Preferences prefs;
bool        rtcOk  = false;
AppState    gState = ST_FACE;
uint8_t     gFace  = 0;
bool        gDirty = true;
const char* gResetReason = "?";

static bool    screenLit      = true;
static uint8_t unreadAtOff    = 0;

// --------------------------------------------------------------- helpers ---
void goState(AppState s) {
  gState = s;
  Btn::flush();
  Btn::markActivity();
  screenEnter(s);
}

void saveSettings() {
  static uint8_t lastSaved = 255;
  if (lastSaved == gFace) return;
  prefs.putUChar("face", gFace);
  lastSaved = gFace;
}

DateTime nowSafe() {
  if (rtcOk) return rtc.now();
  static DateTime base(F(__DATE__), F(__TIME__));
  return base + TimeSpan((int32_t)(millis() / 1000UL));
}

// ----------------------------------------------------------- diagnostics ---
#if PRINT_RESET_REASON
static const char* resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT pin";
    case ESP_RST_SW:        return "SW restart";
    case ESP_RST_PANIC:     return "PANIC crash";
    case ESP_RST_INT_WDT:   return "INT watchdog";
    case ESP_RST_TASK_WDT:  return "TASK watchdog";
    case ESP_RST_WDT:       return "watchdog";
    case ESP_RST_DEEPSLEEP: return "wake";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "unknown";
  }
}
#endif

static void heartbeat() {
#if HEARTBEAT_MS
  static uint32_t last = 0;
  if (millis() - last < HEARTBEAT_MS) return;
  last = millis();
  Serial.printf("[HB] up=%lus heap=%u state=%d screen=%s ble=%s unread=%d\n",
                (unsigned long)(millis() / 1000UL),
                (unsigned)ESP.getFreeHeap(), (int)gState,
                screenLit ? "on" : "standby",
                Notifs::connected() ? "connected" : "advertising",
                Notifs::unread());
#endif
}

// ------------------------------------------------------- light sleep (PM) --
// Lets FreeRTOS drop the CPU into light sleep while idle. BLE keeps its
// connection through this, which is the only way notifications survive a sleep
// state on this chip. Returns quietly if power management isn't in the build.
static void configurePowerManagement() {
#if USE_LIGHT_SLEEP && WATCH_HAS_PM
  watch_pm_config_t pm = {};
  pm.max_freq_mhz       = PM_MAX_FREQ_MHZ;
  pm.min_freq_mhz       = PM_MIN_FREQ_MHZ;
  pm.light_sleep_enable = true;

  esp_err_t e = esp_pm_configure(&pm);
  if (e == ESP_OK) {
    Serial.println(F("[PWR] automatic light sleep enabled (BLE stays connected)"));
  } else {
    Serial.printf("[PWR] light sleep NOT available (%s). Standby idles at full "
                  "clock - works fine, just draws more.\n", esp_err_to_name(e));
  }
#elif USE_LIGHT_SLEEP
  Serial.println(F("[PWR] esp_pm.h not in this build, light sleep unavailable"));
#endif
}

// ------------------------------------------------------------ panel power --
// Order taken from the working sketch: boost on, settle, then I2C, then begin().
static void panelInit() {
  pinMode(OLED_EN_PIN, OUTPUT);
  digitalWrite(OLED_EN_PIN, HIGH);        // 12V rail on
  delay(50);                              // let the TPS61040 settle

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

#if OLED_BUS_CLOCK
  display.setBusClock(OLED_BUS_CLOCK);    // must be before begin()
#endif

  display.begin();                        // U8g2 pulses RES# and inits SSD1309
  display.setContrast(OLED_CONTRAST);
  display.setFontMode(1);                 // transparent glyphs
  display.setDrawColor(1);
  display.clearBuffer();
  display.sendBuffer();
  screenLit = true;
}

// Standby: display off first, then drop VCC. The controller's registers and
// GDDRAM live on VDD, which stays up, so nothing needs re-initialising.
static void screenStandby() {
  if (!screenLit) return;
  display.setPowerSave(1);
  delay(20);
  digitalWrite(OLED_EN_PIN, LOW);
  screenLit   = false;
  unreadAtOff = Notifs::unread();
  Serial.println(F("[PWR] standby: screen + 12V off, BLE stays up"));
}

static void screenWake() {
  if (screenLit) return;
  digitalWrite(OLED_EN_PIN, HIGH);
  delay(120);                             // spec wants >=100ms before display on
#if STANDBY_REINIT
  display.begin();
  display.setFontMode(1);
#endif
  Settings::applyDisplay();
  display.setPowerSave(0);
  display.setDrawColor(1);
  screenLit = true;
  gDirty    = true;
  Serial.println(F("[PWR] wake"));
}

// ----------------------------------------------------------- deep sleep ----
static void goToSleep() {
  saveSettings();
  prefs.end();

  if (screenLit) {                        // no point lighting the panel just to say goodbye
    display.clearBuffer();
    display.setFont(F_BODY);
    display.setDrawColor(1);
    uiCentre("deep sleep", 64, 28);
    display.setFont(F_TINY);
    uiCentre("phone reconnects on wake", 64, 42);
    display.sendBuffer();
    delay(450);
  }

  Notifs::stop();
  screenStandby();

  uint32_t t0 = millis();
  while ((digitalRead(BTN_0_PIN) == LOW || digitalRead(BTN_1_PIN) == LOW) &&
         (millis() - t0) < 5000) {
    delay(20);
  }
  delay(60);

  gpio_pullup_en((gpio_num_t)BTN_0_PIN);
  gpio_pulldown_dis((gpio_num_t)BTN_0_PIN);
  gpio_pullup_en((gpio_num_t)BTN_1_PIN);
  gpio_pulldown_dis((gpio_num_t)BTN_1_PIN);

  digitalWrite(OLED_EN_PIN, LOW);
  pinMode(OLED_RST_PIN, OUTPUT);
  digitalWrite(OLED_RST_PIN, LOW);
  gpio_hold_en((gpio_num_t)OLED_EN_PIN);
  gpio_hold_en((gpio_num_t)OLED_RST_PIN);
  gpio_deep_sleep_hold_en();

  uint64_t mask = (1ULL << BTN_0_PIN) | (1ULL << BTN_1_PIN);
  esp_deep_sleep_enable_gpio_wakeup(mask, ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.println("[PWR] deep sleep");
  Serial.flush();
  esp_deep_sleep_start();
}

// ----------------------------------------------------------------- setup ---
void setup() {
  Serial.begin(115200);
  delay(300);

#if PRINT_RESET_REASON
  esp_reset_reason_t rr = esp_reset_reason();
  gResetReason = resetReasonStr(rr);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.printf ("[BOOT] reset reason: %s\n", gResetReason);
  Serial.println(F("========================================"));
#endif

  // A previous build may have parked these pins with a deep-sleep hold.
  gpio_hold_dis((gpio_num_t)OLED_EN_PIN);
  gpio_hold_dis((gpio_num_t)OLED_RST_PIN);
  gpio_deep_sleep_hold_dis();

  // Drop the core clock before anything else powers up.
  setCpuFrequencyMhz(CPU_FREQ_MHZ);

  Btn::begin();
  panelInit();

  rtcOk = rtc.begin();
  if (!rtcOk) {
    Serial.println(F("[ERR] DS3231 not found"));
  } else {
    // Both of these are enabled by default and neither is used here.
    rtc.disable32K();                     // 32kHz output pin
    rtc.writeSqwPinMode(DS3231_OFF);      // square wave output
    if (rtc.lostPower()) {
      Serial.println(F("[RTC] lost power, seeding from build time"));
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  prefs.begin("watch", false);
  gFace = prefs.getUChar("face", 0);
  if (gFace >= FACE_COUNT) gFace = 0;

  Settings::begin();
  Settings::applyDisplay();      // user contrast/invert/flip over the defaults
  Cal::begin();

  randomSeed(micros());
  configurePowerManagement();

  esp_sleep_wakeup_cause_t why = esp_sleep_get_wakeup_cause();

  // Stagger the radio behind the panel. Bringing both up together is the single
  // heaviest moment on this supply, and it is when the browning out happens.
  delay(BLE_START_DELAY_MS);
  Notifs::begin();

  goState(ST_FACE);
  Btn::markActivity();
  Serial.printf("[BOOT] fw=%s cpu=%luMHz face=%s standby=%us sleep=%us ble=%s\n",
                FW_VERSION, (unsigned long)getCpuFrequencyMhz(), FACES[gFace].name,
                Settings::get().standbyS, Settings::get().sleepS,
                ENABLE_BLE ? "on" : "off");
  if (Settings::get().flashMode)
    Serial.println(F("[BOOT] FLASH MODE ON - deep sleep disabled, USB stays up"));
}

// ------------------------------------------------------------------ loop ---
void loop() {
  Btn::update();
  Notifs::loop();
  BtnEvent ev = Btn::get();

  // ---- standby: screen dark, radio still live ----
  if (!screenLit) {
    bool newNotif = false;
#if WAKE_ON_NOTIFICATION
    newNotif = (Notifs::unread() > unreadAtOff);
#endif

    if (ev != EV_NONE || Btn::anyDown() || newNotif) {
      screenWake();
      Btn::flush();                       // the press that woke us isn't an action
      Btn::markActivity();
      if (newNotif) goState(ST_NOTIF_LIST);
      else          gDirty = true;
      return;
    }

    // A running stopwatch blocks deep sleep outright: the chip resets on wake,
    // which would throw the count away. A sleep time of 0 means never.
    if (Settings::sleepMs() && !stopwatchActive() && !Settings::get().flashMode &&
        (millis() - Btn::lastActivity()) > Settings::sleepMs())
      goToSleep();

    heartbeat();
    delay(20);                            // nothing to draw, idle gently
    return;
  }

  // ---- awake ----
  screenTick(gState, ev);
  heartbeat();

  bool holdScreen = STOPWATCH_KEEPS_SCREEN_ON && stopwatchActive();
  if (!holdScreen && (millis() - Btn::lastActivity()) > Settings::standbyMs()) {
    saveSettings();
    screenStandby();
  }

  delay(2);
}
