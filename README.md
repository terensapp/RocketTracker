# Rocket Tracker

A two-part GPS + LoRa tracker for finding your model rocket after it lands. A tiny transmitter rides inside the rocket, reads GPS once a second, and beams position + altitude over long-range radio to a handheld receiver that shows the live fix on an OLED screen and a phone-friendly web page with one-tap turn-by-turn navigation to the rocket.

No cellular network, no WiFi infrastructure, no subscription — just point-to-point radio that works in a farm field just as well as it works anywhere else.

## How it works

- **Transmitter** (rides in the rocket): reads lat/lon/altitude/satellite count/battery level off a GPS module and its own sensors, and sends a 19-byte packet over LoRa once per second.
- **Receiver** (stays in your hand): listens for those packets, shows the latest fix plus height-above-pad and max-altitude-reached on a small OLED, and hosts its own WiFi hotspot with a web page — connect your phone to it and tap "Navigate to Rocket" (opens Google Maps) to walk straight to the rocket, or watch a live altitude graph of the flight so far. No power switch on the board, but holding the top PRG button for a second puts it to sleep (and pressing it again wakes it back up), so it isn't draining its battery between launches. It also shows the transmitter's battery level, which hitches a ride in every LoRa packet, so you can check there's plenty of charge before walking out to the pad. A "New Launch" button on the web page resets the altitude graph and baseline for the next flight, no power-cycling required.

Full technical detail — packet format, pin mappings, radio settings — is in [`SETUP_README.md`](SETUP_README.md) and in the comments at the top of each `.ino` file.

<p align="center">
  <img src="docs/receiver_web_ui_sample.png" alt="Sample of the receiver's web page, showing battery, GPS fix, altitude graph, and navigation button" width="360">
</p>

<p align="center"><em>Illustrative mockup with sample flight data (not a real device screenshot) — shows the receiver's web page layout: battery, GPS fix, live altitude graph, and the "Navigate to Rocket" / "New Launch" buttons.</em></p>

## Repo contents

| Path | What it is |
| --- | --- |
| `rocket_transmitter/rocket_transmitter.ino` | Flashes onto the board that rides in the rocket (Feather M0 build) |
| `rocket_transmitter_cubecell/rocket_transmitter_cubecell.ino` | Alternative transmitter sketch for the smaller CubeCell GPS build |
| `rocket_receiver/rocket_receiver.ino` | Flashes onto the handheld receiver (works with either transmitter build) |
| `SETUP_README.md` | Step-by-step flashing and bench-test guide |
| `enclosures/transmitter_case.scad` | Parametric 3D-printable case for the Feather M0 transmitter stack |
| `enclosures/transmitter_case_cubecell.scad` | Parametric 3D-printable case for the CubeCell GPS transmitter |

## Bill of Materials

Two boards, no soldering beyond an optional antenna wire. Prices are approximate and will drift.

### Transmitter (goes in the rocket) — ~$69

| Part | Price | Link |
| --- | --- | --- |
| Adafruit Feather M0 with RFM95 LoRa Radio (900MHz) | $34.95 | [adafruit.com/product/3178](https://www.adafruit.com/product/3178) |
| Adafruit Ultimate GPS FeatherWing | $24.95 | [adafruit.com/product/3133](https://www.adafruit.com/product/3133) |
| 3.7V 300mAh LiPo, generic "302040" size (3.0 x 20 x 40mm, JST-PH connector) — a standard size code widely sold on Amazon/AliExpress, not an Adafruit part; search "302040 300mAh lipo" | ~$7-9 | generic |
| 915MHz spring antenna — 17.5mm long, 2.15dBi gain, 50Ω impedance, solders directly to the board's ANT pad (or solder your own quarter-wave wire instead — the sketch comments explain the length) | $0.95 | [adafruit.com/product/4269](https://www.adafruit.com/product/4269) |
| Micro USB cable, for flashing | — | any micro USB cable |

Optional: a CR1220 coin cell ([adafruit.com/product/380](https://www.adafruit.com/product/380), $0.95) gives the GPS module a warm-start battery backup — not required, just speeds up subsequent fixes.

### Receiver (stays in your hand) — check listing for price

| Part | Price | Link |
| --- | --- | --- |
| Meshnology N30, 2-pack (ESP32-S3 + SX1262, includes 1100mAh battery + case + 915MHz antenna per unit) | check listing | [Amazon](https://www.amazon.com/V3-Development-1100mAh-Battery-Protect/dp/B0F1CXG94J) |
| USB-C cable, for flashing | — | any USB-C cable |

You only need one N30 for the receiver — the Amazon listing sells them as a 2-pack, so the second board makes a handy spare or a second ground unit. It ships as a kit (board, battery, case, antenna as separate pieces you assemble) rather than pre-built — expect a few minutes of snapping it together, not soldering.

**Region note:** both sketches default to 915MHz (US). Outside the US, buy 868MHz-band antennas instead and change `LORA_FREQ_MHZ` to `868.0` in both `.ino` files — see the "Frequency/region" note in `SETUP_README.md`.

### Optional: a case for the transmitter

`enclosures/transmitter_case.scad` is a 3D-printable case that holds the whole transmitter stack — board, battery, and a cutout for an inline power switch. See the "3D-printed transmitter case" section in [`SETUP_README.md`](SETUP_README.md) for details and printing notes.

### Alternative transmitter: CubeCell GPS (smaller form factor)

If the Feather M0 stack is too tall to fit your airframe, `rocket_transmitter_cubecell/rocket_transmitter_cubecell.ino` is an alternative transmitter sketch for the **Heltec CubeCell GPS (HTCC-AB02S)** — one small board with the MCU, LoRa radio, and GPS all built in, instead of three stacked boards. It sends the exact same packet format, so it works with the **same receiver sketch, unmodified**. This board also has a small onboard OLED, which this sketch uses to show live GPS/radio status (fix, satellite count, battery, last packet sent) — handy for bench-testing outdoors on battery power without a laptop tethered for the Serial Monitor. See "Alternative build: CubeCell GPS" in [`SETUP_README.md`](SETUP_README.md) for the full picture, including a phaseout-status warning you should read before buying the board.

| Part | Price | Link |
| --- | --- | --- |
| Heltec CubeCell GPS-6502 (HTCC-AB02S) | ~$25.80–25.90 | [heltec.org/project/htcc-ab02s](https://heltec.org/project/htcc-ab02s/) |
| 3.7V 300mAh LiPo, "302040" size — **check the connector**: this board uses JST-SH 1.25mm pitch, not the JST-PH 2.0mm pitch on the Feather build | ~$7-9 | generic |
| u.FL/IPEX stub antenna, 902-928MHz, 3dBi, direct-mount (no pigtail needed) | $6.49 | [DigiKey #1173-1023-ND](https://www.digikey.com/en/products/detail/ttm-technologies-inc/66089-0906/3069145) |
| SS12D00-G4 mini slide switch, spliced inline into the battery lead — a common generic part, search that name on Amazon/AliExpress/eBay; the case's switch pocket is sized to its body/lever dimensions (see `SETUP_README.md`) | ~$3-6 for a pack | generic |
| Micro USB cable, for flashing | — | any micro USB cable |

`enclosures/transmitter_case_cubecell.scad` is the matching case — same snap-lid-and-glue closure and parachute tie tab as the Feather case, but noticeably shallower since there's no board stack to clear.

## Getting started

1. Buy the parts above.
2. Follow [`SETUP_README.md`](SETUP_README.md) for board packages, libraries, flashing steps, and a bench-test checklist — do the bench test before anything goes near a launch pad.
3. Fly, land, open the receiver's web page, tap navigate, go find your rocket.

## Status

The Feather M0 transmitter and the receiver are compile-verified (Adafruit SAMD core + esp32 core, RadioLib/TinyGPSPlus/U8g2 — zero errors, zero warnings) but not yet flight-tested on real hardware. The CubeCell GPS transmitter alternative has also been compile-verified against the real CubeCell board package and toolchain (zero errors) — see the caveat about one packaging-only step in `SETUP_README.md`. Bench-test thoroughly before a launch, regardless of which transmitter build you use.
