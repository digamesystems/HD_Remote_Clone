# Handheld board: design spec (draft)

**Status:** draft for review (2026-11-11). Nothing here has been built. Part numbers are proposals to check for availability (LCSC/JLC) before the schematic is final. Items marked *verify* need a breadboard test or a datasheet check.

The goal is a battery handheld that replaces the original Platinum remote. It sends point-and-press IR with our RF wake, and RF group commands, using the firmware developed on the DevKitC breadboard.

## Decisions (agreed 2026-11-11)

| Topic | Choice |
|---|---|
| MCU | **ESP32-WROOM-32E** module (same chip family as the DevKitC; the pin map and firmware carry over) |
| Power | **1-cell Li-ion/LiPo**, **USB-C** charging, low-quiescent 3.3 V LDO |
| Controls | **7 keys like the original** (UP, DOWN, 1, 2, 3, 4, ALL), a **recessed SETUP button**, and an **A/B IR channel slide switch** |
| Radio | **2×4 socket** for the nRF24 modules we already have and have screened (antenna overhanging the board edge) |
| Indicator | **Red/green bicolor LED**, common cathode |
| IR receiver | **Optional footprint** (TSOP382xx / VS1838B, 3-pin), unpopulated in normal builds, for diagnostics. It takes little space |
| IR LEDs | Two TSAL6200 positions |
| Charge status | **Separate LED** on the charger's STAT pin (not the bicolor LED) |
| USB | USB-C with the USB-serial bridge kept on the board, so early revisions can be flashed and debugged over USB |
| Enclosure | Candidate: OKW Style-Case M in IR-transparent black PMMA (B2803016), or the L (B2804016) if space is tight. Alternative: Smart-Case L. Not chosen yet; needs OKW's drawings for the internal space and pillars |

## Block diagram

```
USB-C ──┬─ ESD (USBLC6) ── CP2102N USB-UART ── auto-reset (2× NPN) ── ESP32 EN / IO0 / TX0 / RX0
        │
        └─ VBUS ── MCP73831 charger ── VBAT ── LiPo cell (protected, JST-PH 2-pin)
                                        │
                                        ├── RT9080 LDO ── 3V3 ── ESP32-WROOM-32E
                                        │                    ├── nRF24 socket (+10 µF, 100 nF)
                                        │                    ├── 8 buttons (active-high, pull-downs)
                                        │                    ├── A/B switch
                                        │                    └── bicolor LED (2× resistor)
                                        ├── divider (2× 1 MΩ) ── ADC (battery voltage)
                                        └── IR LEDs (2×) ── current sinks ── GPIO 4
```

## Pin map (ESP32-WROOM-32E)

Rules: buttons must be RTC GPIOs for `ext1` deep-sleep wake (`ESP_EXT1_WAKEUP_ANY_HIGH`, active-high with pull-downs). Avoid the strapping pins 0, 2, 12 and 15 for buttons; a press at boot could change the boot mode, and GPIO 12 high selects 1.8 V flash. The battery ADC must be on ADC1, since ADC2 is unusable with Wi-Fi (unused here, but kept general).

| Function | GPIO | Notes |
|---|---|---|
| UP | 32 | RTC; same as the breadboard |
| DOWN | 33 | RTC; same as the breadboard |
| Group 1 | 34 | RTC, input-only, **no internal pull**: external pull-down required (all buttons get one anyway) |
| Group 2 | 35 | RTC, input-only |
| Group 3 | 36 (VP) | RTC, input-only |
| Group 4 | 27 | RTC |
| ALL | 25 | RTC |
| SETUP | 26 | RTC; recessed (paperclip) button |
| A/B switch | 21 | Read on wake; no wake needed. Not RTC |
| Battery sense | 39 (VN) | ADC1_CH3, from a 1 MΩ / 1 MΩ divider on VBAT, 100 nF to GND |
| IR LED drive | 4 | Same as the breadboard |
| LED red / green | 22 / 16 | Via resistors, PWM (LEDC). GPIO 16 was the nRF24 IRQ, which the firmware doesn't use |
| nRF24 SCK / MISO / MOSI | 18 / 19 / 23 | VSPI, same as the breadboard |
| nRF24 CSN / CE | 5 / 17 | Same as the breadboard. GPIO 5 is a strapping pin; CSN idles high, which is its default |
| nRF24 IRQ | not connected | Pad or 0 Ω option to GPIO 13 if ever needed |
| IR receiver OUT (optional) | 14 | Same as the breadboard. Footprint only; 100 Ω + 100 nF supply filter |
| UART TX0 / RX0 | 1 / 3 | To the CP2102N |
| EN / IO0 | EN / 0 | Auto-reset from DTR/RTS (DevKitC circuit); also a small EN reset button |

Free afterwards: 13 (spare pad), 2, 12, 15 (strapping; leave alone or use carefully).

## Power

**Cell:** single Li-ion/LiPo **with a protection circuit** (most pouch cells with leads have one), ~500–1000 mAh to suit the enclosure, on a JST-PH 2-pin connector. Check the polarity of the purchased cell: there is no common standard.

**Charging:** MCP73831 (SOT-23-5), programmed to ~250 mA (R_PROG ≈ 4 kΩ). Its STAT pin drives a separate small charge LED (lit while charging), powered from VBUS so it costs the battery nothing. The load runs straight from VBAT (no power path). That's simple, and fine at this charge current, though the charger may not terminate cleanly if the board draws current while charging. *verify*

**3.3 V regulator:** RT9080-33 (SOT-23-5): 600 mA, ~2 µA quiescent, about 310 mV dropout at 600 mA. The ESP32 never uses Wi-Fi or Bluetooth here, so its active current is ~40–70 mA, plus the nRF24 at ~12 mA. The LDO stays in regulation down to about VBAT 3.4 V; the firmware warns at ~3.5 V and refuses to transmit below ~3.3 V.

**Battery sense:** VBAT → 1 MΩ → GPIO 39 → 1 MΩ → GND, 100 nF across the lower resistor. That's a constant 2.1 µA drain, accepted for simplicity.

**Sleep current budget (estimate):**

| Part | Current |
|---|---|
| ESP32 deep sleep, ext1 wake | ~5–10 µA |
| RT9080 quiescent | ~2 µA |
| nRF24 power-down | ~1 µA |
| Battery divider | ~2 µA |
| MCP73831 reverse leakage, no USB | ~1 µA |
| CP2102N | 0 (powered from VBUS only) |
| **Total** | **~11–16 µA** |

That's years of standby on 500 mAh. Real battery life will be set by use: each press is about 0.7 s at ~60 mA plus the IR LED pulses.

## IR transmitter

Two 940 nm LEDs, so we can mix a narrow one (TSAL6200, ±17°) with a wider one, or fit two of the same. Breadboard tests showed range and aim tolerance limit IR, so more light and a wider beam help.

The LEDs run from **VBAT**, not 3.3 V, so the regulator doesn't carry the pulses. VBAT varies from 4.2 V down to 3.3 V, so each LED gets a **current sink** that holds the current roughly constant:

- GPIO 4 → 470 Ω → a common base node, clamped by two 1N4148 diodes in series to GND (~1.3 V).
- For each LED: BC817-40 NPN, base on that node, emitter → R_E to GND, collector → LED cathode, LED anode → VBAT.
- I_LED ≈ (1.3 V − 0.65 V) / R_E, so R_E ≈ 4.3 Ω gives ~150 mA per LED (the breadboard ran one LED at ~97 mA).
- Headroom at VBAT 3.3 V: 3.3 − 1.6 (LED at 150 mA) − 0.65 (R_E) ≈ 1.0 V across the transistor. OK.
- Bulk capacitance on VBAT at the LEDs: 100 µF + 100 nF.

*verify:* build the sink on the breadboard first and check the current with the scope across R_E at 3.3 V and 4.2 V. The TSAL6200's peak rating depends on duty cycle; 150 mA at our ~33 % carrier duty in bursts is believed safe, but check against the datasheet.

## USB and programming

- **USB-C receptacle** (USB 2.0, 16-pin), 5.1 kΩ on CC1 and CC2 to GND (sink role), USBLC6-2SC6 ESD protection on D+/D−.
- **CP2102N** (QFN-24), the same family as the DevKitC's USB bridge, so the driver is already installed and COM11-style behaviour is unchanged. It's powered from VBUS through its own regulator, so it draws nothing from the battery.
  - *Back-powering:* with USB unplugged, the ESP32's TX0 drives the CP2102N's unpowered RXD pin and could leak current through its protection diodes. Put 1 kΩ in series on TX0 and RX0, and check the sleep current with USB unplugged. *verify*
- **Auto-reset:** the DevKitC's two-NPN DTR/RTS circuit on EN and IO0, so `pio run -t upload` works without buttons. EN gets 10 kΩ pull-up + 1 µF; a small tactile EN reset button is included.

## nRF24 socket

- 2×4 female header, 2.54 mm. The module plugs in with its antenna **overhanging the board edge**, with no copper (all layers) under the overhang.
- 10 µF + 100 nF at the socket's VCC pin.
- The firmware puts the radio in power-down between presses (~1 µA). No power switch for now.

## Indicator LED

- 3-lead, 5 mm or 3 mm diffused red/green, common cathode. Pick a ~2.1 V "standard" green (not a 3 V InGaN), or lower its resistor.
- Red: GPIO 22 → 330 Ω. Green: GPIO 16 → 270 Ω. About 4 mA each, PWM for brightness.
- Behaviour copies the original (manual): red while sending IR; steady green on a group command and during the group window; flashing green in setup mode; red flashes at low battery (our choice).

## Buttons and switch

- 8 tactile switches: UP, DOWN, 1–4, ALL, SETUP (SETUP recessed). Each one connects 3V3 to its GPIO, with **100 kΩ pull-down** and an optional 100 nF debounce cap. No current flows unless a button is pressed.
- A/B slide switch (SPDT): GPIO 21 to 3V3 (B) or GND (A). No pull needed. It draws no current in either position.
- The firmware debounces (~20–30 ms) and reads which button caused the wake with `esp_sleep_get_ext1_wakeup_status()`.

## Layout notes

- 2-layer board, ground pour on both sides, stitched.
- Keep the **ESP32 module's antenna area** clear of copper, per Espressif's guidelines. The ESP32's radio isn't used, but this keeps the option open and is free.
- Keep the **nRF24 antenna overhang** clear, and keep the IR LEDs, charger and LDO away from it.
- The IR LEDs sit at the front edge, aimed forward. Their current loop (VBAT cap → LEDs → transistors → R_E → GND) should be short and wide.
- Buttons laid out like the original: arrows large and central, group keys in a row, ALL separate, SETUP on the back.
- Size and shape follow the enclosure, which isn't chosen yet. *open*

## Open questions

1. **Enclosure:** OKW Style-Case M or L (IR-transparent PMMA), or Smart-Case L. Needs OKW's STEP/DXF drawing for the internal height, pillar positions and board outline. Fit concerns: the nRF24 module in its socket is ~12 mm tall (a right-angle header lays it flat if needed); keys through drilled holes (tactile switches with long plungers or keycaps) or a membrane keypad; a filed slot for the USB-C.
2. **Cell:** flexible on size and connector. Pick to suit the enclosure; check the polarity of the purchased cell.

Resolved 2026-11-11: two TSAL6200 positions; optional IR receiver footprint; separate charge LED; keep USB debugging on early revisions.
