/*
  ROCKET TRACKER - TRANSMITTER (CubeCell all-in-one alternative)
  Version:  v1 - pushed 2026-07-27 23:22 UTC.
  Board:    Heltec CubeCell GPS, HTCC-AB02S (ASR6502 MCU + SX1262 LoRa radio +
            onboard Air530Z GPS, all on one small board - no stacking headers)
  Role:     Alternative to rocket_transmitter/rocket_transmitter.ino for a
            smaller form factor. Same job: reads GPS once a second, packs it
            into the same binary packet, sends it over LoRa. Works with the
            SAME receiver sketch (rocket_receiver/rocket_receiver.ino) with NO
            changes there - see the "why this is compatible" note below.

  ------------------------------------------------------------------------
  WHY THIS EXISTS / TRADE-OFFS VS. THE FEATHER M0 BUILD
    The Feather M0 + LoRa + GPS stack is three boards tall and needs its own
    case bay for each. CubeCell GPS puts the MCU, LoRa radio, and GPS chip on
    one small PCB (about 47mm x 24mm), which lets the case shrink a lot.

    The trade-off: Heltec lists the CubeCell ASR650X series as "Phaseout"
    (their term for discontinued/end-of-life) at the time this was written.
    That's a real risk if you need a replacement board down the road - the
    Feather M0 build is the safer long-term bet. This alternative exists for
    anyone who wants the smaller size now and is OK sourcing/stocking spares
    themselves.

  ------------------------------------------------------------------------
  RADIO COMPATIBILITY WITH THE EXISTING RECEIVER - READ THIS
    The receiver sketch uses RadioLib against an SX1262 (Meshnology N30).
    CubeCell GPS also has an SX1262, but Heltec's board package doesn't
    support RadioLib - it uses its own raw point-to-point LoRa driver (the
    `Radio` object below, from LoRaMac-node, NOT full LoRaWAN - no gateway or
    network server involved, just direct radio-to-radio like RadioLib's
    transmit()/receive()).

    Two different libraries can still talk to each other, because LoRa is a
    physical-layer standard - what matters is that both sides agree on:
      - Frequency        915 MHz (matches LORA_FREQ_MHZ in both sketches)
      - Bandwidth         125 kHz (LORA_BANDWIDTH = 0 below = 125kHz)
      - Spreading factor  SF9 (matches LORA_SF in the Feather sketch)
      - Coding rate       4/7 (LORA_CODINGRATE = 3 below = 4/7, matches CR=7
                           naming on the RadioLib side - same coding rate,
                           different number scheme between the two libraries)
      - Sync word         0x12 - CubeCell's `Radio.SetPublicNetwork(false)`
                           sets the sync word to LoRaMac-node's private-network
                           value (0x1424 as a 2-byte MAC register pair), which
                           decodes on the wire as the same sync byte RadioLib
                           calls 0x12. Verified against both libraries' source
                           - don't change one side without checking the other.
    None of these are CubeCell-specific or RadioLib-specific settings - they're
    standard LoRa PHY parameters, so any two radios that agree on all five can
    hear each other regardless of which vendor's software is driving them.

  ------------------------------------------------------------------------
  BOARD PACKAGE / LIBRARIES YOU NEED
    - Arduino IDE > Preferences > Additional Board Manager URLs, add:
        https://resource.heltec.cn/download/package_CubeCell_index.json
    - Tools > Board > Boards Manager > install "CubeCell"
    - Select board: "CubeCell-GPS(HTCC-AB02S)"
    - No extra libraries to install - Radio and GPS support ship inside the
      CubeCell board package itself (LoRaWan_APP.h, GPS_Air530Z.h).
  ------------------------------------------------------------------------

  PACKET FORMAT (identical to the Feather M0 sketch - the receiver can't
  tell which transmitter sent a packet, and doesn't need to):
    uint32_t seq         - increments every packet, lets receiver detect drops
    float    lat         - degrees
    float    lon         - degrees
    float    alt_m       - meters
    uint8_t  sats        - number of satellites used in fix
    uint8_t  fixValid    - 1 = GPS has a valid fix this packet, 0 = no fix yet
    uint8_t  battPercent - this board's own LiPo charge estimate, 0-100
  Total size: 19 bytes.
*/

#include "LoRaWan_APP.h"
#include "GPS_Air530Z.h"

// ---------------------------------------------------------------------
// CONFIGURE ME
// ---------------------------------------------------------------------

// LoRa radio settings - MUST MATCH THE RECEIVER (see compatibility note above
// for how these map onto RadioLib's settings on the receiver side).
#define RF_FREQUENCY        915000000  // Hz. US ISM band. Use 868000000 in the EU.
#define LORA_BANDWIDTH       0         // 0 = 125 kHz
#define LORA_SPREADING_FACTOR 9        // SF9 - must match the receiver
#define LORA_CODINGRATE       3        // 3 = 4/7 - must match the receiver
#define LORA_PREAMBLE_LENGTH   8
#define TX_OUTPUT_POWER_DBM   17       // same ceiling as the Feather build

// How often to send a GPS update
#define TX_INTERVAL_MS 1000

// ---------------------------------------------------------------------

#pragma pack(push, 1)
struct RocketPacket {
  uint32_t seq;
  float    lat;
  float    lon;
  float    alt_m;
  uint8_t  sats;
  uint8_t  fixValid;
  uint8_t  battPercent;
};
#pragma pack(pop)

Air530ZClass GPS;   // onboard GPS, no wiring - it's built into the board

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

typedef enum { STATE_IDLE, STATE_TX } TxState_t;
volatile TxState_t txState = STATE_IDLE;

uint32_t txSeq = 0;
uint32_t lastTxMillis = 0;
RocketPacket pendingPacket; // must stay alive until OnTxDone fires (async send)

// ---------------------------------------------------------------------
// Battery
// ---------------------------------------------------------------------

// getBatteryVoltage() is a CubeCell core function - reads the board's own
// LiPo sense line in millivolts, no extra wiring needed (same idea as the
// Feather M0's VBAT_PIN, just built into the core instead of read manually).
// Same nonlinear single-cell discharge-curve approximation as the Feather
// build - see that sketch's comment for the same caveat.
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

void setup() {
  Serial.begin(115200);
  uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 3000) { delay(10); }

  Serial.println("Rocket transmitter (CubeCell) booting...");
  Serial.print("Firmware built: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);

  GPS.begin();

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetPublicNetwork(false); // private sync word - matches the receiver, see note above
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER_DBM, 0, LORA_BANDWIDTH,
                     LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                     LORA_PREAMBLE_LENGTH, false,
                     true, 0, 0, false, 3000);

  Serial.println("Radio ready. Transmitting GPS fixes...");
}

void loop() {
  // Feed every available GPS byte into the parser continuously - same
  // non-blocking pattern as the Feather build's GPS_SERIAL loop.
  while (GPS.available() > 0) {
    GPS.encode(GPS.read());
  }

  uint32_t now = millis();
  if (txState == STATE_IDLE && now - lastTxMillis >= TX_INTERVAL_MS) {
    lastTxMillis = now;
    sendPacket();
  }

  Radio.IrqProcess();
}

void sendPacket() {
  pendingPacket.seq = txSeq++;
  pendingPacket.fixValid = GPS.location.isValid() ? 1 : 0;
  pendingPacket.lat = pendingPacket.fixValid ? (float)GPS.location.lat() : 0.0f;
  pendingPacket.lon = pendingPacket.fixValid ? (float)GPS.location.lng() : 0.0f;
  pendingPacket.alt_m = GPS.altitude.isValid() ? (float)GPS.altitude.meters() : 0.0f;
  pendingPacket.sats = GPS.satellites.isValid() ? (uint8_t)GPS.satellites.value() : 0;
  pendingPacket.battPercent = batteryPercentFromVoltage(getBatteryVoltage() / 1000.0f);

  txState = STATE_TX;
  turnOnRGB(COLOR_SEND, 0);
  Radio.Send((uint8_t*)&pendingPacket, sizeof(pendingPacket));

  Serial.print("Sending #");
  Serial.print(pendingPacket.seq);
  Serial.print(" fix=");
  Serial.print(pendingPacket.fixValid);
  Serial.print(" lat=");
  Serial.print(pendingPacket.lat, 6);
  Serial.print(" lon=");
  Serial.print(pendingPacket.lon, 6);
  Serial.print(" sats=");
  Serial.print(pendingPacket.sats);
  Serial.print(" batt=");
  Serial.print(pendingPacket.battPercent);
  Serial.println("%");
}

void OnTxDone(void) {
  turnOnRGB(0, 0);
  txState = STATE_IDLE;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.println("Transmit timeout, retrying next cycle");
  txState = STATE_IDLE;
}
