/*
  ROCKET TRACKER - TRANSMITTER (Heltec Wireless Tracker V2 alternative)
  Version:  v1 - first cut, not yet flight-tested. Compile-verified against
            the real ESP32-S3 toolchain (see SETUP_README.md), but the GPS/
            LoRa/display pin behavior on real hardware hasn't been confirmed
            yet the way the CubeCell build eventually was - test thoroughly.
  Board:    Heltec Wireless Tracker V2 (ESP32-S3FN8 + SX1262 LoRa radio +
            UC6580 GNSS + 0.96" ST7735 TFT, all on one small board with a
            built-in RF front-end amplifier for extra LoRa range)
  Role:     Third option for rocket_transmitter/rocket_transmitter.ino -
            same job (read GPS once a second, pack it into the same binary
            packet, send it over LoRa), same receiver sketch, no changes
            there. Replaces the CubeCell alternative: CubeCell GPS is on
            Heltec's own "Phaseout" list, and its onboard GPS antenna has
            multiple independent weak/dead-on-arrival reports on Heltec's
            community forum. This board uses a different, better-regarded
            GNSS chip (UC6580, dual-frequency, multi-constellation) and is
            an actively-sold, actively-supported product.

  ------------------------------------------------------------------------
  PIN MAP - WHERE THIS CAME FROM
    Heltec doesn't publish a plain pin-name table for this board (just a
    schematic PDF and a pinout image), so every pin below is taken from
    Meshtastic's official firmware source for this exact board
    (variants/esp32s3/heltec_wireless_tracker_v2/variant.h in
    github.com/meshtastic/firmware) - real, shipped, community-tested
    firmware for this specific hardware, not a guess from a datasheet.
  ------------------------------------------------------------------------

  RF FRONT-END - WHY THERE ARE EXTRA GPIO WRITES BEFORE RADIO.BEGIN()
    Unlike the Feather/CubeCell builds, this board has a KCT8103L front-end
    chip between the SX1262 and the antenna - it's what gets LoRa TX power
    up to the rated 28dBm instead of the SX1262 chip's own ~22dBm ceiling,
    and it also provides RX gain (not used here - this sketch only
    transmits, never listens). Three extra GPIO pins control it:
      - PA_POWER (7):  powers the front-end chip's own regulator
      - PA_CSD   (4):  chip enable (HIGH = on)
      - PA_CTX   (5):  HIGH = transmit-amplify path (also fine at rest,
                        since this sketch never receives)
    All three are set once at boot and left alone - no need to toggle them
    per-packet. The chip's 4th control line (CPS, TX/RX path select) is
    wired directly to the SX1262's own DIO2 pin, not to an ESP32 GPIO -
    radio.setDio2AsRfSwitch(true) below tells the SX1262 to drive that pin
    itself, so nothing else to do for that one.

  ------------------------------------------------------------------------
  DISPLAY / GPS SHARE ONE POWER RAIL - WHY THE BUTTON ONLY TOGGLES BACKLIGHT
    On the CubeCell build, the OLED and GPS were powered separately, so the
    status screen could be fully powered off between checks to save
    battery. On this board, Vext (GPIO3) powers the GPS, its LNA, AND the
    display controller all together - there's no separate switch for the
    display alone. Since GPS needs to stay powered for the whole flight
    anyway, there's no real battery-saving reason to power the display
    controller off and on. Instead, the button (see below) only toggles
    the TFT backlight (a separate pin, TFT_BL) - the display keeps
    receiving fresh draws every packet either way, so the instant you turn
    the backlight on, it's already showing current status, no lag.

  ------------------------------------------------------------------------
  BOARD PACKAGE / LIBRARIES YOU NEED
    - Arduino IDE > Preferences > Additional Board Manager URLs, add:
        https://espressif.github.io/arduino-esp32/package_esp32_index.json
    - Tools > Board > Boards Manager > install "esp32" (Espressif Systems)
    - Select board: "ESP32S3 Dev Module"
    - Tools > USB CDC On Boot: "Enabled" (needed to see Serial output over
      the USB-C port without a separate UART adapter)
    - Library Manager: install "RadioLib" (Jan Gromes), "TinyGPSPlus"
      (Mikal Hart), "Adafruit GFX Library", and "Adafruit ST7735 and
      ST7789 Library" (both Adafruit) - same RadioLib/TinyGPSPlus already
      used by the other sketches in this repo, plus the two display
      libraries this board's TFT needs that the OLED builds didn't.
  ------------------------------------------------------------------------

  PACKET FORMAT (identical to every other transmitter build in this repo -
  the receiver can't tell which transmitter sent a packet, and doesn't
  need to):
    uint32_t seq         - increments every packet, lets receiver detect drops
    float    lat         - degrees
    float    lon         - degrees
    float    alt_m       - meters
    float    hdop        - horizontal dilution of precision, lower = better
                            geometry/accuracy (under ~2 is good, over ~5 is
                            poor) - 99.9 means the GPS hasn't reported one yet
    uint8_t  sats        - number of satellites used in fix
    uint8_t  fixValid    - 1 = GPS has a valid fix this packet, 0 = no fix yet
    uint8_t  battPercent - this board's own LiPo charge estimate, 0-100
  Total size: 23 bytes.
*/

#include <SPI.h>
#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ---------------------------------------------------------------------
// CONFIGURE ME
// ---------------------------------------------------------------------

// LoRa (SX1262) pins - from Meshtastic's heltec_wireless_tracker_v2
// variant.h (see header comment above). Same CS/DIO1/RST/BUSY/SCK/MISO/
// MOSI numbering convention as the receiver's Meshnology N30 board, which
// is no coincidence - Heltec reuses this pin layout across most of their
// ESP32+SX1262 boards.
#define LORA_CS    8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// KCT8103L RF front-end control pins - see "RF FRONT-END" note above.
#define FEM_PA_POWER 7  // front-end regulator enable
#define FEM_PA_CSD   4  // front-end chip enable (HIGH = on)
#define FEM_PA_CTX   5  // HIGH = TX-amplify path

// LoRa radio settings - MUST MATCH THE RECEIVER.
#define LORA_FREQ_MHZ     915.0   // US ISM band. Use 868.0 in the EU.
#define LORA_BW_KHZ       125.0
#define LORA_SF           9
#define LORA_CR           7
#define LORA_SYNC_WORD    0x12
#define LORA_TX_POWER_DBM 20      // SX1262's own output before the external
                                   // front-end amplifies it further - see
                                   // "RF FRONT-END" note. Comfortably inside
                                   // the chip's safe range; the front-end is
                                   // designed to take whatever this is set to.
#define LORA_TCXO_VOLTAGE 1.8     // this board's TCXO voltage - different
                                   // from the receiver's 1.6V, confirmed
                                   // from the same Meshtastic variant.h

// Vext - powers the GPS, GPS LNA, and the TFT controller together (see
// "DISPLAY / GPS SHARE ONE POWER RAIL" above). Active HIGH on this board -
// opposite polarity from the CubeCell build's Vext, so don't copy that
// convention over by habit.
#define VEXT_ENABLE_PIN 3

// GNSS (UC6580) - separate hardware UART, not the same one as USB/Serial.
// These two names are taken verbatim from Meshtastic's variant.h for this
// board (see the pin map note near the top of this file), but that source
// only gave pin *numbers*, not the actual Serial.begin() call - and
// "GPS_RX_PIN" is genuinely ambiguous between two conventions: "the
// ESP32's own RX pin" (what this file originally assumed) vs. "the pin
// wired to the GPS chip's RX input" (i.e., the ESP32's TX pin, from the
// GPS's point of view). Field testing showed chars arriving but every
// single one failing checksum - garbled data, not "no fix yet" - which
// matches listening on the wrong pin far better than a reception problem.
// So this now assumes the second convention and swaps which one is
// passed as the ESP32's rxPin vs. txPin below. If GPS data is still
// garbled after this, the pin numbers themselves (not just their
// rx/tx role) are the next thing to question.
#define GPS_RX_PIN    33
#define GPS_TX_PIN    34
#define GPS_RESET_PIN 35
#define GPS_BAUD      115200  // UC6580 default - NOT the 9600 baud the
                               // other builds' GPS chips use

// TFT (ST7735S, 0.96" 160x80) - its own SPI bus, separate from the LoRa
// radio's SPI bus above (different SCK/MOSI/CS pins, no shared bus).
#define TFT_CS    38
#define TFT_DC    40
#define TFT_MOSI  42
#define TFT_SCK   41
#define TFT_RST   39
#define TFT_BL    21   // backlight - the only thing the status button
                        // actually toggles, see header note

// Battery ADC - a voltage divider that's only powered on demand (saves a
// tiny continuous draw through the divider resistors between readings).
#define BATTERY_PIN     1
#define BATTERY_CTRL_PIN 2      // HIGH = powers the divider
#define BATTERY_ADC_MULTIPLIER (4.9f * 1.045f)  // from Meshtastic's variant.h

// Status button - labeled "PRG"/"USER" on the board silkscreen.
#define DISPLAY_TOGGLE_BUTTON_PIN 0
#define BUTTON_DEBOUNCE_MS 50

// How often to send a GPS update
#define TX_INTERVAL_MS 1000

// ---------------------------------------------------------------------

#pragma pack(push, 1)
struct RocketPacket {
  uint32_t seq;
  float    lat;
  float    lon;
  float    alt_m;
  float    hdop;
  uint8_t  sats;
  uint8_t  fixValid;
  uint8_t  battPercent;
};
#pragma pack(pop)

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
TinyGPSPlus gps;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

uint32_t txSeq = 0;
uint32_t lastTxMillis = 0;
// hdop starts at the same 99.9 sentinel sendPacket() uses, so showStatus()
// never shows a misleadingly "great" 0.0 HDOP before the first real
// reading comes in
RocketPacket pendingPacket = {0, 0.0f, 0.0f, 0.0f, 99.9f, 0, 0, 0};

// backlight on/off state - see "DISPLAY / GPS SHARE ONE POWER RAIL" above
bool displayOn = false;
bool buttonWasPressed = false;
uint32_t lastButtonEdgeMillis = 0;

// ---------------------------------------------------------------------
// Battery
// ---------------------------------------------------------------------

// Same nonlinear single-cell LiPo discharge-curve approximation used by
// every other transmitter build in this repo - see those sketches'
// comments for the same caveat (good enough for "plenty of charge left,"
// not a lab-grade fuel gauge).
uint8_t batteryPercentFromVoltage(float v) {
  static const float voltage[] = {3.00, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.90, 3.95, 4.00, 4.10, 4.20};
  static const float percent[] = {0,    5,    10,   20,   30,   40,   50,   60,   70,   80,   90,   100};
  const int n = sizeof(voltage) / sizeof(voltage[0]);
  if (v <= voltage[0])     return 0;
  if (v >= voltage[n - 1]) return 100;
  for (int i = 0; i < n - 1; i++) {
    if (v <= voltage[i + 1]) {
      float frac = (v - voltage[i]) / (voltage[i + 1] - voltage[i]);
      return (uint8_t)(percent[i] + frac * (percent[i + 1] - percent[i]));
    }
  }
  return 0;
}

float readBatteryVoltage() {
  pinMode(BATTERY_CTRL_PIN, OUTPUT);
  digitalWrite(BATTERY_CTRL_PIN, HIGH); // power the divider just for this reading
  delay(5);
  analogSetPinAttenuation(BATTERY_PIN, ADC_2_5db); // matches Meshtastic's
                                                    // confirmed config for
                                                    // this board's divider
  uint32_t mv = analogReadMilliVolts(BATTERY_PIN);
  digitalWrite(BATTERY_CTRL_PIN, LOW);
  return (mv / 1000.0f) * BATTERY_ADC_MULTIPLIER;
}

void setup() {
  Serial.begin(115200);
  uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) { delay(10); }

  Serial.println("Rocket transmitter (Wireless Tracker V2) booting...");
  Serial.print("Firmware built: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);

  // RF front-end - set once, left alone. See "RF FRONT-END" note above.
  pinMode(FEM_PA_POWER, OUTPUT);
  pinMode(FEM_PA_CSD, OUTPUT);
  pinMode(FEM_PA_CTX, OUTPUT);
  digitalWrite(FEM_PA_POWER, HIGH);
  digitalWrite(FEM_PA_CSD, HIGH);
  digitalWrite(FEM_PA_CTX, HIGH);

  // Vext on for the whole session - GPS needs continuous power throughout
  // the flight, so there's nothing to gain by cycling it. See "DISPLAY /
  // GPS SHARE ONE POWER RAIL" above.
  pinMode(VEXT_ENABLE_PIN, OUTPUT);
  digitalWrite(VEXT_ENABLE_PIN, HIGH);
  delay(50); // let the GPS/display rail stabilize before touching either

  // Status button + backlight - starts off to save that draw; press to
  // check status before a launch, press again before closing up the case.
  pinMode(DISPLAY_TOGGLE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(1); // landscape, matches this board's 160x80 mounting
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(false);

  // GPS reset line idle-high (not asserted) - see GPS_RESET_MODE in
  // Meshtastic's variant.h, which this is taken from.
  pinMode(GPS_RESET_PIN, OUTPUT);
  digitalWrite(GPS_RESET_PIN, HIGH);
  // Swapped vs. the field-tested-wrong first attempt - see the comment on
  // GPS_RX_PIN/GPS_TX_PIN above for why.
  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                           LORA_SYNC_WORD, LORA_TX_POWER_DBM, 8,
                           LORA_TCXO_VOLTAGE, false);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Radio init failed, code ");
    Serial.println(state);
    // Force the backlight on for this - it defaults off at boot (see
    // "DISPLAY / GPS SHARE ONE POWER RAIL" above) and nothing later in
    // this file ever gets a chance to turn it on via the button, since
    // we're about to hang here forever. Without this, a radio init
    // failure looks identical to a healthy board with the backlight
    // just not toggled on yet - completely blank, no way to tell them
    // apart - which is exactly the trap this fixes.
    digitalWrite(TFT_BL, HIGH);
    tft.setCursor(0, 0);
    tft.println("RADIO INIT FAILED");
    tft.println(state);
    while (true) { delay(1000); }
  }
  radio.setDio2AsRfSwitch(true); // lets the SX1262 drive the front-end's
                                  // TX/RX path pin itself - see "RF
                                  // FRONT-END" note above

  Serial.println("Radio ready. Transmitting GPS fixes...");
}

void loop() {
  checkDisplayButton();

  // Feed every available GPS byte into the parser continuously - never
  // block here, or NMEA sentences get missed.
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  uint32_t now = millis();
  if (now - lastTxMillis >= TX_INTERVAL_MS) {
    lastTxMillis = now;
    sendPacket();
  }
}

// ---------------------------------------------------------------------
// Backlight on/off button - see "DISPLAY / GPS SHARE ONE POWER RAIL" near
// the top of this file for why this only toggles the backlight, not the
// whole display/GPS power rail.
// ---------------------------------------------------------------------
void checkDisplayButton() {
  bool pressed = (digitalRead(DISPLAY_TOGGLE_BUTTON_PIN) == LOW);
  uint32_t now = millis();

  if (pressed != buttonWasPressed && (now - lastButtonEdgeMillis) > BUTTON_DEBOUNCE_MS) {
    lastButtonEdgeMillis = now;
    buttonWasPressed = pressed;
    if (pressed) { // toggle on press, not release - feels more immediate
      displayOn = !displayOn;
      digitalWrite(TFT_BL, displayOn ? HIGH : LOW);
      if (displayOn) showStatus(); // draw immediately instead of waiting
                                    // up to a second for the next packet
    }
  }
}

void sendPacket() {
  pendingPacket.seq = txSeq++;
  pendingPacket.fixValid = gps.location.isValid() ? 1 : 0;
  pendingPacket.lat = pendingPacket.fixValid ? (float)gps.location.lat() : 0.0f;
  pendingPacket.lon = pendingPacket.fixValid ? (float)gps.location.lng() : 0.0f;
  pendingPacket.alt_m = gps.altitude.isValid() ? (float)gps.altitude.meters() : 0.0f;
  // 99.9 is a deliberately-bad sentinel, not a real reading - see the
  // other transmitter sketches' comments for why.
  pendingPacket.hdop = gps.hdop.isValid() ? (float)gps.hdop.hdop() : 99.9f;
  pendingPacket.sats = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;
  pendingPacket.battPercent = batteryPercentFromVoltage(readBatteryVoltage());

  int state = radio.transmit((uint8_t*)&pendingPacket, sizeof(pendingPacket));

  Serial.print("Sent #");
  Serial.print(pendingPacket.seq);
  Serial.print(" fix=");
  Serial.print(pendingPacket.fixValid);
  Serial.print(" lat=");
  Serial.print(pendingPacket.lat, 6);
  Serial.print(" lon=");
  Serial.print(pendingPacket.lon, 6);
  Serial.print(" sats=");
  Serial.print(pendingPacket.sats);
  Serial.print(" hdop=");
  Serial.print(pendingPacket.hdop, 1);
  Serial.print(" batt=");
  Serial.print(pendingPacket.battPercent);
  Serial.print("% | radio state=");
  Serial.print(state);
  Serial.print(" | GPS chars=");
  Serial.print(gps.charsProcessed());
  Serial.print(" sentencesWithFix=");
  Serial.print(gps.sentencesWithFix());
  Serial.print(" checksumFail=");
  Serial.println(gps.failedChecksum());

  if (displayOn) {
    showStatus();
  }
}

// Same diagnostic picture as the CubeCell build's OLED - see that
// sketch's "SATS: 0 FIX: N" header note for what each field means, it
// applies here unchanged since TinyGPSPlus's diagnostics are chip-agnostic.
void showStatus() {
  bool gpsHasData = gps.charsProcessed() > 10; // a few boot bytes don't
                                                // count as "alive" yet

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);

  tft.setCursor(0, 0);
  tft.print("Fix:");
  tft.print(pendingPacket.fixValid ? "Y" : "N");
  tft.print(" Sats:");
  tft.println(pendingPacket.sats);

  tft.setCursor(0, 12);
  if (!gpsHasData) {
    tft.println("chip: NO DATA");
  } else if (gps.sentencesWithFix() == 0) {
    tft.println("chip: searching");
  } else {
    tft.println("chip: OK");
  }

  // HDOP quality label - under 2 is good geometry, 2-5 usable, over 5
  // means don't trust the fix for anything precise.
  const char* hdopLabel = pendingPacket.hdop < 2.0f ? "good"
                         : pendingPacket.hdop < 5.0f ? "ok"
                                                      : "poor";
  tft.setCursor(0, 24);
  tft.print("HDOP:");
  tft.print(pendingPacket.hdop, 1);
  tft.print(" (");
  tft.print(hdopLabel);
  tft.println(")");

  tft.setCursor(0, 36);
  tft.print("batt:");
  tft.print(pendingPacket.battPercent);
  tft.print("%  tx:#");
  tft.println(pendingPacket.seq);

  // Raw chars-received and checksum-failure counts, same numbers the
  // serial log prints - shown here too so you can diagnose a no-fix
  // situation entirely from the screen if Serial isn't cooperating (a
  // common ESP32-S3 gotcha: Tools > USB CDC On Boot must be "Enabled" to
  // see Serial output over the USB-C port at all - see the top of this
  // file). High checksumFail relative to chars points to a baud/wiring
  // problem; chars stuck near 0 means no data is reaching the chip at
  // all; low checksumFail with plenty of chars but still no fix is a
  // genuine reception/antenna problem, not a code or connection problem.
  tft.setCursor(0, 60);
  tft.print("cs:");
  tft.print(gps.charsProcessed());
  tft.print(" cf:");
  tft.println(gps.failedChecksum());

  tft.setCursor(0, 48);
  if (pendingPacket.fixValid) {
    tft.print(pendingPacket.lat, 4);
    tft.print(",");
    tft.println(pendingPacket.lon, 4);
  } else {
    tft.println("no fix yet");
  }
}
