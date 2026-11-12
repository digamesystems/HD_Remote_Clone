# HD Platinum Remote Clone

A DIY replacement for the handheld remote used with Hunter Douglas **Platinum Technology** motorized shades (the 2013-era system, before PowerView). It's built on an ESP32 with an IR LED/receiver and an nRF24L01+ 2.4 GHz radio.

> Personal hobby project. Not affiliated with or endorsed by Hunter Douglas. "Hunter Douglas", "Platinum" and "PowerView" are their trademarks and are used here only to say what this works with.

## Goal

Replace a degraded original remote with a homemade one that can:

- **Point and press (IR, 40 kHz):** control a single shade by aiming at its sensor eye. This is the arrows' default mode (red LED).
- **Group control (RF, 2.4 GHz):** pressing a group key (1–4 or ALL) opens a short window in which the arrows control that group over RF (green LED), as on the original.
- Run on batteries, sleeping deeply between presses and waking on a button press.

The protocols aren't documented, so the first phase is reverse engineering. This repo's firmware is the toolkit for that: it captures, dumps and replays IR, and tests, scans and sniffs nRF24.

## Contributing

New to the project? Start here:

1. **[TESTING.md](TESTING.md):** how changes are tested (host unit tests, on-device tests, manual checks) and the gate a release must pass.
2. **Code layout:** hardware-independent logic belongs in `lib/hdcore/` with a host unit test. Hardware-touching code goes in `src/`. See [Firmware](#firmware) below.
3. **Protocols:** [docs/IR_PROTOCOL.md](docs/IR_PROTOCOL.md) describes the decoded IR frame format and codes, plus the shade tests. [docs/RF_PROTOCOL.md](docs/RF_PROTOCOL.md) covers the 2.4 GHz side, including the RF wake the shade needs alongside IR.
4. **Hardware:** the [pin map](#pin-map), [docs/WIRING.md](docs/WIRING.md) for connecting the nRF24 modules, and `include/hw_config.h`, which records what's actually wired on the breadboard.

## Status

| Stage | State |
|---|---|
| PlatformIO project, serial-menu firmware | ✅ builds |
| ESP32 board identified (DevKitC, WROOM-32D) | ✅ |
| IR protocol decoded (UP `0101`, DOWN `0110`), see [IR_PROTOCOL.md](docs/IR_PROTOCOL.md) | ✅ |
| IR transmit to a shade | ✅ works with our own RF wake, no original remote (2026-11-11); reliable at ~1 ft, range limited by IR aim/power |
| nRF24 module screening (marked `NRF24L01+`, possibly clones) | ⏳ first module passes; rest to test |
| RF settings on our remotes | ✅ published settings confirmed by sniffing (2026-11-11) |
| RF wake frame for IR (`ID 01 00 80`) | ✅ sniffed from both remotes, see [RF_PROTOCOL.md](docs/RF_PROTOCOL.md) |
| RF transmit emulation | ✅ red-mode wake (`wake`, `send`) and group commands (`grp`): groups 1–4 and All, up and down (2026-11-11) |
| Final remote: buttons, red/green indicator LED, deep sleep, enclosure | 🔲 next |

## Technical approach

### What the original remote does

- **IR mode (red LED):** a proprietary IR code on a 40 kHz carrier, aimed at one shade. Pressing the opposite button stops movement.
- **RF mode (green LED):** an nRF24L01+ inside the remote. You select a group, then press OPEN or CLOSE within 5 s. Prior community work ([spitcool/hd-platinum-rf](https://github.com/spitcool/hd-platinum-rf), `docs/rf-protocol.md`, citing FCC filings, a register dump and SDR captures; and the [Arduino forum](https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940)) reports:

  | Parameter | Platinum handheld |
  |---|---|
  | Channels | 33, 52, 71 (2433 / 2452 / 2471 MHz), one packet per channel in rotation, ~30 rotations per action |
  | Data rate | 2 Mbps |
  | Address | 3 bytes, `E7 E7 E7` (the nRF24 power-on default) |
  | Payload | 5 bytes, static: `ID0 ID1 MODE GROUP ACTION` |
  | CRC / auto-ack | CRC-16 / off |
  | Original TX power | about −6 dBm |

  - `ID0 ID1` identify the remote; the shades reportedly ignore unknown IDs. Our two remotes have different IDs, and both work.
  - `MODE` is `20` with a group active. `GROUP` is `01/02/04/08` for groups 1–4, `FF` for All. `ACTION` is `A1` up held, `A2` down held, then `21` / `22` as a release tail.
  - **A red-mode (IR) arrow press also sends RF:** `ID 01 00 80` while held, with no direction. The shade needs it before it will act on the IR. Our finding, 2026-11-11.
  - Example from the forum (another remote's ID): `03 C1 20 01 A1`, group 1 up.
  - Channel 7 / 1 Mbps, quoted in one forum thread, conflicts with this. spitcool's notes associate those settings with the later PowerView Pebble remote.
  - **Confirmed on both our remotes** by sniffing ch 33 (2026-11-11). Details in [RF_PROTOCOL.md](docs/RF_PROTOCOL.md).

### Open questions

- **RF wake-up for IR?** Several secondhand sources say battery-powered Platinum shades ignore IR unless the remote also sends a short RF burst to wake them, which would explain why learning remotes fail. *Yes, by two tests* (2026-11-09): sealed in a foil tube that blocks its RF but lets its IR out, the original remote can't move the shade in IR mode, though it does when bare. And our clone's IR moves the shade only when the original remote is pressed at the same time (aimed away); alone, 0 of 20. See [Shade tests](docs/IR_PROTOCOL.md#shade-tests). The RF it sends is `ID 01 00 80` (sniffed 2026-11-11; spitcool's notes had recorded this frame without knowing its trigger).
- **Can the clone send the wake itself?** *Yes* (2026-11-11): the clone's own `ID 01 00 80` followed by its IR moves the shade with no original remote, including stops. **The ID matters only for group commands** (2026-11-12): a made-up ID gets no group response (0 of 12), but its wake + IR still moves the shade. So point-and-press needs no paired ID; group control needs a paired remote's ID. Next: more IR range (aim, LED power, beam width).
- **IR behavior on hold:** repeat frames, toggle bits or counters.

### Reverse-engineering plan

1. **Capture IR.** A TSOP38238 receiver records raw mark and space timings for every button in both modes, short press and held, several times each, to find any changing bits.
2. **Replay IR.** Send the raw timings back at 40 kHz through a TSAL6200 940 nm LED and confirm a shade responds.
3. **Screen the nRF24 modules.** The batch is probably clones, with a reported ~50% SPI failure rate, so every module gets tested. 2 Mbps is the rate that matters.
4. **Prove the sniffer.** A second module (radio B) transmits known packets at the Platinum settings while the first receives (`beacon`). Without this, an empty sniff means nothing.
5. ✅ **Sniff a group command** (positive control): group 1 + UP showed `ID 20 01 A1`, then the release tail. This also gave us our remote IDs.
6. ✅ **Sniff an IR-mode press**, the RF wake-up experiment: `ID 01 00 80` while the arrow is held.
7. **Fallbacks:** promiscuous sniffing (`promisc`) if the published settings catch nothing, or tapping the remote's own nRF24 SPI bus. Not needed so far.
8. **Emulate.** Transmit with the original's timing: the wake before each IR command (✅ works), then group commands with held repeats and release packets (✅ works).
9. **Build the handheld:** buttons with the remote's behaviour (group keys open a green window for the arrows), a red/green indicator LED, deep sleep, battery and enclosure.

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
| Radio B SCK / MISO / MOSI | 25 / 26 / 27 | optional second nRF24 for loopback checks, on HSPI |
| Radio B CSN / CE | 13 / 21 | |

Buttons are active-high so the original ESP32 can wake from deep sleep with `ext1` / `ESP_EXT1_WAKEUP_ANY_HIGH`.

## Firmware

PlatformIO + Arduino framework. Libraries: [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) 2.8.6 and [RF24](https://github.com/nRF24/RF24) 1.4.11 on `espressif32@6.9.0`.

```
pio run -t upload
pio device monitor        # 115200 baud
```

Type `help` in the monitor. Sending any line stops a running capture, scan or sniff, except a `#` comment.

| Command | What it does |
|---|---|
| `ir [quiet]` | Capture IR; print the decoded HD frame summary (code, frames, duration) plus the raw `uint16_t rawData[]` (`quiet`: summary only) |
| `send <up\|down\|bbbb> [ms]` | Send the RF wake, then HD frames at 40 kHz for `ms` (default 1000). `up`/`down` are channel A; use `1001`/`1010` for B. Max 5000 ms for OPEN, 10000 otherwise |
| Up / Down arrow keys | Send `up` / `down` (channel A) with the RF wake, at the prompt or during capture. Pattern set by `seq` |
| `seq [on\|off\|rf\|gap\|ir <ms>]` | Arrow-key pattern. `on` (default, experimental): RF on ch 33, IR, RF on ch 52, IR, RF on ch 71, IR, with `rf` ms of wake per channel (default 50), a `gap` before each IR (0) and `ir` ms per IR step (170). `off`: one 450 ms wake on all channels, then one 500 ms IR burst |
| `id [hex\|clear]` | Show or set the remote ID used in the wake frame (from a sniff). Saved in the ESP32's flash, not in the source |
| `grp <1-4\|all> <up\|down> [ms]` | RF group command, as in green mode: `ID 20 <group> A1/A2` for `ms` (default 300, max 5000), then the `21`/`22` release tail for 330 ms, on ch 33/52/71. RF only, no aiming. DOWN codes are from spitcool's notes, not yet sniffed by us |
| `wake [ms\|on\|off]` | Send the RF wake (`ID 01 00 80` on ch 33/52/71) alone for `ms` (default 450), or turn the automatic wake before `send` / arrow keys on or off |
| `# <note>` | Comment for the serial log. Doesn't stop a running capture, scan or sniff |
| `play [slot\|last] [n] [gap_ms]` | Replay a capture at 40 kHz |
| `save <slot>` / `list` / `dump [slot]` | Keep up to 8 captures in RAM |
| `test [label]` | nRF24 self-test: register read-back per data rate → PASS/FAIL (needs 2 Mbps) |
| `info` | Print the nRF24 register details |
| `scan` | Carrier-detect sweep over channels 0–125 |
| `sniff` | Receive with the current settings. Prints one line per run of identical packets: count, payload, decoded fields, span and packet gaps |
| `beacon [n]` | Loopback check: radio B sends `n` packets (default 100) with the current settings; prints sent / received |
| `hd` | Preset for the Platinum handheld (the default): ch 33, 2 Mbps, `E7E7E7`, CRC-16, 5 bytes. Use `ch 52` / `ch 71` for the other channels |
| `promisc aa\|55` | Fallback preset for address-less sniffing; turns the duplicate filter on |
| `ch`, `rate`, `aw`, `addr`, `crc`, `pl`, `filter`, `cfg` | Sniffer settings |

Source layout:

```
include/pins.h        pin map
include/hw_config.h   what's wired on the breadboard (gates hardware tests)
src/main.cpp          serial menu / dispatch
src/ir_tools.*        IR capture, dump, replay
src/rf_tools.*        nRF24 self-test, scanner, sniffer, loopback beacon
lib/hdcore/           hardware-independent logic (HD IR frame codec, Platinum RF payloads, parsing,
                      address order, dup filter, packet runs)
test/                 Tier 1 host tests (test_native_*) and Tier 2 device tests (test_device_*)
```

## Testing

```
pio test -e native                          # host unit tests, no hardware
pio test -e esp32dev_test --upload-port COM11    # on-device tests
```

See [TESTING.md](TESTING.md) for the three test tiers and the gate a release must pass.

## References

- [NRF24L01+ hack on Hunter Douglas shades](https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940) (Arduino forum)
- [ESP8266 and NRF24L01+ Hunter Douglas Platinum Remote](https://forum.arduino.cc/t/esp8266-and-nrf24l01-hunter-douglas-platinum-remote/918887) (Arduino forum)
- [spitcool/hd-platinum-rf](https://github.com/spitcool/hd-platinum-rf): protocol research
- [Platinum Technology remote guide (PDF)](https://blindparts.com/wp-content/uploads/1970/01/Hunter-Douglas-Platinum-Technology-Remote-Control-Guide.pdf)
- [Bond forum: IR replay attempts / RF wake theory](https://forum.bondhome.io/t/hunterdouglas-powerrise-2-0-with-platinum-technology/3050)
- [AVS forum: automating PowerRise (2007)](https://www.avsforum.com/threads/automating-hunter-douglas-powerrise.852487/)
