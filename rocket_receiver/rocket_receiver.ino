/*
  ROCKET TRACKER - RECEIVER
  Board:  Meshnology N30 (ESP32-S3 + SX1262, Heltec WiFi LoRa 32 V3 architecture)
  Role:   Handheld unit. Listens for LoRa packets from the rocket, shows the
          latest fix (plus height above the pad and max height reached) on
          the onboard OLED, and hosts a WiFi hotspot with a web page that has
          a one-tap "Navigate" link (opens your phone's map app with
          turn-by-turn directions to the rocket).

  ------------------------------------------------------------------------
  LIBRARIES YOU NEED (Arduino IDE > Tools > Manage Libraries):
    - "RadioLib" by Jan Gromes
    - "U8g2" by oliver (for the OLED)
  (WiFi.h and WebServer.h ship with the ESP32 board package - no separate install.)

  BOARD PACKAGE:
    - Arduino IDE > Tools > Board > Boards Manager > install "esp32" by Espressif Systems
    - Select board: "ESP32S3 Dev Module" (this clone doesn't have its own board
      entry, so a generic ESP32-S3 profile is what you want)
    - Enable USB CDC on Boot if you want serial monitor output over the same
      USB-C cable (Tools > USB CDC On Boot > Enabled)
  ------------------------------------------------------------------------

  PIN NOTE: The pin numbers below match the standard Heltec WiFi LoRa 32 V3
  layout, which this board is based on. Meshnology's product photos include a
  pinout diagram - double check against that before your first flash, since
  clone boards occasionally shift a pin or two.

  PACKET FORMAT (must match the transmitter sketch exactly):
    uint32_t seq, float lat, float lon, float alt_m, uint8_t sats, uint8_t fixValid
    Total size: 18 bytes.
*/

#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------------------------------------------------------------------
// CONFIGURE ME
// ---------------------------------------------------------------------

// LoRa (SX1262) pins - standard Heltec WiFi LoRa 32 V3 layout
#define LORA_CS    8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// OLED (SSD1306 128x64) pins
#define OLED_SDA   17
#define OLED_SCL   18
#define OLED_RST   21

// Some Heltec V3 / clone boards gate power to the OLED and other onboard
// peripherals through this "Vext" pin - it must be driven LOW to turn them on.
// If your OLED stays blank, this is the first thing to check/remove.
#define VEXT_CTRL  36

// LoRa radio settings - MUST MATCH THE TRANSMITTER EXACTLY
#define LORA_FREQ_MHZ     915.0
#define LORA_BW_KHZ       125.0
#define LORA_SF           9
#define LORA_CR           7
#define LORA_SYNC_WORD    0x12
#define LORA_TX_POWER_DBM 17    // only used if this board ever transmits; harmless otherwise
#define LORA_TCXO_VOLTAGE 1.6   // Heltec V3 boards use a TCXO - this value is required for reliable init

// WiFi hotspot the phone connects to
#define AP_SSID "RocketTracker"
#define AP_PASS "findmyrocket"   // WPA2 requires 8+ characters

// If no packet arrives for this long, the web page/OLED will flag the data as stale
#define STALE_AFTER_MS 15000

// ---------------------------------------------------------------------

#pragma pack(push, 1)
struct RocketPacket {
  uint32_t seq;
  float    lat;
  float    lon;
  float    alt_m;
  uint8_t  sats;
  uint8_t  fixValid;
};
#pragma pack(pop)

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ OLED_RST, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
WebServer server(80);

volatile bool loraFlag = false;
void onLoraReceive() {
  loraFlag = true;
}

RocketPacket lastPacket = {0, 0.0f, 0.0f, 0.0f, 0, 0};
bool haveEverReceived = false;
uint32_t lastRxMillis = 0;
uint32_t packetsReceived = 0;
uint32_t packetsLost = 0; // best-effort, based on sequence gaps

// Altitude tracking. GPS altitude is Mean Sea Level (MSL), which is not a very
// useful number to look at mid-flight - what you actually want is height above
// the launch pad. We capture the FIRST valid fix we ever receive as "ground
// level" and report everything after that as a delta from it. This means you
// should power the transmitter on while it's already sitting on the pad,
// before launch, so the baseline is accurate. Rebooting the receiver resets
// the baseline and max height.
bool haveAltBaseline = false;
float baselineAltM = 0.0f;
float maxHeightAboveGroundM = 0.0f;

// GPS altitude typically has more error than horizontal position (commonly
// +/-10-15m without differential correction) - treat this as a rough
// estimate, not a precise altimeter reading.

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("Rocket receiver booting..."));

  // Power up the OLED / Vext rail (see comment above VEXT_CTRL)
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);
  delay(50);

  // Note: the U8g2 constructor above was given the clock/data pins directly,
  // which handles the Wire setup for this non-default I2C pin pair - no
  // separate Wire.begin() call needed here.
  u8g2.begin();
  drawSplash();

  // LoRa radio needs its own SPI pins configured explicitly on this board
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                           LORA_SYNC_WORD, LORA_TX_POWER_DBM, 8,
                           LORA_TCXO_VOLTAGE, false);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio init failed, code "));
    Serial.println(state);
    drawError("Radio init failed");
    while (true) { delay(1000); }
  }

  radio.setDio1Action(onLoraReceive);
  radio.startReceive();
  Serial.println(F("Radio listening..."));

  // Start the WiFi hotspot + web server
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print(F("AP started. Connect to WiFi \""));
  Serial.print(AP_SSID);
  Serial.print(F("\" then open http://"));
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  if (loraFlag) {
    loraFlag = false;
    handleLoraPacket();
  }

  // Periodically refresh the OLED even without new packets, so the
  // "seconds since last fix" counter keeps ticking.
  static uint32_t lastOledRefresh = 0;
  if (millis() - lastOledRefresh > 1000) {
    lastOledRefresh = millis();
    drawStatus();
  }
}

void handleLoraPacket() {
  RocketPacket incoming;
  int state = radio.readData((uint8_t*)&incoming, sizeof(incoming));
  radio.startReceive(); // always re-arm, even on a bad read

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Read failed, code "));
    Serial.println(state);
    return;
  }

  if (haveEverReceived && incoming.seq > lastPacket.seq + 1) {
    packetsLost += (incoming.seq - lastPacket.seq - 1);
  }

  lastPacket = incoming;
  haveEverReceived = true;
  lastRxMillis = millis();
  packetsReceived++;

  if (lastPacket.fixValid) {
    if (!haveAltBaseline) {
      baselineAltM = lastPacket.alt_m;
      haveAltBaseline = true;
    }
    float heightAboveGround = lastPacket.alt_m - baselineAltM;
    if (heightAboveGround > maxHeightAboveGroundM) {
      maxHeightAboveGroundM = heightAboveGround;
    }
  }

  Serial.print(F("Received #"));
  Serial.print(lastPacket.seq);
  Serial.print(F(" fix="));
  Serial.print(lastPacket.fixValid);
  Serial.print(F(" lat="));
  Serial.print(lastPacket.lat, 6);
  Serial.print(F(" lon="));
  Serial.print(lastPacket.lon, 6);
  Serial.print(F(" sats="));
  Serial.println(lastPacket.sats);

  drawStatus();
}

// ---------------------------------------------------------------------
// OLED
// ---------------------------------------------------------------------

void drawSplash() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Rocket Tracker");
  u8g2.drawStr(0, 24, "Starting radio...");
  u8g2.sendBuffer();
}

void drawError(const char* msg) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "ERROR:");
  u8g2.drawStr(0, 24, msg);
  u8g2.sendBuffer();
}

void drawStatus() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  char line[32];

  snprintf(line, sizeof(line), "AP: %s", AP_SSID);
  u8g2.drawStr(0, 9, line);

  IPAddress ip = WiFi.softAPIP();
  snprintf(line, sizeof(line), "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  u8g2.drawStr(0, 19, line);

  if (!haveEverReceived) {
    u8g2.drawStr(0, 33, "Waiting for signal...");
  } else {
    uint32_t ageMs = millis() - lastRxMillis;
    bool stale = ageMs > STALE_AFTER_MS;

    snprintf(line, sizeof(line), "Sats:%d Fix:%s%s", lastPacket.sats,
             lastPacket.fixValid ? "Y" : "N", stale ? "!" : "");
    u8g2.drawStr(0, 31, line);

    snprintf(line, sizeof(line), "%lus ago", ageMs / 1000);
    u8g2.drawStr(88, 31, line);

    snprintf(line, sizeof(line), "Lat: %.5f", lastPacket.lat);
    u8g2.drawStr(0, 42, line);

    snprintf(line, sizeof(line), "Lon: %.5f", lastPacket.lon);
    u8g2.drawStr(0, 53, line);

    if (haveAltBaseline) {
      float heightAboveGround = lastPacket.alt_m - baselineAltM;
      snprintf(line, sizeof(line), "H:%.0fm Max:%.0fm", heightAboveGround, maxHeightAboveGroundM);
    } else {
      snprintf(line, sizeof(line), "Alt(MSL): %.0fm", lastPacket.alt_m);
    }
    u8g2.drawStr(0, 64, line);
  }

  u8g2.sendBuffer();
}

// ---------------------------------------------------------------------
// Web server
// ---------------------------------------------------------------------

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<title>Rocket Tracker</title>";
  html += "<style>"
          "body{font-family:sans-serif;text-align:center;background:#111;color:#eee;padding:20px;}"
          "a.nav{display:inline-block;background:#2b7;color:#fff;padding:18px 32px;"
          "font-size:1.4em;border-radius:10px;text-decoration:none;margin-top:20px;}"
          ".stat{font-size:1.1em;margin:6px 0;}"
          ".stale{color:#f55;}"
          "</style></head><body>";

  html += "<h2>Rocket Tracker</h2>";

  if (!haveEverReceived) {
    html += "<p class='stat'>No signal received yet.</p>";
  } else {
    uint32_t ageMs = millis() - lastRxMillis;
    bool stale = ageMs > STALE_AFTER_MS;

    char buf[64];
    snprintf(buf, sizeof(buf), "Lat: %.6f, Lon: %.6f", lastPacket.lat, lastPacket.lon);
    html += "<p class='stat'>" + String(buf) + "</p>";

    snprintf(buf, sizeof(buf), "Fix: %s | Sats: %d",
             lastPacket.fixValid ? "yes" : "NO", lastPacket.sats);
    html += "<p class='stat'>" + String(buf) + "</p>";

    if (haveAltBaseline) {
      float heightAboveGround = lastPacket.alt_m - baselineAltM;
      snprintf(buf, sizeof(buf), "Height above pad: %.0fm (max: %.0fm)",
               heightAboveGround, maxHeightAboveGroundM);
    } else {
      snprintf(buf, sizeof(buf), "Altitude (MSL): %.0fm", lastPacket.alt_m);
    }
    html += "<p class='stat'>" + String(buf) + "</p>";

    snprintf(buf, sizeof(buf), "Last update: %lus ago", ageMs / 1000);
    html += "<p class='stat" + String(stale ? " stale" : "") + "'>" + String(buf) + "</p>";

    // Two links because phones disagree on how to open a map from a link:
    // "geo:" is the Android/Google Maps convention (also works on iPhone IF
    // Google Maps is installed, since that app registers the scheme) - but
    // stock iOS Safari has no handler for "geo:" on its own, so an iPhone
    // with only Apple Maps installed would tap the button and see nothing
    // happen. The maps.apple.com link covers that case.
    char geo[64];
    snprintf(geo, sizeof(geo), "geo:%.6f,%.6f", lastPacket.lat, lastPacket.lon);
    html += "<a class='nav' href='" + String(geo) + "'>Navigate to Rocket</a>";

    char appleMaps[96];
    snprintf(appleMaps, sizeof(appleMaps), "https://maps.apple.com/?ll=%.6f,%.6f&q=Rocket", lastPacket.lat, lastPacket.lon);
    html += "<p class='stat'><a href='" + String(appleMaps) + "' style='color:#8cf;'>iPhone? Open in Apple Maps instead</a></p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}
