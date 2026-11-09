# HD Platinum IR protocol (point-and-press / red mode)

Reverse engineered on 2026-11-07 from the original Platinum remote. The source of truth in code is [`lib/hdcore/src/hd_ir.h`](../lib/hdcore/src/hd_ir.h).

| Status | |
|---|---|
| Frame format | ✅ decoded; matches the ESP32 capture and the logic analyzer |
| UP / DOWN codes, channel A | ✅ confirmed by both |
| UP / DOWN codes, channel B | ✅ ESP32 capture only (2026-11-08) |
| Other buttons | ✅ **none send IR in normal use**; only the up/down arrows transmit (logic analyzer, 2026-11-07). Setup-mode presses not captured yet. |
| Carrier frequency / duty cycle | 🔲 not measured (manual says 40 kHz) |
| Shade responds to synthesized frames | ❌ **no response** in the one test so far (2026-11-08, see [Shade tests](#shade-tests)) |
| RF wake-up needed before IR? | 🔲 open; a leading explanation, not proven (acceptance A5) |

## When the remote sends IR

The remote has 7 keys: **UP, DOWN, 1, 2, 3, 4, ALL**, plus an **A/B slide switch**, which the manual calls the **Infrared Channel Selector**. It changes the IR code (see [Codes](#codes)). Each shade listens on one channel, and most are set to A (manual pp. 18, 22). Whether the switch also affects RF is unknown.

- **Group keys (1–4, ALL) are RF-only.** Pressing one always flashes the green LED and opens an **RF window of ≈ 6.5 s** (stopwatch measurement, 2026-11-07; the manual says 5 s). Pressing a group key again during the window **restarts** it.
- **The arrow keys are IR by default (red LED).** During an RF window they act as RF group commands instead (green). Once the window expires, they go back to IR.
- So IR is sent **only by UP or DOWN pressed with no recent group key**. Arrows pressed during an RF window send **no IR** (verified 2026-11-07 with the receiver and `ir quiet`).

Not yet known:
- whether arrow presses extend the window
- whether red-mode arrows also key the radio (the RF wake-up question)
- what IR is sent during the manual's setup procedures, which all end with an aimed arrow press: group assignment (p. 5), intermediate stop (p. 8), channel change (p. 22)

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

| Switch | Button | Code (binary) | Hex | Header | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|---|---|---|---|---|---|---|---|---|
| A | Up arrow (OPEN) | `0101` | 0x5 | 2.75 / 2.75 | 1.03 / 1.03 | 1.03 / **2.75** | 1.03 / 1.03 | 1.03 / **2.75** |
| A | Down arrow (CLOSE) | `0110` | 0x6 | 2.75 / 2.75 | 1.03 / 1.03 | 1.03 / **2.75** | 1.03 / **2.75** | 1.03 / 1.03 |
| B | Up arrow (OPEN) | `1001` | 0x9 | 2.75 / 2.75 | 1.03 / **2.75** | 1.03 / 1.03 | 1.03 / 1.03 | 1.03 / **2.75** |
| B | Down arrow (CLOSE) | `1010` | 0xA | 2.75 / 2.75 | 1.03 / **2.75** | 1.03 / 1.03 | 1.03 / **2.75** | 1.03 / 1.03 |

(Each cell is mark / space in ms.)

The four codes fit a simple split, inferred from these four values only:

| Bits | Meaning | Values |
|---|---|---|
| 3–2 | IR channel | `01` = A, `10` = B |
| 1–0 | Command | `01` = OPEN, `10` = CLOSE |

Every code seen has exactly two 1-bits, so all frames are the same length.

In normal use, the arrows are the only keys that send IR (checked with the logic analyzer on channel A). This matches the manual: in point-and-press mode you stop a moving shade by pressing the opposite arrow, so there's no separate STOP code. The group buttons (1–4, All) only apply to RF mode.

It's unknown what the shades do with the 12 unused codes, or whether setup procedures use any of them. Sending unused codes at a shade is a possible experiment, at our own risk.

**Caution:** per the manual (p. 21), holding OPEN for 6 s while aimed at a fully open shade clears its stops and motor limits. A long IR send could do the same, so `send` limits OPEN codes to 5 s.

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

- **Frames repeat back to back** for as long as the button is held (a 2.5 s hold ≈ 150 frames). A quick press sends about 20 frames (0.35 s).
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

## Shade tests

Run 2026-11-08 on one shade, with the remote's switch on A. The shade is a battery model converted to DC power, so it has the same electronics as a battery shade. The original remote operates it in red mode from more than 10 ft.

**Synthesized IR.** `send down 10000` with a TSAL6200 at about 1 ft, aimed in several directions around the sensor eye. The shade was open and did not move. At the LED, the scope showed 3.2 V p-p across the 33 Ω resistor (about 97 mA peak), a carrier that looked like 40 kHz, and a correct-looking frame.

**Foil-shielded original remote**, operated through the foil:

| Shield | RF (group key + arrow) | IR (red-mode arrow) |
|---|---|---|
| Fully wrapped | no response | not applicable |
| Small hole showing the whole LED face | no response | no response at about 30 cm |
| Hole about 1.5 cm across | **works** | works |

**What this does and doesn't show:**
- Blocking RF stopped red mode with the LED visible, and our RF-free IR failed too. Both fit an RF wake-up.
- It isn't proven. We didn't confirm that IR was leaving the small hole, or check for a second emitter. The largest hole leaks RF, so it can't separate the two links.
- Our IR may still differ from the original's. Our timings come from demodulating receivers, and the original's LED drive hasn't been measured.

**Pitfall for anyone repeating this:** a 1.5 cm hole is small compared with the 12.5 cm wavelength but still leaks enough RF to work, so always run the RF control at each hole size.

## Evidence

| File | What |
|---|---|
| [`captures/ir_red_open_close_20261107_132807.log`](../captures/ir_red_open_close_20261107_132807.log) | ESP32 capture, short presses: 5 bursts, codes `0101` / `0110` |
| [`captures/ir_red_hold_open_then_close_20261107_133330.log`](../captures/ir_red_hold_open_then_close_20261107_133330.log) | ESP32 capture, ~2.5 s holds |
| [`captures/la_ir_red_up_arrow_20261107.sr`](../captures/la_ir_red_up_arrow_20261107.sr) | PulseView, up arrow: 150 × `0101`, last frame cut off at release |
| [`captures/la_ir_red_down_arrow_20261107.sr`](../captures/la_ir_red_down_arrow_20261107.sr) | PulseView, down arrow: 74 × `0110` |
| [`captures/ir_ab_channel_20261108.log`](../captures/ir_ab_channel_20261108.log) | ESP32 capture: captures 1–4 switch on A, 5–8 on B |

The receiver OUT was on GPIO 32 for the Nov 7 ESP32 captures, GPIO 14 for Nov 8, and on analyzer D0 for the PulseView captures. The analyzer files open in PulseView, and the serial logs are plain text.

## Firmware support

- `ir` / `ir quiet`: capture and print a decoded summary per burst, e.g. `HD: code=0101 (A UP) frames=147 other=0 duration=2.53 s`.
- `send up|down|bbbb [ms]`: synthesize and transmit frames at 40 kHz for about `ms` (default 1000). `up` and `down` are channel A; use `1001` / `1010` for B. OPEN codes are limited to 5000 ms and others to 10000 ms. `play` replays raw captures and has no such limit.
- Codec: `hd::ir::encodeFrame`, `encodeBurst`, `decodeFrames`, `summarize` in [`hd_ir.h`](../lib/hdcore/src/hd_ir.h), tested in [`test/test_native_ir`](../test/test_native_ir/test_main.cpp) against real captured frames.
