# Research notes: open questions and plan

What we still don't know about the Platinum system, and how the reverse-engineering went. Protocol details are in [IR_PROTOCOL.md](IR_PROTOCOL.md) and [RF_PROTOCOL.md](RF_PROTOCOL.md); this page is the status view. Dates are 2026.

## Open questions

- **RF wake-up for IR?** Several secondhand sources say battery-powered Platinum shades ignore IR unless the remote also sends a short RF burst to wake them, which would explain why learning remotes fail. *Yes, by two tests* (2026-11-09): sealed in a foil tube that blocks its RF but lets its IR out, the original remote can't move the shade in IR mode, though it does when bare. And our clone's IR moves the shade only when the original remote is pressed at the same time (aimed away); alone, 0 of 20. See [Shade tests](IR_PROTOCOL.md#shade-tests). The RF it sends is `ID 01 00 80` (sniffed 2026-11-11; spitcool's notes had recorded this frame without knowing its trigger).
- **Can the clone send the wake itself?** *Yes* (2026-11-11): the clone's own `ID 01 00 80` followed by its IR moves the shade with no original remote, including stops. **The ID matters only for group commands** (2026-11-12): a made-up ID gets no group response (0 of 12), but its wake + IR still moves the shade. So point-and-press needs no paired ID; group control needs a paired ID. Next: more IR range (aim, LED power, beam width).
- **Can the clone pair itself, with its own ID?** *Yes* (2026-11-12). Setup-mode group assignment sends `ID 30 <group> 80` (add) or `ID 31 <group> 80` (remove) with the normal OPEN / CLOSE IR aimed at the shade. The clone's `pair 4 add` with a made-up ID made the shade jog, and group commands with that ID then worked 10 of 10. So no original remote is needed at all. See [Pairing](RF_PROTOCOL.md#pairing-group-assignment).
- **IR behavior on hold:** *answered* (2026-11-07). One press is a burst of identical frames, 17.19 ms apart, for as long as the key is held: no toggle bit, no counter, no shade address. See [IR_PROTOCOL.md](IR_PROTOCOL.md).
- **Does the shade sleep?** *Yes* (2026-11-15). After a few seconds idle, a short press often fails, and the first press of a session needs the RF wake to keep coming for about 1–2 s. Once the shade is awake, a tap of the opposite arrow stops it. The handheld will therefore send wake + IR steps for as long as an arrow is held. See [IR_PROTOCOL.md](IR_PROTOCOL.md).
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
