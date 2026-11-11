# HD Platinum RF protocol (2.4 GHz, nRF24)

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
| Shade checks the ID | not tested yet (try an unknown ID) |
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
| `MODE` | `20` group command; `01` red-mode (IR) press |
| `GROUP` | `01` group 1 (spitcool: one-hot `01/02/04/08`, `FF` All); `00` none |
| `ACTION` | `A1` UP held, `21` UP release tail (spitcool also lists `A2` / `22` for DOWN); `80` wake held; `00` wake release tail |

No counter or rolling code: every packet of a press is identical, and presses repeat the same payload.

In both pairs, bit 7 is set while the key is held and cleared in the tail (`A1` → `21`, `80` → `00`). That's a pattern, not a confirmed meaning.

## What each key press sends

| Press | RF |
|---|---|
| Group key (1–4, All) alone | **nothing.** It only opens the green window on the remote |
| Arrow within the green window | `ID 20 <group> A1` (UP) while held, then `ID 20 <group> 21` for ~0.3 s |
| Arrow in red mode (IR) | `ID 01 00 80` while held, alongside the IR. No direction: the IR carries it |

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

From the shade tests in [IR_PROTOCOL.md](IR_PROTOCOL.md#shade-tests): the shade ignores IR on its own. It acts on IR only when a red-mode RF press from a paired remote arrives within about a second before it, and one RF press arms one IR command. The wake frame carries no direction, which fits the clone test, where the shade followed the clone's IR, not the remote's arrow.

So a replacement should send `ID 01 00 80`, using an ID the shade accepts, for a few hundred ms, then the IR command, every time.

**Shown on 2026-11-11.** Our clone sent `ID 01 00 80` with the working remote's ID for 450 ms. It rotated ch 33/52/71, one packet about every 300 µs, so about 1,500 packets. No `00` tail. The IR followed immediately, and the shade obeyed: up, down and stop, with a new wake for each command. So the tail isn't needed, and sending RF before IR, rather than at the same time, works. Still untested: whether the shade checks the ID, the shortest wake that works, and how the original times its RF against its IR.

## Firmware support

- `sniff`: receive at these settings. One line per run of identical packets.
- `id <hex>`: store the remote ID for the wake. It's saved in the ESP32's flash, not in the source.
- `wake [ms]`: send the wake alone (default 450 ms, rotating ch 33/52/71).
- `send` and the PC arrow keys: wake first, then IR. `wake off` turns that off.

## Open questions

- Does the shade check the ID? (A clone-sent wake + IR with a paired ID works.)
- How long does the shade stay receptive after the wake, and does the original send IR during or after the RF? (Measure with the analyzer: IR receiver OUT plus our module's IRQ during `sniff`.)
- The shortest wake that works (needs a setting for the automatic wake length).
- Sniff channels 52 and 71 to confirm the rotation.
- Group commands from the clone (`ID 20 <group> A1/A2`, then the tail), for RF-only control.

## Evidence

| File | What |
|---|---|
| [`captures/rf_sniff_20261111.log`](../captures/rf_sniff_20261111.log) | Three sniffs: the weak remote (group 1 + UP; red-mode presses) and the working remote (red UP tap, red DOWN hold, group 1 + UP). IDs redacted |
