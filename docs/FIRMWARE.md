# Firmware and pin map

> **TL;DR** (summary checked 2026-11-18 15:07): one PlatformIO codebase with a serial menu (type `help`). The default build is for the breadboard DevKitC; `pio run -e handheld` builds for the handheld PCB. Pins for both are in [`include/pins.h`](../include/pins.h).

One PlatformIO codebase runs on **two hardware targets**. Which one you get depends on the build environment you pick.

| Target | Build env | What it is | Status |
|---|---|---|---|
| **Breadboard** | `esp32dev` (default) | An ESP32-DevKitC wired to an IR receiver, an IR LED and an nRF24 module. This is where the reverse engineering was done and where the firmware is tested. | Working |
| **Handheld PCB, rev 1** | `handheld` | The custom remote board (ESP32-WROOM-32E, keys, A/B slider, indicator LED, no IR receiver). | Builds. Not yet run on hardware (boards ordered 2026-11-17) |

**What this firmware is today.** It is a serial-menu toolkit (type `help` in the monitor): IR capture and replay, nRF24 self-test, scan and sniff, and commands that drive shades directly (`send`, `grp`, `pair`). It is not yet the finished handheld firmware. The remote's button behaviour (group window, hold-to-send arrows, deep sleep) is the next phase and isn't written.

**How the targets are switched.** The `handheld` environment sets a compile-time flag, `-DHD_BOARD_HANDHELD`. Two headers then select what differs:

- [`include/pins.h`](../include/pins.h) holds each target's pin map (it is the only place GPIO numbers are defined) and what the board can have (`HAS_IR_RX`, …). Commands check these: on the handheld, `ir` prints a message instead of touching pins, and `btn` and `led` cover its keys, slider, battery sense and indicator.
- [`include/hw_config.h`](../include/hw_config.h) records what is actually wired on a given bench (`HW_NRF24`, `HW_IR_LOOPBACK`, `HW_BUTTONS`, …). The on-device tests read it and report anything not wired as IGNORED rather than failed. Keep it in sync with your wiring.

```
pio run -t upload         # breadboard
pio run -e handheld       # handheld PCB profile
```

The protocol logic ([`lib/hdcore/`](../lib/hdcore/)) is hardware-independent, shared by both targets and tested on the host. See [TESTING.md](../TESTING.md) for the test tiers and the [README](../README.md) for the project overview.

The pin map below is the **breadboard's**. The handheld's is in [HARDWARE.md](HARDWARE.md#pin-map-esp32-wroom-32e).

## Pin map

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

This is the breadboard (DevKitC). The handheld PCB uses a different map for the keys, slider, battery sense and LEDs: see [HARDWARE.md, Pin map](HARDWARE.md#pin-map-esp32-wroom-32e) and `include/pins.h` (`HD_BOARD_HANDHELD`, env `handheld`).

Buttons are active-high so the original ESP32 can wake from deep sleep with `ext1` / `ESP_EXT1_WAKEUP_ANY_HIGH`.

## Firmware

PlatformIO + Arduino framework. Libraries: [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) 2.8.6 and [RF24](https://github.com/nRF24/RF24) 1.4.11 on `espressif32@6.9.0`.

```
pio run -t upload         # breadboard (esp32dev)
pio device monitor        # 115200 baud
pio run -e handheld       # handheld PCB profile (builds; untested on hardware)
```

The firmware version is shown at boot and in `help`. Type `help` in the monitor. Sending any line stops a running capture, scan or sniff, except a `#` comment.

| Command | What it does |
|---|---|
| `ir [quiet]` | Capture IR; print the decoded HD frame summary (code, frames, duration) plus the raw `uint16_t rawData[]` (`quiet`: summary only) |
| `send <up\|down\|bbbb> [ms]` | Repeat `seq` steps (RF wake, then HD frames at 40 kHz) for `ms` (default 1000). With `wake off`, IR alone. `up`/`down` are channel A; use `1001`/`1010` for B. Max 5000 ms, either direction |
| Up / Down arrow keys | Send `up` / `down` (channel A) with the RF wake, at the prompt or during capture. Pattern set by `seq` |
| `seq [on\|off\|rf\|gap\|ir <ms>]` | Arrow-key pattern. `on` (default, experimental): RF on ch 33, IR, RF on ch 52, IR, RF on ch 71, IR, with `rf` ms of wake per channel (default 50), a `gap` before each IR (0) and `ir` ms per IR step (50, three frames). `off`: one 450 ms wake on all channels, then one 500 ms IR burst |
| `hold [on\|off\|first <ms>]` | Arrow keys send `seq` steps for as long as the key is held, and print the hold time. `first` is the release timeout before the keyboard's first repeat (default 700 ms). Either arrow stops at 5000 ms |
| `id [hex\|new]` | Show the remote ID used in wake, group and setup frames; set it (e.g. an original remote's, from a sniff); or `new` for a fresh random one. Saved in the ESP32's flash, not in the source. A unit with none generates a random one at boot |
| `grp <1-4\|all> <up\|down> [ms]` | RF group command, as in green mode: `ID 20 <group> A1/A2` for `ms` (default 300, max 5000), then the `21`/`22` release tail for 330 ms, on ch 33/52/71. RF only, no aiming. DOWN works on the shades (2026-11-11); the DOWN codes come from spitcool's notes and haven't been sniffed by us |
| `pos <mask> <0-255> [ms50 ms32]` | **Experimental** (fw 0.9.7). Absolute-position frames as spitcool decoded them from the Platinum bridge: `ID 50 <mask> <pos>` for `ms50` (default 1490), then `ID 32 <mask> <pos>` for `ms32` (default 300), on ch 33/52/71, with the stored ID. `mask` is one hex byte (nonzero; the bridge's are one-hot, `01`–`40`), `pos` is decimal 0–255. Each phase max 2000 ms; `0` skips a phase. Works on our shades (2026-11-18): the mask is the handheld group byte, `01`–`08`, combinable; 0 and 255 are the two limits; the ID must be paired. See [RF_PROTOCOL.md](RF_PROTOCOL.md#absolute-position-experimental) |
| `pair <1-4\|all> <add\|remove>` | Group assignment under the stored ID: setup frames (`ID 30\|31 GROUP 80`) interleaved with OPEN / CLOSE IR, using the `seq` timings. Aim at one shade's eye from a few inches. Wake the shade first, or retry |
| `wake [ms\|on\|off]` | Send the RF wake (`ID 01 00 80` on ch 33/52/71) alone for `ms` (default 450), or turn the automatic wake before `send` / arrow keys on or off |
| `# <note>` | Comment for the serial log. Doesn't stop a running capture, scan or sniff |
| `play [slot\|last] [n] [gap_ms]` | Replay a capture at 40 kHz |
| `save <slot>` / `list` / `dump [slot]` | Keep up to 8 captures in RAM |
| `test [label]` | nRF24 self-test: register read-back per data rate → PASS/FAIL (needs 2 Mbps) |
| `info` | Print the nRF24 register details |
| `scan` | Carrier-detect sweep over channels 0–125 |
| `sniff` | Receive with the current settings. Prints one line per run of identical packets: count, payload, decoded fields, span and packet gaps |
| `hd` | Preset for the Platinum handheld (the default): ch 33, 2 Mbps, `E7E7E7`, CRC-16, 5 bytes. Use `ch 52` / `ch 71` for the other channels |
| `promisc aa\|55` | Fallback preset for address-less sniffing; turns the duplicate filter on |
| `ch`, `rate`, `aw`, `addr`, `crc`, `pl`, `filter`, `cfg` | Sniffer settings |
| `btn` | Show button state (breadboard: UP and DOWN; handheld PCB: all keys, the A/B slider and the battery voltage) |
| `led <r\|g\|both\|off>` | Drive the indicator LED (handheld PCB only) |
| `pin <gpio> [ms]` | Logic probe: level, edge count and % low on a GPIO (default 3000 ms). Refuses the flash pins and the IR and nRF24 outputs |

Source layout:

```
include/pins.h        pin maps (breadboard default; handheld PCB with HD_BOARD_HANDHELD)
include/hw_config.h   what's present on each board (gates hardware tests)
src/main.cpp          serial menu / dispatch
src/ir_tools.*        IR capture, dump, replay
src/rf_tools.*        nRF24 self-test, scanner, sniffer, wake / group / setup frames
lib/hdcore/           hardware-independent logic (HD IR frame codec, Platinum RF payloads, parsing,
                      address order, dup filter, packet runs, arrow-key hold tracking)
test/                 Tier 1 host tests (test_native_*) and Tier 2 device tests (test_device_*)
```
