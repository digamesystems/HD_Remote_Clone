# Hunter Douglas Platinum remote replacement — project handoff

Context for Claude Code. Summarized from a planning conversation in Claude chat (Nov 6–7, 2026).

## Goal
Build a DIY replacement for a Hunter Douglas **Platinum Technology** remote (manual © 2013, pre-PowerView). One original remote works; a second is degraded (IR works only within ~30 cm, RF within ~1 m — both paths degraded, so suspect a power problem: batteries, contacts, or regulator; worth cleaning and re-testing).

User is comfortable with **ESP32 + PlatformIO** (Arduino framework assumed).

## What the original remote does
- **Dual mode:** IR (40 kHz carrier per manual) and RF (2.4 GHz).
  - Indicator **red** = IR / point-and-press mode (individual shade, aim at its "sensor eye").
  - Indicator **green** = RF group mode. Press a group button (1–4 or All) no more than 5 s before OPEN/CLOSE.
  - Manual describes IR as press-and-release; pressing the opposite button stops movement.
- **RF uses an nRF24L01+ inside the remote.** Per Arduino forum reverse engineering (one hobbyist, ~2020):
  - Channel 7 (2.407 GHz) on his remote. **Verify on ours; do not assume.**
  - Remote cycles three channels, one 5-byte transfer per channel, cycle repeats ~30×.
  - Holding UP sends the command packet repeatedly (shade moves immediately). On release, a different "release" packet is sent ~60×.
  - Example bytes from that thread: `3 193 32 1 161` (group 1 up, repeated while held), `3 193 32 1 33` (release). Group byte: 1/2/4/8 for groups, 255 for All; 161 = up, 162 = down.
  - Method used: SPI piggyback on the remote's nRF24 (SCK, CSN, MOSI) to dump its register config.
  - A follow-up project emulates part no. 2984495100 with NodeMCU + nRF24L01+, configurable for the remote's ID.

### Open hypothesis: RF "wake-up" burst for IR
Three independent but **secondhand** sources (AVS forum 2007, Bond forum 2022, an HD automation dealer) say battery-powered Platinum shades ignore IR unless the remote first sends a brief RF burst to wake them, which is why learning remotes fail. No capture or timing details found. **Unconfirmed.** The user's observation (the degraded remote works in IR at 30 cm) doesn't discriminate, since its RF still reaches ~1 m.

**Test:** sniff RF while pressing buttons in IR (red) mode, then replay captured IR alone at a shade.

## Hardware

### Parts on hand / arriving
- ESP32 dev board: **ESP32-DevKitC with ESP32-WROOM-32D** — confirmed Nov 7 from the can marking and the ROM boot banner (`ets Jun 8 2016`, original ESP32). No PSRAM; PlatformIO `board = esp32dev`; pin map applies as-is. Enumerates as CP210x on COM11.
- Bridgold kit: TSOP38238 IR receivers + TSAL6200 940 nm IR LEDs (arriving Sat Nov 7)
  - TSOP38238 is a 38 kHz part; fine for capturing a 40 kHz remote at close range
  - TSAL6200: 100 mA continuous, 200 mA peak, ±17° beam
  - Unbranded kit; parts may not be genuine Vishay
- hiBCTR 10-pack "NRF24L01+" PCB-antenna modules (arriving Mon Nov 9)
  - Spec "+7 dB max TX" suggests **Si24R1 clones** (genuine nRF24L01+ maxes at 0 dBm). Inference, not confirmed.
  - One verified-buyer review: 5 of 10 failed SPI comms on a test bench; genuine units passed the same bench. Single data point.
  - Plan: screen every module on arrival. Fallback for flaky sniffing: a genuine Nordic module from Digi-Key.
- Still needed: NPN (2N2222 / PN2222 preferred, 2N3904 OK for 1 LED), resistors (1 kΩ, 33 Ω, 2× 10 kΩ, 100 Ω), caps (100 µF, 10 µF, 2× 100 nF ceramic), 2 momentary N.O. pushbuttons, optional nRF24 adapter bases (onboard 3.3 V regulator).

### Pin map (original ESP32)
| Function | GPIO | Notes |
|---|---|---|
| IR receive (TSOP38238 OUT) | 14 | |
| IR transmit (NPN base via 1 kΩ) | 4 | |
| Button UP | 32 | active-HIGH, external 10 kΩ pull-down, RTC-capable |
| Button DOWN | 33 | active-HIGH, external 10 kΩ pull-down, RTC-capable |
| nRF24 SCK | 18 | VSPI default |
| nRF24 MOSI | 23 | VSPI default |
| nRF24 MISO | 19 | VSPI default |
| nRF24 CSN | 5 | VSPI default CS |
| nRF24 CE | 17 | **move (e.g. 21) on WROVER boards; 16/17 used by PSRAM** |
| nRF24 IRQ (optional) | 16 | **move (e.g. 22) on WROVER boards** |

Buttons are active-high so deep-sleep wake can use `ext1` with `ESP_EXT1_WAKEUP_ANY_HIGH` (original ESP32's ext1 only supports ANY_HIGH or ALL_LOW). On S3/C3, revisit pins and polarity.

### Netlist (sheet 1: IR + buttons)
- TSOP38238 (pinout facing dome: OUT, GND, VS): VS ← 100 Ω ← 3V3; 100 nF VS→GND at the TSOP; GND→GND; OUT→GPIO 14.
- IR LED: 5V/VIN → 33 Ω (22 Ω if fed from 3.3 V) → TSAL6200 anode; cathode → NPN collector; emitter → GND. GPIO 4 → 1 kΩ → base. ~100 mA pulses.
  - 100 µF bulk + 100 nF ceramic across 5V–GND; ceramic placed at the top of the 33 Ω, grounded near the emitter.
  - Two LEDs: duplicate the 33 Ω + LED branch, same collector; use a 2N2222 (≈200 mA).
- Buttons: 3V3 → switch → GPIO 32/33; 10 kΩ GPIO → GND each. Debounce in software (~20–30 ms).

### Netlist (sheet 2: nRF24)
- VCC → 3V3 (3.3 V only!). With an adapter base: feed 5 V, omit the cap.
- 10 µF across VCC/GND soldered at the module (stripe to GND).
- SPI per pin map. Keep SPI wires short (< ~15 cm); retest "failed" modules on short wires before discarding.

## Firmware plan (next step)
One PlatformIO project, serial-menu driven, usable from this weekend through Monday:
1. **IR capture/dump.** IRremoteESP8266, IRrecvDumpV2-style. Raise capture buffer and timeout; dump raw mark/space timings (protocol is proprietary). Capture every button × both modes, short press vs 2 s hold, several repeats each, to detect toggle bits or counters and repeat behavior.
2. **IR replay.** `sendRaw(data, len, 40)`: 40 kHz carrier, exact raw timings, optionally 2–3 back-to-back sends.
3. **nRF24 module self-test.** RF24 lib: `begin()`, `isChipConnected()`, `printDetails()`; label pass/fail.
4. **nRF24 sniffer.** Start at channel 7. Data rate and address width unknown; consider a promiscuous-mode approach or the SPI piggyback register dump of the original remote. Also run it during IR-mode presses (wake-burst test).
5. Later: RF transmit emulation (group up/down, held-repeat + release packets), button handling, deep sleep with ext1 wake.

## Open questions
- Remote's RF ID, channel(s), data rate, and address — get these from the SPI piggyback or a sniff.
- Does IR need an RF wake burst? (see above)
- IR repeat/continuation behavior on hold.
- Exact ESP32 board/module (WROOM vs WROVER vs S3/C3).
- Does the degraded remote recover with new batteries and cleaned contacts?

## References
- Arduino forum, "NRF24L01+ hack on Hunter Douglas shades": https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940
- Arduino forum, "ESP8266 and NRF24L01+ Hunter Douglas Platinum Remote": https://forum.arduino.cc/t/esp8266-and-nrf24l01-hunter-douglas-platinum-remote/918887
- hd-platinum-rf (protocol research; treats handheld RF framing as partly hypothesis; intentionally no TX): https://github.com/spitcool/hd-platinum-rf
- Platinum remote guide (IR 40 kHz, RF 2.4 GHz): https://blindparts.com/wp-content/uploads/1970/01/Hunter-Douglas-Platinum-Technology-Remote-Control-Guide.pdf
- Bond forum thread (IR replay failed; RF wake theory): https://forum.bondhome.io/t/hunterdouglas-powerrise-2-0-with-platinum-technology/3050
- AVS forum (RF wake burst, 2007): https://www.avsforum.com/threads/automating-hunter-douglas-powerrise.852487/
- Remote Central PowerRise IR codes (may not match Platinum): https://www.remotecentral.com/cgi-bin/codes/hunter_douglas/power_rise/
- TSAL6200 datasheet (Vishay)
