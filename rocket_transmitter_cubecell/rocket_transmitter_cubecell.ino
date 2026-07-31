/*
  ROCKET TRACKER - TRANSMITTER (CubeCell all-in-one alternative)
  Version:  v5 - added HDOP (fix quality) to the packet and the OLED,
            so you can see WHY a reading is off, not just that you have one.
  Board:    Heltec CubeCell GPS, HTCC-AB02S (ASR6502 MCU + SX1262 LoRa radio +
            onboard Air530Z GPS, all on one small board - no stacking headers)
  Role:     Alternative to rocket_transmitter/rocket_transmitter.ino for a
            smaller form factor. Same job: reads GPS once a second, packs it
            into the same binary packet, sends it over LoRa. Works with the
            SAME receiver sketch (rocket_receiver/rocket_receiver.ino) with NO
            changes there - see the "why this is compatible" note below.

  ------------------------------------------------------------------------
  "SATS: 0 FIX: N" ON THE RECEIVER - HOW TO TELL WHAT'S WRONG
    This just means the transmitter hasn't gotten a satellite fix yet -
    normal at first, but it can mean two very different things, and the
    receiver's display alone can't tell you which:
      1. The GPS chip isn't getting any data to/from it at all (wiring,
         library, or a dead chip).
      2. The GPS chip is working fine and receiving satellite signal, it
         just hasn't locked a fix yet - completely normal for a while.

    EASIEST WAY TO CHECK: the transmitter's own onboard OLED now shows this
    live (see showStatus() near the bottom) - no laptop needed, so you can
    just take the board outside on battery power and read it directly. The
    second line says "chip: NO DATA" (case 1), "chip: searching" (case 2 -
    working, no fix yet), or "chip: OK" (has a fix). If you'd rather use
    the Serial Monitor instead, open it on the TRANSMITTER (not the
    receiver) at 115200 baud and check the same two things it's built from:

    A) At boot, before "Radio ready..." prints, the GPS library itself
       prints its own baud-detection log:
         GPS Current baudrate detecting...
         GPS Current baudrate detected: 9600
         GPS baudrate updating to 9600
         GPS baudrate updated to 9600
       If you see "GPS baudrate updated failed, use GPS baudrate ..."
       instead, the chip isn't responding at the hardware level - that's
       a wiring/board problem, not a "needs more time" problem.

    B) Every second, sendPacket() below now also prints:
         GPS chars=<N> sentencesWithFix=<N> checksumFail=<N>
       `chars` should be counting up every second the GPS is powered - if
       it's stuck at 0, no data is arriving from the chip at all (same
       hardware-level problem as A). If `chars` is counting up but
       `sentencesWithFix` stays at 0, the chip IS talking, it's just not
       locked onto enough satellites yet - that's case 2 above, and it's
       an antenna/sky-view/time problem, not a code problem.

    If it's case 2, a few things worth knowing (found via the Heltec
    community forum - multiple independent reports, not just one):
      - First fix after power-on can take a LONG time - one documented
        case took over an hour before the first fix, then a few minutes
        on subsequent power-ons. This board has no backup battery for the
        GPS almanac (unlike the Feather build's optional CR1220), so it's
        plausible every power-cycle needs close to a full cold-start wait,
        not just the very first one. Give it real time (30+ minutes)
        before assuming something's wrong.
      - It needs a genuinely open sky view - indoors, it may never get a
        fix no matter how long you wait. Test outdoors, away from windows.
      - The onboard antenna on this board has multiple independent reports
        of being weak (or occasionally DOA from the factory) - if you've
        confirmed case 2 (chars counting, no fix) and given it real time
        outdoors and it's still stuck, an external active GPS antenna via
        the board's GPS u.FL connector (separate from the LoRa one) is a
        documented fix for others who hit this. See the Heltec community
        forum threads on "no GPS fix" for more if you land here.

  ------------------------------------------------------------------------
  OLED IS OFF BY DEFAULT - PRESS THE USER BUTTON TO CHECK STATUS
    The screen draws real power, and this board isn't opened up between
    launches to flip a switch, so it starts OFF at boot to save battery -
    press the board's user button (labeled "PRG" on some silkscreens, wired
    to pin P3_3 - confirmed against Heltec's own factory test sketch for
    this board) once to turn it on, press again to turn it off. Good
    routine: power the board on, wait a bit, press the button and confirm
    "chip: OK" with a real fix before it goes in the rocket, then press
    again to turn the screen off before closing up the case. It defaults
    off on every fresh boot, not just the first one - the toggle state
    itself isn't remembered across power cycles.

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
    float    hdop        - horizontal dilution of precision, lower = better
                            geometry/accuracy (under ~2 is good, over ~5 is
                            poor) - 99.9 means the GPS hasn't reported one yet
    uint8_t  sats        - number of satellites used in fix
    uint8_t  fixValid    - 1 = GPS has a valid fix this packet, 0 = no fix yet
    uint8_t  battPercent - this board's own LiPo charge estimate, 0-100
  Total size: 23 bytes.
*/

#include "LoRaWan_APP.h"
#include "GPS_Air530Z.h"
#include "HT_SSD1306Wire.h"

// This board has a 0.96" 128x64 OLED built in - `display` is declared
// extern here and defined inside the CubeCell board package itself (not
// something this sketch constructs), same as Heltec's own factory test
// sketch for this board. Used below to show live GPS/radio status without
// needing a laptop tethered to the Serial Monitor - handy for bench-testing
// outdoors on battery power alone. See showStatus() near the bottom.
extern SSD1306Wire display;

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

// User button that toggles the OLED on/off - see "OLED IS OFF BY DEFAULT"
// above. Confirmed against Heltec's own Factory_Test_AB02S.ino, which reads
// this same pin as a plain INPUT (not INPUT_PULLUP - this board already has
// a hardware pull-up on it) and treats LOW as pressed.
#define DISPLAY_TOGGLE_BUTTON_PIN P3_3
#define BUTTON_DEBOUNCE_MS 50

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

Air530ZClass GPS;   // onboard GPS, no wiring - it's built into the board

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

typedef enum { STATE_IDLE, STATE_TX } TxState_t;
volatile TxState_t txState = STATE_IDLE;

uint32_t txSeq = 0;
uint32_t lastTxMillis = 0;
// must stay alive until OnTxDone fires (async send) - hdop starts at the
// same 99.9 sentinel sendPacket() uses, so showStatus() doesn't show a
// misleadingly "great" 0.0 HDOP before the first real reading comes in
RocketPacket pendingPacket = {0, 0.0f, 0.0f, 0.0f, 99.9f, 0, 0, 0};

// OLED on/off toggle state - see "OLED IS OFF BY DEFAULT" above
bool displayOn = false;
bool buttonWasPressed = false;
uint32_t lastButtonEdgeMillis = 0;

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

  // OLED starts OFF (see "OLED IS OFF BY DEFAULT" above) - just set up the
  // button that toggles it. The display itself isn't touched until the
  // first time it's turned on, so no power goes to the Vext rail at all
  // until you actually ask for it.
  pinMode(DISPLAY_TOGGLE_BUTTON_PIN, INPUT);

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
  checkDisplayButton();

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

// ---------------------------------------------------------------------
// OLED on/off button - see "OLED IS OFF BY DEFAULT" near the top of this
// file for why this exists and how to use it.
// ---------------------------------------------------------------------
void checkDisplayButton() {
  bool pressed = (digitalRead(DISPLAY_TOGGLE_BUTTON_PIN) == LOW);
  uint32_t now = millis();

  if (pressed != buttonWasPressed && (now - lastButtonEdgeMillis) > BUTTON_DEBOUNCE_MS) {
    lastButtonEdgeMillis = now;
    buttonWasPressed = pressed;
    if (pressed) { // toggle on press, not release - feels more immediate
      setDisplayOn(!displayOn);
    }
  }
}

void setDisplayOn(bool on) {
  displayOn = on;

  if (on) {
    // Vext LOW = powered on, despite how that reads - documented
    // convention on CubeCell boards. Re-init every time since the display
    // is fully powered down (not just blanked) while off.
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);
    display.init();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    showStatus(); // draw immediately instead of waiting up to a second for the next packet
  } else {
    display.clear();
    display.display();
    display.stop(); // same call Heltec's own factory test sketch uses before sleep
    pinMode(Vext, ANALOG); // let the rail float off - cuts real power, not just blanks the screen
  }
}

void sendPacket() {
  pendingPacket.seq = txSeq++;
  pendingPacket.fixValid = GPS.location.isValid() ? 1 : 0;
  pendingPacket.lat = pendingPacket.fixValid ? (float)GPS.location.lat() : 0.0f;
  pendingPacket.lon = pendingPacket.fixValid ? (float)GPS.location.lng() : 0.0f;
  pendingPacket.alt_m = GPS.altitude.isValid() ? (float)GPS.altitude.meters() : 0.0f;
  // 99.9 is a deliberately-bad sentinel, not a real reading - GSA sentences
  // (which carry HDOP) can lag behind the first fix by a few seconds, so
  // this avoids ever claiming a fake "great" 0.0 HDOP before the GPS has
  // actually reported one.
  pendingPacket.hdop = GPS.hdop.isValid() ? (float)GPS.hdop.hdop() : 99.9f;
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
  Serial.print(" hdop=");
  Serial.print(pendingPacket.hdop, 1);
  Serial.print(" batt=");
  Serial.print(pendingPacket.battPercent);
  Serial.print("% | GPS chars=");
  Serial.print(GPS.charsProcessed());
  Serial.print(" sentencesWithFix=");
  Serial.print(GPS.sentencesWithFix());
  Serial.print(" checksumFail=");
  Serial.println(GPS.failedChecksum());

  if (displayOn) {
    showStatus();
  }
}

// Shows the same diagnostic picture on the onboard OLED that the serial
// log above prints - same idea, but readable with the board out on
// battery power alone, no laptop needed. See the "SATS: 0 FIX: N" note
// near the top of this file for what each field means. Only called while
// displayOn is true (see setDisplayOn()) - the display is fully powered
// down otherwise, so touching it here would do nothing useful anyway.
void showStatus() {
  bool gpsHasData = GPS.charsProcessed() > 10; // a few boot bytes don't
                                                // count as "alive" yet

  display.clear();

  String line0 = "Fix:" + String(pendingPacket.fixValid ? "Y" : "N") +
                 " Sats:" + String(pendingPacket.sats);
  display.drawString(0, 0, line0);

  String gpsHealth = !gpsHasData                     ? "chip: NO DATA"
                    : (GPS.sentencesWithFix() == 0)   ? "chip: searching"
                                                       : "chip: OK";
  display.drawString(0, 13, gpsHealth);

  // HDOP quality label - rough, standard GPS convention: under 2 is good
  // geometry, 2-5 is usable but not great, over 5 means don't trust the fix
  // for anything precise. This is the number to watch if a fix "seems a
  // little off" - it directly answers whether the satellite geometry
  // right now supports a precise reading, separate from just having sats.
  String hdopLabel = pendingPacket.hdop < 2.0f ? "good"
                    : pendingPacket.hdop < 5.0f ? "ok"
                                                 : "poor";
  display.drawString(0, 26, "HDOP:" + String(pendingPacket.hdop, 1) + " (" + hdopLabel + ")");

  display.drawString(0, 39, "batt:" + String(pendingPacket.battPercent) +
                             "%  tx:#" + String(pendingPacket.seq));

  String line4 = pendingPacket.fixValid
    ? (String(pendingPacket.lat, 4) + "," + String(pendingPacket.lon, 4))
    : String("no fix yet");
  display.drawString(0, 52, line4);

  display.display();
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
