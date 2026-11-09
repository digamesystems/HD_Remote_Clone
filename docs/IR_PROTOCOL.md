# HD Platinum IR protocol (point-and-press / red mode)

Reverse engineered on 2026-11-07 from the original Platinum remote. The source of truth in code is [`lib/hdcore/src/hd_ir.h`](../lib/hdcore/src/hd_ir.h).

| Status | |
|---|---|
| Frame format | ✅ decoded; matches the ESP32 capture and the logic analyzer |
| UP / DOWN codes, channel A | ✅ confirmed by both |
| UP / DOWN codes, channel B | ✅ ESP32 capture only (2026-11-08) |
| Other buttons | ✅ **none send IR in normal use**; only the up/down arrows transmit (logic analyzer, 2026-11-07). Setup-mode presses not captured yet. |
| Carrier frequency / duty cycle | 🔲 not measured (manual says 40 kHz) |
| Shade responds to synthesized frames | ❌ no response in one test (2026-11-08), which may have had no travel available; see [Shade tests](#shade-tests) |
| RF needed alongside IR? | ⚠️ **well supported, not proven** (2026-11-09 foil-tube test): the remote's own IR fails when its RF is blocked. The RF burst itself hasn't been observed (acceptance A5) |

## When the remote sends IR

The remote has 7 keys: **UP, DOWN, 1, 2, 3, 4, ALL**, plus an **A/B slide switch**, which the manual calls the **Infrared Channel Selector**. It changes the IR code (see [Codes](#codes)). Each shade listens on one channel, and most are set to A (manual pp. 18, 22). Whether the switch also affects RF is unknown.

- **Group keys (1–4, ALL) are RF-only.** Pressing one always flashes the green LED and opens an **RF window of ≈ 6.5 s** (stopwatch measurement, 2026-11-07; the manual says 5 s). Pressing a group key again during the window **restarts** it.
- **The arrow keys are IR by default (red LED).** During an RF window they act as RF group commands instead (green). Once the window expires, they go back to IR.
- So IR is sent **only by UP or DOWN pressed with no recent group key**. Arrows pressed during an RF window send **no IR** (verified 2026-11-07 with the receiver and `ir quiet`).

Not yet known:
- whether arrow presses extend the window
- what RF red-mode arrows send. The foil-tube test shows the shade needs RF alongside IR (see [Shade tests](#shade-tests)), but the RF itself hasn't been captured
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

All tests used one shade, with the remote's switch on A. The shade is a battery model converted to DC power, so it has the same electronics and firmware as a battery shade. The original remote operates it in red mode from more than 10 ft.

**These shades are top-down/bottom-up, and the arrows act reversed: DOWN opens, UP closes.** Before any shade test, put the shade mid-travel and check that the original remote moves it both ways. Otherwise a command can fail only because the shade is already at that end.

### Summary

**IR carries the command, and the shade also needs RF alongside it.** The original remote moves the shade when bare, but not when sealed in a foil tube that blocks its RF while its IR still gets out. Point-and-press moves only the aimed shade, and the IR frames carry the direction bits, so IR selects the shade and gives the command. What the RF carries is still open: a bare wake-up, the command, or the remote's pairing ID. That decides what a replacement must transmit.

This is the first hands-on evidence we know of for the RF wake-up claim, which until now existed only as secondhand forum reports.

### 2026-11-09: foil tube ("burrito")

<img src="images/foil_tube_bench.jpg" width="420" alt="Remote sealed in a long aluminium foil tube on the bench next to the ESP32 test rig"> <img src="images/foil_tube_mouth.jpg" width="300" alt="Looking into the open end of the foil tube: the remote's IR window is visible at the back">

**Method.** The remote was sealed in a long aluminium foil tube, about 5 cm across (estimated from the photo), with the back end twisted shut and the front end open. IR leaves through the open end. 2.4 GHz should be attenuated, because a round metal tube only passes it if it's wider than about 7 cm (waveguide cutoff), and the remote sits some way back from the mouth. Buttons were pressed through the foil.

**Why a tube.** Smaller covers leak. With a hand wrapped around the remote's front, our receiver still decoded 14–18 of 20 frames per press (log Part 1). Our 2026-11-08 attempts with a hole in a full wrap couldn't separate the links either (see below).

**IR gets out (receiver checks, `ir quiet`, log Parts 2 and 3).**

| Burrito position | Result |
|---|---|
| Open end aimed at the receiver, 30 cm (two runs) | 14 of 14 presses decoded cleanly, 20 frames each (one 24-frame longer press), no bad frames |
| Pointed up, 90° off axis, 30 cm | 6 of 6 clean, probably by reflection from nearby surfaces |
| Pointed into free space away from the receiver | nothing in about a minute |
| Swept up and down out of the receiver's plane | mostly clean near the axis, some presses at 17, 10 or 6 frames, and no valid frames at the extremes |

**Shade results** (shade with travel available, about 30 cm from the sensor eye):

| Remote | IR out | RF out | Shade |
|---|---|---|---|
| Bare, red-mode arrow | yes | yes, if it sends any | **moves** |
| Wrapped, red-mode arrow, aimed at the sensor eye | yes, decoded cleanly at the same 30 cm | blocked | **no response** |
| Bare, group key + arrow (green mode) | none (green mode sends no IR) | yes | **moves** |
| Wrapped, group key + arrow | none | blocked | **no response** (the RF control: the foil blocks RF) |

The bare, wrapped, bare sequence was repeated with the same results. Exact trial counts weren't recorded.

**Reading.** The same remote, distance and code worked bare and failed wrapped. The foil can't change the IR's code or timing, and our receiver confirmed the IR reaches 30 cm cleanly, so the difference we can show is that RF was blocked.

**Caveats.**
- The tube narrows the IR beam compared with the bare remote. The clean decode at the same distance makes this unlikely to matter.
- The shade's receiver may be less sensitive than ours (TSOP38238 / VS1838B type). It ignored IR at a distance where ours decoded every frame, and the bare remote works from more than 10 ft.
- The RF itself hasn't been observed, so its timing, channel and content are unknown.

**To repeat it.** Record the tube's opening width and how far back the remote sits, and mark the insertion depth on the remote. Re-run the green-mode control after any rewrap. Check that IR gets out with a receiver at the same distance as the shade.

### 2026-11-08: synthesized IR and foil with a hole

**Synthesized IR.** `send down 10000` with a TSAL6200 at about 1 ft, aimed in several directions around the sensor eye. The shade did not move. **This test was flawed:** the shade was open and DOWN opens these shades, so it may have had no travel available. A failure is consistent with the foil-tube result, but this test doesn't show it on its own.

<img src="images/clone_led_drive_scope.png" width="420" alt="Scope trace across the 33 ohm LED resistor: header burst, then four bit bursts">

The scope across the 33 Ω LED resistor showed about 3.2 V p-p (about 97 mA peak) and the expected frame: header, then marks with short and long spaces. The burst tops are flat, so the supply holds up. The on-screen timebase reads 50 µs/div, which doesn't match the trace (individual 40 kHz cycles would be visible at that setting), so treat its horizontal scale as unknown.

<img src="images/clone_led_off.jpg" width="200" alt="Clone IR LED on the breadboard, off"> <img src="images/clone_led_on.jpg" width="200" alt="Clone IR LED glowing purple through a phone camera while sending"> <img src="images/remote_ir_led_on.jpg" width="300" alt="Original remote's IR window glowing purple through a phone camera; the red mode indicator is lit on top"> <img src="images/remote_ir_led_off.jpg" width="300" alt="Original remote's IR window, idle">

Phone camera views of the clone LED (off, sending) and the original remote's single IR window (sending, idle). A camera only shows that an LED emits; it can't compare output or show that a cover blocks IR.

**Foil wrap with a hole**, operated through the foil:

| Shield | RF (group key + arrow) | IR (red-mode arrow) |
|---|---|---|
| Fully wrapped | no response | not applicable |
| Small hole showing the whole LED face | no response | no response at about 30 cm |
| Hole about 1.5 cm across | **works** | works |

The small-hole result agrees with the foil tube, but on its own it was weak: the IR leaving the hole was never checked, and the travel issue above applies. A 1.5 cm hole is small compared with the 12.5 cm wavelength but still leaks enough RF to work, so always run the RF control for each shield.

### Next

1. **IR clone plus the remote's RF.** Cover only the remote's IR window with a small foil patch. Check the patch with `ir quiet` (receiver against the window: no captures), and check that group key + arrow still moves the shade. Then send the clone's IR while pressing the same arrow on the patched remote in red mode. Controls: the clone alone, the patched remote alone, and opposite directions to see which device the shade obeys. If the shade follows the clone, our IR encoding works once the shade is awake.
2. **nRF24 sniff.** Screen the modules, check the carrier scan and promiscuous mode by having one module transmit to another, then scan and sniff during red-mode presses for the burst's timing, channel and payload, and whether it carries the remote's pairing ID.
3. **Measure the foil directly** with a module at the tube's mouth, remote bare vs. wrapped.
4. **Probe the remote's own LED drive** for the true carrier frequency, duty cycle and timings.

## Evidence

| File | What |
|---|---|
| [`captures/ir_red_open_close_20261107_132807.log`](../captures/ir_red_open_close_20261107_132807.log) | ESP32 capture, short presses: 5 bursts, codes `0101` / `0110` |
| [`captures/ir_red_hold_open_then_close_20261107_133330.log`](../captures/ir_red_hold_open_then_close_20261107_133330.log) | ESP32 capture, ~2.5 s holds |
| [`captures/la_ir_red_up_arrow_20261107.sr`](../captures/la_ir_red_up_arrow_20261107.sr) | PulseView, up arrow: 150 × `0101`, last frame cut off at release |
| [`captures/la_ir_red_down_arrow_20261107.sr`](../captures/la_ir_red_down_arrow_20261107.sr) | PulseView, down arrow: 74 × `0110` |
| [`captures/ir_ab_channel_20261108.log`](../captures/ir_ab_channel_20261108.log) | ESP32 capture: captures 1–4 switch on A, 5–8 on B |
| [`captures/ir_shield_tests_20261109.log`](../captures/ir_shield_tests_20261109.log) | ESP32 captures for the foil-tube tests: hand over the remote (Part 1), tube aimed and off axis (Part 2), tube at 30 cm (Part 3) |
| [`docs/images/`](images/) | Photos of the foil tube, the remote's IR window, the clone LED, and the scope trace of the clone's LED drive |

The receiver OUT was on GPIO 32 for the Nov 7 ESP32 captures, GPIO 14 from Nov 8, and on analyzer D0 for the PulseView captures. The analyzer files open in PulseView, and the serial logs are plain text.

## Firmware support

- `ir` / `ir quiet`: capture and print a decoded summary per burst, e.g. `HD: code=0101 (A UP) frames=147 other=0 duration=2.53 s`.
- `send up|down|bbbb [ms]`: synthesize and transmit frames at 40 kHz for about `ms` (default 1000). `up` and `down` are channel A; use `1001` / `1010` for B. OPEN codes are limited to 5000 ms and others to 10000 ms. `play` replays raw captures and has no such limit.
- PC Up/Down arrow keys in the serial monitor send `up` / `down` for 500 ms per press.
- Lines starting with `#` are comments: echoed to the log, and they don't stop a running capture. Use them to annotate tests.
- Codec: `hd::ir::encodeFrame`, `encodeBurst`, `decodeFrames`, `summarize` in [`hd_ir.h`](../lib/hdcore/src/hd_ir.h), tested in [`test/test_native_ir`](../test/test_native_ir/test_main.cpp) against real captured frames.
