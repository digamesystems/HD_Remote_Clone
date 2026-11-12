# Hardware

KiCad project for the handheld replacement remote: [`hd_remote/hd_remote.kicad_pro`](hd_remote/hd_remote.kicad_pro).

- **Tool:** KiCad 10 (the files open in 9.0 or newer).
- **Spec:** [docs/HARDWARE.md](../docs/HARDWARE.md) holds the design decisions, pin map, power budget and open questions. Change the spec first, then the schematic.
- **Breadboard wiring** (the DevKitC prototype this board replaces): [docs/WIRING.md](../docs/WIRING.md).

## Status

| Sheet | State |
|---|---|
| Power and USB (`power_usb.kicad_sch`) | draft, 2026-11-12. ERC clean apart from labels waiting for the MCU sheet. Values and *verify* items: [docs/HARDWARE.md](../docs/HARDWARE.md#schematic-power-and-usb-sheet-draft-2026-11-12) |
| MCU, IR, nRF24, buttons/LEDs | not started |
| Layout | not started; the board outline waits on the enclosure choice |

## Generator scripts

`tools/` holds the Python scripts that drew the first drafts (`gen_power_usb.py`, using `schgen.py` and `sexpr.py`). They pull symbols from the installed KiCad 10 libraries and connect every pin to a net label, so connectivity is by name. **Once a sheet has been edited in KiCad, the KiCad file is the master**: don't rerun its generator over it.

    python -I hardware/tools/gen_power_usb.py

The KiCad library path is set at the top of `tools/sexpr.py`, or with the `KICAD_SYMBOL_DIR` environment variable.

## Conventions

- Local KiCad state (`*.kicad_prl`), backups and caches are git-ignored. Commit the `.kicad_pro`, `.kicad_sch` and `.kicad_pcb` files, plus any project-specific symbol and footprint libraries.
- Parts should come with an LCSC number where possible, so JLC assembly stays an option.
