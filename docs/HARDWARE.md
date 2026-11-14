# Handheld board: design spec (draft)

**Status:** draft for review (2026-11-11). Nothing here has been built. Part numbers are proposals to check for availability (LCSC/JLC) before the schematic is final. Items marked *verify* need a breadboard test or a datasheet check.

The goal is a battery handheld that replaces the original Platinum remote. It sends point-and-press IR with our RF wake, and RF group commands, using the firmware developed on the DevKitC breadboard.

## Decisions (agreed 2026-11-11)

| Topic | Choice |
|---|---|
| MCU | **ESP32-WROOM-32E** module (same chip family as the DevKitC; the pin map and firmware carry over) |
| Power | **1-cell Li-ion/LiPo**, **USB-C** charging, low-quiescent 3.3 V LDO |
| Controls | **7 keys like the original** (UP, DOWN, 1, 2, 3, 4, ALL), a **recessed SETUP button**, and an **A/B IR channel slide switch** |
| Radio | **2×4 socket** for the nRF24 modules we already have and have screened (the module lies flat over the board, its PCB antenna over a no-copper keep-out on both layers) |
| Indicator | **Red/green bicolor LED**, common cathode |
| IR receiver | **None** (removed 2026-11-13, PCB layout). It was an optional, unfitted diagnostic footprint; it would have needed a window in the front wall to see out, and its lines added crossings at the key area. IR capture stays on the breadboard (DevKitC + TSOP38238/VS1838B) |
| IR LEDs | Two TSAL6200 positions |
| Charge status | **Separate LED** on the charger's STAT pin (not the bicolor LED) |
| USB | USB-C with the USB-serial bridge kept on the board, so early revisions can be flashed and debugged over USB |
| Enclosure | **Custom 3D-printed clamshell** (decided 2026-11-13, replacing the OKW candidates): 150 × 45 × 27 mm, modelled in `hardware/enclosure/`, whose README holds the PCB constraints (outline, holes, keep-outs) |

## Block diagram

```
USB-C ──┬─ ESD (USBLC6) ── CP2102N USB-UART ── auto-reset (2× NPN) ── ESP32 EN / IO0 / TX0 / RX0
        │
        └─ VBUS ─┬─ MCP73831 charger ── VBAT ── LiPo cell (protected, JST-PH 2-pin)
                 │                        │  └── divider (2× 1 MΩ) ── ADC (battery voltage)
                 │                        │ Q104 P-FET (on when no USB)
                 └── D102 Schottky ──────┴── VSYS  (power path, Microchip AN1149)
                                        │
                                        ├── RT9080 LDO ── 3V3 ── ESP32-WROOM-32E
                                        │                    ├── nRF24 socket (+10 µF, 100 nF)
                                        │                    ├── 8 buttons (active-high, pull-downs)
                                        │                    ├── A/B switch
                                        │                    └── bicolor LED (2× resistor)
                                        └── IR LEDs (2×) ── current sinks ── GPIO 4
```

## Pin map (ESP32-WROOM-32E)

Rules: buttons must be RTC GPIOs for `ext1` deep-sleep wake (`ESP_EXT1_WAKEUP_ANY_HIGH`, active-high with pull-downs). Avoid the strapping pins 0, 2, 12 and 15 for buttons; a press at boot could change the boot mode, and GPIO 12 high selects 1.8 V flash. The battery ADC must be on ADC1, since ADC2 is unusable with Wi-Fi (unused here, but kept general).

| Function | GPIO | Notes |
|---|---|---|
| UP | 32 | RTC; same as the breadboard |
| DOWN | 33 | RTC; same as the breadboard |
| Group 1 | 26 | RTC (was 34 until 2026-11-13) |
| Group 2 | 34 | RTC, input-only, **no internal pull**: external pull-down required (all buttons get one anyway) (was 35) |
| Group 3 | 27 | RTC (was 36) |
| Group 4 | 36 (VP) | RTC, input-only (was 27). See the GPIO 36/39 note below |
| ALL | 25 | RTC |
| SETUP | 35 | RTC, input-only; recessed (paperclip) button (was 26) |
| A/B switch | 14 | Via R202 (10 kΩ series). Read on wake; no wake needed. Moved from GPIO 21 on 2026-11-14: GPIO 14 is in the module's front-facing pin column, so the line no longer runs round the module. R202 limits current if GPIO 14 drives during boot (widely reported for this JTAG pin) against the switch, which ties the pin hard to GND or 3V3 |
| Battery sense | 39 (VN) | ADC1_CH3, from a 1 MΩ / 1 MΩ divider on VBAT, 100 nF to GND |
| IR LED drive | 4 | Same as the breadboard |
| LED red / green | 22 / 16 | Via resistors, PWM (LEDC). GPIO 16 was the nRF24 IRQ, which the firmware doesn't use |
| nRF24 SCK / MISO / MOSI | 18 / 19 / 23 | VSPI, same as the breadboard |
| nRF24 CSN / CE | 5 / 17 | Same as the breadboard. GPIO 5 is a strapping pin; CSN idles high, which is its default |
| nRF24 IRQ | not connected | Pad or 0 Ω option to GPIO 13 if ever needed |
| (spare) | 14 | Was the optional IR receiver's OUT (removed 2026-11-13); now unconnected, no-connect flag in the schematic |
| UART TX0 / RX0 | 1 / 3 | To the CP2102N |
| EN / IO0 | EN / 0 | Auto-reset from DTR/RTS (DevKitC circuit); also small EN (reset) and IO0 (BOOT) buttons. BOOT added 2026-11-12 as a fallback if auto-reset fails on an early board |

Free afterwards: 13 (spare pad / debug strobe TP609), 21 (spare since 2026-11-14), 2, 12, 15 (strapping; leave alone or use carefully). Firmware: enable an internal pull on unused pins (Espressif checklist).

**Key pins re-assigned (2026-11-13, PCB layout):** the RTC pins all sit in the module's left pin column, and their order up that column decides how the eight key lines fan out to the keys. The first assignment crossed itself 20 times there. A search over every valid assignment (key pins interchangeable; VBAT_SENSE on an ADC1 pin) found 3 crossings at best; the one used has 4 but keeps VBAT_SENSE on GPIO 39 and only one key (G4) on GPIO 36/39, the same as before. UP, DOWN and ALL are unchanged, so the breadboard firmware (UP 32, DOWN 33) still matches. Across the whole board the ratsnest crossings fell from 187 to 159.

**GPIO 36/39 note:** ESP32 erratum: while the SAR ADC (or Wi-Fi) is powered, GPIO 36 and 39 can show short (~80 ns) low pulses. Here that means one key line (G4 on 36) next to the battery-sense ADC (39). The ~20 ms software debounce ignores pulses that short; don't rely on edge interrupts on GPIO 36 without debouncing.

## Power

**Cell:** single Li-ion/LiPo **with a protection circuit** (most pouch cells with leads have one), ~500–1000 mAh to suit the enclosure, on a JST-PH 2-pin connector. Check the polarity of the purchased cell: there is no common standard.

**Reverse-polarity protection (added 2026-11-12):** a reversed cell would destroy the charger and the LDO, so the battery lead runs through a P-MOSFET (Q103, AO3401A, SOT-23) with its gate to GND: drain to the battery side, source to VBAT. With the cell the right way round, the body diode lifts the source, Vgs ≈ −VBAT turns the FET fully on (~50–80 mΩ, ~20 mV at a 300 mA IR peak), and the charger still charges through it, since the channel conducts both ways. With the cell reversed, Vgs = 0 and the body diode is reverse-biased, so nothing flows. Mark + and − on the silkscreen at J102 as well. Any SOT-23 P-FET with |Vgs(th)| under ~1.5 V and Vgs max ≥ 8 V will do. *verify* the drop and the charger's termination on the first board.

**Charging:** MCP73831 (SOT-23-5), programmed to ~256 mA (R_PROG 3.9 kΩ). Its STAT pin drives a separate small charge LED (lit while charging), powered from VBUS so it costs the battery nothing. **Power path (added 2026-11-13, Microchip AN1149):** the board's loads run from **VSYS**, not VBAT. With USB present, VBUS feeds VSYS through Schottky D102, and P-FET Q104 (gate = VBUS) disconnects the cell, so the charger sees only the cell: charging terminates cleanly whatever the board is doing, and the board runs from USB with or without a cell fitted. Without USB, R114 (100 kΩ) pulls Q104's gate low and Q104 connects the cell to VSYS (~20 mV at 300 mA). The charger output, Q103, JP101 and the battery divider stay on VBAT, so the ADC still reads the cell.

- **D102:** STPS1H100AY (ST, 100 V 1 A, SMA, LCSC C181833), chosen for **low reverse leakage**: with USB absent it is reverse-biased by the cell and its leakage is a permanent battery drain. Rated 4 µA max at 100 V and 25 °C; at ~4 V it should be far lower (estimate under 1 µA; *verify* with the JP101 measurement, and note leakage roughly doubles every 10 °C). Common 40 V Schottkys leak 50–500 µA at rated voltage (PMEG4010CEH: 50 µA), too much for a ~15 µA budget. Rohm's "super low IR" RB168M series would be smaller (SOD-123) but had no LCSC stock (checked 2026-11-13). The price of a 100 V part is a higher forward drop, ~0.5–0.6 V at our currents, so VSYS ≈ 4.4–4.5 V from a 5 V port: plenty for the LDO.
- **Q104:** AO3401A (same JLC basic part as Q103, C15127): |Vgs(th)| 0.5–1.3 V, within AN1149's 0 to −2.4 V.
- **Conditions to keep:** VSYS from USB must stay above VBAT minus Q104's body-diode drop, or the cell would help feed the load while charging: 4.4 V against a full 4.2 V cell gives margin, but a weak USB port (4.4–4.5 V at the device) narrows it. Diode leakage must stay small enough that R114 still pulls the gate low (10 µA × ~41 kΩ, R114 in parallel with the VBUS divider ≈ 0.4 V: fine).
- **No cell fitted, USB present:** the board runs; the charger output hunts (no cell), so the battery ADC reading is meaningless. Firmware should treat "USB present and VBAT reading odd" as "no battery".

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
| D102 reverse leakage at ~4 V, no USB (power path) | < 1 µA (estimate; rated 4 µA max at 100 V, 25 °C) *verify* |
| CP2102N | 0 (powered from VBUS only) |
| **Total** | **~11–16 µA** |

That's years of standby on 500 mAh. Real battery life will be set by use: each press is about 0.7 s at ~60 mA plus the IR LED pulses.

**Caveats (added 2026-11-13):**
- **TX0's pull-up into the unpowered CP2102N.** GPIO 1 (U0TXD) has a weak internal pull-up at reset. If it stays on in deep sleep, it feeds the CP2102N's RXD clamp through R107: about (3.3 − 0.6 V) / (~45 kΩ + 10 kΩ) ≈ **50 µA**, three to five times the whole budget. The pull-up, not R107, sets most of that current. The pads are believed to go high-impedance in deep sleep, but that's unconfirmed. If the JP101 measurement shows it, fix it in firmware (disable the pull-up or isolate the pin before sleeping); no hardware change is needed. *verify*
- **Pin states before sleep.** The firmware must leave every GPIO at its idle level (nRF24 powered down, CSN high, CE low, IR drive low). A pin driving against one of the 100 kΩ pulls costs ~33 µA; one floating at a mid level can cost more.
- **Not on the board:** the cell's protection IC (typically ~3 µA) and its self-discharge (a few % a month) cost about as much again. JP101 doesn't see either.

## IR transmitter

Two 940 nm LEDs, so we can mix a narrow one (TSAL6200, ±17°) with a wider one, or fit two of the same. Breadboard tests showed range and aim tolerance limit IR, so more light and a wider beam help.

The LEDs run from **VSYS** (VBAT when on battery, ~4.4–4.5 V from USB; changed from VBAT 2026-11-13 with the power path), not 3.3 V, so the regulator doesn't carry the pulses. VSYS varies from ~4.5 V down to 3.3 V, so each LED gets a **current sink** that holds the current roughly constant:

- GPIO 4 → 470 Ω → a common base node, clamped by two 1N4148 diodes in series to GND (~1.3 V).
- 100 kΩ from the base node to GND, so the LEDs stay off while GPIO 4 floats (during boot, and in deep sleep unless the pin is held). Added 2026-11-12.
- For each LED: BC817-40 NPN, base on that node, emitter → R_E to GND, collector → LED cathode, LED anode → VBAT.
- I_LED ≈ (V_clamp − Vbe) / R_E. Corrected 2026-11-12 (review): the first draft used Vbe = 0.65 V, but a BC817-40 needs ~0.75–0.8 V at 150 mA, and the clamp sits near 1.3 V with ~4 mA through the diodes. That leaves only ~0.5 V across R_E, so 4.3 Ω would give ~110–120 mA, not 150. The schematic now shows **3.3 Ω (~150–165 mA)**; set the final value from the breadboard. With only ~0.5 V across R_E, the current depends noticeably on Vbe and the diode drops, so measure rather than trust the formula. (The breadboard ran one LED at 88 mA, measured; see below.)
- Headroom at VSYS 3.3 V: 3.3 − 1.6 (LED at 150 mA) − 0.5 (R_E) ≈ 1.2 V across the transistor. OK.
- **Dissipation on USB** (VSYS ~4.5 V): ~2.4 V across each BC817 × 150 mA × 50 % duty ≈ 0.18 W during a burst (~0.7 s), against 0.25 W rated (SOT-23). Within rating; check the transistor temperature on the first board during repeated presses on USB. *verify*
- Bulk capacitance on VSYS at the LEDs: 100 µF + 100 nF.

*verify:* build the sink on the breadboard first and check the current with the scope across R_E at 3.3 V and 4.2 V.

**Measured carrier (2026-11-12, breadboard, scope across the 33.8 Ω series resistor):** 39.7 kHz at **~50 % duty** (IRremoteESP8266's default), 2.96 V peak = **88 mA** per pulse, flat tops, no visible sag. The TSAL6200 is rated 100 mA continuous and 200 mA peak at 50 % duty (t_p ≤ 100 µs), so ~150 mA at 50 % stays inside the peak rating. Average current during a burst is half the peak (not a third, as first assumed), so each press costs about 50 % more than first estimated. A 33 % duty, set in firmware, would cut that if range allows.

## USB and programming

- **USB-C receptacle** (USB 2.0, 16-pin), 5.1 kΩ on CC1 and CC2 to GND (sink role), USBLC6-2SC6 ESD protection on D+/D−.
- **CP2102N** (QFN-24), the same family as the DevKitC's USB bridge, so the driver is already installed and COM11-style behaviour is unchanged. It's powered from VBUS through its own regulator, so it draws nothing from the battery.
  - *Back-powering:* with USB unplugged, the ESP32's TX0 drives the CP2102N's unpowered RXD pin and could leak current through its protection diodes. Series resistors on TX0 and RX0 limit this; check the sleep current with USB unplugged. *verify* Datasheet check (2026-11-13, CP2102N rev 1.3, Table 3.10): with VIO below 3.3 V a UART pin's absolute maximum is VIO + 2.5 V, so 2.5 V when unpowered, against the ESP32's 3.3 V TX idle. The series resistor limits the diode current to about (3.3 − 0.6 V) / 1 kΩ ≈ 2.7 mA while the ESP32 is awake (in deep sleep TX0 isn't driven). That's the same current-limiting argument Silicon Labs uses for the VBUS divider, but 2.7 mA is a real drain during presses. **Decided 2026-11-13:** R107 (ESP32 TX0 → CP2102N RXD) is **10 kΩ** (~0.27 mA). With ~10 pF of pin capacitance that's a ~0.1 µs RC, fine at 921 600 baud (1.1 µs per bit); confirm uploads at full speed on the first board. R106 (CP2102N TXD → ESP32 RX0) stays 1 kΩ: the CP2102N doesn't drive it while unpowered.
- **Auto-reset:** the DevKitC's two-NPN DTR/RTS circuit on EN and IO0, so `pio run -t upload` works without buttons. EN gets 10 kΩ pull-up + 1 µF; a small tactile EN reset button is included.

### Reset and recovery with the case closed (decided 2026-11-13)

The first prototype has **no reset reachable from outside the case**. The RESET (EN) and BOOT buttons are small SMD parts on top of the board, under the top shell. That's acceptable for a prototype, because a stuck board can still be recovered three ways:

1. **USB, case closed.** Plug in USB and open the serial port (or flash): the CP2102N's auto-reset pulls EN low. It's a hardware reset, so it works from deep sleep or a hang. It only works with USB connected, since the CP2102N is powered from VBUS.
2. **Watchdogs.** The ESP32's task and RTC watchdogs reset a hung CPU. The handheld firmware must keep them enabled.
3. **Open the case** (four screws underneath): on-board RESET and BOOT, or unplug the battery's JST for a full power cycle.

Deep sleep alone isn't a risk: any of the eight keys wakes the chip (ext1).

- **Bring-up:** test the auto-reset circuit first on the first board (it's still *verify*). If it fails, every reset or flash means opening the case.
- **Later revision**, if the remote goes to someone who won't open it: a **RESET pinhole next to SETUP** in the floor. Move the RESET switch to the board's underside as a 5 mm tactile switch like SETUP's, and add one floor hole.

### Other schematic parts that matter to the enclosure (reviewed 2026-11-13)

Apart from the parts the case model already covers (`hardware/enclosure/assembly.py`), the schematic is small SMD parts. Three exceptions:

- **Battery connector** (JST PH S2B-PH-K, side entry): now placed on top of the board with a board-edge notch for the leads from the cell underneath. See the PCB constraints in `hardware/enclosure/README.md`.
- **Keystone 5000 test loops** (TP604–606): through-hole wire loops roughly 7–9 mm tall (check the datasheet). Keep them out from under the nRF24 module (11 mm) and the key face, or use flat pads on production boards.
- **DNP parts usable only with the case open:** none left. The 1×6 UART header J601 and the optional IR receiver footprint (U301) were both removed on 2026-11-13.

## nRF24 socket

- 2×4 female header, 2.54 mm. The module plugs in lying flat over the board, antenna end toward the front. Its PCB antenna (the module's front 8 mm) sits over a no-copper keep-out on both layers (rule area in the PCB). Corrected 2026-11-13: the first draft had the antenna overhanging the board edge, before the case design.
- 10 µF + 100 nF at the socket's VCC pin.
- The firmware puts the radio in power-down between presses (~1 µA). No power switch for now.
- **Pulls (added 2026-11-12, review):** 100 kΩ pull-up on CSN (R402) and 100 kΩ pull-down on CE (R403). While the ESP32 boots or sleeps, its pins float; a floating CSN plus noise on SCK could clock garbage into the radio's registers, and a floating-high CE could start TX/RX. CSN is GPIO 5, a strapping pin whose default is high, so the pull-up agrees with it. Each costs nothing while its line sits at its idle level.

### nRF24 supply filtering

Evaluated 2026-11-12. The radio draws ~11–13 mA (genuine part, TX at 0 dBm or RX); clones at their "+7 dBm" setting may draw 25–35 mA (unverified). With 10 µF at the socket, even a 30 mA step lasting 5 µs before the LDO responds dips the rail by only ~15 mV. The breadboard worked on long jumpers with no capacitor at all. So the capacitors are adequate; the risks lie elsewhere:

- **IR pulses on VBAT.** ~300 mA at 40 kHz through the cell's ~0.2–0.3 Ω (cell + protection) sags VBAT ~60–90 mV per pulse, which can push the LDO into dropout near the 3.3 V cutoff. Mitigated because the firmware never transmits RF and IR at the same time, and by the 100 µF at the LEDs. Keep it that way, and keep the low-battery cutoff above the LDO's dropout.
- **Shared ground return.** Keep the IR current loop (VBAT cap → LEDs → transistors → R_E → GND) short and routed to the battery connector away from the radio.
- **ESP32 digital noise** on 3V3: modest with Wi-Fi off. FB401 lets a ferrite bead be fitted later if range or packet loss suggests supply noise (check for bead/ceramic resonance if one is fitted).

Layout: 100 nF right at the socket's VCC/GND pins, the 10 µF next to it, solid ground pour under the module except under its antenna (the keep-out).

## Indicator LED

- 3-lead, 5 mm or 3 mm diffused red/green, common cathode. Pick a ~2.1 V "standard" green (not a 3 V InGaN), or lower its resistor.
- Red: GPIO 22 → 330 Ω. Green: GPIO 16 → 270 Ω. About 4 mA each, PWM for brightness.
- Behaviour copies the original (manual): red while sending IR; steady green on a group command and during the group window; flashing green in setup mode; red flashes at low battery (our choice).

## Buttons and switch

- 8 tactile switches: UP, DOWN, 1–4, ALL, SETUP (SETUP recessed). Each one connects 3V3 to its GPIO, with **100 kΩ pull-down** and an optional 100 nF debounce cap. No current flows unless a button is pressed.
- A/B slide switch (SPDT): GPIO 21 to 3V3 (B) or GND (A). No pull needed. It draws no current in either position.
- The firmware debounces (~20–30 ms) and reads which button caused the wake with `esp_sleep_get_ext1_wakeup_status()`.

## Board assembly (planned 2026-11-13)

**JLC assembles the SMD parts:** the ESP32-WROOM-32E, CP2102N, charger, LDO, FETs and transistors, all passives, the A/B slide switch, and the USB-C receptacle (SMD pads plus through-hole shell legs, routine for JLC). The schematic sticks to parts JLC stocks for this reason.

**The THT parts are hand-soldered**, about 40 easy joints:

| Part | Qty | Note |
|---|---|---|
| Key switches, 6 × 6 mm, 13 mm tall | 7 | flat on the board, soldered from underneath |
| SETUP switch, 6 × 6 mm, 5 mm tall | 1 | mounted on the **underside** (plunger down, behind the floor pinhole), soldered on top |
| IR LEDs (TSAL6200) | 2 | legs bent 90° so the axis is **3.0 mm above the board** (footprint `..._Horizontal_O1.27mm_Z3.0mm`); a small printed bending jig helps |
| Indicator LED (5 mm, 3-lead) | 1 | its height sets how far the dome stands proud of the key face (2 mm): use a spacer or jig |
| nRF24 socket (2×4) | 1 | low-profile type planned (a standard one leaves 0.7 mm under the key face) |
| Battery JST (S2B-PH-K) | 1 | |
| Ground test loops (Keystone 5000) | 3 | optional |

JLC can place THT parts too, at extra cost per part type and only from stock. Here the heights need hand-setting anyway (IR LED axis, indicator protrusion), and SETUP mounts upside down.

### Key switch sourcing (checked 2026-11-13)

Needed: 6 × 6 mm, 4 straight through-hole legs at **6.5 × 4.5 mm** (KiCad `SW_PUSH_6mm_H13mm`), **13.0 mm from the board to the top of the plunger**, ~0.25 mm travel. 7 per remote, plus spares.

| Part | Force | Where | Notes |
|---|---|---|---|
| **Same Sky TS02-66-130-BK-160-SCR-D** (chosen; 100 ordered 2026-11-13) | 160 gf (1.6 N) | DigiKey 2223-TS02-66-130-BK-160-SCR-D-ND: 4,939 in stock, US$0.14 each (1), $0.114 (25) | Datasheet confirms the 13.0 mm height above the PCB and 0.25 mm travel. Same series: 100 gf (`-100-`, lighter) |
| TE / Alcoswitch 2-1825910-6 (FSMJ series) | 100 gf | DigiKey: ~29,600 in stock, CA$0.38 (1) | 13.00 mm, 0.25 mm travel, 100k cycles |
| Kinghelm KH-6X6X13H-TJ | 2.6 N (~260 gf) | LCSC / JLC C2837523 | Useful if JLC ever places the THT switches |

160 gf is a typical remote-key feel; 100 gf is light, 260 gf firm. **SETUP switch:** the same series at 5.0 mm (TS02-66-50-BK-160-SCR-D: DigiKey 2223-TS02-66-50-BK-160-SCR-D-ND, ~18,000 in stock, US$0.12). For prototype 1, a 6×6×5 from the BOJACK assortment on hand will do once its dimensions are checked. The leg spacing and plunger diameter are on the datasheets' drawings (not checked here): the test print's key fixture has holes at 6.5 × 4.5 mm, so it checks the real switch on arrival.

## Layout notes

- 2-layer board, ground pour on both sides, stitched.
- **ESP32 orientation (2026-11-13):** antenna end at the right-hand (−Y) board edge, holding the remote pointing away. The key GPIOs must be RTC pins, which all sit in the module's left pin column (pins 4–12), so that column has to face the keys; the SPI and UART pins (right column) then face the nRF24 socket and CP2102N at the back. With the antenna at the other edge, 14 key and SPI lines crossed each other round the module.
- Keep the **ESP32 module's antenna area** clear of copper, per Espressif's guidelines.
- **Battery under the ESP32 antenna area (noted 2026-11-14):** the cell's keep-out (cell 30 mm wide + 0.5 mm margin, ±15.5 mm) overlaps the ESP32 antenna keep-out by a 1.5 mm strip along the −Y side (Y −15.5 to −14.0, X −47 to −16.5), so the cell's foil-laminated edge sits about 1 mm under the inner end of the antenna area. No effect in this design, since the ESP32's radio is unused (no Wi-Fi or Bluetooth; all RF is the nRF24). If the ESP32 radio is ever used, expect detuning and lost range: use a narrower cell (25 mm wide leaves ~4 mm clearance), shift the cell toward +Y in the case model, or accept the loss. The two keep-outs don't conflict in DRC: one forbids copper, the other THT pins and underside parts. The ESP32's radio isn't used, but this keeps the option open and is free.
- Keep the **nRF24 antenna overhang** clear, and keep the IR LEDs, charger and LDO away from it.
- The IR LEDs sit at the front edge, aimed forward. Their current loop (VBAT cap → LEDs → transistors → R_E → GND) should be short and wide.
- Buttons laid out like the original: arrows large and central, group keys in a row, ALL separate, SETUP on the back.
- Size, shape, mounting holes and keep-outs come from the enclosure: import `docs/images/enclosure/enclosure_pcb_outline.dxf` onto Edge.Cuts and follow the PCB constraints in `hardware/enclosure/README.md`.

## Schematic: Power and USB sheet (draft, 2026-11-12)

[`hardware/hd_remote/power_usb.kicad_sch`](../hardware/hd_remote/power_usb.kicad_sch) implements the Power and USB sections above. ERC: no errors; the only warnings are the labels to the ESP32 (`EN`, `IO0`, `U0TXD`, `U0RXD`), which connect when the MCU sheet exists. Values chosen there that the sections above don't state:

| Item | Value | Note |
|---|---|---|
| Passives | 0603 | hand-reworkable; JLC-assemblable |
| USB-C receptacle | HRO TYPE-C-31-M-12 footprint | common, cheap 16-pin USB 2.0 part; shield to GND |
| CP2102N supply | bus-powered: VREGIN = VBUS (4.7 µF + 100 nF), VDD = VIO (4.7 µF + 100 nF) | checked 2026-11-13 against the CP2102N data sheet rev 1.3, §2.1: 4.7 µF + 0.1 µF on *each* power pin. VDD's cap was 1 µF; now 4.7 µF (C103). Pinout matches Fig. 5.2 (QFN24, centre pad GND) |
| CP2102N VBUS sense | 22 kΩ / 47 kΩ divider from VBUS | Silicon Labs show 22.1 kΩ / 47.5 kΩ; the JLC basic E12 values give 3.40 V instead of 3.41 V (changed 2026-11-13). Checked: rev 1.3 §2.3 requires the divider in bus-powered designs too (VBUS abs. max is VIO + 2.5 V; rev 1.1 drew VBUS direct, rev 1.2 added the divider). 5 V gives 3.40 V on the pin, above VIH = VIO − 0.6 V |
| CP2102N RST | 1 kΩ pull-up to VIO | checked: rev 1.3 §2.1 recommends 1 kΩ to VIO. Unused inputs (CTS, DSR, DCD, RI) can float: the GPIO have weak pull-ups |
| Auto-reset | 2× MMBT3904, 10 kΩ base resistors | the DevKitC circuit |
| Charge current | R_PROG 3.9 kΩ, ~256 mA | 1000 V / R_PROG; was 4.02 kΩ (~249 mA), changed 2026-11-13 to a JLC basic part. The MCP73831's own tolerance is ±10%; ~0.5C on a 500 mAh cell |
| Charger caps | 4.7 µF on VBUS and VBAT | MCP73831 datasheet minimum |
| Power path | D102 STPS1H100AY (SMA), Q104 AO3401A, R114 100 kΩ VBUS→GND, TP102 on VSYS | Microchip AN1149; see [Power](#power). LDO input (U104 pins 1, 3) and C107 moved to VSYS (2026-11-13) |
| Charge LED | 0603, 1 kΩ from VBUS to STAT | lit while charging |
| Battery connector | JST-PH 2-pin, side entry (S2B-PH-K) | + and − on the silkscreen; *check the cell's polarity* |
| Reverse-polarity FET | Q103, AO3401A (SOT-23), gate to GND | see [Power](#power); placed last so the earlier references kept their numbers |
| LDO | RT9080-33GJ5, 1 µF in and out | drawn with the AP2112K symbol (same SOT-23-5 pinout: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT). Checked 2026-11-13 against Richtek DS9080-09: pinout matches; EN may be tied to VIN (must not float); C_IN ≥ 1 µF; C_OUT ≥ 1 µF nominal, ≥ 0.7 µF effective. The ESP32's 10 µF + 100 nF on the MCU sheet sit on the same rail. Dropout 0.31 V typ / 0.53 V max at 600 mA; far less at our ~120 mA |

**LCSC part numbers (added 2026-11-13):** every SMD part has an `LCSC` field, found with the jlcsearch index of JLC's catalogue and confirmed by C number; stock wasn't available for most, so check it when ordering. Passives are JLC basic parts except R303/R304 (3.3 Ω 0805, C2907250) and the 100 µF (C15008). The ICs, connectors and switches are extended parts (one setup fee each per order). Notes:

- **SW201/SW202:** KMR221NGLFS (C269272), the 4.2 × 2.8 mm body without the ground tab, to fit KiCad's 4-pad `SW_Push_1P1T_NO_CK_KMR2`. Check the land pattern against C&K's drawing before layout.
- **SW509:** PCM12SMTR (C221841), matching the footprint. The case model's slider also allows an MSK12C02; that needs a different footprint.
- **U103:** MCP73831T-2ACI/OT (C424093), the 4.20 V `-2` variant.
- **No LCSC number (hand-soldered THT, or not a part):** the 8 key switches (Same Sky TS02, DigiKey), IR LEDs, indicator, nRF24 socket (low-profile part not chosen), ground loops TP604–606, test pads and JP101.

## Reference design review (2026-11-14)

The ESP32 core, USB-serial and auto-reset were compared line by line against Espressif's [ESP32-WROOM-32E datasheet v2.1](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf) (section 9, Peripheral Schematics), the [ESP32-DevKitC V4 schematic](https://dl.espressif.com/dl/schematics/esp32_devkitc_v4-sch.pdf) and Espressif's [ESP32 schematic checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html). The power path was checked against Microchip AN1149 separately (see [Power](#power)).

| Item | References | Ours | Result |
|---|---|---|---|
| 3V3 supply | ≥ 500 mA (checklist) | RT9080, 600 mA | ✓ |
| 3V3 decoupling at the module | 22 µF + 0.1 µF (datasheet, DevKitC); ≥ 10 µF + 0.1 µF (checklist) | was 10 µF + 100 nF | **C201 raised to 22 µF** |
| EN RC | 10 kΩ / 1 µF (datasheet, checklist); DevKitC uses 0.1 µF | 10 kΩ / 1 µF, C203 ~5 mm from the pin | ✓. Keep the EN trace short (checklist) |
| Slow supply ramps | checklist: for slow rise/fall (e.g. battery charging) an RC may not be enough; consider a supervisor (~3.0 V) on EN | none | Accepted for now: the power path gives a fast VSYS from USB, and the firmware cuts off at ~3.3 V VBAT, well above the ESP32's operating minimum. Revisit if bring-up shows resets during slow battery discharge or recovery |
| Strapping pins | GPIO0 pull-up, no large caps on IO0; GPIO2 low/float; MTDI (GPIO12) low; MTDO; GPIO5 high | IO0: internal pull-up + BOOT + Q102, no cap; GPIO2, 12, 15 unconnected; GPIO5 = CSN with 100 kΩ pull-up | ✓ |
| Auto-reset | DTR/RTS → 10 kΩ → 2 NPN (DevKitC) | same, MMBT3904 | ✓ (still *verify* on the first board) |
| U0TXD series resistor | 499 Ω for harmonic suppression (checklist); DevKitC 0 Ω | R107 10 kΩ (chosen for back-powering) | ✓; slower edges, fine at 921 600 baud |
| CP2102N | VBUS divider 22.1 k / 47.5 k; 4.7 µF + 0.1 µF (DevKitC) | 22 k / 47 k; 4.7 µF + 100 nF per pin | ✓ |
| USB ESD | ESD diodes on D+/D− (DevKitC: LESD5D5.0) | USBLC6-2SC6 | ✓ |
| ADC pin | 0.1 µF filter at the pin (checklist) | C109 at GPIO 39 | ✓ |
| Input-only pins (34–39) | no internal pulls: add external | all key pins have 100 kΩ pull-downs | ✓ |
| GPIO 36/39 glitch | ~80 ns pulses while RTC/ADC is powered (checklist) | one key on GPIO 36 | ✓ noted; debounce in software |
| RESET/BOOT switches (KMR221NGLFS) | C&K KMR2 "NG" recommended layout: 0.9 × 1.0 mm pads at ±2.05 / ±0.8 mm | KiCad `SW_Push_1P1T_NO_CK_KMR2` | ✓ exact match |

## Schematic: MCU sheet (draft, 2026-11-12)

[`hardware/hd_remote/mcu.kicad_sch`](../hardware/hd_remote/mcu.kicad_sch): the ESP32-WROOM-32E with every GPIO in the pin map on a named global label (`BTN_UP`, `IR_TX`, `NRF_CSN`, ...), which the IR, nRF24 and buttons sheets will use. Also:

| Item | Value | Note |
|---|---|---|
| 3V3 decoupling | **22 µF** (0805, C45783) + 100 nF at the module | was 10 µF; raised 2026-11-14 to match the module datasheet's reference circuit and the DevKitC (see [Reference design review](#reference-design-review-2026-11-14)) |
| EN | 10 kΩ pull-up, 1 µF to GND, RESET button | DevKitC values |
| BOOT | button from IO0 to GND | IO0 has an internal pull-up |
| Buttons | C&K KMR2 footprint (small SMD tactile) | for RESET and BOOT; the user keys are on the buttons sheet |
| GPIO 2, 12, 15 | not connected | strapping pins; 12 must be low at boot |
| GPIO 13 | label `NRF_IRQ_OPT` | for the optional nRF24 IRQ link |

Part references are numbered by sheet: 1xx Power and USB, 2xx MCU, 3xx IR.

## Schematic: IR sheet (draft, 2026-11-12)

[`hardware/hd_remote/ir.kicad_sch`](../hardware/hd_remote/ir.kicad_sch) implements [IR transmitter](#ir-transmitter) above. (The optional receiver was removed on 2026-11-13.)

| Item | Value | Note |
|---|---|---|
| IR LEDs | 2× TSAL6200, 5 mm, footprint bent 90° (`LED_D5.0mm_Horizontal_O1.27mm_Z3.0mm`) | lying flat, pointing out of the board edge; either position can take a wider-angle LED |
| Current sinks | BC817-40 (SOT-23), R_E 3.3 Ω 0805 | ~0.1 W peak in R_E, hence 0805; value set from the breadboard (was 4.3 Ω, see [IR transmitter](#ir-transmitter)) |
| Clamp | 2× 1N4148W (SOD-123), base resistor 470 Ω, 100 kΩ pull-down | |
| Bulk on VSYS | 100 µF 1206 ceramic + 100 nF | a 100 µF ceramic keeps well under half its capacity at 4 V; check the derating, or use a tantalum or polymer cap |
| ~~Receiver (DNP)~~ | removed 2026-11-13 | was a TSOP382xx footprint (U301) with a 100 Ω + 100 nF supply filter (R305, C303); GPIO 14 is now spare |

## Schematic: nRF24 and Buttons/LEDs sheets (draft, 2026-11-12)

[`nrf24.kicad_sch`](../hardware/hd_remote/nrf24.kicad_sch) (4xx) and [`ui.kicad_sch`](../hardware/hd_remote/ui.kicad_sch) (5xx) complete the schematic. **ERC on the whole design: 0 errors, 0 warnings**, and every GPIO net in the netlist matches the pin map above.

| Item | Value | Note |
|---|---|---|
| nRF24 socket | 2×4 female, 2.54 mm, vertical; pins numbered as on the module (odd row GND, CE, SCK, MISO; even row VCC, CSN, MOSI, IRQ) | **low-profile** part (body ≤ ~5 mm; added 2026-11-13): the module lies flat over it, and a standard 8.5 mm socket leaves only 0.7 mm under the key face. Same footprint. The module's pins (~6 mm below its spacer) may bottom out in a short socket: trim them, or pick a socket open at the bottom. Machined round-pin strips are low but don't take square header pins well. Part not chosen yet. Orientation and antenna overhang are layout tasks |
| nRF24 supply | 3V3 → FB401 (0603: 0 Ω fitted; ferrite-bead option) → `NRF_VCC` with 10 µF (0805) + 100 nF at the socket, and a DNP 100 µF (1206) for a PA/LNA module | see [nRF24 supply filtering](#nrf24-supply-filtering) |
| nRF24 IRQ | to GPIO 13 through a 0 Ω, DNP | firmware doesn't use IRQ |
| nRF24 pulls | R402 100 kΩ CSN to `NRF_VCC`, R403 100 kΩ CE to GND | added 2026-11-12, see [nRF24 socket](#nrf24-socket) |
| Keys | Same Sky TS02-66-130-BK-160-SCR-D (13 mm, 160 gf) for the 7 keys; TS02-66-50-BK-160-SCR-D (5 mm) for SETUP, on the board's underside | footprints `SW_PUSH_6mm_H13mm` / `_H5mm` (6.5 × 4.5 mm legs); see [Key switch sourcing](#key-switch-sourcing-checked-2026-11-13). Part numbers in each symbol's MPN field (2026-11-13) |
| Mounting holes | H601–H604, M2 (2.2 mm, unplated) on the Test and debug sheet | at (±61, ±15.5) mm from the board centre, ~5.5 mm head keep-outs; for M2 self-tappers into the case standoffs (2026-11-13) |
| Key pull-downs | 100 kΩ each; 100 nF debounce caps DNP | firmware debounces |
| A/B switch | SPDT slide (PCM12 footprint): common to GPIO 21, throws to GND (A) and 3V3 (B) | which throw is "A" depends on the part and its orientation |
| Indicator | 3-lead red/green common cathode, 5 mm; red 330 Ω, green 270 Ω | *check which outer leg is red* on the part bought |

## Test points (2026-11-12)

Chosen for bring-up and for the checks still open in this spec. SMD pads (1.5 mm) on the back unless noted.

| Ref | Net | Sheet | Use |
|---|---|---|---|
| JP101 | VBAT_BATT – VBAT_JP | Power and USB | **Battery current link**, a bridged solder jumper between the battery connector and everything else (it sits before the reverse-polarity FET Q103). Cut it and put a µA meter across the pads to measure total battery current: the sleep budget (~11–16 µA) and the CP2102N back-powering check with USB unplugged. Re-bridge with solder afterwards |
| TP101 | PROG | Power and USB | charge current: V(PROG) = 1 V at the full ~250 mA |
| TP301, TP302 | IR_E1, IR_E2 | IR | top of each emitter resistor: scope to GND, I_LED = V / R_E, at VBAT 3.3 and 4.2 V |
| TP303 | IR_BASE | IR | the clamp voltage (~1.3 V) and the drive waveform |
| TP601–TP603 | VBAT, +3V3, VBUS | Test and debug | rail checks; LDO dropout under IR load |
| TP102 | VSYS | Power and USB | power-path output: should read ~VBUS − 0.5 V with USB, ~VBAT without (added 2026-11-13) |
| TP604–TP606 | GND | Test and debug | THT loops (Keystone 5000) for scope ground clips: one near the IR stage, one near the radio |
| TP607 | IR_TX | Test and debug | carrier and duty cycle on the analyzer |
| TP608 | VBAT_SENSE | Test and debug | calibrate the battery ADC against a meter |
| TP609 | GPIO 13 (`NRF_IRQ_OPT`) | Test and debug | **debug strobe**: the firmware toggles it at RF / IR step boundaries, so the analyzer can time the wake against the IR. Shared with the optional nRF24 IRQ link (R401, DNP): fit one or the other |
| ~~J601~~ | (removed 2026-11-13) | Test and debug | Was a DNP 1×6 header (GND, 3V3, U0TXD, U0RXD, EN, IO0) for an external USB-serial adapter in case the onboard CP2102N failed. Removed to simplify routing: its EN line crossed the board, and it only worked with the case open. Replaced on 2026-11-14 by the four pads below |
| TP610, TP611 | U0TXD, U0RXD | Test and debug | **UART fallback** (replaces J601): ESP32 TX0 → adapter RX, RX0 ← adapter TX, for flashing and monitoring with an external 3.3 V USB-serial adapter if the CP2102N fails. Unplug USB-C first; power from the adapter's 3V3 (to TP602) or from the battery, not both |
| TP612, TP613 | EN, IO0 | Test and debug | reset and boot strap for the same fallback: hold IO0 low, pulse EN low to enter the bootloader (or let the adapter's DTR/RTS drive them). GND from TP604–606 |

The nRF24's SPI lines need no test points: the socket's through-hole pins on the back take analyzer clips.

## Reference: the original Platinum remote's layout (recorded 2026-11-13)

From the front/back figure in Hunter Douglas's Platinum remote guide (described here, not reproduced; the guide's images stay out of the repo).

**Front, from the tip:**
- **Indicator light** at the very tip.
- **Infrared Channel Selector:** an A/B slide switch across the width, labelled CHANNEL.
- **OPEN / CLOSE:** one tall oval with ▲ on the upper half and ▼ on the lower half.
- **Group keys:** 1 and 2 on either side of OPEN; 3 and 4 on either side of CLOSE.
- **ALL**, centred below, inside an arc labelled GROUPS.
- The maker's logo runs lengthwise down the lower half of the face.

**Back:**
- **Setup:** a pinhole about a quarter to a third of the way down from the tip, slightly off-centre.
- A label with the maker's name and the FCC ID, in the middle.
- **Battery compartment** with a door, in the lower half.
- **Master Reset:** a second pinhole, just above the battery compartment.

**Shape:** long and rounded. Widest near the top (key) end, tapering toward the bottom, which is the grip, with a curved back.

**How our case maps onto it** (`hardware/enclosure/`):
- **Same:** the order and arrangement of the top face, with the indicator, A/B slider, arrow oval, groups and ALL. SETUP is a floor pinhole near the front, as on the original.
- **Different:**
  - We have no Master Reset pinhole. The original's Master Reset concerns the remote's identity (duplicating a remote, manual p. 7); our `id` command covers that. See also *Reset and recovery* above for the ESP32 reset, which is a different thing.
  - We charge over USB-C, with no battery door.
  - Our case is a straight box with rounded edges, not tapered.
- **The original's logo area is our empty back half.** A taper toward the grip, or a recessed name, is the obvious way to use it (open idea).

**FCC ID: UXURC4U2** (read from our remote's back label, 2026-11-13; grantee code UXU, product code RC4U2). The FCC filing's test reports and internal photos show the hardware, including where the radio antenna sits, which bears on the hand-shielding question.

The filing's summary page ([fccid.io/UXURC4U2](https://fccid.io/UXURC4U2), read 2026-11-13) lists:

| Field | Value |
|---|---|
| Grantee | Hunter Douglas Window Fashions |
| Description | "IF/RF Remote Control kit", model RC4U2 |
| Grant date | 2012-05-02 |
| Frequency range | **2433–2471 MHz**: exactly our sniffed channels 33, 52 and 71 (`docs/RF_PROTOCOL.md`), an independent confirmation |
| Output power | 0.0001436 W = 0.14 mW ≈ **−8.4 dBm**. That's in line with our ~−6 dBm working figure. The summary doesn't say whether it's conducted or EIRP |

Exhibits on that page: internal photos, external photos, test report, test setup photos, user manual, label, and letters. Automated downloads were blocked (HTTP 403); John viewed the internal photos in a browser. They're Hunter Douglas's, so they're described here, not committed.

**Internal photos** (reviewed 2026-11-13; positions read off the photos' ruler, in cm from the tip, i.e. the IR end):

- **Board:** one PCB about 13 cm long, tapered like the case, wider at the tip end.
- **Key side:**
  - **Keys:** seven gold interdigitated contact pads, ~1.5–5.5 cm, so a **rubber membrane keypad** with conductive pads, not tactile switches.
  - **A/B and indicator:** a slide switch (S10) near the tip, and the indicator LED at the end.
  - **Radio section:** ~8–12 cm: crystal, two QFN chips (nRF24 and microcontroller), programming pads.
  - **Antenna:** a small blue part labelled **A1** at ~7.3 cm, beside a copper-free patch (~7.5–9.5 cm). Very likely a chip antenna (inferred from the "A" designator and the clear patch). That puts the **antenna about mid-board, on the key side**.
  - A sticker on the test unit reads **"R-6dB"**: probably the transmit-power setting. That fits our ~−6 dBm figure and the filing's −8.4 dBm.
- **Battery side:**
  - **Pinhole buttons:** two tactile switches at ~2.8 cm and ~8.2 cm, behind the back's SETUP and Master Reset pinholes.
  - **IR LED:** **one** IR LED lying flat and poking out of the tip, like ours (we have two).
  - **Batteries:** silkscreen cell outlines at ~7.5–12.5 cm, **directly under the radio section and antenna**.
  - **Unidentified:** a coil spring and wire near the tip, and a wire at the far end, probably battery contacts.

**What we take from it:**
1. **Hand shielding is a smaller worry than feared.** The original puts its antenna around mid-length, in or near the grip, with the batteries underneath, at about −6 dBm, and it works. Our nRF24 antenna (ending ~108 of 150 mm from the front) is in a comparable place. The grip test drops from must-do to nice-to-have.
2. **SETUP on an underside switch near the tip matches the original.** No change needed.
3. **Later-revision idea:** a rubber keypad on PCB contact pads instead of 13 mm tactile switches would make the case much thinner. Most of our height goes on the switches and keycaps. The cost is a moulded keypad and a different key design.


This design uses 22 of the WROOM-32E's 26 usable GPIOs (free: 13, plus strapping pins 2, 12 and 15), so it won't fit a XIAO, which has 11. A smaller second revision could fit, by:

- putting all 8 buttons on **one ADC pin through a resistor ladder**; the cost is no simultaneous presses, which the original never needs. **Wake needs care:** deep-sleep wake on a GPIO is digital, so a ladder tap that sits below the logic-high threshold won't wake the chip. Either design the ladder so every key clears the threshold (which squeezes the ADC spacing), or OR every key onto a separate wake pin through diodes (one more pin), or wake with the ULP reading the ADC (more current and firmware). Corrected 2026-11-12: the first draft said any press would wake it;
- folding the A/B switch into the ladder (the IR receiver footprint is already gone).

That's 11 pins: nRF24 5, IR 1, bicolor LED 2, ladder 1, battery sense 1, A/B 1. The XIAO's own USB-C and LiPo charger would replace the CP2102N, the MCP73831 and probably the LDO. The costs are no spare pins, single-press-only buttons and an ESP32-S3 firmware port. Agreed 2026-11-11: WROOM first, XIAO as a possible later revision.

## Open questions

1. **Cell:** flexible on size and connector. Pick to suit the enclosure; check the polarity of the purchased cell.

Resolved 2026-11-13: the enclosure (custom printed clamshell, see Decisions). Resolved 2026-11-11: two TSAL6200 positions; optional IR receiver footprint; separate charge LED; keep USB debugging on early revisions.
