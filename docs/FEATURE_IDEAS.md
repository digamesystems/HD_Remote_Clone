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
- **Editing presets:** stored in flash and edited over USB serial, so changing one needs no rebuild. A phone page with sliders is proposed below ([Web interface](#web-interface-and-an-always-on-bridge)).
- **Testable before the boards arrive:** the key-to-action logic can live in `lib/hdcore/` with host tests.

**Alternative considered (Claude):** keep the slider stock and run a key's preset on a long press (about 1 s). It costs no switch position, but it's less discoverable, and a slow tap could fire it by accident. The switch, with the setting above, is the recommended option.

**Open before building:**

- ~~Which slider position is A on the handheld PCB~~ Answered at bring-up (2026-11-25, John): A is GPIO 14 low, the knob toward the BOOT edge, matching the original remote's orientation.
- ~~Does the stock slider change any RF frame?~~ Yes: on B the group action bytes are `A3`/`23` (UP) and `A4`/`24` (DOWN), and channel-A shades obeyed them (sniffed 2026-11-25; [RF_PROTOCOL.md](RF_PROTOCOL.md#the-ab-slider-in-group-mode)).

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
- **Firmware:** the rotation reverses which slider position reads as A, so swap them in the firmware or its settings. On rev 1, A is GPIO 14 low (confirmed at bring-up, 2026-11-25).

## Web interface and an always-on bridge

Raised by John (2026-11-24): a web interface for the device, used both to control the shades from a phone and to configure presets ("move the sliders until the shades are in the right place, then save the configuration to a preset"). Then: an always-on bridge could be just an ESP32 DevKit, an nRF24 module and USB power, since the breadboard already reaches shades across the house, and a bridge without an external antenna makes a cleaner installation. Then: much of this can be tested on one of the five rev 1 boards with a separate firmware build that ignores most of the handheld's hardware.

### Where a web page can run

- **Handheld (proposed, Claude): a setup page on demand only.** The handheld's sleep budget is about 15 µA ([HARDWARE.md](HARDWARE.md)), and Wi-Fi that stays reachable draws roughly 100 mA, so an always-on page would empty the cell in about a day. Instead, a key combination starts a Wi-Fi access point with a setup page that turns itself off after a few minutes idle. Uses: editing the A/B presets, group names, pairing management (behind a confirmation; never Master Reset), the user-trial key log ([BRINGUP.md](BRINGUP.md), first user trial), battery voltage and firmware version, and firmware updates over Wi-Fi (the default partition table already has a second app slot for this).
- **Bridge (proposed, Claude): everyday phone control.** A wall-powered device that uses RF only: group commands, absolute position and pairing all work from a paired ID with no line of sight ([RF_PROTOCOL.md](RF_PROTOCOL.md)). It would offer a page with a button set and a position slider per group, schedules and scenes, and later a REST or MQTT interface for Home Assistant or an ESPHome component.
- **One web module for both** (proposed): the page and the preset storage live in `lib/`, so the handheld's setup mode and the bridge share them.

### Presets set with sliders

How it would work (proposed, Claude):

- **The shades only receive; they never report position, so the slider is the commanded position.** That's enough because position frames are absolute and a shade already at its target doesn't move (RF_PROTOCOL, 2026-11-18). Saving a preset stores the slider values, and replaying it works from wherever the shades are.
- **Send on slider release, not while dragging.** Each `pos` command takes about 1.8 s on air.
- **Save to a key:** e.g. "save to B → G2", stored in NVS. The stored format is the same short list of `pos <mask> <position>` steps as in the A/B preset idea above, so the phone page replaces serial editing without changing anything else.
- **Label positions as open and closed** in each shade's terms, set once, not as raw 0–255. Our arrows are reversed and 0 is fully down.
- **Granularity is per group, not per shade.** Position frames address a group mask, so all shades in a group go to the same place. Shades that need different heights need different groups, and the handheld has four plus ALL.

### Bridge hardware

- **Minimum (John's suggestion):** ESP32 DevKit + nRF24 module + USB power. The breadboard's wiring and pin map already do this; add 10 µF (or more) at the module's VCC and GND.
- **Prefer no external antenna** (John): a plain module in a plastic box, if the margin test below passes. Notes (Claude): mount it away from metal (TV, metal shelf, the router); put the ESP32's and the nRF24's antennas at opposite ends of the box, ideally at right angles, with no copper under either one. Separation gives most of what cross-polarization would; indoors, reflections limit the polarization gain to a few dB.
- **If the margin falls short (proposed, Claude):** stay within the transmit limit. A home-built device needs no FCC authorization (47 CFR §15.23: five or fewer, personal use, not sold), but it must still meet the technical standards, which for this signal (§15.249) work out to roughly −1 dBm EIRP; the original remote runs at about −6 dBm. That rules out PA+LNA modules such as the EBYTE E01-2G4M27D (27 dBm): the amplifier adds its gain whatever the PA setting. Options instead, in order: move the bridge (central, higher, away from metal); a plain module with a modest external antenna, with the PA setting lowered to allow for the antenna's gain; or a second bridge nearer the far shades.
- **Its own ID (Claude):** pair the bridge under its own ID alongside the handheld's, so either can be removed without touching the other. Pairing capacity per shade is unknown, so pair only the groups it needs.
- **Listening (Claude, idea):** while idle the nRF24 can sniff the remotes' frames, as `sniff` does, so the bridge can track what the handhelds do. Absolute positions and full open/close give exact state; arrow presses only give an approximate one. Whether sniffing and sending share one module cleanly is untested.
- **A later dedicated board** (Claude, only if a bridge proves worthwhile): ESP32 module + nRF24 with both antennas at board edges, USB-C, nothing else. Much simpler than the handheld.

### Wi-Fi and the shade channels

- **Overlap:** the shades use ch 33/52/71 = 2433/2452/2471 MHz. Wi-Fi channel 6 covers 2433; channel 11 covers 2452 and 2471. Putting the router on Wi-Fi channel 1 avoids all three.
- **The bridge's own Wi-Fi is quiet when idle (Claude).** As a client with the Arduino default modem sleep, the ESP32 wakes for the router's DTIM beacons (every 100–300 ms typically) and transmits only occasionally (acks, power-save frames, ARP/mDNS, keepalives). Receiving doesn't disturb the nRF24; only its transmissions can. The router and other devices on the channel are the bigger source.
- **The handheld's setup page is noisier:** an access point beacons about every 102 ms whether or not anyone is connected. That's one more reason for its timeout.
- **Shade commands repeat each frame many times, so they tolerate the odd collision. Sniffing is more fragile.**
- **Measuring channel activity:** the firmware's existing `scan` command (a carrier-detect sweep over channels 0–125 using the nRF24's received power detector, above about −64 dBm; [FIRMWARE.md](FIRMWARE.md)) gives a rough 2.4 GHz activity view, e.g. with the bridge's Wi-Fi idle and busy. Si24R1 clones may not implement the detector the same way; check first.

### Testing on a rev 1 board (John's suggestion)

A rev 1 board on USB already has the bridge's parts: WROOM-32E, the nRF24 socket, USB-C, and the LED and keys for status and local control. Proposed (Claude):

- **A `bridge` build environment** for the handheld board (`-DHD_BOARD_HANDHELD` plus a bridge-role flag), using `pins.h` as is and skipping IR, deep sleep and key handling. It runs Wi-Fi as a client, the web page, and `grp` / `pos` / `pair`, and keeps the serial console. LED: green when Wi-Fi is connected, a red flash per RF send.
- **One test answers two questions.** The board's antenna geometry is the handheld's, so the Wi-Fi coexistence margin measured as a bridge also applies to the handheld's setup page.
- **Battery loss for free.** With no cell fitted, the foil pouch isn't under the nRF24 antenna (HARDWARE.md, nRF24 socket: about 12.5 mm below it). Comparing with and without a cell on the bench measures that loss.
- **The handheld case doubles as a tidy bridge box.**
- **Order:** the board must pass bring-up stages 1–6 first (stage 7, the battery, is optional on USB). One board goes to the handheld firmware and user trial, another to this.
- **Flash:** about 31 % of the 1.25 MB app slot is used now (fw 0.9.13, 2026-11-24). Wi-Fi and a web server add several hundred KB (estimate, not measured), which still fits.

### Open before building

1. **What position means on a top-down shade: answered.** Our shades each have one moving rail (John, 2026-11-25), and the position is the rail's height on both types: 0 at the bottom, 255 at the top. Bottom-up from `pos 08 0/255` (2026-11-18); top-down from hd-shade-bridge on the handheld, which set groups 1 and 3 where its page draws them (John, 2026-11-26; [RF_PROTOCOL.md](RF_PROTOCOL.md#absolute-position-experimental)).
2. **A new target while a shade is moving** (also listed as untested in RF_PROTOCOL). A slider dragged twice does exactly this. Testable now.
3. **Range from the bridge's location:** send `grp` to the farthest shades 10 times at each nRF24 power level, from MIN upward, and note the lowest level that gets 10/10. Run the bridge a few dB above that and within the transmit limit (see Bridge hardware). A clone's power steps may differ from the datasheet's 6 dB. The firmware's `pa` command sets the level ([FIRMWARE.md](FIRMWARE.md)).
4. **Wi-Fi coexistence:** repeat 3 with Wi-Fi idle and busy (streaming to the device), and with the router on different channels.
5. **Sniffing while also sending**, if the bridge is to track the handhelds.
6. **Safety:** any network path that moves shades keeps the 5 s send cap in firmware, needs a password, and puts destructive pairing actions behind a confirmation. Wi-Fi credentials go in NVS through the setup page, never in the repo.
7. **Compare with spitcool's bridge work** (their public repo only; ask John before posting anything to it).
