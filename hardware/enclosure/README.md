# Enclosure (3D-printed)

> **TL;DR** (summary checked 2026-11-26): the handheld's case is a two-part 3D-printed clamshell, 150 × 45 × 27 mm, in MJF nylon (PA12), with 7 keycaps and an A/B slider. The shells split at the IR LED axis and close with M2 screws from underneath. The models are Python (build123d) scripts that output STEP and STL. `assembly.py` checks the case against the board's KiCad STEP export and lists every dimension that is still a guess. The table below records the decisions and their reasons.

Code-CAD models for the handheld's case and test prints, written in Python with [build123d](https://github.com/gumyr/build123d). Each part is a script that produces STEP and STL files.

Status (2026-11-22): `parts/remote_box.py` models the case as a two-part clamshell (`bottom`, `top`) plus 7 keycaps (`caps`), 150 × 45 × 27 mm, with standoffs, key pads, IR LED and USB-C openings, key and indicator holes (the indicator through a light pipe), a lip, battery ribs and the A/B slider (since 2026-11-21 one part that rides on the board, see Decisions). `assembly.py` checks the case against the real board's STEP export (`--board`; 76 checks pass) and against placeholder models of the parts the board has no model for. The dimensions those checks depend on are labelled measured, datasheet, guess or test print (`INPUT_SOURCES` in `assembly.py`), and every run lists the guesses. The first test print (JLC3DP, ordered 2026-11-13) arrived 2026-11-20; its results are under Test piece.

## Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-11-13 | **Material: MJF nylon (PA12)**, printed by JLC (John) | Tough rather than brittle; takes self-tapping screws and heat-set inserts |
| 2026-11-13 | ~~Lid 2 mm thick, with a lip~~ superseded by the clamshell below; the key face stays 2 mm and the lip moved to the top shell | |
| 2026-11-13 | Key layout (preview): indicator 14.8 mm from the front, UP 32, DOWN 49 (13 × 26 mm), keys 1–4 in a row at 68 (7 × 7 mm, 9.6 mm pitch), ALL at 82 (8 × 22 mm); centres measured from the IR end | Arrows under the thumb while aiming; group keys just behind them, since an arrow follows within the ~6.5 s window; ALL apart to avoid stray presses |
| 2026-11-13 | **Board mounting: 4 separate standoffs** (5 mm, M2 self-tappers through the board, 122 × 31 mm apart), not shared with the 4 case posts | Most robust; the case can be opened without disturbing the board |
| 2026-11-13 | **Support pads under each key switch** (3.5 mm, 0.1 mm below the board, no board holes) | With support only at the corners, ~140 mm apart, a ~5 N press would flex a 1.6 mm board by roughly 1 mm (beam estimate): mushy keys, stressed solder joints. Pads only need a keep-out on the board's underside, between the switch pins |
| 2026-11-13 | **Battery under the board**, on the floor of the back section; placeholder cell 503035 (5 × 30 × 35 mm, ~500 mAh; replaced by the chosen cell, see 2026-11-14), 92–127 mm from the front. Board raised so its underside is 7.5 mm above the floor (swollen cell 5.5 + pin tips 1.5 + 0.5) | A 1000 mAh cell on top of the board took two-thirds of the back section, where the ESP32, nRF24, charger and USB parts must go, and it hit the board screws. Use is under ~1 mAh/day (estimate), so ~500 mAh is ample |
| 2026-11-13 | **Clamshell split at the IR LED axis** (14.1 mm above the bottom): bottom shell with floor, standoffs and pads; top shell with the key face and a lip along the long sides. M2 × ~8 screws from underneath (counterbored) into bosses in the top shell. USB-C opening a rounded rectangle (straight sides) | The LEDs and USB-C reach into the end walls, so the board couldn't go into an open-top box (149 mm of board and parts vs 146 mm inside). Open-top notches closed by lid tabs worked but left 11–12 mm notches and long thin tabs at both ends. The split keeps the ends solid with one straight seam; straight-sided USB opening avoids an undercut, since the USB centre is 1.4 mm below the split |
| 2026-11-13 | **IR LEDs 1.5 mm proud**, with a 1 mm × 45° countersink. Reviewed and **kept for version 1** (John): most IR margin first; the front end is exposed in a drop, so if drops damage them, the fallback is flush tips with a deeper/wider countersink | Guarantees the wall can't clip the beam's edges (the LED's own lens sets its ±17° beam). The original has one LED behind its tip; we have two |
| 2026-11-13 | **USB-C receptacle recessed 1 mm** behind the outer face; nRF24 socket at least 4.5 mm from the USB pads | Flush, the front shell-leg pads hung 0.5 mm off the board edge; the socket was 0.46 mm from the USB pads, leaving no room for D+/D−/CC/VBUS |
| 2026-11-13 | **Top face follows the original Hunter Douglas layout**: indicator at the tip, CHANNEL A/B slider (printed knob and fork over an MSK12C02 / PCM12 side-actuated switch, 1.5 mm travel), one oval for the UP/DOWN arrows (two caps, two switches), groups 1/2 and 3/4 beside the arrows, ALL below. SETUP is a floor pinhole 22 mm from the front, over a 5 mm switch on the board's underside (moved 10 mm right of centre on 2026-11-13, see below) | Familiar to anyone who has used the original (manual pp. 3–5 photos); 8 mm shorter key area than our first row layout |
| 2026-11-13 | **Keycaps**: separate parts with a 1 mm × 0.8 mm retaining flange under the face, 1.5 mm proud, clear of the switch plungers (see the review changes below for the play and legend depth); legends recessed (▲/▼ on the arrows, bold 1–4 and ALL reading upright with the remote pointing away) | Caps can't fall out; no preload on the switches |
| 2026-11-13 | ~~**Finish: natural grey shells, dyed-black caps and A/B knob, legends filled with white paint** (John)~~ superseded 2026-11-21 (see below) | Close to the original's dark keys on a light body; recessed legends on black caps need the fill to be readable |
| 2026-11-13 | **Corner radius 12 mm** (was 4); corner posts move onto the corner diagonal, lip stops where the side walls start to curve | Softer, more tapered look. A sweep (4–14 mm, all fit checks at each) found 12 the largest that keeps the IR LED countersinks on the flat part of the front wall; 14 puts them on the curve |
| 2026-11-13 | **Top and bottom perimeter edges rounded 2.5 mm** (smaller than the 12 mm vertical corners) | Softer in the hand and less boxy; 1.8 mm of material remains through the curve with the 2 mm wall (measured). Much above ~3 mm the wall gets thin there |
| 2026-11-13 | **Battery held by four L-shaped corner ribs** on the floor (1.5 mm tall, 1 mm thick, 6 mm legs, 0.5 mm clearance) plus a thin (≤ 0.2 mm) double-sided tape pad; an insulating sheet (Kapton or fish paper) on the board's underside over the cell | Locates the cell without clamping it; open sides leave room for the leads, removal and swelling. Vertically the cell is already trapped between the floor and the board. Thick foam tape would push a swollen cell into the pin-tip allowance |
| 2026-11-13 | **Review changes** (external review of the first cut): legends recessed **0.8 mm** (was 0.4), digits 4.5 mm and ALL 4.0 mm; keycap play at rest **0.3 mm** (was 0.1), with a 0.2 mm relief pocket in each flange over the plunger so the flange stays 0.8 mm; an **outside pocket round the USB-C opening** (12.5 × 7 mm, 1 mm deep) so a plug's moulded housing can sink in by the 1 mm recess | MJF's grain fills fine recesses (≥ 0.8 mm wide × deep is the guideline the review cited); ±0.3 mm per part can eat a 0.1 mm gap, and shims under the board can only reduce it, so start large and tune on the print; without the pocket a plug housing hits the case 1 mm before the plug seats. All three get test-print samples |
| 2026-11-13 | **SETUP pinhole 10 mm right of centre** (`SETUP_Y = -10`), still 22 mm from the front | Found at PCB placement: on the centre line, the underside SETUP switch's through-hole pins land on the A/B switch's pads (26 mm), and nowhere between A/B and the indicator (14 mm) fits a 6 mm switch without hitting one of them. KiCad DRC is clean from 8 mm off-centre; 10 leaves margin. The case checks can't see this kind of clash, since the parts sit on opposite sides of the board |
| 2026-11-13 | **ESP32 antenna end at the −Y (right-hand) board edge**, was +Y; the lip gap moved with it (`LIP_GAPS`) | Found at PCB routing: the key GPIOs must be RTC pins, all in the module's left pin column, which faced the back; the nRF24 SPI pins faced the front. Rotating the module 180° stops the two bundles crossing. All 51 case checks pass |
| 2026-11-14 | **Cell chosen (John): Adafruit #1578** (500 mAh, 29 × 36 × 4.75 mm). Model now `BATT_W, BATT_L = 29, 36` (rib pocket 30 × 37 mm); `BATT_T` stays 5.0 as a thickness allowance; front edge still 92 mm from the front, so the cell now ends 128 mm from the front | In stock, protected, documented JST-PH polarity, rated for our ~256 mA charge. Keeping `BATT_T` leaves the board height, and the ordered test piece, unchanged. The board's battery keep-out grew 1 mm toward the back to match. Measure the real cell on arrival and set the ribs from it. All 51 case checks pass |
| 2026-11-14 | **Charge LED window**: a 1.6 mm hole through the back wall at Y +7.75 (`CHG_Y`), 0.3 mm above the board top (`CHG_Z`), in the bottom shell | The charge LED is now a side-view part at the board's back edge (the case had no window for the old top-view LED). The hole sits on the flat wall (edge 8.6 of ±10.5 mm) and 0.7 mm clear of the USB-C plug pocket. Three new checks; all 54 pass |
| 2026-11-15 | **Keys 1–4 are rounded squares** (8 × 8 mm, `KEY_CORNER_R` 2 mm; were 8 mm circles) | A round cap in a round hole can turn, and its number with it; the corners stop it, leaving a few degrees of play at the 0.3 mm clearance. `assembly.py` now fails if any key outline is a circle. The ordered test piece still has a round 8 mm cap: its results for plunger play, clearance and feel still apply |
| 2026-11-15 | **Indicator through a light pipe** (John): the WP59EGW LED sits flush on the board and a Bivar PLP2-250 (3 mm, 6.35 mm long, 3.3 × 0.8 mm head) presses into a `PIPE_HOLE_D` 2.92 mm hole in the key face; the old 5.2 mm hole for a 5 mm LED standing 2 mm proud is gone. `assembly.py` checks the pipe-to-LED gap (0.95 mm; 0.5–1.5 wanted) and the face thickness against the pipe's panel range | A free-standing LED needed ~9 mm of bare lead and a spacer to hit its height, and stuck out of the face. Flush LED + pipe: each part has one fixed position, nothing to adjust, a low head that suits drops. MJF hole size varies more than the pipe's +0.08/−0.05 mm fit window: test-fit on the next print, glue if loose |
| 2026-11-15 | **Battery pocket 31 mm** (`BATT_W` 30): the cell datasheet says 30 ± 0.1 mm wide, not Adafruit's 29 | 0.5 mm play per side for a 30 mm cell, 1.0 for 29; never a press fit. Board keep-out was already 31 mm. Measure the real cell, including the protection-board bulge |
| 2026-11-14 | **A/B slider drive: a solid block with a notch** (`AB_BLOCK_T, AB_BLOCK_W = 2.4, 4.6`, `AB_NOTCH_PLAY = 0.15`), replacing the thin stem with two 1.2 × 1.6 mm prongs | The prongs were fragile in MJF PA12, most at risk in blind assembly (the fork must land round a 1.3 mm actuator you can't see) and in drops. The notch walls are now 1.5 mm wide and the block's full 2.4 mm thick; the block stays inside the switch body's width at both ends of travel, clear of its mounting legs. New check: the notch holds the actuator with 0.15 mm play each side; all 55 pass. **Deferred until the test print shows real fit and finish** (John: a little sloppy is fine for the first one): make the slot the hard stop (knob travel = switch travel + notch play, ~1.8 mm, instead of 2.1, so end-of-travel force goes into the case, not the switch) and a ~0.3 mm × 45° lead-in chamfer on the notch mouth |
| 2026-11-16 | **nRF24 socket: Samtec SLW-104-01-G-D** (4.57 mm), with each module's header pins trimmed to ~2.6 mm (John) | The module measured 0.8 mm board, 3.0 mm header spacer and a crystal 3.5 mm above the board. On a standard 8.5 mm socket the crystal sits 1.9 mm into the key face; the SLW leaves 2.0 mm. Rejected: a raised area on the top shell (looks awkward) and soldering the module directly (antenna ~8 mm from the cell). Open: a hold-down rib under the face against drops |
| 2026-11-17 | **Relief flat on the back, −Y corner post**, beside the battery connector J102: one flat 2.3 mm from the post axis (`POST_RELIEF_D`), from 1.0 mm below the board underside (`POST_RELIEF_BELOW`, the top of the board's standoff under H604) up to 7 mm above the board top (`POST_RELIEF_H`) (John) | J102's body hangs 0.7 mm over the board's post notch (the notch is there for the post, so that is fine while it clears) and sat 0.35 mm from the 6 mm post, less than print and soldering tolerances allow for. The flat leaves 1.05 mm. The corner posts don't support the board, so clearance wins over post strength. Below the board top the screw's counterbore (2.1 mm radius) leaves only a 0.2 mm skin against the flat; it carries no load (the screw clamps through the post above the counterbore's ledge), but MJF may not hold it: check it on the first print. Chosen over moving the connector on the board, which would change the JLC files. New checks: the connector clears its post by at least 0.8 mm; the relief reaches at least 1 mm below the standoff top |
| 2026-11-21 | **From the test print** (John): IR LED holes **5.2 mm** (`LED_HOLE_D`, was 5.4); legends **1.0 mm deep, digits 5.5 mm, ALL 4.5 mm** (was 0.8, 4.5, 4.0: the deepest and largest practical); **keep the case size** and the 0.3 mm board-to-wall gap, with margin checks added (candidate E; A to D dropped) | 5.2 fit his LEDs best without slop; the 0.8 mm legends read best of the three depths and the shallower ones were hard to see; the print came out the same size or slightly larger, so the gap opens rather than closes (see Test piece). New checks: board edge to the walls ≥ 0.3 mm on the model; every real board part ≥ 0.5 mm from the case and loose parts, except a listed few (`CLEAR_EXCEPTIONS`: IR LEDs in their holes, plungers under caps, the A/B actuator, and the ESP32 and J102 at the board edge, ≥ 0.2 mm). All 65 pass with the board STEP. Unchanged and confirmed: cap play 0.3, standoff holes 1.8 with M2 × 6, SETUP pinhole 1.6, the USB-C pocket |
| 2026-11-21 | **A/B slider rides on the board** (John), replacing the hanging drive block of 2026-11-14: one printed part whose drive block reaches down and slides on the board's solder mask, with every bottom edge of both teeth rounded 0.4 mm (`AB_FOOT_R`, also a lead-in onto the actuator); block narrowed to 4.0 mm (`AB_BLOCK_W`, was 4.6); notch roof 0.6 mm above the actuator (`AB_NOTCH_ROOF`); 0.5 mm between the plate and the face (`AB_FACE_PLAY`). **Knob, slot and plate centred over the drive block** (`AB_KNOB_SHIFT` 2.8 mm toward the back; the slot is now 28.8 mm from the front), with the arrow oval's front end trimmed 0.3 mm (`ARROWS` (50.15, 0, 33.7, 14), was (50, 0, 34, 14)). **A support pad under the foot** (`AB_PAD_XY`), like the key pads | The old block hung in the air: the slider rested on the switch's actuator, so a press loaded the switch vertically, and the block sat about 0.1 mm off the board. Now a press goes knob → foot → board → pad → floor, and the switch only ever feels the sideways push. The foot crosses only the AB_SW trace, under mask. Considered and dropped the same day: a separate keeper pressed or heat-staked onto pegs (an extra part, staking and tight tolerances). The column removes the offset that made the slider rock; the trim keeps 2.1 mm between the slot and the arrow hole. New checks: the foot clears the switch's mounting-tab pads (0.41 mm, want ≥ 0.3), the notch roof clears the actuator, the notch still grips with the slider lifted, and the pad clears the switch's locating-peg holes (0.69 mm). All 72 pass with the board STEP. A later board could turn SW509 round for more room (docs/FEATURE_IDEAS.md) |
| 2026-11-21 | **Finish: dyed-black shells, natural grey keys and A/B slider, legends unfilled** (John), replacing the 2026-11-13 finish | Black hides smudges; on grey caps the 1.0 mm recesses read by shadow (Blender render), and white fill would barely show on grey |
| 2026-11-21 | **Case order: five cases, after the first PCB arrives and is measured** (John) | The fitted board settles the outline, pin protrusion and slider clearances first; five in one shipment roughly halves the cost per case |
| 2026-11-21 | **Counterbore 4.0 mm** (`CB_D`, was 4.2) for a 1.0 mm corner-post wall, JLC's minimum; **J102 post: cut through** (`POST_WINDOW_D` 1.7), a window into the counterbore instead of the 0.2 mm skin, stopping 0.2 mm under the screw-head seat; **light pipe hole stays 2.92 mm** (datasheet), reamed or glued on the first case if needed (all John) | The M2 heads measure 3.2 mm, so 4.0 leaves 0.4 mm per side. A skin that thin may not print; a deliberate opening prints cleanly and carries no load. New checks: post wall, head clearance, window open and below the seat (probed on the solid: no material where the skin was, the seat intact). All 76 pass with the board STEP |
| 2026-11-22 | **1.8 mm pilots for the M2 self-tappers** (John), both the board standoffs (`SO_HOLE_D`) and the case-screw bosses (`PILOT_D`) | Test piece (John): 2.0 mm cut too easily, 1.6 mm made the driver slip, 1.8 mm had a good bite. The model already used 1.8, so no geometry changes |

PCB layout constraints from the case so far: 4 × 2.2 mm holes at (±61, ±15.5) mm from the board centre with ~5.5 mm head keep-outs; board outline with 9.7 mm corner radii (following the inner wall, 0.3 mm gap) and notches (radius 4 mm) around the case posts at (±67.9, ±15.4) mm; **IR LED axis on the split plane** (footprint LED_D5.0mm_Horizontal_O1.27mm_Z3.0mm: axis 3.0 mm above the board top), LED pads 8.4 mm from the front with the tips 1.5 mm proud; USB-C face 1 mm inside the back face, board edge 0.3 mm from the inside wall; lip gap in the top shell wherever a part reaches the board edge near the split (now: the ESP32 antenna end, at the −Y edge since 2026-11-13); **battery leads**: a notch in the right-hand (−Y) board edge, 128–133 mm from the front and 3 mm deep, for the leads to pass from the cell under the board to the JST PH socket (S2B-PH-K, side entry) on top, mouth facing the front at 140 mm, centred 9 mm right of the centre line; the lip has a gap there too; bottom-side keep-outs (~4 mm) under each key switch for the pads; SETUP switch on the underside at 22 mm from the front, 10 mm right of centre; **no THT pins over the battery** (92–127 mm from the front, ±15 mm), or trim them flush, since only 0.5 mm separates pin tips from a swollen cell. Board top surface 11.1 mm above the case bottom, leaving 13.9 mm to the underside of the key face (nRF24 on a standard socket with the module measured 2026-11-16: 15.8 mm, 1.9 mm into the face; **decided 2026-11-16 (John): Samtec SLW-104-01-G-D**, 4.57 mm, module pins trimmed: 11.9 mm, 2.0 mm clear. Open: a hold-down rib for drops).

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

`assembly.py` builds the case from `parts/remote_box.py`, adds placeholder models of the parts inside (board, switches, IR LEDs, USB-C, ESP32, nRF24 on its socket, battery, A/B slider, SETUP switch, indicator, keycaps) and runs every fit check: clearances, key-face webs, keycap travel and retention, legends, the A/B slider at both ends, openings on the flat end walls, USB-C routing room, and nRF24 headroom. It exits non-zero if any check fails, so run it after every change. `--nrf-socket-h <mm>` sets the nRF24 socket height (default 4.57, the Samtec SLW; 8.5 is a standard header, which fails the key-face check), and every run ends with a list of the inputs that are still guesses or wait for the test print.

```powershell
$py = "$env:USERPROFILE\.venvs\hd_enclosure\Scripts\python.exe"
& $py hardware\enclosure\assembly.py                          # checks + exploded view
& $py hardware\enclosure\assembly.py --explode 0 --top-alpha 0.2   # assembled, see-through top
& $py hardware\enclosure\assembly.py --finish                 # black shells, grey keys, unfilled legends
& $py hardware\enclosure\assembly.py --sweep CORNER_R 8 10 12 14   # rebuild + check per value
& $py hardware\enclosure\assembly.py --board hardware\enclosure\out\hd_remote_board.step   # the real board
```

With `--board`, the real board (a KiCad STEP export) replaces the placeholders it covers (PCB, key switches, IR LEDs, ESP32, A/B switch, SETUP switch, indicator, nRF24 socket, battery JST) and every part is clash-checked. Export it after any board change; `out/` isn't committed. From this folder:

```
kicad-cli pcb export step --force --user-origin 100x80mm --subst-models -o out/hd_remote_board.step ../hd_remote/hd_remote.kicad_pcb
```

The remaining placeholders (USB-C, whose model KiCad's library lacks, the battery and its leads, the keycaps and the nRF24 module) follow the layout explored on 2026-11-13 and the module measured on 2026-11-16. KiCad's 3D model for J401 is the standard socket scaled to the SLW's 4.57 mm height, since the library has no SLW model.

## Case assembly

After the board is assembled (see `docs/HARDWARE.md`, Board assembly):

1. Stick a thin (≤ 0.2 mm) double-sided tape pad on the floor and press the cell down between its four corner ribs, leads toward the back.
2. Put Kapton tape or fish paper on the board's underside over the cell area.
3. Feed the battery leads up through the notch in the right-hand board edge. Lower the board onto its four standoffs and fit the M2 self-tapping screws.
4. Plug in the battery JST and the nRF24 module.
5. Set the A/B switch on the board to A. Drop the keycaps (flanges down) and the A/B slider (knob at A) into the top shell from inside.
6. Lower the top shell onto the bottom: the slider's rounded foot lands on the board with its notch round the switch actuator (the rounding guides it on). Turn it over, check the slider moves A to B and back, and fit the four M2 case screws from underneath.

## Before ordering the cases (checklist, 2026-11-21)

What stands between now and a case order. Items are open unless ticked; the recommendations are Claude's, not decisions.

**Decisions for John**
- [x] **Finish (decided 2026-11-21, John):** dyed-black shells, natural grey keys and A/B slider, legends unfilled.
- [x] **Timing (decided 2026-11-21, John):** order after the first PCB arrives and is measured.
- [x] **Quantity (decided 2026-11-21, John):** five cases. Estimates in `docs/COSTS.md`: about $32–35 each in one shipment, plus about $20–25 of DHL duty per shipment (the test piece's).

**Design items still open**
- [x] **South-west post relief (decided 2026-11-21, John): cut through.** A window into the counterbore replaces the 0.2 mm skin (`POST_WINDOW_D`); the screw-head seat stays whole.
- [x] **Corner post walls (decided 2026-11-21, John: 1.0 mm if the checks allow):** done by narrowing the counterbore to 4.0 mm (`CB_D`, was 4.2), 0.4 mm clear of the 3.2 mm heads; the posts keep their size.
- [x] **A/B hard stop (decided 2026-11-26, John): leave it as is; the slider is rarely moved.** The slot allows 2.1 mm of knob travel against the 1.8 mm needed (1.5 mm switch travel plus 0.3 mm notch play), so the switch's own end stops take the push at each end. A slot cut to 1.8 mm would put the stop inside MJF's ±0.3 mm tolerance and could keep the switch from reaching its end position. Check on the first case: the switch clicks fully at both ends.
- [x] **nRF24 hold-down rib (decided 2026-11-26, John): no rib.** Worst case, tape holds the module.
- [x] **Case geometry for the battery leads (decided 2026-11-26, John):** keep the J102 post relief, its window and the −Y lip gap, so a board fits either way. John decides at assembly, board by board, whether to fit the connector or solder the leads. Claude's reading: the relief isn't a structural risk. It's on one post, the head seat stays whole, the remaining wall passes the 1.0 mm check, and the screw clamps through solid post above the counterbore. Snug that screw rather than torquing it.
- [ ] **Battery leads per board: connector, shortened leads, or soldered straight to the board** (raised 2026-11-26; the method for each board is decided at assembly, John). Background:
  - **The problem:** the cell's leads are about 11 cm long, and the route through the board notch to J102 needs about 3 cm. Pushed under the board, the extra 8 cm goes into the gap beside the cell, directly under the ESP32's antenna (89–107 mm from the front, at the −Y edge). John spotted this in the viewer. The handheld doesn't use Wi-Fi today, but the bridge build and the setup page in `docs/FEATURE_IDEAS.md` would.
  - **Option A, keep J102 and shorten the leads** to about 4.5 cm: a re-crimped PH plug, or a solder splice with heat-shrink placed by the notch, behind the antenna. The case stays as it is.
  - **Option B, no connector:** solder the leads into J102's plated holes, fed up from **underneath** so they never cross the top of the board. The leads are about 2 cm plus about 1 cm of slack to tilt the board during assembly. Strain relief: tack the wires to the underside a few mm from the joints (hot glue or Kapton).
    - For: no connector or plug height, no extra lead, nothing to work loose, one hand-fitted part fewer.
    - Against: the cell and board become one unit. Replacing the cell means desoldering a live cell, one wire at a time.
  - **How often a cell needs replacing** (Claude's estimate): one or two charge cycles a year (`docs/HARDWARE.md`), so cycle wear doesn't matter. Age sets the life, typically 4–6 years at room temperature, and visible swelling would be the usual sign.
  - **What option B would change in the case:** the J102 post relief flat and its window into the counterbore exist only for the connector body (decided 2026-11-17 and 2026-11-21, John). The lip gap at 127–134 mm on the −Y side (`LIP_GAPS`) exists only for the leads crossing the board edge. Without the connector, both could go, and the post would get its full wall back. The PCB's lead notch stays on rev 1 and does no harm. On a rev 2 board, J102 could become two plain pads and the notch could go.
  - **Still to work out:** which boards get which build (board 1 might keep its connector for bring-up). The case keeps the relief and lip gap either way (above). If any board is soldered directly, add that route to the assembly steps; the model's lead route (over the board to the plug) stays as the case's worst case.
- [x] **Screw pilots (decided 2026-11-22, John):** 1.8 mm for the board standoffs and the case-screw bosses, from the test piece's three pilot sizes.
- [x] **Light pipe hole (decided 2026-11-21, John):** the datasheet's 2.92 mm; ream it or use a touch of glue on the first case if the fit is off.

**Measurements from parts arriving** (they replace the inputs `assembly.py` lists as guesses)
- [x] **PCB outline and thickness** against 145.4 × 40.4 × 1.6 mm: the last fit-margin check (Fit margins, below). **Board 1 measures 145.5 × 40.4 × 1.6 mm** (John, 2026-11-26): 0.1 mm long, inside the ±0.1 mm outline tier. With the test print's scale, about 0.45 mm per end and 0.30 mm per side remain (Claude's arithmetic, under Measurements that decide it).
- [ ] **Battery cell and plug** (Adafruit #1578, due 2026-11-25): the cell's size for the ribs, the plug's envelope (`PLUG_L`).
  - **Cell:** 35.5 × 29.4 × 5.0 mm (John, 2026-11-26). It's inside the modelled 36 × 30 × 5.0, so the rib pocket (37 × 31 mm) leaves about 0.75 mm at each end and 0.8 mm each side; the tape pad holds it. Left at the datasheet size so other cells still fit (Claude's recommendation).
  - **Leads** (John, 2026-11-26, with a photo): about **11 cm** long. They leave at one corner of the end with the protection board (the 29.4 mm end). The cell's label reads PKCELL LP503035, 3.7 V 500 mAh. In the model, the run from the cell's back end through the board notch to J102 is about 3 cm (Claude's estimate), so about 8 cm is extra. Open: shorten the leads or stow the extra.
  - **J102** (a loose S2B-PH-K, John, 2026-11-26): 4.8 mm tall and 5.5 mm wide, against the 6.0 and 5.9 modelled (`JST_H` now 4.8; the width stays 5.9, the footprint's). The cell's plug snaps in securely.
  - **Plug** (estimated by Claude from John's side photo of a mated pair, 2026-11-26, scaled by J102's 4.8 mm height): the housing reaches about **2.2 mm** out of J102's mouth, about 4 mm tall, and the wires leave straight out of it, low down. `PLUG_L` is now 3.0, from that estimate plus margin (was a 6.0 guess); `PLUG_H` stays 4.5. A caliper reading would replace the estimate.
- [x] **M2 screw heads:** **3.2 mm** across, **1.3 mm** tall (measured by John, 2026-11-21; `HEAD_D`, `HEAD_H`), 0.4 mm clear in the 4.0 mm counterbore.
- [ ] **On the first fitted board:** THT pin protrusion below the board (`PIN_PROTRUDE`), the IR LED tip positions after bending, the A/B actuator's thickness (`AB_SW_ACT`), and the solder on SW509's mounting tabs beside the slider's foot.
  - Board 1 (John, 2026-11-26): the key and SETUP leads stick out no more than 1 mm (under the 1.5 mm allowance). SW509's metal can stands about **1.7 mm** above the board, against the datasheet's 1.4 (`AB_SW_BODY` updated; the notch roof follows it). There's no obvious solder in the slider's path. In his photo, the tab fillets stay on their pads (Claude's reading). The actuator is about **1.0 mm** thick, as modelled (John, 2026-11-26). **Key plunger tops** above the board, five of the seven DigiKey TS02 keys (John, 2026-11-26): 13.3, 12.9, 13.2, 13.0 and 13.3 mm, against `SW_H` 13.0. With the 0.3 mm modelled cap play, that gives about 0.0–0.4 mm of play at rest (Claude's arithmetic), so the two 13.3 mm keys have none. Not recorded: which keys these are, and whether each switch body sits flat on the board. **J101 overhangs the board edge by 1.3 mm** (John, 2026-11-26), which is what the model gives (2.3 mm from the outer wall to the board edge, minus the 1.0 mm `USB_RECESS`). The recess and the 1.0 mm wall behind the plug pocket come out as designed. **The SETUP switch's plunger tip is 4.5 mm below the board** (John, 2026-11-26). The board's underside is 7.5 mm above the floor, which leaves about 3.0 mm of clearance. A paperclip reaches the plunger about 5 mm in from outside. Still open: the IR LEDs, and the leads of J102 and D501 once they're fitted.

**Decided 2026-11-26 (John):** order the cases without first measuring the IR LEDs, D501, the trimmed nRF24 module or the remaining pin tips on board 1. Those parts are fitted to the printed case when it arrives, using it as the jig.

**Preparing the order**
- [ ] Apply what's decided, rerun `python assembly.py --board out/hd_remote_board.step`: all checks pass, and no guess left that a check depends on.
- [ ] Export STL and STEP from a tagged commit and check the meshes are watertight. Expect JLC's "< 0.8 mm wall" warning on the engraved legends; it's harmless (the test piece had it).
- [ ] Get a JLC3DP quote for the real parts to replace the estimates. Try uploading the caps and slider as one file (about $6 a case less, if JLC accepts it).
- [ ] Record the order in `docs/COSTS.md` and in this README.

## Test piece: `parts/test_piece.py`

One small MJF print that replaces the model's placeholder values with measured ones. All sizes come from `remote_box.py`, so the samples test what the case uses. `python parts/test_piece.py --export` runs its checks (23) and writes STL and STEP files to `out/` (not committed; rerun to regenerate). Order in **MJF PA12, natural grey**, the case material. **First print ordered 2026-11-13 from JLC3DP** (STLs exported from commit `76482ef`; all seven meshes checked watertight).

| Sample | How to test | Sets |
|---|---|---|
| IR LED holes 5.2 / 5.4 / 5.6 mm, with the outside countersink | Push a TSAL6200 into each from inside: want a snug slide, no wobble, no force | `LED_HOLE_D` |
| USB-C opening + outside pocket | Push each of your real USB-C cables in from outside: the plug housing should sit in the pocket. Final seating is checked on the first board, with the receptacle 1 mm inside the wall | `USB_POCKET_*`, `USB_RECESS` |
| Screw bosses, pilots 1.6 / 1.8 / 2.0 mm | Drive an M2 self-tapper into each: want firm threads, no splitting, removable | `SO_HOLE_D`, `PILOT_D` |
| SETUP pinholes 1.4 / 1.6 / 1.8 mm | A straightened paperclip should pass freely, a fingertip shouldn't press anything | `SETUP_HOLE_D` |
| Legends 0.4 / 0.6 / 0.8 mm deep | Fill with white paint, scrape, compare legibility | `LEGEND_DEPTH`, sizes |
| Key stack: base + face + caps marked .0 / .1 / .2 / .3 (mm of play) | Fit one 6 mm × 13 mm tactile switch (legs through the base), screw on the face, try each cap: want no click at rest and a crisp click without a dead zone | `CAP_PLAY` |

The key fixture reproduces the case's 0.90 mm plunger-to-face gap, so the best cap's play transfers directly. Shims under the board can only reduce the play in the real case.

### First print: results (arrived 2026-11-20; observations by John, 2026-11-21)

JLC's file check had flagged every part with curved or multi-letter engraving (the plate, the key base, caps .0, .2 and .3) for walls under 0.8 mm; the key face and cap .1, with no such engraving, weren't flagged. The likely cause is the plastic between and inside engraved strokes, not a structural wall (inference, not checked). The parts were printed anyway.

| Sample | John's observation | Model now |
|---|---|---|
| Surface | Light, grainy finish with no visible printer tracks. He likes the texture: it should give a good grip and probably hold paint well | n/a |
| Legends | The 0.8 mm samples are the best; "ALL 4" is very readable. The shallower ones are harder to make out in normal lighting. His view: use the deepest and largest legends practical on the keys | `LEGEND_DEPTH` 0.8 (now 1.0) |
| Key stack | Worked well with his 13 mm 6 × 6 switches: the travel is right and the click is satisfying. The 0.3 mm cap worked well, and it isn't loose side to side in the key face's hole | `CAP_PLAY` 0.3, `CAP_CLEAR` 0.3 |
| Screws | The key fixture's four posts held M2 × 6 screws very well. Those posts have the case's standoff hole (1.8 mm) in a 6 mm post, the same as the case screw pilots in the 6 mm top bosses, so this covers `PILOT_D` too (the case's M2 × 8 gets about 1 mm more thread). The plate's three pilot sizes, tried 2026-11-22 (John) with M2 screws: **2.0 mm** taps easily, maybe too easily; **1.8 mm** is fairly easy with a good bite while cutting the threads; **1.6 mm** is tougher, and the screwdriver occasionally slipped | `SO_HOLE_D`, `PILOT_D` 1.8 |
| USB-C | His cables fit | `USB_POCKET_*` |
| SETUP pinholes | The middle one (1.6 mm) suits his usual paperclips | `SETUP_HOLE_D` 1.6 |
| IR LED holes | The smallest (5.2 mm) fits his LEDs well without being sloppy | `LED_HOLE_D` 5.4 (now 5.2) |

**Caliper readings (John):**

| Feature | Modelled | Measured | Difference |
|---|---|---|---|
| Plate length | 110.0 | 110.3 | +0.3 mm (+0.27 %) |
| Plate width | 46.0 | 46.0 | 0 |
| Wall thickness | 2.0 | 1.9–2.0 | 0 to −0.1 mm |
| Wall length | 86.0 | 86.0 | 0 |
| Boss diameter | 5.0 | 4.9 | −0.1 mm |

**Reading against the 0.3 mm board-to-wall gap** (Claude's analysis, one print only): the print came out the same size or slightly larger outside, and its walls the same or slightly thinner, so a case printed the same way would have a cavity the same size or larger than modelled, not smaller. Scaled from the plate, the 146 mm cavity grows by about 0.4 mm (about +0.2 mm per end), and thinner walls add up to 0.1 mm per side. With the board outline at the ±0.1 mm tier, worst case per side: length 0.30 + 0.2 − 0.05 ≈ **+0.45 mm**, width 0.30 + 0 − 0.05 = **+0.25 mm**. Both clear the 0.2 mm threshold under "How to read them" below. Caveats: one print of a smaller part; the shells are larger and hollow, so they may shrink or warp differently, and a closed cavity isn't the same as an outside dimension.

Also from John (2026-11-21): the finish reminds him of sand casting, "which isn't bad". The test piece's round 8 mm caps spin freely in their holes, which bears out the rounded-square keys 1–4 (decision of 2026-11-15).

Not reported yet: the paint fill. Not testable on this piece: the light pipe hole (`PIPE_HOLE_D`, test-fit or ream on the first case).

## Fit margins: findings and candidate changes (2026-11-17)

Written after the board went into JLC's placement preview. **Everything under "Candidate changes" is a proposal, not a decision: the model hasn't been changed, and each item waits for measurements from the test print and the first boards.** Only the post relief (see the decision table) has been decided and built.

### Findings

| # | Finding | Why it matters |
|---|---|---|
| 1 | **Board edge to the case's inner wall is 0.30 mm on all four sides**, and around the corners (PCB 145.4 × 40.4 mm in the STEP, inner cavity 146.0 × 41.0 mm; `PCB_GAP`). | It's a nominal value with no margin. A routed outline that comes out a little large plus a printed cavity that comes out a little small can close it, and the board then binds on the wall instead of dropping onto its standoffs. The board's 2.2 mm screw holes leave only about 0.1 mm of slack, so nothing else steers it. JLC's stated tolerances (next section) can use up the whole gap in the length direction. |
| 2 | **J102 (battery connector) was 0.35 mm from the back corner post** (measured on the real KiCad model; the placeholder gave the same). Its body overhangs the board edge by 0.7 mm at the post notch. | Nothing overlapped, so the case checks passed, but 0.35 mm is below the tolerances. Fixed by the post relief (decided 2026-11-17, John). |
| 3 | **The checks measure overlap, not margin.** `check_board` fails only on more than 0.01 mm³ of overlap, and KiCad DRC reports a part crossing the board edge only as a silkscreen warning. | A connector 0.3 mm from a post passes like one 5 mm away. J102's warning was filed with the intentional overhangs (USB-C, IR LEDs). |
| 4 | **Courtyards outside the board outline:** U201 (the ESP32's antenna keep-out, by design), D303 and D304 (IR LEDs, by design), J101 (USB-C, by design), and **J102 (not intended: the 0.7 mm at the notch)**. | The first four are the expected overhangs; anything else should be an error. |
| 5 | **The USB-C recess is tied to the board's distance from the back wall.** The receptacle sits 1.0 mm behind the back face and the plug pocket is that deep, so the wall behind the pocket is 1.0 mm; the check wants at least 0.8 mm. | Any increase in the gap between the board and the back wall deepens the pocket one for one. Only about 0.2 mm is available before that wall is too thin. |
| 6 | **The board is 1.51 mm thick in KiCad's STEP**, but the case model (`PCB_T`) and JLC's standard thickness are 1.6 mm. | Not fit-critical (0.09 mm), but KiCad's board setup should say 1.6 so the export matches what arrives. |

### Vendor tolerances (JLC's published figures, checked 2026-11-17)

Until now the case and board were sized from remembered figures. These are from JLC's own pages (see the sources below the table), compared with what the design asks of them.

| Spec | JLC's figure | Our design | Reading |
|---|---|---|---|
| MJF PA12 dimensional tolerance | ±0.3 mm within 100 mm; **±0.4 % above 100 mm** (assumes no warping, deformation, painting or oil spraying) | Inner cavity 146.0 mm long (±0.58 mm, so ±0.29 mm per end) and 41.0 mm wide (±0.3 mm, so ±0.15 mm per side) | The length is over 100 mm, which roughly doubles the tolerance |
| MJF PA12 minimum wall | 1.0 mm at 5 × 5 mm, 1.2 at 10 × 10, 1.5 at 50 × 50, 2.0 at 100 × 100 and at 200 × 200; more than 1.5 mm recommended for protrusions, positioning features, snaps and fasteners | Walls and floor 2.0 mm (at the large-part minimum, no margin). **Corner posts: a 0.9 mm tube round the counterbore, on all four posts.** The relief's skin is 0.2 mm, and the two-step proposal's wall was 0.7 mm | The post tubes, the skin and the two-step wall are all under the 1.0 mm minimum |
| MJF PA12 holes | Holes shrink (more with thicker walls); tolerance ±0.3 mm; allow extra room and ream or tap afterwards for precision. Listed: Φ1.5 mm with depth 1.5–4.5 mm, Φ2.0 mm with depth 2.0–6.0 mm | Light pipe hole 2.92 mm against the pipe's +0.08/−0.05 mm window; screw pilots 1.8 mm and 6 mm deep (between the listed sizes); LED holes 5.4 mm | The pipe's press fit is below what the printer holds, so plan to ream it. The test piece covers the pilot and LED sizes |
| Engraved and embossed detail | At least 0.8 mm deep and 0.8 mm wide | Legends 0.8 mm deep. The test piece's 0.4 and 0.6 mm samples are below the stated minimum | 0.8 mm is at the limit; check the legend stroke width is 0.8 mm or more |
| Clearance between assembled parts | 0.2–0.4 mm for fitted parts; 0.6 mm minimum for moving parts | Keycap clearance 0.3 mm; board to wall 0.3 mm | Inside the band, but the band doesn't cover the length tolerance above |
| PCB outline (JLCPCB, routed) | ±0.2 mm regular, ±0.1 mm high precision (the page doesn't say which is the default) | Outline 145.4 × 40.4 mm | Choose the tier when ordering, and check the cost difference |
| PCB thickness | ±10 % for boards 1.0 mm and thicker, so 1.44–1.76 mm for 1.6 mm | Model 1.6 mm; the board sits on its underside, so everything on top moves with the thickness | Up to ±0.16 mm on the board top and on the key plungers, against a 0.90 mm plunger-to-face gap and 0.0–0.3 mm of cap play |

Sources: [JLC3DP, PA12-HP Nylon](https://jlc3dp.com/help/article/PA12-HP-Nylon), [JLC3DP, 3D Printing Design Guideline](https://jlc3dp.com/help/article/3D-Printing-Design-Guideline), [JLCPCB PCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities). JLC's pages disagree with each other in places (for instance the clearance guidance), so recheck before relying on a number.

**Worst-case arithmetic for the 0.3 mm board-to-wall gap** (each figure taken at its limit, all stacked):
- **Length, per end:** 0.30 − 0.29 (printed cavity) − 0.05 to 0.10 (board outline, half of ±0.1 or ±0.2) = **−0.04 to −0.09 mm**. The gap can close.
- **Width, per side:** 0.30 − 0.15 − 0.05 to 0.10 = **+0.05 to +0.10 mm**. Positive, with almost nothing to spare.
- With candidate C (West end +0.2 mm, gap 0.5 mm) the same sum is +0.11 to +0.16 mm.

Whether the limits really stack is something the test print answers: its plate is 110 mm long, so the ±0.4 % figure applies to it too.

### Candidate changes (proposed, pending measurements)

**Outcome (decided 2026-11-21, John):** keep the case size and the 0.3 mm gap, and add the checks (E). A to D are not adopted. The table is kept as the record of what was considered.

Starting point: a gap of about 0.5–0.6 mm on every side. Revise the numbers once the measurements below are in.

| ID | Proposal | Effect | What else has to change |
|---|---|---|---|
| A | **Widen the case (Y) by 0.6 mm** (0.3 mm per side) | Gap 0.6 mm on both long sides. No effect on the PCB layout: the keys, LEDs, slider, SETUP and connectors are centred on Y = 0. | Pin the board standoff positions to the board's holes instead of `SO_Y = W/2 − 7`. The corner posts follow the walls outward; the 1 mm slack in the notch absorbs it. |
| B | **Lengthen the East (front, IR LED) end by about 0.3 mm** | Gap 0.6 mm at the front. | The IR LEDs are hand-bent: solder them with the leads about 0.3 mm longer so the tips stay 1.5 mm proud of the face. Re-anchor everything measured from the front face (key holes, indicator, A/B slot, SETUP pinhole, battery) to the board frame so it doesn't shift against the board. |
| C | **Lengthen the West (back, USB) end by 0.2 mm at most** | Gap 0.5 mm at the back. The USB recess becomes 1.2 mm, leaving the minimum 0.8 mm of wall behind the pocket. The charge LED's window is 0.2 mm further from the LED, a negligible loss of light. | The connector can't move (it's on the board), so the back gap is the hardest to open. More than 0.2 mm needs that wall redesigned (for example a locally thicker wall or a different recess budget). |
| D | **Alternative to A to C: thin the side walls from 2.0 to 1.7 mm** (keep the outer size) | Gap 0.6 mm all round, with no re-anchoring. | Split the floor from the side wall (they share `WALL`). The USB pocket's remaining wall falls under 0.8 mm, so the recess or pocket depth needs a look. The corner posts shift outward 0.3 mm. |
| E | **Add margin checks** | Catch this class of problem. | In `assembly.py`: a minimum board-to-wall gap, and a minimum clearance (about 0.5 mm) between every board part and the case. A board-edge check script that errors on any courtyard crossing the outline except an explicit list (U201, D303, D304, J101), each with its reason. |

### Measurements that decide it

From the test print, when it arrives. Its plate and wall are printed from the same constants as the case, so the printer's dimensional error shows up in them:

1. **Plate length and width** (modelled 110.0 × 46.0 mm): the scale error in X and Y. The length is over 100 mm, so JLC's ±0.4 % (±0.44 mm) applies, as it does to the case's 146 mm cavity.
2. **Wall thickness** (2.0 mm) **and wall length** (86 mm): the inner cavity is the outer size minus two walls, so these predict how far the real cavity differs from 146.0 × 41.0.
3. **Boss diameter** (5.0 mm) and **hole diameters**: already needed for the pilot and LED sizes; they also show whether features grow or shrink.

From the first boards:

4. **PCB outline length, width and thickness** against 145.4 × 40.4 × 1.6 mm. **Measured on board 1: 145.5 × 40.4 × 1.6 mm** (John, 2026-11-26).
   - Reading (Claude): the extra 0.1 mm takes 0.05 mm from each end. Each end has 0.30 nominal, −0.05 from the board and about +0.2 from the print's scale (test piece), so about **0.45 mm**. Each side has 0.30 nominal and no change from the board or print, so **0.30 mm**, or up to 0.4 mm if the walls print thin.
   - Both clear the 0.2 mm threshold, so the gap and case size stand as decided on 2026-11-21. That rests on one board and one test print: check the other four boards' lengths.

How to read them (proposal): take the worst case of the printed cavity shrinking and the board outline growing. If the nominal 0.3 mm gap still leaves at least about 0.2 mm after both, keep it and add only the checks (E). If not, adopt A to C (or D), scaled to what was measured.

### Open item: revisit the relief on the south-west post

**Resolved 2026-11-21 (John): cut through.** See the decision table. The notes below are kept as the record.

Noted by John, 2026-11-17: come back to the relief flat (the "notch") on the south-west corner post, the one beside J102 and the USB end (case coordinates −67.9, −15.4 mm; KiCad 32.05, 95.45), because the walls get thin around the screw's capture ledge.

What is thin: between the board underside less 1 mm and the board top, the flat is 2.3 mm from the post axis and the screw's counterbore is 2.1 mm in radius, which leaves a **0.2 mm skin**. Above the board top the walls are 1.2 mm (round the clearance hole) and 1.4 mm (round the pilot). The ledge itself, where the screw head seats, is at the board top (`CB_TOP_BELOW_SPLIT` below the split) and is not cut.

Why it was accepted for now: the skin carries no load, because the screw clamps through the post above the ledge, and the corner posts don't support the board, so clearance came first.

What to look at: how the skin prints on the first case, and whether it should be made deliberately thicker or deliberately open. Options raised so far (proposals, none decided):
- a **two-step relief**: the full flat above the board top and a shallower flat (2.8 mm from the axis, a 0.7 mm wall) below it, down to 1 mm below the standoff top. It was built and shown on 2026-11-17, then replaced by the single flat at John's request;
- **lowering the screw-head seat** about 1.6 mm on all four posts, which allows the full flat all the way down, with M2 × 10 case screws instead of M2 × 8 to keep the thread engagement in the top boss;
- **cutting through on purpose**, so the flat opens a window into the counterbore instead of leaving a 0.2 mm skin that may not print.

Decide this together with the margin changes above: widening or lengthening the case shifts the corner posts relative to the board's notches.

## Views

Generated from the model by `views.py` into [`docs/images/enclosure/`](../../docs/images/enclosure/); rerun it after a change (the shaded views need OCP CAD Viewer open, or pass `--no-shots`). Internal parts are placeholders until the PCB is laid out.

| | |
|---|---|
| ![Assembled, black shells and grey keys](../../docs/images/enclosure/enclosure_finish.png) | ![Exploded, both shells see-through](../../docs/images/enclosure/enclosure_exploded.png) |
| Finished look: black-dyed MJF shells, natural-grey keys, unfilled legends | Exploded: top shell (with keycaps and A/B slider) lifted 30 mm |

**PCB constraints** (top view, front end at the right): board outline, case-post notches, battery-lead notch, standoff holes with screw-head keep-outs, part positions, and keep-outs on the underside.

![PCB constraints](../../docs/images/enclosure/enclosure_pcb_constraints.svg)

**Sections** through the centre line and through an IR LED, with heights above the outside bottom.

![Sections](../../docs/images/enclosure/enclosure_section.svg)

**Top, bottom and end views** (hidden edges dashed in the top and bottom views): [`enclosure_views.svg`](../../docs/images/enclosure/enclosure_views.svg).

**Board outline for KiCad:** [`enclosure_pcb_outline.dxf`](../../docs/images/enclosure/enclosure_pcb_outline.dxf), layer `Edge.Cuts`. Import it with *File → Import → Graphics* onto Edge.Cuts. It includes the four standoff holes as circles; delete those and place M2 mounting-hole footprints at the same spots.

## Board model from KiCad

`kicad-cli pcb export step` exports the laid-out board with component models, so the case can be built around it. On this machine KiCad 10's CLI is at `%LOCALAPPDATA%\Programs\KiCad\10.0\bin\kicad-cli.exe` (not on PATH).

## License

The models (scripts, `requirements.txt` and the STEP/STL output) are CERN-OHL-S-2.0, like the KiCad design (see `hardware/LICENSE`). Each script starts with an SPDX line. This README, like the project's other docs, is CC-BY-SA-4.0.
