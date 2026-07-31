/*
  ROCKET TRACKER - TRANSMITTER
  Version:  v3 - pushed 2026-07-31 16:40 UTC. This is when the source itself
            was last changed - update it whenever this file changes. It's
            deliberately separate from the "Firmware built" timestamp printed
            to Serial at boot, which only tells you when THAT PARTICULAR
            UPLOAD was compiled - not useful if you write code and don't get
            around to flashing it until hours (or days) later.
  Board:    Adafruit Feather M0 with RFM95 LoRa Radio - 900MHz (product 3178)
  Add-on:   Adafruit Ultimate GPS FeatherWing (product 3133), stacked on top
  Role:     Lives inside the rocket. Reads GPS, packs it into a small binary
            packet, and transmits it over LoRa once per second.

  ------------------------------------------------------------------------
  LIBRARIES YOU NEED (Arduino IDE > Tools > Manage Libraries):
    - "RadioLib" by Jan Gromes
    - "TinyGPSPlus" by Mikal Hart

  BOARD PACKAGE:
    - Arduino IDE > Tools > Board > Boards Manager > install "Adafruit SAMD Boards"
    - Select board: "Adafruit Feather M0"
    - (Also install the Adafruit + SAMD USB Native drivers if prompted on first upload)
  ------------------------------------------------------------------------

  PACKET FORMAT (must match the receiver sketch exactly):
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
  Total size: 23 bytes - small on purpose to keep airtime (and power draw) low.
*/

#include <RadioLib.h>
#include <TinyGPSPlus.h>

// ---------------------------------------------------------------------
// CONFIGURE ME
// ---------------------------------------------------------------------

// Feather M0 LoRa radio pins (these are fixed by the board itself - do not change
// unless you're on a different board than the Feather M0 RFM95 900MHz).
#define RFM95_CS   8
#define RFM95_RST  4
#define RFM95_INT  3

// LoRa radio settings - MUST MATCH THE RECEIVER EXACTLY
#define LORA_FREQ_MHZ     915.0   // US ISM band. Use 868.0 if you're in the EU.
#define LORA_BW_KHZ       125.0
#define LORA_SF           9       // spreading factor: higher = more range, slower/more power
#define LORA_CR           7       // coding rate
#define LORA_SYNC_WORD    0x12    // private network sync word (not the LoRaWAN public one)
#define LORA_TX_POWER_DBM 17      // safe max for RFM95 over PA_BOOST; don't go above 20

// How often to send a GPS update
#define TX_INTERVAL_MS 1000

// GPS is wired to the Feather's hardware serial port (the FeatherWing stacks on
// top and connects RX/TX automatically - no wiring needed).
#define GPS_SERIAL   Serial1
#define GPS_BAUD     9600

// The Feather M0 has a built-in resistor divider on the LiPo's BAT line,
// wired to this analog pin - see Adafruit's "Power Management" guide for
// this board. Reading it tells you the battery level with no extra hardware.
#define VBAT_PIN A7

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

SX1276 radio = new Module(RFM95_CS, RFM95_INT, RFM95_RST, RADIOLIB_NC);
TinyGPSPlus gps;

uint32_t txSeq = 0;
uint32_t lastTxMillis = 0;

// ---------------------------------------------------------------------
// Battery
// ---------------------------------------------------------------------

// Straight from Adafruit's Feather M0 "Power Management" guide: the BAT line
// is divided by 2 before it reaches this pin, and the SAMD21's ADC here is
// 10-bit (0-1023) by default, so this reconstructs the real battery voltage.
float readBatteryVoltage() {
  float measured = analogRead(VBAT_PIN);
  measured *= 2;    // undo the onboard divide-by-2
  measured *= 3.3;  // reference voltage
  measured /= 1024; // 10-bit ADC
  return measured;
}

// Rough single-cell LiPo state-of-charge estimate from voltage. The discharge
// curve is nonlinear (it sags fast under ~3.7V), so this is a piecewise
// approximation, not a lab-grade fuel gauge - good enough to answer "is there
// plenty of charge for this flight," not precise enough for "42% remaining."
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

void blinkLed(uint8_t times, uint16_t ms) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(ms);
    digitalWrite(LED_BUILTIN, LOW);
    delay(ms);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  // Don't block forever waiting for a USB serial monitor - the rocket won't have one.
  uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) { delay(10); }

  Serial.println(F("Rocket transmitter booting..."));
  // __DATE__/__TIME__ are filled in by the compiler at build time, so this
  // always reflects the actual firmware running on the board - no manual
  // version number to remember to bump before every upload.
  Serial.print(F("Firmware built: "));
  Serial.print(F(__DATE__));
  Serial.print(F(" "));
  Serial.println(F(__TIME__));

  GPS_SERIAL.begin(GPS_BAUD);

  int state = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                           LORA_SYNC_WORD, LORA_TX_POWER_DBM);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Radio init failed, code "));
    Serial.println(state);
    // Blink forever so a failure is obvious even without a serial monitor attached.
    while (true) { blinkLed(1, 100); }
  }
  Serial.println(F("Radio ready. Transmitting GPS fixes..."));
}

void loop() {
  // Feed every available GPS byte into the parser continuously - GPS parsing
  // must never block, or we'll miss NMEA sentences.
  while (GPS_SERIAL.available() > 0) {
    gps.encode(GPS_SERIAL.read());
  }

  uint32_t now = millis();
  if (now - lastTxMillis >= TX_INTERVAL_MS) {
    lastTxMillis = now;
    sendPacket();
  }
}

void sendPacket() {
  RocketPacket packet;
  packet.seq = txSeq++;
  packet.fixValid = gps.location.isValid() ? 1 : 0;
  packet.lat = packet.fixValid ? (float)gps.location.lat() : 0.0f;
  packet.lon = packet.fixValid ? (float)gps.location.lng() : 0.0f;
  packet.alt_m = (gps.altitude.isValid()) ? (float)gps.altitude.meters() : 0.0f;
  // 99.9 is a deliberately-bad sentinel, not a real reading - GSA sentences
  // (which carry HDOP) can lag behind the first fix by a few seconds, so
  // this avoids ever claiming a fake "great" 0.0 HDOP before the GPS has
  // actually reported one.
  packet.hdop = (gps.hdop.isValid()) ? (float)gps.hdop.hdop() : 99.9f;
  packet.sats = (gps.satellites.isValid()) ? (uint8_t)gps.satellites.value() : 0;
  packet.battPercent = batteryPercentFromVoltage(readBatteryVoltage());

  int state = radio.transmit((uint8_t*)&packet, sizeof(packet));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.print(F("Sent #"));
    Serial.print(packet.seq);
    Serial.print(F(" fix="));
    Serial.print(packet.fixValid);
    Serial.print(F(" lat="));
    Serial.print(packet.lat, 6);
    Serial.print(F(" lon="));
    Serial.print(packet.lon, 6);
    Serial.print(F(" sats="));
    Serial.print(packet.sats);
    Serial.print(F(" hdop="));
    Serial.print(packet.hdop, 1);
    Serial.print(F(" batt="));
    Serial.print(packet.battPercent);
    Serial.println(F("%"));
    blinkLed(1, 30); // quick blink = packet sent, useful for bench testing
  } else {
    Serial.print(F("Transmit failed, code "));
    Serial.println(state);
  }
}
