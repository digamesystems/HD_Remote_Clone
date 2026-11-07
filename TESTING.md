# Testing

Tests are split into three tiers by what they need to run. That decides when they can block a deploy.

## Tier 1: host unit tests (no hardware)

```
pio test -e native
```

These run on the PC with Unity and the MSYS2 gcc compiler, and take seconds. They cover hardware-independent logic in [`lib/hdcore/`](lib/hdcore/src), which the firmware and the tests share.

| Suite | Covers |
|---|---|
| `test_native_parse` | serial command tokenizing, hex parsing, data-rate parsing |
| `test_native_rf` | nRF24 on-air ↔ register address byte order, sniffer duplicate filter |

**Rule:** logic that can run without hardware goes in `lib/hdcore/`, with a Tier 1 test, rather than in `src/`. This includes protocol encoding, IR frame analysis and state machines.

## Tier 2: on-device tests (ESP32, extra hardware optional)

```
pio test -e esp32dev --upload-port COM11                         # all device suites
pio test -e esp32dev --upload-port COM11 -f test_device_ir       # one suite
```

These flash a **test firmware** to the board, run Unity on it and report over serial. Afterwards the board is left running the test firmware, so re-upload the app (`pio run -t upload`).

[`include/hw_config.h`](include/hw_config.h) declares what's wired on the breadboard:

- Hardware declared **absent**: the test is **IGNORED** and reported as *not verified*.
- Hardware declared **present**: the test **must pass**. A failure blocks deploy.

Update `hw_config.h` whenever the breadboard changes. We can add loopbacks or other diagnostic wiring to make hardware testable.

| Suite | Needs | Covers |
|---|---|---|
| `test_device_board` | board only | chip is the original ESP32 (D0WD), no PSRAM (so not WROVER), flash ≥ 4 MB |
| `test_device_rf` | `HW_NRF24` | `begin()`, `isChipConnected()`, 200× register read-back, sniffer RX config |
| `test_device_ir` | `HW_IR_LOOPBACK` | TSAL6200 → TSOP38238 round trip of a synthetic frame within ±max(200 µs, 25%); 10/10 reliability |

Planned: RF loopback (a second nRF24 on HSPI receives what the first sends) to verify TX emulation; button and deep-sleep wake tests (`HW_BUTTONS`).

**Implementation note:** `pio test` defines `UNIT_TEST`, which switches IRremoteESP8266 into a host-stub mode. [`scripts/no_unit_test_define.py`](scripts/no_unit_test_define.py) strips that define for the esp32dev environment so device tests use the real library.

## Tier 3: manual acceptance (real shades, original remote)

These can't be automated. A feature is **done** once John confirms it and it's recorded below.

| # | Check | Result | Date |
|---|---|---|---|
| A1 | Capture IR from the original remote in red mode, every button | | |
| A2 | Replay a captured IR code; target shade responds | | |
| A3 | RF scan shows the original remote's channels | | |
| A4 | Sniffer decodes the original remote's group up/down/release packets | | |
| A5 | RF-wake hypothesis: IR replay alone vs. with RF burst | | |

## Gate rules

| Step | Must pass |
|---|---|
| Any change | `pio run` builds |
| **cp** (commit + push) | build + **all** Tier 1 suites |
| **cpd** (+ deploy) | the above + Tier 2 suites for the areas changed (`-f test_device_<area>`), then upload the app |
| Feature "done" | its Tier 3 row confirmed |

If any required step fails: **halt**. No commit, push or deploy. Report the failure.

If Tier 2 tests are ignored because hardware is absent, the build may still be deployed, but the report must say plainly which areas were **not verified**.
