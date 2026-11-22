# Research notes: open questions and plan

What we still don't know about the Platinum system, and how the reverse-engineering went. Protocol details are in [IR_PROTOCOL.md](IR_PROTOCOL.md) and [RF_PROTOCOL.md](RF_PROTOCOL.md); this page is the status view. Dates are 2026.

## Open questions

- **RF wake-up for IR?** Several secondhand sources say battery-powered Platinum shades ignore IR unless the remote also sends a short RF burst to wake them, which would explain why learning remotes fail. *Yes, by two tests* (2026-11-09): sealed in a foil tube that blocks its RF but lets its IR out, the original remote can't move the shade in IR mode, though it does when bare. And our clone's IR moves the shade only when the original remote is pressed at the same time (aimed away); alone, 0 of 20. See [Shade tests](IR_PROTOCOL.md#shade-tests). The RF it sends is `ID 01 00 80` (sniffed 2026-11-11; spitcool's notes had recorded this frame without knowing its trigger).
- **Can the clone send the wake itself?** *Yes* (2026-11-11): the clone's own `ID 01 00 80` followed by its IR moves the shade with no original remote, including stops. **The ID matters only for group commands** (2026-11-12): a made-up ID gets no group response (0 of 12), but its wake + IR still moves the shade. So point-and-press needs no paired ID; group control needs a paired ID. Next: more IR range (aim, LED power, beam width).
- **Can the clone pair itself, with its own ID?** *Yes* (2026-11-12). Setup-mode group assignment sends `ID 30 <group> 80` (add) or `ID 31 <group> 80` (remove) with the normal OPEN / CLOSE IR aimed at the shade. The clone's `pair 4 add` with a made-up ID made the shade jog, and group commands with that ID then worked 10 of 10. So no original remote is needed at all. See [Pairing](RF_PROTOCOL.md#pairing-group-assignment).
- **IR behavior on hold:** *answered* (2026-11-07). One press is a burst of identical frames, 17.19 ms apart, for as long as the key is held: no toggle bit, no counter, no shade address. See [IR_PROTOCOL.md](IR_PROTOCOL.md).
- **Does the shade sleep?** *Yes* (2026-11-15). After a few seconds idle, a short press often fails, and the first press of a session needs the RF wake to keep coming for about 1–2 s. Once the shade is awake, a tap of the opposite arrow stops it. The handheld will therefore send wake + IR steps for as long as an arrow is held. See [IR_PROTOCOL.md](IR_PROTOCOL.md).
- **Absolute positioning?** For presets ("Sunset Mode") without knowing where a shade starts. spitcool has decoded the Platinum bridge's position frames (`ID 50|32 <mask> <pos>`), but replaying them hasn't been shown to move a shade. *Yes* (2026-11-18): the clone's `pos` (fw 0.9.7) sends them under the working remote's handheld-paired ID, and the group-4 shades go to the target: 0 fully down, 255 fully up, 64 / 128 / 192 roughly a quarter / half / three quarters, the same point from either end, and no movement when already there. The mask is the handheld group byte, the bits combine, and a made-up ID gets no response. No bridge is needed. See [Absolute position](RF_PROTOCOL.md#absolute-position-experimental).
- **Still open:** why the weaker original remote sends a `00` tail after its wake, the shade's receptive window after an RF wake, and how long holding the opposite arrow takes to reverse a moving shade.

## Reverse-engineering plan

1. ✅ **Capture IR.** A TSOP38238 receiver records raw mark and space timings for every button in both modes, short press and held, several times each, to find any changing bits.
2. ✅ **Replay IR.** Send the raw timings back at 40 kHz through a TSAL6200 940 nm LED and confirm a shade responds.
3. **Screen the nRF24 modules.** The batch is probably clones, with a reported ~50% SPI failure rate, so every module gets tested. 2 Mbps is the rate that matters.
4. ~~**Prove the sniffer.** A second module (radio B) transmits known packets at the Platinum settings while the first receives (`beacon`). Without this, an empty sniff means nothing.~~ Superseded: the sniffer caught the original remote directly (step 5), and the clone's group commands moved the shades, so radio B was never built and was dropped from the project (2026-11-18).
5. ✅ **Sniff a group command** (positive control): group 1 + UP showed `ID 20 01 A1`, then the release tail. This also gave us our remote IDs.
6. ✅ **Sniff an IR-mode press**, the RF wake-up experiment: `ID 01 00 80` while the arrow is held.
7. **Fallbacks:** promiscuous sniffing (`promisc`) if the published settings catch nothing, or tapping the remote's own nRF24 SPI bus. Not needed so far.
8. **Emulate.** Transmit with the original's timing: the wake before each IR command (✅ works), then group commands with held repeats and release packets (✅ works).
9. **Build the handheld** (in progress: PCB and enclosure designed, see [docs/BRINGUP.md](BRINGUP.md)): buttons with the remote's behaviour (group keys open a green window for the arrows), a red/green indicator LED, deep sleep, battery and enclosure.

## The original remotes

Two original remotes, both from about 2012 and dropped many times. One works. The other is degraded: IR works only within about 30 cm and RF within about 1 m. Both paths are weak, which points at power (batteries, contacts, regulator, or a drop-cracked joint); not verified, and new batteries and cleaned contacts haven't been tried. Each has its own RF ID, and both move the shades. The degraded one sends a `00` tail after its wake; the working one doesn't. Design implication: the clone should survive drops.

**Never Master Reset either remote** (manual p. 7): it may erase pairing.

## Manual notes (paraphrased)

Read in full 2026-11-11. Page numbers refer to the Platinum remote guide linked in the README.

- **Pairing** (p. 5): SETUP, the LED flashes green for about 30 s, press a group key, aim at the shade's eye from 2–3 inches, OPEN adds the shade to the group and CLOSE removes it; the shade jogs; SETUP again saves. A shade can be in several groups. No stated limit on how many remotes a shade remembers. (Sniffed and reproduced by the clone: see [Pairing](RF_PROTOCOL.md#pairing-group-assignment).)
- **Duplicate remote** (p. 7): Master Reset (about 10 s, flashing red), then OPEN on the original within 18 inches. The backup copies the original's identity. Not tried on purpose.
- **Intermediate stop** (p. 8): hold ALL 6 s, aim, OPEN, then ALL to save; one per shade. Holding ALL never made our remote flash green (John, 2026-11-12), so it apparently lacks the feature. To confirm: `sniff` while holding ALL about 8 s.
- **Express mode** (p. 9, PowerRise only): in green mode, holding OPEN or CLOSE 2 s opens or closes fully. Not tried: `grp 4 up 2500`.
- **Limit reset** (p. 21): holding OPEN 6 s at a fully open shade resets its stops and limits. This is why the firmware caps every press at 5 s in both directions (decided 2026-11-18, John).
- **IR channel** (pp. 1, 18, 22): the A/B slider is the remote's IR channel selector; a shade's channel is changed with its own button plus OPEN. All our shades are on A.
- **LED meanings:** red = IR press; steady green = radio press during the group window (the manual says the window is 5 s; we measured about 6.5 s); flashing green for about 30 s = setup mode; flashing red = backup remote waiting after a Master Reset; no light = dead batteries.

## Planned shade tests

Test plan agreed with John (items 1–4, 2026-11-11; item 5 from the 2026-11-12 review). Not run yet except where noted.

1. **Fix the aim:** prop, tape or clamp the IR LED; then move it off-axis or back until it misses about half the time, since differences only show at the margin.
2. **`seq off` vs `seq on`:** at least 10 presses each (20 if hand-held, alternating in blocks of 5), logging `# y` / `# n` per press.
3. **Gap sweep** (needs the fixed mount): `seq gap` 0, 100, 200, 400, 800 ms, about 10 presses each. A sharp drop past some gap gives the shade's listening window after RF. The first attempt (2026-11-15) was spoiled by the tape.
4. **`seq rf 20`:** shorter RF per channel; watch for each RF + IR pair acting as a separate command.
5. **Made-up-ID wake with counts:** wake + IR under `12 34` and `C1 0E` vs the working ID, at least 10 presses each, fixed mount. The "any ID works for the wake" result so far is hand-aimed without counts.
6. Also open: how long until the shade sleeps, whether rotating channels within each step shortens the wake, the real remote's hold-to-reverse time, and whether an opposite group command stops a moving shade.

## Outreach

- **spitcool/hd-platinum-rf** (protocol research, no TX): John opened [issue #1](https://github.com/spitcool/hd-platinum-rf/issues/1) as a sister project. Comments posted with John's approval, 2026-11-18: [answers to their open questions](https://github.com/spitcool/hd-platinum-rf/issues/1#issuecomment-6989642349) (links pinned to `db5d13d`), and [bridge position frames from our clone](https://github.com/spitcool/hd-platinum-rf/issues/1#issuecomment-6995420597) (pinned to `8f2dad8`). Their 2026-11-16 commit cites our `c36fbbb`.
- Later options (not decided): upstream the IR protocol to IRremoteESP8266, write-ups on the Arduino and Home Assistant threads, an ESPHome component.
