# HD Platinum RF protocol (2.4 GHz, nRF24)

> **TL;DR** (summary checked 2026-11-19): the remote's radio is an nRF24 at 2 Mbps on channels 33/52/71 in rotation, with address `E7E7E7`, CRC-16 and auto-ack off. Its payload is a static 5 bytes, `ID0 ID1 MODE GROUP ACTION`, with no rolling code. The air settings are from spitcool's research, and our sniffs confirmed them on ch 33. A red-mode arrow sends the **wake** `ID 01 00 80` while held; the shade needs it alongside the IR and accepts it from any ID. **Group commands** (`ID 20 <group> A1/A2`, then a `21/22` release tail) work only from an ID paired to the shade. A clone can **pair its own ID** by sending `ID 30|31 <group> 80` together with the OPEN/CLOSE IR. **Absolute position** (spitcool's bridge frames `ID 50|32 <mask> <pos>`) also works from a handheld-paired ID: the mask is the group byte (bits combine), and the position runs 0–255 between the limits, so resending is harmless.

What the Platinum handheld remote sends over RF, from our sniffs of two remotes on 2026-11-11. For the IR side, and the shade tests that showed the shade needs RF alongside IR, see [IR_PROTOCOL.md](IR_PROTOCOL.md).

The air settings come from prior work by [spitcool/hd-platinum-rf](https://github.com/spitcool/hd-platinum-rf) (`docs/rf-protocol.md`, which cites FCC filings, a community register dump and SDR captures). Our sniffs confirm them on our own remotes. The red-mode **wake frames**, and what they mean, are our own findings.

## Status

| Item | State |
|---|---|
| Air settings: ch 33/52/71, 2 Mbps, address `E7E7E7`, CRC-16, 5-byte payload, auto-ack off | ✅ confirmed on both our remotes (received on ch 33) |
| Payload layout `ID0 ID1 MODE GROUP ACTION` | ✅ confirmed |
| Group command, e.g. group 1 + UP = `ID 20 01 A1`, then `ID 20 01 21` | ✅ seen on both remotes |
| A group key alone sends nothing | ✅ (working remote) |
| Red-mode arrow sends `ID 01 00 80` (the wake), no direction | ✅ both remotes, both arrows |
| Release tail `ID 01 00 00` after the wake | weak remote only; the working remote sends none |
| Nothing is sent when the green window expires | ✅ one observation (working remote) |
| A clone sending the wake + IR moves the shade | ✅ 2026-11-11, no original remote involved: reliable with the LED about 1 ft from the sensor eye; misses traced to IR aim. See [Shade tests](IR_PROTOCOL.md#2026-11-11-clone-only-rf-wake--ir) |
| Clone group commands move the shades (`grp`) | ✅ 2026-11-11: groups 1–4 independently and All, UP and DOWN, with the working remote's ID. RF only, no aiming. DOWN (`A2` / `22`) and groups 2–4 / All are confirmed by the shades' response; we haven't sniffed them from a remote |
| Shade checks the ID | **Group commands: yes.** With a made-up ID (`12 34`), `grp 4` got 0 responses in 12 trials; with the working remote's ID, 12 of 12 (2026-11-12). **Red-mode wake: no.** A wake with `12 34` or `C1 0E`, then IR, moved a group-4 shade, as did both real IDs (2026-11-12). The 2026-11-11 result, where `12 34` also failed for wake + IR, is put down to hand-aimed IR (trials not counted) |
| Group assignment (setup mode) frames | ✅ 2026-11-12, working remote: add (setup + group 4 + OPEN) = `ID 30 08 80`; remove (setup + group 4 + CLOSE) = `ID 31 08 80`, then a `00` tail. SETUP and the group key send nothing |
| **Clone pairs under its own ID** | ✅ **2026-11-12:** `pair 4 add` with a made-up ID (`C1 0E`) at a group-4 shade: the shade jogged, then `grp 4` with that ID moved it 5 of 5 up and 5 of 5 down. Before pairing it got no response. **2026-11-15:** `pair 4 remove` jogged the shade, and `grp 4` with that ID then got no response, while `grp 4` with the working remote's ID still worked. So removal drops only the clone's entry |
| Channels 52 and 71 | not sniffed yet (taken from spitcool's notes) |

## Air settings

| Parameter | Value |
|---|---|
| Channels | 33, 52, 71 (2433 / 2452 / 2471 MHz), one packet per channel in rotation |
| Data rate | 2 Mbps |
| Address | 3 bytes, `E7 E7 E7` (the nRF24 power-on default) |
| CRC | 16-bit |
| Payload | 5 bytes, static length |
| Auto-ack | off. No packet-control field: ShockBurst-compatible frames, 88 bits on air (spitcool). On an nRF24L01+ that means EN_AA = 0 and ARC = 0 |
| Packet pacing | one packet per ~185 µs channel slot. On a single channel we see one every 556 µs or more |

Our receiver was a clone module marked `NRF24L01+` (see [WIRING.md](WIRING.md)), set up with RF24 as: auto-ack off, dynamic payloads off, CRC-16, 5-byte payload, 3-byte address. It decoded the remote's frames without trouble.

## Payload

`ID0 ID1 MODE GROUP ACTION`

| Field | Values seen |
|---|---|
| `ID0 ID1` | The remote's ID. **Our two remotes have different IDs, and both move the shade**, so a shade accepts more than one remote. Treat an ID as private. It's redacted as `xx xx` / `yy yy` here |
| `MODE` | `20` group command; `01` red-mode (IR) press; `30` setup-mode add (group assignment, OPEN); `31` setup-mode remove (CLOSE); `50` then `32` absolute position (the Platinum bridge's frames; there GROUP is a group mask and ACTION a position, see [Absolute position](#absolute-position-experimental)) |
| `GROUP` | one-hot `01/02/04/08` for groups 1–4, `FF` All (from spitcool); `00` none. We sniffed `01`; the others work when the clone sends them |
| `ACTION` | `A1` UP held, `21` UP release tail (sniffed); `A2` DOWN held, `22` DOWN release tail (from spitcool, and they work when the clone sends them); `80` held and `00` release tail, for both the red-mode wake and setup-mode frames |

No counter or rolling code: every packet of a press is identical, and presses repeat the same payload.

In both pairs, bit 7 is set while the key is held and cleared in the tail (`A1` → `21`, `80` → `00`). That's a pattern, not a confirmed meaning.

### All known frames

Every frame type we know of, in one place. "Works" means a shade responded when our clone sent it.

| Frame | Meaning | Source | Status |
|---|---|---|---|
| `ID 01 00 80` while held; some remotes add a `00` tail | red-mode wake; the IR carries the command | our sniff | works from any ID (not checked) |
| `ID 20 <group> A1`, then tail `21` | group UP | our sniff | works; ID checked |
| `ID 20 <group> A2`, then tail `22` | group DOWN | spitcool | works; ID checked |
| `ID 30 <group> 80` + OPEN IR | setup: add the shade that sees the IR to the group | our sniff | works (the clone pairs its own ID) |
| `ID 31 <group> 80`, then tail `00` + CLOSE IR | setup: remove the shade from the group | our sniff | works |
| `ID 50 <mask> <pos>` ~1.49 s, then `ID 32 <mask> <pos>` ~0.30 s | go to an absolute position (0 = fully down, 255 = fully up on our shades) | spitcool (Platinum bridge) | works from a handheld-paired ID; ID checked |

`<group>` is one-hot (`01`, `02`, `04`, `08` = groups 1–4) or `FF` for All. A position `<mask>` uses the same bits, and they combine (`06` = groups 2 and 3).

Not known: whether the A/B slider changes any frame, the duplicate-remote copy (manual p. 7), intermediate stop and express mode, and anything else the bridge sends.

## What each key press sends

| Press | RF |
|---|---|
| Group key (1–4, All) alone | **nothing.** It only opens the green window on the remote |
| Arrow within the green window | `ID 20 <group> A1` (UP) while held, then `ID 20 <group> 21` for ~0.3 s |
| Arrow in red mode (IR) | `ID 01 00 80` while held, alongside the IR. No direction: the IR carries it |
| SETUP, and the group key after it | nothing |
| OPEN in setup mode, after a group key | `ID 30 <group> 80` while held, plus the **normal OPEN IR** (`0101` on channel A). Adds the shade that sees the IR to the group |
| CLOSE in setup mode, after a group key | `ID 31 <group> 80` while held, then `ID 31 <group> 00`, plus the **normal CLOSE IR** (`0110`). Removes the shade that sees the IR from the group |
| SETUP again (save and exit) | nothing |

### Red-mode wake timing

**Working remote:** `80` only, with no tail. A quick tap gave 439 ms of `80`, and a ~2 s hold gave 1583 ms.

**Weak remote:** `80` while held, then a `00` tail. In every clean press, the total was at least ~345 ms:

| `80` (held) | `00` (tail) | Total |
|---|---|---|
| 32 ms | 317 ms | 349 ms |
| 117 ms | 227 ms | 344 ms |
| 167 ms | 169 ms | 336 ms |
| 172 ms | 177 ms | 349 ms |
| 192 ms | 158 ms | 350 ms |
| 259 ms | 88 ms | 347 ms |
| 285 ms | 59 ms | 344 ms |
| 412 ms | 320 ms | 732 ms |
| 1594 ms | 319 ms | 1913 ms |

A rule that fits: `80` while the key is held, then `00`. A release before ~345 ms is padded out to ~345 ms with `00`; a longer hold gets a ~320 ms `00` tail. That's a hypothesis from 9 presses. Why the two remotes differ (firmware version? the weak remote's power problem?) is unknown.

## What the shade needs

From the shade tests in [IR_PROTOCOL.md](IR_PROTOCOL.md#shade-tests): the shade ignores IR on its own. It acts on IR only when a red-mode RF wake arrives within about a second before it, and one RF press arms one IR command. The wake frame carries no direction, which fits the clone test, where the shade followed the clone's IR, not the remote's arrow.

So a replacement should send `ID 01 00 80` for a few hundred ms, then the IR command, every time.

**The ID matters for group commands, apparently not for the wake** (2026-11-12):

| Test (group-4 shade) | Result |
|---|---|
| `grp 4 up` with a made-up ID (`12 34`) | 0 of 12 |
| `grp 4 up` / `down` with the working remote's ID | 12 of 12 |
| Wake + IR with `12 34`, `C1 0E`, the working ID or the weak remote's ID | moves the shade with each |
| IR alone (`wake off`, `seq off`), ID `12 34` | no response |

So the red-mode wake is required but seems to accept any ID, while group commands need an ID paired with that group. That fits the manual's description of point-and-press IR as the default mode, which needs no setup (p. 3): pairing (group assignment, p. 5) only matters for group control. The weak remote's ID also fails `grp 4` (not paired to group 4) yet wakes the same shade for IR.

On 2026-11-11, `12 34` also seemed to fail for wake + IR. We now put that down to hand-aimed IR, which was still hit-and-miss then and wasn't counted. A second made-up ID, `C1 0E`, also worked for wake + IR (2026-11-12).

For a replacement remote, point-and-press needs no paired ID at all. Group control needs a paired ID, and the clone can now **pair its own** (see [Pairing](#pairing-group-assignment)). Copying a paired remote's ID also works.

**Shown on 2026-11-11.** Our clone sent `ID 01 00 80` with the working remote's ID for 450 ms. It rotated ch 33/52/71, one packet about every 300 µs, so about 1,500 packets. No `00` tail. The IR followed immediately, and the shade obeyed: up, down and stop, with a new wake for each command. So the tail isn't needed, and sending RF before IR, rather than at the same time, works. Still untested: the shortest wake that works, and how the original times its RF against its IR.

The same day, interleaving RF and IR (RF on one channel, IR, the next channel, IR, and so on: `seq`) made aiming feel much less critical. That's a hand-held impression, with a controlled test planned. See [IR_PROTOCOL.md](IR_PROTOCOL.md#2026-11-11-clone-only-rf-wake--ir).

## Group commands from the clone

**2026-11-11.** The clone sent `ID 20 <group> A1` (UP) or `A2` (DOWN) for 300 ms, then the `21` / `22` release tail for 330 ms, rotating ch 33/52/71 (`grp`). It used the working remote's ID. These timings copy a quick tap on the working remote. No IR was sent, and the clone wasn't aimed.

| Command | Result |
|---|---|
| Groups 1, 2, 3, 4, UP and DOWN | each group's shades moved, independently of the others |
| All, UP and DOWN | all groups moved |

So RF group control works from our own radio. This also confirms, by the shades' response, the parts of the layout we hadn't sniffed: groups `02` / `04` / `08`, All = `FF`, and DOWN = `A2` / `22`. Trial counts weren't recorded. The household has four groups.

**2026-11-18 (group 4, default `grp` timings; reported by John, log not committed):**

- **A group command latches.** A ~0.63 s `grp` send (300 ms held + 330 ms tail) moved the shade from one limit to the other. So the 5 s cap on `grp` doesn't limit travel.
- **The release tail isn't a stop.** The shade kept moving after the `21` / `22` tail.
- **Travel times**, from the `RF group` line to John's Enter (add about 0.3–0.6 s for reaction): down 20.6, 20.8, 21.5 s (mean about 21.0 s); up 28.7, 26.6, 25.7 s (mean about 27.0 s). Up is about 25% slower. These are top-down/bottom-up shades, where DOWN opens.
- **One miss:** at 14:51:03 a `grp 4 down` sent all its packets and the shade didn't move. About 1 miss in 35 group commands so far.
- **Not tested yet:** whether an opposite group command stops a moving shade (the opposite IR arrow does). A timed preset would need it.

## Absolute position (experimental)

[spitcool/hd-platinum-rf](https://github.com/spitcool/hd-platinum-rf) has decoded position frames from the original **Platinum bridge** (`docs/rf-protocol.md`, "Position packets correlated with original-bridge commands — 2026-10-23"). They use the handheld's air settings (2 Mbps, `E7E7E7`, CRC-16, 5 bytes, no ack); only the payload differs. Layout per spitcool:

| Byte | Meaning |
|---|---|
| 0–1 | ID (the bridge sends its room ID) |
| 2 | `50` for about 1.49 s, then `32` for about 0.30 s |
| 3 | one-hot shade mask (bridge: `1 << (group_field - 1)`, up to `40`) |
| 4 | target position, raw 0–255 (`00`, `80`, `FF` seen) |

spitcool saw these at 2433 and 2452 MHz, none at 2471. Whether a shade moves when they're replayed is **unverified** in their notes.

The firmware's `pos <mask> <0-255> [ms50 ms32]` (fw 0.9.7) sends them with the stored ID, rotating ch 33/52/71 like `grp`, with spitcool's phase timings.

**Shade test, 2026-11-18 (John; [log](../captures/rf_position_20261118.log)).** Clone only, no bridge, using the working remote's ID (a handheld group pairing, not a bridge room ID). Watched: the two group-4 shades (top-down/bottom-up).

| Test | Result |
|---|---|
| `pos 08 255` / `pos 08 0` | both shades ran all the way up / all the way down |
| `pos 08 128`, from the top and from the bottom | both stopped at the same middle point either way, at slightly different speeds |
| `pos 08 128` with the shades already at 128 | no movement (twice) |
| `pos 08 64` / `pos 08 192` | about a quarter / three quarters of the way up |
| mask `01`, `02`, `04`, `08`, sums such as `06` | each bit is one handheld group (1, 2, 3, 4); a sum addresses those groups together. Group 4 responds to `08`, not `04` |
| `id 1234` (made up), then `pos 08 …` | no response; back to the working remote's ID, works |

So, on our shades:

- **Position frames work from a handheld-paired ID.** No bridge or bridge room ID is needed.
- **The position is absolute.** 0 is one limit (fully down here) and 255 the other (fully up), roughly linear in between. Repeating a command at the target doesn't move the shade, so resending is safe, and presets don't need to know where a shade starts.
- **Each shade goes to the target on its own.** Shades of different speeds end up level.
- **The mask is the handheld group byte** (`01`, `02`, `04`, `08`), and bits combine. spitcool's bridge masks run up to `40`; groups 5–7 don't exist on our handheld.
- **The ID is checked**, as for group commands (the red-mode wake isn't checked).

Not yet tested: "up" and "down" on a standard (bottom-up) shade, the shortest phase timings that work, whether the `32` phase is needed, a shade that's moving when a new position arrives, and the first command of the run (mask `04`), whose note said group 4 moved; John's later check showed group 4 doesn't respond to `04`, so that note is unresolved.

## Pairing (group assignment)

**From the manual (p. 5):** group assignment is how a shade learns a remote. In setup mode (SETUP on the back, LED flashing green), you press a group key, aim at the shade from a few inches away and press OPEN to add the shade to that group, or CLOSE to remove it. The shade jogs to confirm, and SETUP again saves and exits. A shade can belong to several groups, so it stores (remote ID, group) entries. That fits our findings: both our remotes (different IDs) work, an unknown ID gets no group response, and the weak remote's ID works on some groups but not group 4.

**On air (sniffed 2026-11-12, working remote, group 4; [capture](../captures/rf_ir_setup_mode_20261112.log)):**

| Step | RF | IR |
|---|---|---|
| SETUP | nothing | nothing |
| group key | nothing | nothing |
| OPEN (add) | `ID 30 08 80` while held (611 ms) | normal OPEN, `0101` (32 frames, ~0.57 s, on two presses) |
| CLOSE (remove) | `ID 31 08 80` while held, then `ID 31 08 00` (129–535 ms) | normal CLOSE, `0110` |
| SETUP (save) | nothing | nothing |

So the RF frame carries the ID, the group and add/remove (MODE bit 0), and the aimed IR picks which shade acts on it. There's no handshake or secret beyond the ID. The CLOSE presses were made with the remote aimed away from every shade; afterwards the remote still controlled group 4, which supports the IR doing the selecting.

**Clone pairing (2026-11-12).** The firmware's `pair <group> <add|remove>` sends `ID 30|31 GROUP 80` interleaved with the OPEN or CLOSE IR (RF on ch 33, IR, ch 52, IR, ch 71, IR, about 0.66 s), with no release tail. With a made-up ID (`C1 0E`) at a group-4 shade:

| Step | Result |
|---|---|
| `grp 4 up` before pairing | no response |
| `pair 4 add`, a few inches from the sensor eye | the shade jogged |
| `grp 4` after pairing | 5 of 5 up, 5 of 5 down |
| `pair 4 remove` (2026-11-15) | the shade jogged |
| `grp 4` after removal | no response, up or down |
| `grp 4` with the working remote's ID, after removal | up and down both work: the remote's entry is untouched |

**The shade has to be awake.** The first `pair 4 remove` and `pair 4 add` attempts on 2026-11-15 (about 0.68 s each, LED 2–3 inches from the eye) didn't jog. That fits the shade test the same day: after a few seconds idle, the shade needs about 1–2 s of RF and IR before it responds ([IR_PROTOCOL.md](IR_PROTOCOL.md#2026-11-15-wake-from-sleep-press-length-stop-and-reverse)). Later that day, with the shade awake from recent commands, both jogged, retried where needed. The working remote's setup add, remove and add again jogged both group-4 shades every time, **whether or not the entry already existed**. So a jog confirms the command was received, not that anything changed.

So **a clone can pair and unpair under its own ID, with no original remote**. Anyone who has lost their remotes can do this. Pairing needs the IR from a few inches away, so it needs physical access to the shade, as with the original remote. The remote's save press (SETUP again) sent nothing we could see, and the clone didn't need it.

**Duplicate remote (manual p. 7):** after a Master Reset, the new remote copies the original's identity from a few inches away. It doesn't create a new pairing, so two duplicated remotes share one ID. Not sniffed. Don't Master Reset a remote to try it.

The manual doesn't say how many remotes a shade can store. With `pair ... remove` working, test IDs can be added and removed to probe that, leaving the real remotes' entries alone.

## Firmware support

- `sniff`: receive at these settings. One line per run of identical packets.
- `id [hex|new]`: show, set or regenerate the remote ID for wake, group and setup frames. It's saved in the ESP32's flash, not in the source; a unit with none generates a random one at boot (fw 0.9.6).
- `wake [ms]`: send the wake alone (default 450 ms, rotating ch 33/52/71).
- `send` and the PC arrow keys: wake first, then IR (the arrow keys interleave them; see `seq`). `wake off` turns that off.
- `grp <1-4|all> <up|down> [ms]`: a group command, held frames for `ms` (default 300, max 5000), then the release tail.
- `pos <mask> <0-255> [ms50 ms32]`: experimental absolute-position frames (see [Absolute position](#absolute-position-experimental)).
- `pair <1-4|all> <add|remove>`: group assignment with the stored ID, 3 steps (setup frames, then IR). Aim at one shade's sensor eye from a few inches. Wake the shade first (an arrow press) or repeat until it jogs.

## Open questions

- ~~Does the shade check the ID?~~ For group commands, yes. For the red-mode wake, no: two made-up IDs both worked (2026-11-12).
- ~~How does pairing work on air?~~ Sniffed, and the clone pairs itself (2026-11-12; see [Pairing](#pairing-group-assignment)). Remove works too (2026-11-15). Still open: the backup-remote copy (p. 7), which we won't try on our remotes.
- How many remotes can a shade store? The manual doesn't say. Probe with test IDs added and removed by `pair`.
- How long does the shade stay receptive after the wake, and does the original send IR during or after the RF? (Measure with the analyzer: IR receiver OUT plus our module's IRQ during `sniff`.)
- The shortest wake that works (needs a setting for the automatic wake length).
- Sniff channels 52 and 71 to confirm the rotation.
- ~~Do shades accept spitcool's bridge position frames from a handheld-paired ID?~~ Yes, and the position is absolute (2026-11-18; see [Absolute position](#absolute-position-experimental)). Still open: the shortest timings that work, and whether the `32` phase is needed.
- Does an opposite group command stop a moving shade?
- ~~Group commands from the clone~~ Done 2026-11-11 (see [Group commands from the clone](#group-commands-from-the-clone)). Still open: does a shorter hold or tail work, and how does a long hold behave (does the shade keep moving only while frames arrive)?

## Evidence

| File | What |
|---|---|
| [`captures/rf_sniff_20261111.log`](../captures/rf_sniff_20261111.log) | Three sniffs: the weak remote (group 1 + UP; red-mode presses) and the working remote (red UP tap, red DOWN hold, group 1 + UP). IDs redacted |
| [`captures/rf_ir_setup_mode_20261112.log`](../captures/rf_ir_setup_mode_20261112.log) | Setup-mode group assignment: RF for add and remove, IR summaries, and the clone pairing result. ID redacted |
| [`captures/rf_position_20261118.log`](../captures/rf_position_20261118.log) | Absolute-position frames from the clone (`pos`): serial lines for the first runs, John's notes for the rest. ID redacted |
| [`captures/shade_timing_20261115.log`](../captures/shade_timing_20261115.log) | Clone shade timing tests, and the clone's pairing remove (section 7) |
