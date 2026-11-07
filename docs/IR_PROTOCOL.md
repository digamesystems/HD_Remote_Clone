# HD Platinum IR protocol (point-and-press / red mode)

Reverse engineered on 2026-11-07 from the original Platinum remote. The source of truth in code is [`lib/hdcore/src/hd_ir.h`](../lib/hdcore/src/hd_ir.h).

| Status | |
|---|---|
| Frame format | ✅ decoded; matches the ESP32 capture and the logic analyzer |
| UP / DOWN codes | ✅ confirmed by both |
| Other buttons | ✅ **none send IR**; only the up/down arrows transmit (checked with the logic analyzer, 2026-11-07) |
| Carrier frequency / duty cycle | 🔲 not measured (manual says 40 kHz) |
| Shade responds to synthesized frames | 🔲 untested (acceptance A2) |
| RF wake-up needed before IR? | 🔲 unknown (acceptance A5) |

## When the remote sends IR

The remote has 7 keys: **UP, DOWN, 1, 2, 3, 4, ALL**.

- **Group keys (1–4, ALL) are RF-only.** Pressing one always flashes the green LED and opens a short **RF window** (the manual says ~5 s).
- **The arrow keys are IR by default (red LED).** During an RF window they act as RF group commands instead (green). Once the window expires, they go back to IR.
- So IR is sent **only by UP or DOWN pressed with no recent group key**.

Not yet known:
- the exact window length, and whether arrow presses extend it
- whether arrows in green mode also send IR
- whether red-mode arrows also key the radio (the RF wake-up question)

## Frame structure

A frame is a **header followed by 4 data bits, MSB first**. "Mark" means IR on (carrier bursts) and "space" means IR off. Data is **pulse-distance encoded**: every bit starts with the same short mark, and the length of the space after it gives the bit value.

| # | Part | Type | Duration |
|---|---|---|---|
| 1 | Header | mark | 2.75 ms |
| 2 | Header | space | 2.75 ms |
| 3 | Bit 3 (MSB) | mark | 1.03 ms |
| 4 | Bit 3 value | space | 1.03 ms = **0**, 2.75 ms = **1** |
| 5 | Bit 2 | mark | 1.03 ms |
| 6 | Bit 2 value | space | 0 / 1 as above |
| 7 | Bit 1 | mark | 1.03 ms |
| 8 | Bit 1 value | space | 0 / 1 |
| 9 | Bit 0 (LSB) | mark | 1.03 ms |
| 10 | Bit 0 value | space | 0 / 1 (also serves as the gap before the next frame) |

There is no stop bit and no extra gap between frames. The next header mark follows right after bit 0's space.

## Codes

| Button | Code (binary) | Hex | Header | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|---|---|---|---|---|---|---|---|
| Up arrow (OPEN) | `0101` | 0x5 | 2.75 / 2.75 | 1.03 / 1.03 | 1.03 / **2.75** | 1.03 / 1.03 | 1.03 / **2.75** |
| Down arrow (CLOSE) | `0110` | 0x6 | 2.75 / 2.75 | 1.03 / 1.03 | 1.03 / **2.75** | 1.03 / **2.75** | 1.03 / 1.03 |

(Each cell is mark / space in ms.)

These are the **only two IR commands**. No other button on the remote sends IR (checked with the logic analyzer on the receiver output). This matches the manual: in point-and-press mode you stop a moving shade by pressing the opposite arrow, so there's no separate STOP code. The group buttons (1–4, All) only apply to RF mode.

UP and DOWN differ only in the last two bits. With just two codes in use, it's unknown whether the leading `01` means anything or what the shades do with the 14 unused codes. Sending unused codes at a shade is a possible experiment, at our own risk.

### Up arrow, `0101`

```
       header          bit3=0    bit2=1         bit1=0    bit0=1
 ┌─────────┐         ┌──┐      ┌──┐           ┌──┐      ┌──┐
 │  2.75   │  2.75   │1 │ 1.03 │1 │   2.75    │1 │ 1.03 │1 │   2.75    ┌─ next header
─┘         └─────────┘  └──────┘  └───────────┘  └──────┘  └───────────┘
   mark      space    m    s    m      s       m    s    m      s
```

### Down arrow, `0110`

```
       header          bit3=0    bit2=1         bit1=1         bit0=0
 ┌─────────┐         ┌──┐      ┌──┐           ┌──┐           ┌──┐
 │  2.75   │  2.75   │1 │ 1.03 │1 │   2.75    │1 │   2.75    │1 │ 1.03 ┌─ next header
─┘         └─────────┘  └──────┘  └───────────┘  └───────────┘  └──────┘
```

## Frame timing

| Component | Duration |
|---|---|
| Header (mark + space) | 2.75 + 2.75 = 5.50 ms |
| 4 bit marks | 4 × 1.03 = 4.12 ms |
| Spaces for a code with two 1-bits | 2 × 1.03 + 2 × 2.75 = 7.56 ms |
| **Total** | **17.18 ms** (measured: **17.19 ms**) |

UP and DOWN both have two 1-bits, so their frames are the same length. A code with a different number of 1-bits would be 1.72 ms longer or shorter per 1-bit.

## Button-press behaviour

```
press ────────────────────────────── hold ────────────────────── release
      [frame][frame][frame][frame] … [frame][fra┤ cut off
       17.19 ms each, back to back, all identical
```

- **Frames repeat back to back** for as long as the button is held (a 2.5 s hold ≈ 150 frames). A short press still sends many frames.
- **Every frame in a burst is identical**: no toggle bit, no rolling counter, no checksum.
- **On release, transmission stops immediately**, even partway through a frame (seen in the analyzer captures). There is **no release or stop frame**.
- **No shade address:** whichever shade's sensor the remote is aimed at responds. This fits the manual's point-and-press description.

## Measured vs. nominal values

Measurements come from demodulating receivers (TSOP38238 / VS1838B), which distort mark and space lengths by tens of microseconds. The nominal values are what `send` transmits.

| Element | ESP32 capture (µs) | Logic analyzer @100 kHz (µs) | Nominal (µs) |
|---|---|---|---|
| Header mark | 2700–2750 | 2700–2750 | 2750 |
| Header space | 2750–2800 | 2750–2800 | 2750 |
| Bit mark | 990–1050 | 990–1050 | 1030 |
| 0-bit space | 1050–1100 | 1050–1110 | 1030 |
| 1-bit space | 2750–2800 | 2750–2800 | 2750 |

The decoder treats any duration above **1900 µs** as long (header or 1-bit space), well clear of both clusters.

**To refine:** probe the remote's IR LED drive (transistor base or LED cathode) with the logic analyzer at ≥1 MHz to get the true mark and space lengths, the carrier frequency and the duty cycle.

## Evidence

| File | What |
|---|---|
| [`captures/ir_red_open_close_20261107_132807.log`](../captures/ir_red_open_close_20261107_132807.log) | ESP32 capture, short presses: 5 bursts, codes `0101` / `0110` |
| [`captures/ir_red_hold_open_then_close_20261107_133330.log`](../captures/ir_red_hold_open_then_close_20261107_133330.log) | ESP32 capture, ~2.5 s holds |
| [`captures/la_ir_red_up_arrow_20261107.sr`](../captures/la_ir_red_up_arrow_20261107.sr) | PulseView, up arrow: 150 × `0101`, last frame cut off at release |
| [`captures/la_ir_red_down_arrow_20261107.sr`](../captures/la_ir_red_down_arrow_20261107.sr) | PulseView, down arrow: 74 × `0110` |

In all cases the receiver OUT was on GPIO 32 for the ESP32 captures and on analyzer D0 for the PulseView captures. The analyzer files open in PulseView, and the serial logs are plain text.

## Firmware support

- `ir` / `ir quiet`: capture and print a decoded summary per burst, e.g. `HD: code=0101 (UP) frames=147 other=0 duration=2.53 s`.
- `send up|down|bbbb [ms]`: synthesize and transmit frames at 40 kHz for about `ms` (default 1000).
- Codec: `hd::ir::encodeFrame`, `encodeBurst`, `decodeFrames`, `summarize` in [`hd_ir.h`](../lib/hdcore/src/hd_ir.h), tested in [`test/test_native_ir`](../test/test_native_ir/test_main.cpp) against real captured frames.
