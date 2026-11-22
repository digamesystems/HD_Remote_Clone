# Board design review checklist

A reusable checklist to run before ordering any board revision. Rev 1's own pass is recorded in [BRINGUP.md, Before ordering](BRINGUP.md#before-ordering); this file turns it, and what rev 1 taught us, into a list for rev 2 onward. Proposed 2026-11-22 (Claude, from the rev 1 record); edit freely.

**How to use:** copy the list into the revision's bring-up doc, tick items with a date and who checked, and link the evidence. A box ticked from a model, a library footprint or a guessed dimension is not ticked: say what it rests on.

## Lessons from rev 1 (why some items are here)

- **J401 was mirrored** (found 2026-11-16 by John with a meter on a real module). The schematic pin numbers were right; KiCad's `PinSocket_2x04` puts pad 2 left of pad 1, for a board flipped face-down, but the module sits face-up, so VCC and GND swapped. Two AI reviews didn't catch it. → *Mating parts* below.
- **Pads boxed in by a keep-out** (2026-11-14): the antenna keep-out ended at the ESP32's pad ends, leaving pins 25–31 and 34 unreachable. Crossing counts didn't show it. → *Layout*.
- **A clearance pass on guessed numbers** (2026-11-16): the case check passed the nRF24 with 0.7 mm to spare using placeholder module dimensions; calipers showed the crystal 1.9 mm into the key face. → *Fit*.
- **JLC rotations** (2026-11-17): every SOT-23 +180°, SOT-23-5/-6 −90°, the ESP32 −90° plus a 3.81 mm shift. Now in `CPL_FIXES` in `make_jlc.py`, but a new package needs its own check. → *Fab outputs*.
- **No revision mark on the silkscreen** (noticed 2026-11-22): rev 1 boards carry no rev text. → *Silkscreen*.

## Checklist

### Schematic
- [ ] Every small part's pinout checked against the manufacturer's drawing (not the KiCad symbol) by someone who hasn't seen the design: transistors, MOSFETs, regulators, diodes (direction), ESD arrays, the USB bridge.
- [ ] `python -I hardware/tools/check_pins.py` passes (`include/pins.h` matches the schematic).
- [ ] ERC clean, or each remaining warning explained.
- [ ] Power path reviewed end to end: USB in, charger, battery, regulator, reverse-polarity and ESD parts; current ratings against the worst case (IR LED pulses, radio TX).
- [ ] Every strapping pin's state at boot is what the ESP32 needs (GPIO 0, 2, 5, 12, 15).

### Mating parts (sockets, connectors, LEDs, switches)
- [ ] For each part that mates with something physical: pad → (x, y, net) table, seen from the board's top, compared with a photo or drawing of the **real** mating part from the same side. Meter the real part where possible (pin 1, VCC, GND).
- [ ] Polarized through-hole parts: square pad = which lead, checked against the datasheet (LED cathode, battery +).
- [ ] Any "check before routing" item is treated as blocking, not as a note.

### Layout
- [ ] DRC: 0 errors and 0 unconnected, or each remaining item explained (board-edge overhangs by design).
- [ ] Every connected pad has a legal way out after the last placement or keep-out change (not just crossing counts).
- [ ] Antenna keep-outs (ESP32, nRF24) clear on both layers.
- [ ] `python -I hardware/tools/check_fp_overrides.py` passes.

### Fit (board in the case)
- [ ] `hardware/enclosure/assembly.py` passes against the board's STEP export, and its report lists no guessed dimensions for parts on hand (measure them first).
- [ ] Keys, LEDs, light pipe, USB-C and A/B slider line up with their case openings.

### Silkscreen
- [ ] Board name and **revision** printed (e.g. `HD Remote rev 2`), plus the date or commit.
- [ ] Pin 1 / polarity marks visible after assembly; reference designators not under parts or on pads.

### Fab outputs and order
- [ ] `python -I hardware/tools/make_jlc.py` regenerates Gerbers, BOM and CPL; they are committed.
- [ ] JLC dry run: every BOM line matches its LCSC number and is in stock; every polarized part lines up with its pin-1 mark on the placement page; a new package's rotation is added to `CPL_FIXES`.
- [ ] Ask John before ordering (cost and schedule are his decisions). After the order: tag the commit (`hw-revN`), log it in the bring-up doc, add its costs to `docs/COSTS.md`.

### Independent review
- [ ] At least one check by a human who didn't design it, or a physical check against real parts: AI reviewers share blind spots with the AI that helped design it.
