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
| Layout | not started; the board outline waits on the enclosure choice |

ERC (2026-11-12): **0 errors, 0 warnings** on the whole design. References are numbered by sheet: 1xx Power and USB, 2xx MCU, 3xx IR, 4xx nRF24, 5xx Buttons and LEDs, 6xx Test and debug.

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
