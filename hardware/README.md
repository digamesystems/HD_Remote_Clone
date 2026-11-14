# Hardware

KiCad project for the handheld replacement remote: [`hd_remote/hd_remote.kicad_pro`](hd_remote/hd_remote.kicad_pro).

- **Tool:** KiCad 10 (the files open in 9.0 or newer).
- **Spec:** [docs/HARDWARE.md](../docs/HARDWARE.md) holds the design decisions, pin map, power budget and open questions. Change the spec first, then the schematic.
- **Breadboard wiring** (the DevKitC prototype this board replaces): [docs/WIRING.md](../docs/WIRING.md).

## Status

| Sheet | State |
|---|---|
| Power and USB (`power_usb.kicad_sch`) | draft, 2026-11-12. Values and *verify* items: [docs/HARDWARE.md](../docs/HARDWARE.md#schematic-power-and-usb-sheet-draft-2026-11-12) |
| MCU (`mcu.kicad_sch`) | draft, 2026-11-12: [docs/HARDWARE.md](../docs/HARDWARE.md#schematic-mcu-sheet-draft-2026-11-12) |
| IR (`ir.kicad_sch`) | draft, 2026-11-12: [docs/HARDWARE.md](../docs/HARDWARE.md#schematic-ir-sheet-draft-2026-11-12) |
| nRF24 (`nrf24.kicad_sch`), Buttons and LEDs (`ui.kicad_sch`) | draft, 2026-11-12: [docs/HARDWARE.md](../docs/HARDWARE.md#schematic-nrf24-and-buttonsleds-sheets-draft-2026-11-12) |
| Test and debug (`test.kicad_sch`) | draft, 2026-11-12: [docs/HARDWARE.md](../docs/HARDWARE.md#test-points-2026-11-12). Test points on nets local to other sheets live on those sheets |
| Layout | started 2026-11-13: all 109 footprints and 82 nets pulled in from the schematic (DRC schematic parity: 0 issues), parked below the sheet. Board outline (145.45 × 40.45 mm) from `docs/images/enclosure/enclosure_pcb_outline.dxf`, board centre at (100, 80) mm on the sheet, front end (IR LEDs) at +X, top view not mirrored; M2 holes H601–H604 placed and locked at (±61, ±15.5) mm. Design rules: KiCad defaults, inside JLC's standard process (0.2 mm track/clearance, 0.3 mm min hole). **U201 is modified locally:** its 12 ground-pad vias are 0.3 mm drill / 0.6 mm pad (the library has 0.2 mm, below JLC's standard drill), so DRC reports one expected `lib_footprint_mismatch`; don't update U201 from the library. **Case-fixed parts placed and locked** (2026-11-13) from `hardware/enclosure` coordinates: 7 key switches, SETUP (underside, 22 mm from the front, 10 mm right of centre), IR LEDs D303/D304, indicator D501, A/B SW509, USB-C J101, battery JST J102, ESP32 U201, nRF24 socket J401. Keep-outs: battery (named rule area `Battery` + custom rule in `hd_remote.kicad_dru`: no THT component pins over the cell; plated vias allowed), bottom side under each key's support pad, screw heads/standoffs, and the nRF24 module's PCB antenna. Expected DRC: silkscreen clipped at the board edge where the LEDs, USB-C and JST overhang it. **Rest of the parts: first-pass placement** (2026-11-13, scripted, unlocked): each part at the nearest free spot to the pads it connects to, ICs first; no courtyard or keep-out errors. It proves the fit, not a final layout: move decoupling caps to their pins (C201 especially), tighten the USB pair, tidy the silkscreen. **Check before routing:** J401's pin-1 orientation against a real module. **First-draft placement complete** (2026-11-13, by hand): every part on the board, DRC errors 0. **Routing setup** (2026-11-13): net classes Power 0.5 mm (+3V3, VBUS, NRF_VCC) and HighCurrent 0.6 mm (VBAT, VBAT_BATT, VBAT_JP, IR_K1/K2, IR_E1/E2; 0.6 matches the SOT-23 pads and carries well over the ~300 mA peak), Default 0.2 mm; GND pour on B.Cu over the whole board (0.3 mm clearance, thermal reliefs), kept out of both antenna keep-outs and the screw heads. **ESP32 rotated 180°** (2026-11-13): antenna end at the −Y edge, so its key/RTC pins face the keys and its SPI/UART pins face the back (see docs/HARDWARE.md, Layout notes); 21 parts re-homed (EN, battery-sense and 3V3 decoupling to the new pin positions; BOOT, J601, JP101/Q103, charger caps and the underside test pads out of the module's new footprint and antenna keep-out); case model updated, 51/51 checks pass. **Power path added** (2026-11-13, Microchip AN1149): D102 STPS1H100AY, Q104 AO3401A, R114 100 kΩ, TP102; loads (LDO, IR LEDs) moved from VBAT to the new VSYS rail; schematic ERC 0, board synced (parity 0), new parts placed by the charger. Next: route, IR loops and USB pair first |

Synced with the enclosure (2026-11-13): mounting holes H601–H604 (M2), key switch MPNs (Same Sky TS02), low-profile nRF24 socket noted on J401.

ERC (2026-11-13): **0 errors, 0 warnings** on the whole design. References are numbered by sheet: 1xx Power and USB, 2xx MCU, 3xx IR, 4xx nRF24, 5xx Buttons and LEDs, 6xx Test and debug.

## Generator scripts

`tools/` holds the Python scripts that drew the first drafts (`gen_power_usb.py`, `gen_mcu.py`, `gen_ir.py`, `gen_nrf24.py`, `gen_ui.py`, `gen_test.py`, using `schgen.py` and `sexpr.py`). They pull symbols from the installed KiCad 10 libraries and connect every pin to a net label, so connectivity is by name. **Once a sheet has been edited in KiCad, the KiCad file is the master**: don't rerun its generator over it.

    python -I hardware/tools/gen_power_usb.py
    python -I hardware/tools/gen_mcu.py
    python -I hardware/tools/gen_ir.py
    python -I hardware/tools/gen_nrf24.py
    python -I hardware/tools/gen_ui.py
    python -I hardware/tools/gen_test.py

The KiCad library path is set at the top of `tools/sexpr.py`, or with the `KICAD_SYMBOL_DIR` environment variable.

## Conventions

- Local KiCad state (`*.kicad_prl`), backups and caches are git-ignored. Commit the `.kicad_pro`, `.kicad_sch` and `.kicad_pcb` files, plus any project-specific symbol and footprint libraries.
- Parts should come with an LCSC number where possible, so JLC assembly stays an option.
- **License:** the KiCad design is CERN-OHL-S-2.0 ([LICENSE](LICENSE)); the generator scripts in `tools/` are GPL-2.0-or-later, like the firmware.
