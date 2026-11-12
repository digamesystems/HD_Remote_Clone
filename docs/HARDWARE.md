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
| EN / IO0 | EN / 0 | Auto-reset from DTR/RTS (DevKitC circuit); also small EN (reset) and IO0 (BOOT) buttons. BOOT added 2026-11-12 as a fallback if auto-reset fails on an early board |

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
- 100 kΩ from the base node to GND, so the LEDs stay off while GPIO 4 floats (during boot, and in deep sleep unless the pin is held). Added 2026-11-12.
- For each LED: BC817-40 NPN, base on that node, emitter → R_E to GND, collector → LED cathode, LED anode → VBAT.
- I_LED ≈ (1.3 V − 0.65 V) / R_E, so R_E ≈ 4.3 Ω gives ~150 mA per LED (the breadboard ran one LED at ~97 mA).
- Headroom at VBAT 3.3 V: 3.3 − 1.6 (LED at 150 mA) − 0.65 (R_E) ≈ 1.0 V across the transistor. OK.
- Bulk capacitance on VBAT at the LEDs: 100 µF + 100 nF.

*verify:* build the sink on the breadboard first and check the current with the scope across R_E at 3.3 V and 4.2 V.

**Measured carrier (2026-11-12, breadboard, scope across the 33.8 Ω series resistor):** 39.7 kHz at **~50 % duty** (IRremoteESP8266's default), 2.96 V peak = **88 mA** per pulse, flat tops, no visible sag. The TSAL6200 is rated 100 mA continuous and 200 mA peak at 50 % duty (t_p ≤ 100 µs), so ~150 mA at 50 % stays inside the peak rating. Average current during a burst is half the peak (not a third, as first assumed), so each press costs about 50 % more than first estimated. A 33 % duty, set in firmware, would cut that if range allows.

## USB and programming

- **USB-C receptacle** (USB 2.0, 16-pin), 5.1 kΩ on CC1 and CC2 to GND (sink role), USBLC6-2SC6 ESD protection on D+/D−.
- **CP2102N** (QFN-24), the same family as the DevKitC's USB bridge, so the driver is already installed and COM11-style behaviour is unchanged. It's powered from VBUS through its own regulator, so it draws nothing from the battery.
  - *Back-powering:* with USB unplugged, the ESP32's TX0 drives the CP2102N's unpowered RXD pin and could leak current through its protection diodes. Put 1 kΩ in series on TX0 and RX0, and check the sleep current with USB unplugged. *verify*
- **Auto-reset:** the DevKitC's two-NPN DTR/RTS circuit on EN and IO0, so `pio run -t upload` works without buttons. EN gets 10 kΩ pull-up + 1 µF; a small tactile EN reset button is included.

## nRF24 socket

- 2×4 female header, 2.54 mm. The module plugs in with its antenna **overhanging the board edge**, with no copper (all layers) under the overhang.
- 10 µF + 100 nF at the socket's VCC pin.
- The firmware puts the radio in power-down between presses (~1 µA). No power switch for now.

### nRF24 supply filtering

Evaluated 2026-11-12. The radio draws ~11–13 mA (genuine part, TX at 0 dBm or RX); clones at their "+7 dBm" setting may draw 25–35 mA (unverified). With 10 µF at the socket, even a 30 mA step lasting 5 µs before the LDO responds dips the rail by only ~15 mV. The breadboard worked on long jumpers with no capacitor at all. So the capacitors are adequate; the risks lie elsewhere:

- **IR pulses on VBAT.** ~300 mA at 40 kHz through the cell's ~0.2–0.3 Ω (cell + protection) sags VBAT ~60–90 mV per pulse, which can push the LDO into dropout near the 3.3 V cutoff. Mitigated because the firmware never transmits RF and IR at the same time, and by the 100 µF at the LEDs. Keep it that way, and keep the low-battery cutoff above the LDO's dropout.
- **Shared ground return.** Keep the IR current loop (VBAT cap → LEDs → transistors → R_E → GND) short and routed to the battery connector away from the radio.
- **ESP32 digital noise** on 3V3: modest with Wi-Fi off. FB401 lets a ferrite bead be fitted later if range or packet loss suggests supply noise (check for bead/ceramic resonance if one is fitted).

Layout: 100 nF right at the socket's VCC/GND pins, the 10 µF next to it, solid ground pour under the module except under the antenna overhang.

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

## Schematic: Power and USB sheet (draft, 2026-11-12)

[`hardware/hd_remote/power_usb.kicad_sch`](../hardware/hd_remote/power_usb.kicad_sch) implements the Power and USB sections above. ERC: no errors; the only warnings are the labels to the ESP32 (`EN`, `IO0`, `U0TXD`, `U0RXD`), which connect when the MCU sheet exists. Values chosen there that the sections above don't state:

| Item | Value | Note |
|---|---|---|
| Passives | 0603 | hand-reworkable; JLC-assemblable |
| USB-C receptacle | HRO TYPE-C-31-M-12 footprint | common, cheap 16-pin USB 2.0 part; shield to GND |
| CP2102N supply | bus-powered: VREGIN = VBUS (4.7 µF + 100 nF), VDD = VIO (1 µF + 100 nF) | *verify* against the CP2102N datasheet |
| CP2102N VBUS sense | 22.1 kΩ / 47.5 kΩ divider from VBUS | as on Silicon Labs' CP2102N-EK; *verify* |
| CP2102N RST | 1 kΩ pull-up to VIO | *verify* the value |
| Auto-reset | 2× MMBT3904, 10 kΩ base resistors | the DevKitC circuit |
| Charge current | R_PROG 4.02 kΩ, ~250 mA | 1000 V / R_PROG |
| Charger caps | 4.7 µF on VBUS and VBAT | MCP73831 datasheet minimum |
| Charge LED | 0603, 1 kΩ from VBUS to STAT | lit while charging |
| Battery connector | JST-PH 2-pin, side entry (S2B-PH-K) | *check the cell's polarity* |
| LDO | RT9080-33GJ5, 1 µF in and out | drawn with the AP2112K symbol (same SOT-23-5 pinout: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT); *verify* against the RT9080 datasheet. The ESP32's 10 µF bulk goes on the MCU sheet |

LCSC part numbers aren't filled in yet.

## Schematic: MCU sheet (draft, 2026-11-12)

[`hardware/hd_remote/mcu.kicad_sch`](../hardware/hd_remote/mcu.kicad_sch): the ESP32-WROOM-32E with every GPIO in the pin map on a named global label (`BTN_UP`, `IR_TX`, `NRF_CSN`, ...), which the IR, nRF24 and buttons sheets will use. Also:

| Item | Value | Note |
|---|---|---|
| 3V3 decoupling | 10 µF (0805) + 100 nF at the module | per Espressif's guidance |
| EN | 10 kΩ pull-up, 1 µF to GND, RESET button | DevKitC values |
| BOOT | button from IO0 to GND | IO0 has an internal pull-up |
| Buttons | C&K KMR2 footprint (small SMD tactile) | for RESET and BOOT; the user keys are on the buttons sheet |
| GPIO 2, 12, 15 | not connected | strapping pins; 12 must be low at boot |
| GPIO 13 | label `NRF_IRQ_OPT` | for the optional nRF24 IRQ link |

Part references are numbered by sheet: 1xx Power and USB, 2xx MCU, 3xx IR.

## Schematic: IR sheet (draft, 2026-11-12)

[`hardware/hd_remote/ir.kicad_sch`](../hardware/hd_remote/ir.kicad_sch) implements [IR transmitter](#ir-transmitter) above, plus the optional receiver.

| Item | Value | Note |
|---|---|---|
| IR LEDs | 2× TSAL6200, 5 mm, footprint bent 90° (`LED_D5.0mm_Horizontal_O1.27mm_Z3.0mm`) | lying flat, pointing out of the board edge; either position can take a wider-angle LED |
| Current sinks | BC817-40 (SOT-23), R_E 4.3 Ω 0805 | ~0.1 W peak in R_E, hence 0805 |
| Clamp | 2× 1N4148W (SOD-123), base resistor 470 Ω, 100 kΩ pull-down | |
| Bulk on VBAT | 100 µF 1206 ceramic + 100 nF | a 100 µF ceramic keeps well under half its capacity at 4 V; check the derating, or use a tantalum or polymer cap |
| Receiver (DNP) | TSOP382xx footprint (Vishay MINICAST), 100 Ω + 100 nF supply filter | not fitted in normal builds; a VS1838B has the same pin order but a different body |

## Schematic: nRF24 and Buttons/LEDs sheets (draft, 2026-11-12)

[`nrf24.kicad_sch`](../hardware/hd_remote/nrf24.kicad_sch) (4xx) and [`ui.kicad_sch`](../hardware/hd_remote/ui.kicad_sch) (5xx) complete the schematic. **ERC on the whole design: 0 errors, 0 warnings**, and every GPIO net in the netlist matches the pin map above.

| Item | Value | Note |
|---|---|---|
| nRF24 socket | 2×4 female, 2.54 mm, vertical; pins numbered as on the module (odd row GND, CE, SCK, MISO; even row VCC, CSN, MOSI, IRQ) | orientation and antenna overhang are layout tasks |
| nRF24 supply | 3V3 → FB401 (0603: 0 Ω fitted; ferrite-bead option) → `NRF_VCC` with 10 µF (0805) + 100 nF at the socket, and a DNP 100 µF (1206) for a PA/LNA module | see [nRF24 supply filtering](#nrf24-supply-filtering) |
| nRF24 IRQ | to GPIO 13 through a 0 Ω, DNP | firmware doesn't use IRQ |
| Keys | 6 mm THT tactile, tall plunger (13 mm); SETUP short (5 mm), recessed | placeholders until the enclosure is chosen |
| Key pull-downs | 100 kΩ each; 100 nF debounce caps DNP | firmware debounces |
| A/B switch | SPDT slide (PCM12 footprint): common to GPIO 21, throws to GND (A) and 3V3 (B) | which throw is "A" depends on the part and its orientation |
| Indicator | 3-lead red/green common cathode, 5 mm; red 330 Ω, green 270 Ω | *check which outer leg is red* on the part bought |

## Test points (2026-11-12)

Chosen for bring-up and for the checks still open in this spec. SMD pads (1.5 mm) on the back unless noted.

| Ref | Net | Sheet | Use |
|---|---|---|---|
| JP101 | VBAT_BATT – VBAT | Power and USB | **Battery current link**, a bridged solder jumper between the battery connector and everything else. Cut it and put a µA meter across the pads to measure total battery current: the sleep budget (~11–16 µA) and the CP2102N back-powering check with USB unplugged. Re-bridge with solder afterwards |
| TP101 | PROG | Power and USB | charge current: V(PROG) = 1 V at the full ~250 mA |
| TP301, TP302 | IR_E1, IR_E2 | IR | top of each emitter resistor: scope to GND, I_LED = V / 4.3 Ω, at VBAT 3.3 and 4.2 V |
| TP303 | IR_BASE | IR | the clamp voltage (~1.3 V) and the drive waveform |
| TP601–TP603 | VBAT, +3V3, VBUS | Test and debug | rail checks; LDO dropout under IR load |
| TP604–TP606 | GND | Test and debug | THT loops (Keystone 5000) for scope ground clips: one near the IR stage, one near the radio |
| TP607 | IR_TX | Test and debug | carrier and duty cycle on the analyzer |
| TP608 | VBAT_SENSE | Test and debug | calibrate the battery ADC against a meter |
| TP609 | GPIO 13 (`NRF_IRQ_OPT`) | Test and debug | **debug strobe**: the firmware toggles it at RF / IR step boundaries, so the analyzer can time the wake against the IR. Shared with the optional nRF24 IRQ link (R401, DNP): fit one or the other |
| J601 (DNP) | GND, 3V3, U0TXD, U0RXD, EN, IO0 | Test and debug | 1×6 2.54 mm header for an external 3.3 V USB-serial adapter, if the onboard CP2102N misbehaves. Unplug USB-C first; power the board from the adapter's 3V3 or from the battery, not both |

The nRF24's SPI lines need no test points: the socket's through-hole pins on the back take analyzer clips.

## Later revision idea: XIAO ESP32-S3

This design uses 22 of the WROOM-32E's 26 usable GPIOs (free: 13, plus strapping pins 2, 12 and 15), so it won't fit a XIAO, which has 11. A smaller second revision could fit, by:

- putting all 8 buttons on **one ADC pin through a resistor ladder**. Any press also pulls the pin high, which can wake the chip from deep sleep; the cost is no simultaneous presses, which the original never needs;
- dropping the IR receiver footprint, and maybe folding the A/B switch into the ladder.

That's 11 pins: nRF24 5, IR 1, bicolor LED 2, ladder 1, battery sense 1, A/B 1. The XIAO's own USB-C and LiPo charger would replace the CP2102N, the MCP73831 and probably the LDO. The costs are no spare pins, single-press-only buttons and an ESP32-S3 firmware port. Agreed 2026-11-11: WROOM first, XIAO as a possible later revision.

## Open questions

1. **Enclosure:** OKW Style-Case M or L (IR-transparent PMMA), or Smart-Case L. Needs OKW's STEP/DXF drawing for the internal height, pillar positions and board outline. Fit concerns: the nRF24 module in its socket is ~12 mm tall (a right-angle header lays it flat if needed); keys through drilled holes (tactile switches with long plungers or keycaps) or a membrane keypad; a filed slot for the USB-C.
2. **Cell:** flexible on size and connector. Pick to suit the enclosure; check the polarity of the purchased cell.

Resolved 2026-11-11: two TSAL6200 positions; optional IR receiver footprint; separate charge LED; keep USB debugging on early revisions.
