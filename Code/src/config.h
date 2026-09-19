#pragma once

// ---------------- Pins ----------------
#define I2C_SDA_PIN        3
#define I2C_SCL_PIN       10

#define BTN_0_PIN          0    // UP   / back / left-turn / jump
#define BTN_1_PIN          1    // DOWN / into menu / right-turn / duck

#define OLED_RST_PIN       4    // SSD1309 RES#  - handed to U8g2, it pulses this itself
#define OLED_EN_PIN        6    // TPS61040 EN   - 12V panel rail, active HIGH

// No battery sensing. The divider tap is GPIO5 = ADC2_CH0 and this chip's IDF
// refuses ADC2 reads, so there is nothing to measure. Reinstating it needs the
// tap moved to an ADC1 pin (GPIO0-4) on a board revision.

// ---------------- Power ----------------
// STANDBY: screen and the 12V boost off, CPU and BLE still running. The panel
// rail is the big load (~50-60mA reflected onto 3V3); the C3 idling with BLE
// connected is a fraction of that. Because the radio stays up, the phone link
// never drops and notifications keep arriving while the screen is dark.
#define IDLE_SCREEN_OFF_MS    60000UL   // screen dark, BLE still live

// DEEP SLEEP: radio off, ~20uA, phone reconnects on wake. Nothing can be
// received while here - the C3 has no wake-on-BLE. Measured from the last
// button press, so it fires DEEP_SLEEP_AFTER_MS - IDLE_SCREEN_OFF_MS after the
// screen goes dark. Set equal to IDLE_SCREEN_OFF_MS to sleep the moment the
// screen turns off; set to 0 to never deep sleep.
#define DEEP_SLEEP_AFTER_MS   90000UL    // total idle before the radio goes down

// Automatic light sleep during standby. The BLE controller stays powered and
// the link stays up, so notifications still arrive, but the CPU idles between
// packets - a few mA instead of tens. Needs power management compiled into the
// Arduino build; the log says whether it was accepted.
#define USE_LIGHT_SLEEP       1
#define PM_MAX_FREQ_MHZ       CPU_FREQ_MHZ
#define PM_MIN_FREQ_MHZ       40

// Light the screen and jump to the notification list when something arrives
// while in standby.
#define WAKE_ON_NOTIFICATION  1

// Re-run the full U8g2 init when leaving standby. The controller keeps its
// registers on VDD so this normally isn't needed; set to 1 if the display
// comes back garbled.
#define STANDBY_REINIT        0

// CPU clock. BLE needs at least 80MHz on the C3; 160 buys nothing here and
// draws roughly twice the core current. Dropping to 80 is free headroom.
#define CPU_FREQ_MHZ          80

// Wait this long after the panel comes up before starting the radio, so the
// boost converter's inrush and the BLE init don't land on the rail together.
#define BLE_START_DELAY_MS    400

// ---------------- Display ----------------
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT     64
#define OLED_BUS_CLOCK   400000   // 0 = leave U8g2 at its 100kHz default
// Panel current scales with contrast. Lowering this takes real load off the
// 12V rail. 0-255. Raise it once the supply is solid.
#define OLED_CONTRAST    100

// Selected rows in menus. 1 = solid white bar (looks better, lights ~1500
// extra pixels). 0 = outline + caret, a fraction of the current draw. Set to 1
// once you're confident the supply can take it.
#define UI_SOLID_SELECTION  0

// ---------------- Diagnostics ----------------
// Periodic logging costs current (every printf blocks while the CDC drains).
// HEARTBEAT_MS 0 turns the recurring log off; the boot banner stays.
#define PRINT_RESET_REASON  1
#define HEARTBEAT_MS        0         // launch build: recurring log off

// ---------------- Timing ----------------
#define DEBOUNCE_MS        25
#define COMBO_WINDOW_MS    90
#define REPEAT_DELAY_MS    500
#define REPEAT_RATE_MS     140
#define HOLD_EXIT_MS       900

#define FW_VERSION         "1.0"

// ---------------- BLE ----------------
// Back ON. If the resets return specifically on connect/notify, that is the
// transmit current spike hitting a marginal rail, not a software fault - try
// BLE_LOW_TX_POWER below before anything else.
#define ENABLE_BLE          1
#define BLE_DEVICE_NAME    "InfiniTime"
#define BLE_FW_REV         "1.14.0"
// Drops radio output to roughly a third. Shortens range, cuts the current
// spike. Delete the setPower line in notifications.cpp if your NimBLE version
// rejects it.
#define BLE_LOW_TX_POWER    1
// Transmit power. N12 is near the floor: much smaller current spike, range
// drops to roughly arm's length, which is all a watch needs. If your NimBLE
// version rejects the name, try ESP_PWR_LVL_N9.
#define BLE_TX_LEVEL        ESP_PWR_LVL_N12
// Bonding stores pairing keys in NVS so the phone can reconnect without going
// through the app again. Only matters if you turn deep sleep back on. Leave at
// 0 unless you're re-pairing constantly - changing it means forgetting the
// device on the phone and pairing again.
#define BLE_BONDING         0

// --- radio duty cycle: the main BLE current knobs ---
// Connection interval, units of 1.25ms. 0xA0 = 200ms, 0x140 = 400ms. The radio
// wakes once per interval while connected, so small numbers here are expensive.
// Notifications are not latency critical; 200-400ms is plenty.
#define BLE_CONN_MIN      0xA0
#define BLE_CONN_MAX      0x140
// Advertising interval, units of 0.625ms. 0x320 = 500ms, 0x640 = 1000ms.
// This runs continuously whenever the phone is not connected. Lower numbers
// reconnect faster after a wake but cost current the whole time.
#define BLE_ADV_MIN       0x320
#define BLE_ADV_MAX       0x640
