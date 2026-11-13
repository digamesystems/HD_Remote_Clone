# Enclosure (3D-printed)

Code-CAD models for the handheld's case and test prints, written in Python with [build123d](https://github.com/gumyr/build123d). Each part is a script that produces STEP and STL files.

Status (2026-11-13), preliminary: `parts/remote_box.py` models the case as a two-part clamshell (`bottom`, `top`) plus 7 keycaps (`caps`), 150 × 45 × 27 mm, with standoffs, key pads, IR LED and USB-C openings, key and indicator holes, and a lip. Keycaps and battery ribs are modelled; the A/B slider is a placeholder in `assembly.py`. Internal parts (board, LEDs, USB, ESP32, nRF24, battery) exist only as placeholder previews used for clearance checks.

## Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-11-13 | **Material: MJF nylon (PA12)**, printed by JLC | Tough rather than brittle; takes self-tapping screws and heat-set inserts |
| 2026-11-13 | ~~Lid 2 mm thick, with a lip~~ superseded by the clamshell below; the key face stays 2 mm and the lip moved to the top shell | |
| 2026-11-13 | Key layout (preview): indicator 14.8 mm from the front, UP 32, DOWN 49 (13 × 26 mm), keys 1–4 in a row at 68 (7 × 7 mm, 9.6 mm pitch), ALL at 82 (8 × 22 mm); centres measured from the IR end | Arrows under the thumb while aiming; group keys just behind them, since an arrow follows within the ~6.5 s window; ALL apart to avoid stray presses |
| 2026-11-13 | **Board mounting: 4 separate standoffs** (5 mm, M2 self-tappers through the board, 122 × 31 mm apart), not shared with the 4 case posts | Most robust; the case can be opened without disturbing the board |
| 2026-11-13 | **Support pads under each key switch** (3.5 mm, 0.1 mm below the board, no board holes) | With support only at the corners, ~140 mm apart, a ~5 N press would flex a 1.6 mm board by roughly 1 mm (beam estimate): mushy keys, stressed solder joints. Pads only need a keep-out on the board's underside, between the switch pins |
| 2026-11-13 | **Battery under the board**, on the floor of the back section; placeholder cell 503035 (5 × 30 × 35 mm, ~500 mAh), 92–127 mm from the front. Board raised so its underside is 7.5 mm above the floor (swollen cell 5.5 + pin tips 1.5 + 0.5) | A 1000 mAh cell on top of the board took two-thirds of the back section, where the ESP32, nRF24, charger and USB parts must go, and it hit the board screws. Use is under ~1 mAh/day (estimate), so ~500 mAh is ample |
| 2026-11-13 | **Clamshell split at the IR LED axis** (14.1 mm above the bottom): bottom shell with floor, standoffs and pads; top shell with the key face and a lip along the long sides. M2 × ~8 screws from underneath (counterbored) into bosses in the top shell. USB-C opening a rounded rectangle (straight sides) | The LEDs and USB-C reach into the end walls, so the board couldn't go into an open-top box (149 mm of board and parts vs 146 mm inside). Open-top notches closed by lid tabs worked but left 11–12 mm notches and long thin tabs at both ends. The split keeps the ends solid with one straight seam; straight-sided USB opening avoids an undercut, since the USB centre is 1.4 mm below the split |
| 2026-11-13 | **IR LEDs 1.5 mm proud**, with a 1 mm × 45° countersink | Guarantees the wall can't clip the beam's edges (the LED's own lens sets its ±17° beam) |
| 2026-11-13 | **USB-C receptacle recessed 1 mm** behind the outer face; nRF24 socket at least 4.5 mm from the USB pads | Flush, the front shell-leg pads hung 0.5 mm off the board edge; the socket was 0.46 mm from the USB pads, leaving no room for D+/D−/CC/VBUS |
| 2026-11-13 | **Top face follows the original Hunter Douglas layout**: indicator at the tip, CHANNEL A/B slider (printed knob and fork over an MSK12C02 / PCM12 side-actuated switch, 1.5 mm travel), one oval for the UP/DOWN arrows (two caps, two switches), groups 1/2 and 3/4 beside the arrows, ALL below. SETUP is a floor pinhole 22 mm from the front, over a 5 mm switch on the board's underside | Familiar to anyone who has used the original (manual pp. 3–5 photos); 8 mm shorter key area than our first row layout |
| 2026-11-13 | **Keycaps**: separate parts with a 1 mm × 0.8 mm retaining flange under the face, 1.5 mm proud, 0.1 mm clear of the switch plungers; legends recessed 0.4 mm (▲/▼ on the arrows, bold 1–4 and ALL reading upright with the remote pointing away) | Caps can't fall out; no preload on the switches |
| 2026-11-13 | **Finish: natural grey shells, dyed-black caps and A/B knob, legends filled with white paint** | Close to the original's dark keys on a light body; recessed legends on black caps need the fill to be readable |
| 2026-11-13 | **Corner radius 12 mm** (was 4); corner posts move onto the corner diagonal, lip stops where the side walls start to curve | Softer, more tapered look. A sweep (4–14 mm, all fit checks at each) found 12 the largest that keeps the IR LED countersinks on the flat part of the front wall; 14 puts them on the curve |
| 2026-11-13 | **Top and bottom perimeter edges rounded 2.5 mm** (smaller than the 12 mm vertical corners) | Softer in the hand and less boxy; 1.8 mm of material remains through the curve with the 2 mm wall (measured). Much above ~3 mm the wall gets thin there |
| 2026-11-13 | **Battery held by four L-shaped corner ribs** on the floor (1.5 mm tall, 1 mm thick, 6 mm legs, 0.5 mm clearance) plus a thin (≤ 0.2 mm) double-sided tape pad; an insulating sheet (Kapton or fish paper) on the board's underside over the cell | Locates the cell without clamping it; open sides leave room for the leads, removal and swelling. Vertically the cell is already trapped between the floor and the board. Thick foam tape would push a swollen cell into the pin-tip allowance |

PCB layout constraints from the case so far: 4 × 2.2 mm holes at (±61, ±15.5) mm from the board centre with ~5.5 mm head keep-outs; board outline with 9.7 mm corner radii (following the inner wall, 0.3 mm gap) and notches (radius 4 mm) around the case posts at (±67.9, ±15.4) mm; **IR LED axis on the split plane** (footprint LED_D5.0mm_Horizontal_O1.27mm_Z3.0mm: axis 3.0 mm above the board top), LED pads 8.4 mm from the front with the tips 1.5 mm proud; USB-C face 1 mm inside the back face, board edge 0.3 mm from the inside wall; lip gap in the top shell wherever a part reaches the board edge near the split (now: the ESP32 antenna end); **battery leads**: a notch in the right-hand (−Y) board edge, 128–133 mm from the front and 3 mm deep, for the leads to pass from the cell under the board to the JST PH socket (S2B-PH-K, side entry) on top, mouth facing the front at 140 mm, centred 9 mm right of the centre line; the lip has a gap there too; bottom-side keep-outs (~4 mm) under each key switch for the pads; **no THT pins over the battery** (92–127 mm from the front, ±15 mm), or trim them flush, since only 0.5 mm separates pin tips from a swollen cell. Board top surface 11.1 mm above the case bottom, leaving 13.9 mm to the underside of the key face (nRF24 on a standard socket: 13.2 mm, too tight; plan a low-profile socket).

## Setup (Windows)

The virtual environment lives **outside Dropbox**. Dropbox locks files while pip writes them, which breaks the install, and OCP alone is several hundred MB.

```powershell
python -m venv $env:USERPROFILE\.venvs\hd_enclosure
& $env:USERPROFILE\.venvs\hd_enclosure\Scripts\python.exe -m pip install -r hardware\enclosure\requirements.txt
```

Tested with Python 3.11.9.

## Viewing models in VS Code

1. Install the **OCP CAD Viewer** extension (`bernhard-42.ocp-cad-viewer`).
2. Select the interpreter `~\.venvs\hd_enclosure\Scripts\python.exe` (Ctrl+Shift+P → *Python: Select Interpreter*). PlatformIO is unaffected; it uses its own Python.
3. Open the viewer (OCP CAD Viewer icon in the activity bar), then run a part script. A script calls `show(...)` from `ocp_vscode` to send its model to the viewer.

## Fit checks: `assembly.py`

`assembly.py` builds the case from `parts/remote_box.py`, adds placeholder models of the parts inside (board, switches, IR LEDs, USB-C, ESP32, nRF24 on its socket, battery, A/B slider, SETUP switch, indicator, keycaps) and runs every fit check: clearances, key-face webs, keycap travel and retention, legends, the A/B slider at both ends, openings on the flat end walls, USB-C routing room, and nRF24 headroom. It exits non-zero if any check fails, so run it after every change.

```powershell
$py = "$env:USERPROFILE\.venvs\hd_enclosure\Scripts\python.exe"
& $py hardware\enclosuressembly.py                          # checks + exploded view
& $py hardware\enclosuressembly.py --explode 0 --top-alpha 0.2   # assembled, see-through top
& $py hardware\enclosuressembly.py --finish                 # grey shells, black keys, white legends
& $py hardware\enclosuressembly.py --sweep CORNER_R 8 10 12 14   # rebuild + check per value
```

The internal parts are placeholders positioned from the layout explored on 2026-11-13; replace them with the board's STEP export once the PCB is laid out.

## Board model from KiCad

`kicad-cli pcb export step` exports the laid-out board with component models, so the case can be built around it. On this machine KiCad 10's CLI is at `%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe` (not on PATH).

## License

The models (scripts, `requirements.txt` and the STEP/STL output) are CERN-OHL-S-2.0, like the KiCad design (see `hardware/LICENSE`). Each script starts with an SPDX line. This README, like the project's other docs, is CC-BY-SA-4.0.
