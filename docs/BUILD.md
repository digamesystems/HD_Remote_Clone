# Build your own

> **TL;DR** (summary checked 2026-11-18): today you can build a **breadboard** version: an ESP32, an nRF24 radio module and an IR LED, driven from a PC over USB. It moves Platinum shades by point-and-press, and by group once paired, with no original remote. The **handheld** (custom PCB, battery, case) is designed but hasn't been built or tested yet, so don't order boards from these files yet.

This page is the short path. The reasons, measurements and evidence are in the other docs: [IR_PROTOCOL.md](IR_PROTOCOL.md), [RF_PROTOCOL.md](RF_PROTOCOL.md) and [FIRMWARE.md](FIRMWARE.md).

## Before you start

- **Shades:** Hunter Douglas shades with **Platinum Technology** (the 2013-era system, before PowerView) that work with the Platinum handheld remote. We've tested on our own shades only: top-down/bottom-up models, battery versions converted to DC power. Other Platinum shades will probably behave the same, but that hasn't been checked.
- **What you get:** a breadboard you control by typing commands in a serial terminal on a PC. It isn't a handheld yet.
- **Skills:** basic breadboard wiring and [PlatformIO](https://platformio.org/). Soldering is optional (a capacitor on the radio module).
- **Original remote:** not needed.

## Parts

| Part | Qty | Notes |
|---|---|---|
| ESP32-DevKitC (ESP32-WROOM-32) | 1 | Other ESP32 boards may need pin changes (WROVER modules use GPIO 16/17 for PSRAM) |
| nRF24L01+ module, 2×4 header, PCB antenna | 1 | Cheap modules are often clones; check yours with `test` |
| 10 µF capacitor | 1 | Across the module's VCC and GND |
| TSAL6200 940 nm IR LED | 1 | |
| NPN transistor, 2N2222 or PN2222 | 1 | Switches the LED |
| Resistors: 1 kΩ, 33 Ω | 1 each | Base resistor; LED current limit (from 5 V) |
| Capacitors: 100 µF, 100 nF | 1 each | Across 5 V and GND, near the LED |
| Breadboard, female-to-male jumpers, USB cable | | The nRF24 header doesn't plug into a breadboard directly |

## Wiring

- **nRF24 module:** follow [WIRING.md](WIRING.md). It covers the header pinout, a multimeter check before powering, and the GPIO table (CE 17, CSN 5, SCK 18, MOSI 23, MISO 19). **3.3 V only:** 5 V can destroy the module.
- **IR LED:** 5 V (the DevKitC's 5V pin) → 33 Ω → LED anode (long leg). LED cathode → NPN collector, emitter → GND. GPIO 4 → 1 kΩ → base. Put the 100 µF and 100 nF across 5 V and GND near the LED. This gives pulses of roughly 90–100 mA, within the LED's 200 mA peak rating.

The full pin map is in [FIRMWARE.md](FIRMWARE.md#pin-map).

## Flash the firmware

```
git clone https://github.com/digamesystems/HD_Remote_Clone.git
cd HD_Remote_Clone
pio run -t upload
pio device monitor
```

The monitor runs at 115200 baud. Type `help` for the command list. If the upload can't connect, hold BOOT on the DevKitC and tap EN.

Type `test` to check the radio module. It should report PASS at 2 Mbps.

## Set your ID

The radio frames carry a 2-byte remote ID. Pick your own **random** 4-digit hex ID (not `0000` or `FFFF`) and store it:

```
id 3a7c
```

The ID is saved in the ESP32's flash. Use a random value rather than a well-known one: a neighbour's clone with the same ID could control any shade you pair with it.

## Point-and-press

1. Aim the IR LED at the shade's sensor eye. Start close (about 30 cm) and hold the LED still.
2. Type `send up 2000` or `send down 2000`. Each sends the RF wake and the IR together for 2 s.
3. Once that works, the **Up/Down arrow keys** in the terminal send the same. `hold on` makes them send for as long as you hold the key.

Things to know:

- **The shade sleeps.** After a few seconds idle it needs RF and IR together for about 1–2 s before it responds. Once it's moving, a short press of the opposite arrow stops it.
- **Direction:** on top-down/bottom-up shades the arrows can be reversed (on ours, DOWN opens).
- **Channel B:** shades listen on IR channel A by default. For a shade set to B, use `send 1001` (up) or `send 1010` (down).
- **Range:** with our single LED, it's reliable at about 30 cm with the LED fixed in place. Farther away it works, but less reliably, and it depends on aim.
- **Don't hold OPEN for 6 s at a fully open shade:** the manual says that resets its limits. `send` caps every code at 5 s for this reason, in both directions, because on some shades DOWN is the one that opens.

## Group control (optional)

Group commands go over RF only, with no aiming, but each shade must first learn your ID.

1. Wake the shade: press an arrow at it until it moves, then stop it.
2. From a few inches, aim at its sensor eye and type `pair 1 add` (groups 1–4 or `all`). The shade jogs briefly when it accepts.
3. `grp 1 up` / `grp 1 down` now move every shade you've paired to group 1.

To undo, type `pair 1 remove` while aimed at the shade. On ours, removing the clone's ID left the original remote's pairing intact. We don't know how many IDs a shade can hold, so remove the ones you stop using. **Don't Master Reset** an original remote, and don't reset a shade, unless you mean to: either may clear pairings.

## The handheld

The handheld is a battery remote with the original's seven keys, an A/B slider, a red/green light and USB-C charging. It is designed, and the first five boards were ordered on 2026-11-17, but nothing has been powered up yet. Until the bring-up ([BRINGUP.md](BRINGUP.md)) passes, treat the design files as unproven. This page will get a handheld section once they work.

Design details: [HARDWARE.md](HARDWARE.md) (spec), [BOM.md](BOM.md) (parts), [hardware/](../hardware/README.md) (PCB), [hardware/enclosure/](../hardware/enclosure/README.md) (case).

## If it doesn't work

| Symptom | Try |
|---|---|
| `test` fails | Check the wiring and the 10 µF cap, keep SPI wires under ~15 cm, and try another module |
| `send` says there's no ID | Set one with `id <hex>` |
| Shade doesn't respond | Get closer, hold the LED still, aim straight at the sensor eye and press longer (2 s). Check the channel (A/B). If it still never responds, try another radio module: `test` only checks that the ESP32 can talk to it, not that it transmits |
| `pair` gets no jog | Wake the shade first (move it with an arrow), then retry |
| `grp` does nothing | The shade isn't paired to this ID or group: run `pair` again |

Still stuck? Open an issue on the repo with the serial output. Remove your ID from it first.
