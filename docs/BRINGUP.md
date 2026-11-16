# Handheld board bring-up plan (rev 1)

Written 2026-11-15, before the boards arrive, so the first power-up follows a plan rather than enthusiasm. Work through the stages in order and **stop at the first failure**: each stage assumes the ones before it passed. Record results in the log at the end (copy it per board; boards are numbered by hand on arrival).

Background: [HARDWARE.md](HARDWARE.md) (spec, power path, sleep budget), [hardware/README.md](../hardware/README.md) (layout status), `include/pins.h` (pin map, checked against the schematic by `hardware/tools/check_pins.py`).

## Before ordering

The boards don't need to wait for the enclosure test piece (noted 2026-11-16): every sample on it sets a case value (hole sizes, pilots, keycap play), none moves a part on the board, so its findings go into the next print.

- [x] **Datasheet pinout review** (2026-11-16, two independent agent reviews: no issues. BC817-40, 1N4148W, USBLC6-2SC6 and AO3401A were checked against the manufacturers' rendered pinout drawings; AO3401A's pin numbers come from AOS's labelled drawing plus SOT-23 convention, as AOS prints none) of the small parts, done by someone (or a fresh Claude Code session) who hasn't seen the design: AO3401A (Q103, Q104), BC817-40 (Q301, Q302), MMBT3904 (Q101, Q102), RT9080-33GJ5 (U104, drawn with the AP2112K symbol), MCP73831 (U103), USBLC6-2SC6 (U101), STPS1H100AY (D102) and 1N4148W (D301, D302) direction, CP2102N QFN-24 (U102). (D301/D302 checked 2026-11-16 in the layout and KiCad 3D view: anodes toward IR_BASE, cathodes toward GND, band at the silkscreen bar.)
- [x] **J401 pin 1** against a real nRF24 module (2026-11-16, John). **Found mirrored:** KiCad's `PinSocket_2x04` puts pad 2 to the left of pad 1 (for a board flipped face-down onto the socket), but the module sits face-up, so every pair was swapped, VCC↔GND included. The module was metered: square pin = GND, the pin directly across = 3.3 V (matches the breadboard wiring). Fixed: J401 is now `PinHeader_2x04_P2.54mm_Vertical` at the same hole positions (schematic, `gen_nrf24.py` and `fp_overrides.json` too), re-routed by John. Seen from the top with the antenna to the right: left column 1 GND (square), 3 CE, 5 SCK, 7 MISO; right column 2 VCC, 4 CSN, 6 MOSI, 8 IRQ. DRC 0 errors, 0 unconnected, parity 0.
- [x] **Position check of the other parts that mate with something physical** (pad positions and nets against the real part, seen from the top; not just pin numbers). Done 2026-11-16:
  - J101 USB-C: pass (D+ A6/B6, D− A7/B7, CC1/CC2 separate, VBUS and GND on all four corners, shield GND).
  - J102 battery: pass on paper (opening faces +X; looking into it, pad 1 = + is on the left, Adafruit's convention). Still meter the real plug (stage 7).
  - D303/D304 IR LEDs: pass (square = cathode). When bending the leads, the long leg goes into the round pad.
  - D501: pass (datasheet-checked 2026-11-15; reversed only swaps red and green).
  - SW509 A/B: pass (common is the middle pad; reversed only swaps A and B).
  - [x] **SW501–SW508 keys:** pass. The footprint treats the legs 6.5 mm apart as always connected; Same Sky's TS02 datasheet (11/03/2025, p.2, SCR version) shows ①–② and ③–④ joined, 6.5 mm apart. John's BOJACK 6 × 6 mm switches meter the same way.
  - [x] **D101 charge LED:** pass. The lens faces −X toward the window; Kingbright's APA1606SURCK datasheet (V.20A, p.1) puts the cathode on pin 1 (symbol and polarity mark), and pad 1 is on STAT. Still glance at it in JLC's preview.
- [x] Silkscreen tidy-up (2026-11-16): TP601 label off its pad and clear of the battery outline, TP608 and TP101 labels shown, TP609 label horizontal under its pad. DRC: only the expected board-edge clipping (J101, J102, D303, D304) and U201's library mismatch remain.
- [ ] JLC outputs: Gerbers, BOM, CPL.
- [ ] **JLC dry run:** upload the Gerbers, turn on PCB Assembly, upload the BOM and CPL, *Process BOM & CPL*, and stop before paying (nothing is committed until checkout). Check:
  - Bill of Materials page: every part matches its LCSC number and is in stock.
  - Component Placements page: every polarized part lines up with its pin-1 dot. D301/D302 band at the silkscreen bar; D101, D102; Q101–Q104, Q301, Q302; U101–U104 and U201's pin 1; USB-C overhanging the edge as designed. JLC's CPL rotations for SOD-123 and SOT-23 can come out 180° off. Fix rotations on that page; a footprint problem means fixing KiCad and re-exporting.
- [ ] **At ordering:** look at the placement page again before paying (stock can change). After paying, watch email for a day or so: JLC's engineers may query a placement and hold assembly until you reply.
- [ ] Same week, DigiKey (hand-fitted, not from JLC): D501 Kingbright WP59EGW (1747631), Bivar PLP2-250 light pipes (several: the fit test uses four hole sizes), J401 socket Samtec SLW-104-01-G-D (612-SLW-104-01-G-D-ND; one per board plus spares).

## Before the boards arrive

- [x] **D501 (bicolor indicator):** Kingbright WP59EGW; its datasheet's lead order matches the footprint (pad 1 = red anode, square pad; 2 = common cathode; 3 = green anode). When fitting: diode-test the leads first, seat it flush, 2–3 s per joint.
- [ ] **Light pipe:** test-fit a Bivar PLP2-250 in a printed 2.92 mm hole (try 2.85 / 2.95 / 3.05 mm on the next print); glue it if loose.
- [ ] **IR current-sink breadboard test** (the *verify* in [HARDWARE.md](HARDWARE.md), IR transmitter; never run, confirmed 2026-11-16). Needs 2× 1N4148 and an NPN (2N2222 preferred; not on hand), 470 Ω, 100 kΩ, ~3.3 Ω for R_E (two 6.8 Ω in parallel), a TSAL6200. GPIO 4 → 470 Ω → base node; node → two diodes in series → GND, and → 100 kΩ → GND; emitter → R_E → GND; collector → LED cathode; LED anode → supply. `send up 3000`, scope across R_E (I = V_peak / R_E) and the base node (~1.3 V), at 3.3 V and 4.2 V LED supply. Pass: the two currents within ~10 % of each other, none above 200 mA. A 2N2222's Vbe differs from the BC817-40's, so this proves the regulation, not the board's exact current (stage 5).
- [ ] `python -I hardware/tools/check_pins.py` passes (pins.h matches the schematic).
- [ ] `pio run -e handheld` builds.

## Equipment

- Multimeter (continuity, volts, a µA range for stage 7).
- Current-limited 5 V source: a bench supply into a USB-C breakout, or at least a USB power meter in line with a charger. Not a bare laptop port for the first power-up.
- A USB-C data cable to the PC.
- The breadboard DevKitC with its IR receiver (the PCB has none), for stage 5.
- Fine tweezers/probe clips for the underside test pads.

Underside test pads: TP102 VSYS, TP602 +3V3, TP603 VBUS, TP601 VBAT, TP608 VBAT_SENSE, TP101 PROG, TP607 IR_TX, TP301/TP302 IR emitters, TP303 IR base, TP609 GPIO 13, TP610 U0TXD, TP611 U0RXD, TP612 EN, TP613 IO0. GND: TP604–TP606 (top).

## Stage 1: unpowered checks (no battery, no nRF24 module)

1. Visual: solder bridges on U102 (CP2102N QFN) and U201's castellations; D303/D304 and D501 orientation (square pad = cathode for the TSAL6200s); J102 and J401 seated.
2. Resistance to GND (TP604): **VBUS (TP603)**, **VSYS (TP102)**, **+3V3 (TP602)**, **VBAT (TP601)**. None should read a short (a few ohms). Expect kilohms or more; +3V3 may read lower through the ESP32 and caps, but not near zero.
3. VBUS to VSYS should show D102's diode drop on the diode range in one direction only.

## Stage 2: first power-up from USB (no battery, no nRF24 module)

1. Current-limited supply at 5.0 V, limit ~200 mA. Plug in.
2. Current at idle: expect tens of mA (ESP32 running from flash, or in the ROM bootloader if blank). **Pull the plug if it hits the limit.**
3. Voltages, against TP604:
   - VBUS (TP603) ≈ 5.0 V.
   - VSYS (TP102) ≈ 4.4–4.5 V (VBUS minus D102's ~0.5–0.6 V).
   - +3V3 (TP602) = 3.3 V ± 3 %.
   - VBAT (TP601): the charger's output with no cell; may float or sit near 4.2 V. Note it, don't judge it yet.
4. Charge LED D101: with no cell, the MCP73831's STAT may flicker or stay off. Note it.
5. Feel U104, U103, U102 and D102 after a minute: none should be warm.

## Stage 3: USB serial and flashing

1. On the PC, a new CP210x COM port appears (Device Manager). If not: check VDD_USB (C104) ≈ 3.3 V and CP_RST high.
2. Flash: `pio run -e handheld -t upload --upload-port COMx`. Auto-reset should enter the bootloader on its own.
   - If it can't connect: hold BOOT (SW202), tap RESET (SW201), release BOOT, retry.
   - Last resort: an external USB-serial adapter on TP610/TP611 with TP612 (EN) and TP613 (IO0).
3. Monitor at 115200 baud: the banner "HD Platinum remote bring-up firmware" and the `help` text.
4. `help`, then RESET (SW201): the board reboots cleanly.

## Stage 4: inputs and indicator

1. `btn` with nothing pressed: every key 0. Then hold each key in turn and run `btn`: exactly that key reads 1 (UP, DOWN, ALL, G1–G4, SETUP; SETUP is on the underside).
2. A/B slider: `btn` in each position. **Record which position reads A** (the firmware currently assumes GPIO 14 high = B), and fix `main.cpp`/the docs if it's the other way.
3. `btn` VBAT reading: with no cell it reflects the charger output; recheck in stage 6.
4. Indicator D501: `led r`, `led g`, `led both`, `led off`. Red and green should each light alone (a swap means D501's lead order doesn't match the footprint), and `both` should look amber-ish.

## Stage 5: IR

1. Breadboard DevKitC (its own firmware, `ir` capture) a few cm in front of the board, aimed at D303/D304.
2. On the PCB: `wake off` (no nRF24 module yet), then `send up`. The DevKitC should decode channel A `0101`; `send down` → `0110`.
3. Optional: scope or logic analyzer on TP607 (IR_TX, 40 kHz bursts) and TP301/TP302 (emitter: ~0.5 V peaks ≈ 150 mA through the 3.3 Ω).
4. **LED current, the board's real value:** emitter peaks on TP301/TP302 on USB (VSYS ~4.5 V) and again in stage 7 on the cell. Target ~150 mA, never above 200 mA; change R_E (R303/R304, 0805) if it's off. Then the BC817 temperature during repeated presses on USB (~0.18 W each, 0.25 W rated).

## Stage 6: nRF24 module and radio

1. **Trim the module's header pins** to ~2.6 mm below the black plastic spacer (from 5.3–5.4 mm; the SLW socket takes 2.16–2.92 mm). A 2.6 mm spacer (card or PCB scrap) held against the plastic makes a cutting guide for flush cutters. Keep untrimmed modules for the breadboard: short pins grip poorly in Dupont jumpers.
2. Unplug USB. Fit the module in J401, antenna toward the front, inside the silkscreen outline. Check it seats fully and doesn't rock. Plug in again; recheck +3V3 and the supply current.
3. `test`: PASS at every data rate.
4. `sniff` with an original remote's group key + arrow nearby: frames appear (IDs are private: don't paste them anywhere public).
5. On a shade: `wake on`, arrow keys at the prompt → the shade responds (point-and-press), then `grp 4 up` with the paired ID.

## Stage 7: battery

1. **Cell polarity first:** meter the Adafruit #1578's JST plug: its + must land on J102 pin 1 (VBAT_BATT). Q103 blocks a reversed cell, so a wrong one shouldn't damage anything; it just won't power the board.
2. USB unplugged, cell plugged in: VSYS (TP102) ≈ cell voltage, +3V3 = 3.3 V, the board runs; `btn` VBAT ≈ the cell voltage (the divider halves it; the firmware doubles it back).
3. USB plugged in with the cell: D101 lights while charging; PROG (TP101) shows the charge current (MCP73831 datasheet); VSYS returns to ~4.4–4.5 V; the board keeps running throughout the plug/unplug.
4. **Battery current through JP101:** JP101 is a 0 Ω 0805 in the battery path (fitted by JLC). To measure, lift it (hot tweezers or two irons), put the meter (µA/mA range) across its pads, measure, then refit it. Measure: running idle, and later in deep sleep against the ~15 µA budget ([HARDWARE.md](HARDWARE.md), Sleep current budget; watch for the ~50 µA TX0 pull-up case).

## Stage 8: in the case

1. Board into the bottom shell on its standoffs; check the USB-C, charge LED window and IR LEDs line up with their openings.
2. Keys through the top shell: every key clicks, none stuck; the A/B slider moves the switch both ways.
3. Repeat stage 4's `btn` check closed up.

## Log

| Stage | Result | Notes (readings, surprises) |
|---|---|---|
| 1 Unpowered | | |
| 2 First power-up | | VBUS / VSYS / 3V3 / idle mA: |
| 3 Serial + flash | | COM port: |
| 4 Inputs + indicator | | A/B position that reads high: |
| 5 IR | | |
| 6 nRF24 | | |
| 7 Battery | | run mA / sleep µA: |
| 8 In the case | | |
