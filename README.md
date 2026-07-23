# Rocket Tracker

A two-part GPS + LoRa tracker for finding your model rocket after it lands. A tiny transmitter rides inside the rocket, reads GPS once a second, and beams position + altitude over long-range radio to a handheld receiver that shows the live fix on an OLED screen and a phone-friendly web page with one-tap turn-by-turn navigation to the rocket.

No cellular network, no WiFi infrastructure, no subscription — just point-to-point radio that works in a farm field just as well as it works anywhere else.

## How it works

- **Transmitter** (rides in the rocket): reads lat/lon/altitude/satellite count off a GPS module and sends an 18-byte packet over LoRa once per second.
- **Receiver** (stays in your hand): listens for those packets, shows the latest fix plus height-above-pad and max-altitude-reached on a small OLED, and hosts its own WiFi hotspot with a web page — connect your phone to it and tap "Navigate to Rocket" to jump straight into your maps app.

Full technical detail — packet format, pin mappings, radio settings — is in [`SETUP_README.md`](SETUP_README.md) and in the comments at the top of each `.ino` file.

## Repo contents

| Path | What it is |
| --- | --- |
| `rocket_transmitter/rocket_transmitter.ino` | Flashes onto the board that rides in the rocket |
| `rocket_receiver/rocket_receiver.ino` | Flashes onto the handheld receiver |
| `SETUP_README.md` | Step-by-step flashing and bench-test guide |

## Bill of Materials

Two boards, no soldering beyond an optional antenna wire. Prices are approximate and will drift.

### Transmitter (goes in the rocket) — ~$69

| Part | Price | Link |
| --- | --- | --- |
| Adafruit Feather M0 with RFM95 LoRa Radio (900MHz) | $34.95 | [adafruit.com/product/3178](https://www.adafruit.com/product/3178) |
| Adafruit Ultimate GPS FeatherWing | $24.95 | [adafruit.com/product/3133](https://www.adafruit.com/product/3133) |
| 3.7V LiPo battery (500mAh is a good size/weight tradeoff) | $7.95 | [adafruit.com/product/1578](https://www.adafruit.com/product/1578) |
| 915MHz spring antenna (or solder your own quarter-wave wire — the sketch comments explain the length) | $0.95 | [adafruit.com/product/4269](https://www.adafruit.com/product/4269) |
| Micro USB cable, for flashing | — | any micro USB cable |

Optional: a CR1220 coin cell ([adafruit.com/product/380](https://www.adafruit.com/product/380), $0.95) gives the GPS module a warm-start battery backup — not required, just speeds up subsequent fixes.

### Receiver (stays in your hand) — check listing for price

| Part | Price | Link |
| --- | --- | --- |
| Meshnology N30, 2-pack (ESP32-S3 + SX1262, includes 1100mAh battery + case + 915MHz antenna per unit) | check listing | [Amazon](https://www.amazon.com/V3-Development-1100mAh-Battery-Protect/dp/B0F1CXG94J) |
| USB-C cable, for flashing | — | any USB-C cable |

You only need one N30 for the receiver — the Amazon listing sells them as a 2-pack, so the second board makes a handy spare or a second ground unit. It ships as a kit (board, battery, case, antenna as separate pieces you assemble) rather than pre-built — expect a few minutes of snapping it together, not soldering.

**Region note:** both sketches default to 915MHz (US). Outside the US, buy 868MHz-band antennas instead and change `LORA_FREQ_MHZ` to `868.0` in both `.ino` files — see the "Frequency/region" note in `SETUP_README.md`.

## Getting started

1. Buy the parts above.
2. Follow [`SETUP_README.md`](SETUP_README.md) for board packages, libraries, flashing steps, and a bench-test checklist — do the bench test before anything goes near a launch pad.
3. Fly, land, open the receiver's web page, tap navigate, go find your rocket.

## Status

Both sketches are compile-verified (Adafruit SAMD core + esp32 core, RadioLib/TinyGPSPlus/U8g2 — zero errors, zero warnings) but not yet flight-tested on real hardware. Bench-test thoroughly before a launch.
