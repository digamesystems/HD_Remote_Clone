# Feature ideas

Ideas for the clone that go beyond what the stock Platinum remote does. **Nothing here is decided or built** unless an entry says so. Each idea notes who raised it, and recommendations are labelled as such. Dates are 2026.

## A/B slider as a preset switch

**Idea (John, 2026-11-19):** use the A/B slider to change what the group keys do. In **A**, the group keys (1–4, ALL) work as they do now. In **B**, they trigger preset blind positions instead. That adds five one-press presets without new buttons.

**Background.** On the stock remote, the slider is the Infrared Channel Selector (manual pp. 1, 18, 22): it switches the IR codes between channel A (`0101` / `0110`) and channel B (`1001` / `1010`), so that two shades close together can be told apart by point-and-press ([IR_PROTOCOL.md](IR_PROTOCOL.md)). Whether it changes any RF frame hasn't been checked. Presets are possible because the shades accept absolute-position frames from a handheld-paired ID ([RF_PROTOCOL.md](RF_PROTOCOL.md#absolute-position-experimental), 2026-11-18). The bring-up firmware already has three as serial commands (`bedtime`, `waketime`, `sunset`; see [FIRMWARE.md](FIRMWARE.md)).

**The trade-off.** As a mode switch, the slider can no longer select IR channel B. That costs nothing in a house where every shade is on channel A, but it would be a regression for anyone whose shades use channel B.

**Proposed (Claude), for discussion:**

- **Make the slider's role a setting**, stored in flash and changed over serial:
  - *stock*: A/B selects the IR channel, as on the original remote;
  - *preset*: A = normal, B = presets, with point-and-press always on channel A.
- **Group and ALL keys in B:** one press runs that key's preset (five presets). A preset is a short list of `pos <mask> <position>` steps.
- **Arrows in B:** unchanged point-and-press (channel A), so the remote never loses its basic function.
- **SETUP (pairing):** only in A, so the group keys have one meaning during pairing.
- **Indicator LED:** a distinct pattern while a preset is sending (for example blinking green). Each step takes about 1.8 s on air, and the pattern also shows which mode the remote is in.
- **Repeating is safe.** Positions are absolute, and a shade already at its target doesn't move. The remote gets no confirmation from the shades, so "press again if one didn't move" is a reasonable answer.
- **Editing presets:** stored in flash and edited over USB serial, so changing one needs no rebuild.
- **Testable before the boards arrive:** the key-to-action logic can live in `lib/hdcore/` with host tests.

**Alternative considered (Claude):** keep the slider stock and run a key's preset on a long press (about 1 s). It costs no switch position, but it's less discoverable, and a slow tap could fire it by accident. The switch, with the setting above, is the recommended option.

**Open before building:**

- **Which slider position is A** on the handheld PCB (GPIO 14), to be confirmed at bring-up.
- **Does the stock slider change any RF frame?** Sniff with the switch on B.

## A swoopier case for a later revision

Raised by John (2026-11-21), after the first renders: the current case is a good first cut, but a later release could have a more sculpted, "swoopier" shape. Not for prototype 1.

- **Starting points:** the test print's finish and fit results carry over (enclosure README, Test piece), as do `assembly.py`'s fit checks, which would catch a shape that no longer clears the board.
- **Constraints a new shape has to keep:** the IR LEDs and USB-C on flat stretches of the end walls, the 2 mm minimum walls for MJF, the board's outline and screw holes (or a new board), and the split at the IR LED axis.
- **Tools:** a Blender render script (on trial, not in the repo yet) makes it quick to compare shapes and colours before printing.

## A/B switch turned around on a later board

Raised by John (2026-11-21), while moving the A/B knob over its drive block: on rev 1 the knob ends up about 2 mm from the arrow oval, because SW509's actuator points toward the back (the arrows) and the knob sits over the actuator. The rev 1 case shortens the arrow oval 0.3 mm (decided 2026-11-21, John) to keep the 2 mm minimum between the two openings. Not for rev 1, whose boards are made.

- **Rotate SW509 180°** so its actuator points toward the front (the indicator). The drive block and knob then sit in front of the switch, moving away from the arrows by roughly 5–6 mm (estimate). Reroute AB_SW to match.
- **Move R509 and R510** to give the drive block and its support pad more room around the switch.
- **Case:** move the slot, knob and support pad to the new actuator position, and give the arrow oval back its full length.
- **Firmware:** the rotation reverses which slider position reads as A, so swap them in the firmware or its settings. Which position is A is already to be confirmed at rev 1 bring-up.
