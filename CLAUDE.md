# Hunter Douglas Platinum remote replacement — project handoff

Context for Claude Code. Summarized from a planning conversation in Claude chat (Nov 6–7, 2026).

## Working conventions (shorthand commands)
- **Single-letter commands** (fast dev loop; letters combine, e.g. "cp", "cpd", "t1 d"; run in the order c → p → d):
  - **"d"**: deploy. One step, no tests: `pio run -t upload --upload-port COM11` (builds and uploads; a failed build uploads nothing), then reopen the serial monitor in a separate window (`Start-Process pio device monitor --port COM11 --baud 115200 --filter time`).
  - **"c"**: local git commit of all current changes with a descriptive message. Runs `pio run` first and halts if the build fails (the user's rule: never commit a broken build). No tests.
  - **"p"**: push to GitHub (`origin/main`, private repo digamesystems/HD_Remote_Clone). If any pushed commit hasn't passed t1, say so.
  - **"t1"**: Tier 1 native tests, `pio test -e native`.
  - **"t2"**: Tier 2 board tests, `pio test -e esp32dev_test --upload-port COM11` (all device suites). Leaves **test firmware** on the board; say so, and that "d" restores the app.
  - **"cpdt"**: the full gated release: `pio run`, t1, then t2 for test_device_board plus each changed area (`-f test_device_<area>`), then c, p, d. If any step fails, **halt**: no commit, push, or deploy; report the errors. Tier 2 tests IGNORED for absent hardware (per `include/hw_config.h`) don't block, but say plainly which areas were not verified.
  - Typical times (incremental): build 13 s, t1 15 s, t2 ~20 s per suite, d ~25 s.
- Serial/upload must run **outside the Bash sandbox**; sandboxed reads of COM11 return nothing.
  - A monitor window holding COM11 blocks upload/t2. Ask the user to close it (Ctrl+C) first, or close it if they say to.
  - If upload can't connect, have the user hold BOOT and tap EN.
- Put hardware-independent logic in `lib/hdcore/` with Tier 1 tests, not in `src/`. The user can wire loopbacks or diagnostics on the breadboard to make hardware testable; ask when that would help, and keep `include/hw_config.h` in sync with what's wired.
- **Community contribution is a goal, not just solving our own problem.** Little public work exists on Platinum IR/RF (scattered, mostly dormant forum threads; no published IR protocol found). Write docs and captures so outsiders can reuse them: state what's confirmed vs hypothesis, keep evidence files committed, and keep code reusable (`lib/hdcore/`). Later options: make the repo public, upstream the IR protocol to IRremoteESP8266, post write-ups on the Arduino/HA threads where people got stuck, maybe an ESPHome component.

## Goal
Build a DIY replacement for a Hunter Douglas **Platinum Technology** remote (manual © 2013, pre-PowerView). One original remote works; a second is degraded (IR works only within ~30 cm, RF within ~1 m — both paths degraded, so suspect a power problem: batteries, contacts, or regulator; worth cleaning and re-testing).

User is comfortable with **ESP32 + PlatformIO** (Arduino framework assumed).

## What the original remote does
- **Dual mode:** IR (40 kHz carrier per manual) and RF (2.4 GHz).
  - Indicator **red** = IR / point-and-press mode (individual shade, aim at its "sensor eye").
  - Indicator **green** = RF group mode. Press a group button (1–4 or All) no more than 5 s before OPEN/CLOSE.
  - **Observed key/mode behaviour (user, Nov 7):** 7 keys: UP, DOWN, 1, 2, 3, 4, ALL.
    - Group keys 1–4 and ALL work **only in RF**; pressing one always flashes the green LED.
    - The arrows are **IR (red) by default**. After a group key, the arrows act in green/RF mode for a time window, then revert to red/IR.
    - In normal use, only the arrows send IR (analyzer-verified; setup-mode presses not captured yet). **Arrows in green mode (right after a group key) send no IR** (verified with `ir quiet`, Nov 7): green = RF only.
    - **A/B slide switch** is the **Infrared Channel Selector** (manual pp. 1, 18, 22). It changes the IR code (captured Nov 8). The manual lists **4 groups**, not 8. Whether it affects RF is unknown.
      - The forum's group byte (1/2/4/8, 255 = All) still needs verifying on our remote.
    - **RF window ≈ 6.5 s** from a group-key press until the arrows send IR again (user stopwatch, Nov 7; the manual says 5 s). **Pressing a group key again during the window restarts it** (it's a retriggerable timer).
    - Unknown: whether arrow presses extend the window, and whether red-mode arrows key the radio (RF-wake question).
    - The clone should copy this UX.
  - Manual describes IR as press-and-release; pressing the opposite button stops movement.
- **IR protocol: OUR CAPTURES (Nov 7, working remote, red mode, VS1838B/TSOP on GPIO 32).** Full write-up: `docs/IR_PROTOCOL.md` (keep it updated with new findings). Log: `captures/ir_red_open_close_20261107_132807.log`.
  - Frame = header mark ~2.75 ms + space ~2.75 ms, then **4 bits**, each a ~1.0 ms mark + space (~1.07 ms = 0, ~2.75 ms = 1). Frame period exactly **17.19 ms**, back to back, no extra gap.
  - One press = one burst of identical frames, lasting as long as the key is held (about 20 frames / 0.35 s for a quick press). No toggle bit, no counter, **no shade address** (fits point-and-press).
  - **Codes:** switch A: UP (OPEN) = `0101`, DOWN (CLOSE) = `0110` (ESP32 log + logic analyzer). Switch B: UP = `1001`, DOWN = `1010` (ESP32 log, `captures/ir_ab_channel_20261108.log`). Inferred layout: bits 3–2 = channel (`01` A, `10` B), bits 1–0 = command (`01` OPEN, `10` CLOSE). Our shade listens on A. **No other button sends IR in normal use** (logic analyzer); stop = press the opposite arrow.
  - On release the remote stops mid-frame (last frame truncated); no release/stop frame. Analyzer files: `captures/la_ir_red_{up,down}_arrow_20261107.sr` (100 kHz, D0 = receiver OUT).
  - Timing measured through a demodulating receiver: marks read slightly short and spaces slightly long. The true carrier and duty cycle still need measuring with the analyzer on the LED drive.
  - Implication: replay can be synthesized from a 4-bit code rather than raw timings. **RF-wake question still open** (see below).
- **RF uses an nRF24L01+ inside the remote.** Working parameters (Nov 9) come from spitcool/hd-platinum-rf `docs/rf-protocol.md` (2026-10-25), whose evidence includes FCC filings, a community register dump and their own SDR captures. Firmware defaults and `hd` preset use them (`lib/hdcore/src/hd_rf.h`). **Not yet verified on our remote.**
  - Channels 33, 52, 71 (2433/2452/2471 MHz), one packet per channel in rotation, ~30 rotations per action, ~193 µs between packets. A single-channel listener sees ~1 in 3.
  - 2 Mbps; 3-byte address `E7 E7 E7`; static 5-byte payload; CRC-16; auto-ack off; TX ~−6 dBm. Frames have no packet-control field (88 bits on air), which clones may handle differently: the group-mode sniff is the real proof our modules can hear the remote.
  - Payload `ID0 ID1 MODE GROUP ACTION`: MODE `20` group active, `01` reportedly no group; GROUP one-hot `01/02/04/08`, `FF` All, `00` none; ACTION `A1`/`A2` held up/down, `21`/`22` release tail; `80` seen with mode 01 / group 00 (trigger unknown). No rolling code seen.
  - **Shades reportedly reject unknown IDs, so the clone must use our remote's ID. Treat the ID as private: redact it (`xx xx`) in anything published.**
  - The Arduino forum thread's example `3 193 32 1 161` = `03 C1 20 01 A1` fits this layout, but its "channel 7, 1 Mbps" doesn't; spitcool associates those with the later PowerView Pebble. Ignore channel 7.
  - **Hypothesis to test:** red-mode arrow presses send `ID 01 00 80` (no group selected), and that's the RF needed alongside IR. If the payload differs, record it exactly.
  - Licensing: spitcool's docs are CC BY-SA 4.0 / AGPL. Use the facts, cite the source; don't copy their prose or code. Our repo has no LICENSE yet: add one before exchanging code. John opened spitcool/hd-platinum-rf#1 ("sister project"); a follow-up comment waits on today's red-mode sniff.

### RF "wake-up" for IR: confirmed by two tests (Nov 9)
**Clone IR + remote RF (Nov 9):** clone alone 0/20; remote alone aimed away, ~20 presses, nothing; clone + remote pressed together moves the shade; clone UP + remote DOWN → shade follows the clone. So our IR encoding is correct, and the red-mode RF is an enable, not the command. **One RF enable arms one IR command, briefly:** a second clone command needs another remote arrow press (either arrow), including same-direction repeats and stop-while-moving; the IR must follow the remote press within ~1 s (rough). The clone must send RF immediately before every IR command. Next unknowns: the receptive window after the RF (1/2/5 s delay test), and the RF's content (sniff or SPI tap).

Three independent but **secondhand** sources (AVS forum 2007, Bond forum 2022, an HD automation dealer) say battery-powered Platinum shades ignore IR unless the remote also sends a brief RF burst to wake them, which is why learning remotes fail. No capture or timing details found anywhere.

**Our evidence (foil-tube test, Nov 9, user's design; full write-up in `docs/IR_PROTOCOL.md`, Shade tests):** the remote sealed in a ~5 cm foil tube, back twisted shut, front open. Its IR decodes cleanly on our receiver at 30 cm; group key + arrow from inside the tube does NOT move the shade (RF blocked); a red-mode arrow at 30 cm from the sensor eye does NOT move the shade; the bare remote moves it both ways. Repeated bare/wrapped/bare. So: **IR carries the command (and selects the shade); RF is required alongside it.** Open: is the RF a bare wake-up, or does it carry the command or the pairing ID? That decides what the clone transmits.

Lessons: a hand barely blocks IR (14–18 of 20 frames still decode), and a foil hole ≥1.5 cm leaks RF. Always verify a shield with the receiver (IR) and the green-mode control (RF). These shades are **top-down/bottom-up with reversed arrows (DOWN opens, UP closes)**: test from mid-travel, because the Nov 8 `send down` on an open shade may have had no travel.

Public summary page (private until shared): https://claude.ai/artifact/X8bRVQXfk2P1AV8owhsCF2. Exact trial counts, tube width and the remote's insertion depth weren't recorded.

**Our shades are battery models converted to DC power:** the user replaced the battery pack with an always-on DC brick and a compatible cable, because battery changes meant ladder work. They have the **same electronics and firmware as battery shades**, so any power-saving or wake-up behaviour would still be active; constant power does *not* weaken the hypothesis. The original batteries drained quickly, which weakly suggests a receiver that's often awake (the motor and the packs also matter).

Next:
1. **Clone IR + remote RF** (Claude web's idea, modified): small foil patch over only the remote's IR window (verify with `ir quiet`: no captures; verify group key + arrow still moves the shade). Then clone `send` while pressing the same arrow on the patched remote in red mode. Controls: clone alone, patched remote alone, opposite directions. Shade follows the clone → our IR encoding works once awake.
2. **nRF24 bring-up** (modules arriving Nov 9; one ESP32, the user prefers not to use a second): (a) screen modules with `test n`, keep those passing at 2 Mbps; (b) wire a second module as radio B and run `beacon` (loopback at the HD config, ch 33/52/71); (c) positive control: group 1 + UP should sniff as `ID 20 01 A1` then `21`, giving our ID; (d) red-mode arrow sniff: payload, count, timing, test the `01 00 80` hypothesis; (e) optional: `scan` at the foil tube's mouth.
3. Measure foil attenuation with a module at the tube's mouth; probe the remote's LED drive; optionally scope its battery current for radio-shaped steps.

**Manual notes for the clone:** holding OPEN 6 s at a fully open shade resets its stops and limits (p. 21), so `send` caps OPEN codes at 5 s (`play` isn't capped). Group assignment uses SETUP + group key + aimed arrow (p. 5), so IR selects the shade. A backup remote copies identity over RF (Master Reset, then OPEN on the original within 18", p. 7): a good sniffing target, but don't Master Reset either remote before capturing, since it may erase pairing.

## Hardware

### Parts on hand / arriving
- ESP32 dev board: **ESP32-DevKitC with ESP32-WROOM-32D** — confirmed Nov 7 from the can marking and the ROM boot banner (`ets Jun 8 2016`, original ESP32). No PSRAM; PlatformIO `board = esp32dev`; pin map applies as-is. Enumerates as CP210x on COM11.
- Bridgold kit (arrived Nov 7): 5× TSOP38238 IR receivers (black, pinout facing dome: OUT, GND, VS) + 5× TSAL6200 940 nm IR LEDs (clear blue-tint, long leg = anode)
  - TSOP38238 is a 38 kHz part; fine for capturing a 40 kHz remote at close range
  - TSAL6200: 100 mA continuous, 200 mA peak, ±17° beam
  - Unbranded kit; parts may not be genuine Vishay
- Gikfun EK8460 (arrived Nov 7): 10× **VS1838B** 38 kHz demodulating receivers (metal X shield; pinout facing front: OUT, GND, VCC; 2.7–5.5 V) + 10× 5 mm 940 nm IR LEDs (spares, good for loopback). A second receiver type to cross-check captures. Raw carrier frequency and duty cycle come from the logic analyzer on the remote's LED drive instead.
- 38 kHz "round dot" long-range IR receivers, CH0038K-compatible (arriving Sun Nov 8). A second demodulating receiver to cross-check captures against the unbranded TSOP38238s.
- hiBCTR 10-pack "NRF24L01+" PCB-antenna modules (arriving Mon Nov 9)
  - Spec "+7 dB max TX" suggests **Si24R1 clones** (genuine nRF24L01+ maxes at 0 dBm). Inference, not confirmed.
  - One verified-buyer review: 5 of 10 failed SPI comms on a test bench; genuine units passed the same bench. Single data point.
  - Plan: screen every module on arrival. Fallback for flaky sniffing: a genuine Nordic module from Digi-Key.
- **Logic analyzer (owned):** 24 MS/s, 8-channel, works with sigrok/PulseView (`fx2lafw` driver). USB 0925:3881; WinUSB installed via Zadig and working in PulseView as "Saleae Logic", D0–D7 (Nov 7). Inputs −0.5 to 5.25 V. Primary tool for RF reverse engineering (see firmware plan step 4).
- Still needed: NPN (2N2222 / PN2222 preferred, 2N3904 OK for 1 LED), resistors (1 kΩ, 33 Ω, 2× 10 kΩ, 100 Ω), caps (100 µF, 10 µF, 2× 100 nF ceramic), 2 momentary N.O. pushbuttons, optional nRF24 adapter bases (onboard 3.3 V regulator).

### Pin map (original ESP32)
| Function | GPIO | Notes |
|---|---|---|
| IR receive (TSOP38238 / VS1838B OUT) | 14 | moved 14→32 on Nov 7, back to 14 on Nov 8 (user wiring choice) |
| IR transmit (NPN base via 1 kΩ) | 4 | |
| Button UP | 32 | active-HIGH, external 10 kΩ pull-down, RTC-capable (RTC_GPIO9); swapped with IR RX (not wired yet) |
| Button DOWN | 33 | active-HIGH, external 10 kΩ pull-down, RTC-capable |
| nRF24 SCK | 18 | VSPI default |
| nRF24 MOSI | 23 | VSPI default |
| nRF24 MISO | 19 | VSPI default |
| nRF24 CSN | 5 | VSPI default CS |
| nRF24 CE | 17 | **move (e.g. 21) on WROVER boards; 16/17 used by PSRAM** |
| nRF24 IRQ (optional) | 16 | **move (e.g. 22) on WROVER boards** |
| Radio B SCK / MISO / MOSI | 25 / 26 / 27 | optional second nRF24 for loopback (`beacon`, t2), own SPI bus (HSPI, remapped: default SCK 14 is IR RX, MISO 12 is a strapping pin) |
| Radio B CSN / CE | 13 / 21 | `HW_NRF24_B` in hw_config.h |

Buttons are active-high so deep-sleep wake can use `ext1` with `ESP_EXT1_WAKEUP_ANY_HIGH` (original ESP32's ext1 only supports ANY_HIGH or ALL_LOW). On S3/C3, revisit pins and polarity.

### Netlist (sheet 1: IR + buttons)
- TSOP38238 (pinout facing dome: OUT, GND, VS): VS ← 100 Ω ← 3V3; 100 nF VS→GND at the TSOP; GND→GND; OUT→GPIO 14.
- IR LED: 5V/VIN → 33 Ω (22 Ω if fed from 3.3 V) → TSAL6200 anode; cathode → NPN collector; emitter → GND. GPIO 4 → 1 kΩ → base. ~100 mA pulses.
  - 100 µF bulk + 100 nF ceramic across 5V–GND; ceramic placed at the top of the 33 Ω, grounded near the emitter.
  - Two LEDs: duplicate the 33 Ω + LED branch, same collector; use a 2N2222 (≈200 mA).
- Buttons: 3V3 → switch → GPIO 32 (UP) / 33 (DOWN); 10 kΩ GPIO → GND each. Debounce in software (~20–30 ms).

### Netlist (sheet 2: nRF24)
- VCC → 3V3 (3.3 V only!). With an adapter base: feed 5 V, omit the cap.
- 10 µF across VCC/GND soldered at the module (stripe to GND).
- SPI per pin map. Keep SPI wires short (< ~15 cm); retest "failed" modules on short wires before discarding.
- Radio B (optional, loopback): same power rules and its own 10 µF cap; SCK 25, MISO 26, MOSI 27, CSN 13, CE 21. A few cm from the main module is fine (beacon uses PA_MIN).

## Firmware plan (next step)
One PlatformIO project, serial-menu driven, usable from this weekend through Monday:
1. **IR capture/dump.** IRremoteESP8266, IRrecvDumpV2-style. Raise capture buffer and timeout; dump raw mark/space timings (protocol is proprietary). Capture every button × both modes, short press vs 2 s hold, several repeats each, to detect toggle bits or counters and repeat behavior.
2. **IR replay.** `send up|down|bbbb [ms]` synthesizes HD frames from the 4-bit code (`lib/hdcore/src/hd_ir.*`) and sends them at 40 kHz. Raw `play` of captures also exists. Needs the IR LED driver to test on a shade (acceptance A2/A5).
3. **nRF24 module self-test.** RF24 lib: `begin()`, `isChipConnected()`, `printDetails()`; label pass/fail.
4. **RF config via logic analyzer (preferred first step).** Open the *degraded* remote and clip onto its nRF24 SPI bus (SCK, MOSI, MISO, CSN), plus CE, IRQ and the IR LED drive. PulseView's nRF24L01 decoder (on top of SPI) shows RF_CH (channels and hopping), RF_SETUP (rate), SETUP_AW/TX_ADDR (address), EN_AA/CONFIG (CRC), and W_TX_PAYLOAD (packets with timing).
   - **Wake-burst test:** capture CE/SPI together with the IR LED drive during red-mode presses. RF activity before IR confirms the hypothesis and gives its timing.
   - Also gives the exact IR carrier frequency and duty cycle.
   - Caveat: 24 MS/s is fine for SPI ≤ ~4 MHz. QFN-20 nRF24 pins 1–6 = CE, CSN, SCK, MOSI, MISO, IRQ.
5. **nRF24 sniffer.** Exact-match sniffing at the published Platinum settings (the default, `hd` preset); output is one line per run of identical packets (count, decoded payload, span, gaps), so no packet is dropped by printing. Promiscuous mode (`promisc aa|55`) is the fallback if that catches nothing; the SPI piggyback (step 4) confirms the settings directly.
6. Later: RF transmit emulation (group up/down, held-repeat + release packets), button handling, deep sleep with ext1 wake.

## Open questions
- Our remote's RF ID; confirm the published channels, rate and address on it (sniff, or SPI piggyback).
- What RF a red-mode arrow press sends (`ID 01 00 80`?), and whether its timing precedes the IR.
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
