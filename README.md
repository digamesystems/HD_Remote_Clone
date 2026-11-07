# HD Platinum Remote Clone

A DIY replacement for the handheld remote used with Hunter Douglas **Platinum Technology** motorized shades (the 2013-era system, before PowerView). It's built on an ESP32 with an IR LED/receiver and an nRF24L01+ 2.4 GHz radio.

> Personal hobby project. Not affiliated with or endorsed by Hunter Douglas. "Hunter Douglas", "Platinum" and "PowerView" are their trademarks and are used here only to say what this works with.

## Goal

Replace a degraded original remote with a homemade one that can:

- **Point and press (IR, 40 kHz):** control a single shade by aiming at its sensor eye.
- **Group control (RF, 2.4 GHz):** send OPEN, CLOSE and STOP to shade groups 1–4 or All, as the original does in its green-LED mode.
- Run on batteries, sleeping deeply between presses and waking on a button press.

The protocols aren't documented, so the first phase is reverse engineering. This repo's firmware is the toolkit for that: it captures, dumps and replays IR, and tests, scans and sniffs nRF24.

## Contributing

New to the project? Start here:

1. **[TESTING.md](TESTING.md):** how changes are tested (host unit tests, on-device tests, manual checks) and the gate every change must pass before it's committed or deployed.
2. **Code layout:** hardware-independent logic belongs in `lib/hdcore/` with a host unit test. Hardware-touching code goes in `src/`. See [Firmware](#firmware) below.
3. **Hardware:** the [pin map](#pin-map) and `include/hw_config.h`, which records what's actually wired on the breadboard.

## Status

| Stage | State |
|---|---|
| PlatformIO project, serial-menu firmware | ✅ builds |
| ESP32 board identified (DevKitC, WROOM-32D) | ✅ |
| IR capture / replay of the original remote | ⏳ hardware arriving |
| nRF24 module screening (likely Si24R1 clones) | ⏳ modules arriving |
| RF channel / address / data-rate discovery | 🔲 |
| RF transmit emulation | 🔲 |
| Final remote: buttons, deep sleep, enclosure | 🔲 |

## Technical approach

### What the original remote does

- **IR mode (red LED):** a proprietary IR code on a 40 kHz carrier, aimed at one shade. Pressing the opposite button stops movement.
- **RF mode (green LED):** an nRF24L01+ inside the remote. You select a group, then press OPEN or CLOSE within 5 s. Prior community work ([Arduino forum](https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940)) reports:
  - The remote rotates through three channels, sending one 5-byte packet per channel, and repeats that cycle ~30×.
  - While a button is held, the command packet repeats. On release, a separate "release" packet is sent ~60×.
  - Example payload: `03 C1 20 <group> <cmd>`. Group is `01/02/04/08` or `FF` for All; the command is `A1` for up, `A2` for down, and has bit 7 cleared on release.
  - One remote used channel 7. **All of this has to be verified against our own remote.**

### Open questions

- **Our remote's RF parameters:** channel(s), data rate, address and remote ID.
- **RF wake-up for IR?** Several secondhand sources say battery-powered Platinum shades ignore IR unless the remote first sends a short RF burst to wake them, which would explain why learning remotes fail. *Unconfirmed.* The test: sniff RF while pressing buttons in IR mode, then replay the captured IR on its own at a shade.
- **IR behavior on hold:** repeat frames, toggle bits or counters.

### Reverse-engineering plan

1. **Capture IR.** A TSOP38238 receiver records raw mark and space timings for every button in both modes, short press and held, several times each, to find any changing bits.
2. **Replay IR.** Send the raw timings back at 40 kHz through a TSAL6200 940 nm LED and confirm a shade responds.
3. **Screen the nRF24 modules.** The batch is probably clones, with a reported ~50% SPI failure rate, so every module gets tested first.
4. **Find the RF channels.** Sweep carrier detect (RPD) across channels 0–125 while pressing remote buttons.
5. **Sniff RF packets.** Use the 2-byte-address "promiscuous" trick to catch packets without knowing the address, then lock in the real address, CRC and payload size. The fallback is to tap the original remote's nRF24 SPI bus and read its register configuration directly.
6. **Emulate.** Transmit group commands with the original's timing: held repeats, then release packets.

## Hardware

- **MCU:** ESP32-DevKitC with ESP32-WROOM-32D (PlatformIO `esp32dev`)
- **IR receive:** TSOP38238 (38 kHz part, fine for a 40 kHz remote at close range)
- **IR transmit:** TSAL6200 LED driven by an NPN transistor (2N2222) at ~100 mA pulses
- **RF:** nRF24L01+ module with a PCB antenna, 3.3 V supply and a 10 µF cap at the module

### Pin map

| Function | GPIO | Notes |
|---|---|---|
| IR receive (TSOP OUT) | 14 | TSOP VS via 100 Ω, 100 nF decoupling |
| IR transmit (NPN base) | 4 | via 1 kΩ; LED + 33 Ω from 5 V |
| Button UP | 32 | active-high, 10 kΩ pull-down, RTC wake |
| Button DOWN | 33 | active-high, 10 kΩ pull-down, RTC wake |
| nRF24 SCK / MISO / MOSI | 18 / 19 / 23 | VSPI |
| nRF24 CSN | 5 | |
| nRF24 CE | 17 | |
| nRF24 IRQ (optional) | 16 | |

Buttons are active-high so the original ESP32 can wake from deep sleep with `ext1` / `ESP_EXT1_WAKEUP_ANY_HIGH`.

## Firmware

PlatformIO + Arduino framework. Libraries: [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) 2.8.6 and [RF24](https://github.com/nRF24/RF24) 1.4.11 on `espressif32@6.9.0`.

```
pio run -t upload
pio device monitor        # 115200 baud
```

Type `help` in the monitor. Sending any line stops a running capture, scan or sniff.

| Command | What it does |
|---|---|
| `ir` | Capture IR and dump each frame as a `uint16_t rawData[]` array |
| `play [slot\|last] [n] [gap_ms]` | Replay a capture at 40 kHz |
| `save <slot>` / `list` / `dump [slot]` | Keep up to 8 captures in RAM |
| `test [label]` | nRF24 self-test (200× register read-back) → PASS/FAIL |
| `info` | Print the nRF24 register details |
| `scan` | Carrier-detect sweep over channels 0–125 |
| `sniff` | Receive and print payloads with the current settings |
| `promisc aa\|55` | Preset for address-less sniffing |
| `ch`, `rate`, `aw`, `addr`, `crc`, `pl`, `filter`, `cfg` | Sniffer settings |

Source layout:

```
include/pins.h        pin map
include/hw_config.h   what's wired on the breadboard (gates hardware tests)
src/main.cpp          serial menu / dispatch
src/ir_tools.*        IR capture, dump, replay
src/rf_tools.*        nRF24 self-test, scanner, sniffer
lib/hdcore/           hardware-independent logic (parsing, address order, dup filter)
test/                 Tier 1 host tests (test_native_*) and Tier 2 device tests (test_device_*)
```

## Testing

```
pio test -e native                          # host unit tests, no hardware
pio test -e esp32dev --upload-port COM11    # on-device tests
```

See [TESTING.md](TESTING.md) for the three test tiers and the gate a build must pass before it's committed or deployed.

## References

- [NRF24L01+ hack on Hunter Douglas shades](https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940) (Arduino forum)
- [ESP8266 and NRF24L01+ Hunter Douglas Platinum Remote](https://forum.arduino.cc/t/esp8266-and-nrf24l01-hunter-douglas-platinum-remote/918887) (Arduino forum)
- [spitcool/hd-platinum-rf](https://github.com/spitcool/hd-platinum-rf): protocol research
- [Platinum Technology remote guide (PDF)](https://blindparts.com/wp-content/uploads/1970/01/Hunter-Douglas-Platinum-Technology-Remote-Control-Guide.pdf)
- [Bond forum: IR replay attempts / RF wake theory](https://forum.bondhome.io/t/hunterdouglas-powerrise-2-0-with-platinum-technology/3050)
- [AVS forum: automating PowerRise (2007)](https://www.avsforum.com/threads/automating-hunter-douglas-powerrise.852487/)
