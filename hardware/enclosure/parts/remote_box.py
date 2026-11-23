# SPDX-License-Identifier: CERN-OHL-S-2.0
# Remote case: a clamshell split horizontally at the IR LED axis (agreed 2026-11-13).
#   bottom: floor, board standoffs, key-support pads, lower half-holes for the IR LEDs and USB-C
#   top:    key face (key and indicator holes), upper half-holes, alignment lip
# Screws go in from underneath, through the bottom shell into bosses in the top shell.
# Origin at the box centre; +X is the front (IR) end. Preliminary: most sizes are placeholders.
# Run with the hd_enclosure interpreter while OCP CAD Viewer is open.
from build123d import *
from ocp_vscode import show

L, W = 150, 45         # outer footprint, mm
H = 25                 # height of the body below the key face, mm
TOP_T = 2.0            # key face thickness; overall height H + TOP_T
WALL = 2.0             # wall and floor thickness, mm
CORNER_R = 12          # vertical corner radius, mm. 12 is the most that keeps the IR LED
                       # countersinks on the flat part of the front wall (sweep 2026-11-13: 14 fails)
EDGE_R = 2.5           # rounding of the top and bottom perimeter edges (2026-11-13). With a
                       # 2 mm wall, ~1.8 mm of material remains through the curve at 2.5
Z0 = -H / 2            # outside bottom; heights below are measured from here
Z_TOP = H / 2 + TOP_T  # outside top
FLOOR_TOP = Z0 + WALL

# --- Battery: LiPo pouch lying on the floor under the board, in the back section ---
# Cell chosen 2026-11-14: Adafruit #1578, 500 mAh, nominal 29 x 36 x 4.75 mm (re-measure on arrival
# and set W/L from calipers). BATT_T stays at the old 5.0 mm placeholder as a thickness allowance:
# it sets the board height, which the ordered test piece already uses.
# BATT_W is 30, not Adafruit's 29 (2026-11-15): the cell's own datasheet drawing (PKCELL LP-503035)
# says 30 +-0.1 x 35 +-0.1 x 5.0 mm. With BATT_CLEAR the rib pocket is 31 mm wide, so a 30 mm cell
# gets 0.5 mm per side and a 29 mm cell 1.0 mm; never a press fit. The board's battery keep-out is
# already +-15.5 mm (31 mm). Measure the real cell, including the protection-board bulge.
BATT_T, BATT_W, BATT_L = 5.0, 30.0, 36.0
BATT_SWELL = 0.10      # allow ~10 % thickness growth over the cell's life
BATT_FRONT_FROM_FRONT = 92.0  # cell's front edge, mm from the front end (behind the ALL pad)
BATT_X = L / 2 - BATT_FRONT_FROM_FRONT - BATT_L / 2
# Held by four L-shaped corner ribs on the floor (sides stay open for the leads, removal and
# swelling) plus a thin (<= 0.2 mm) double-sided tape pad; an insulating sheet (Kapton or fish
# paper) goes on the board's underside over the cell. Never clamp a pouch cell.
BATT_CLEAR = 0.5       # cell to rib, each side
BATT_RIB_H, BATT_RIB_T, BATT_RIB_LEG = 1.5, 1.0, 6.0   # rib height, thickness, leg length

# --- PCB: its height is set by the battery underneath ---
PIN_PROTRUDE = 1.5     # THT pins below the board (trim, or keep THT parts off the cell area)
UNDER_CLEAR = 0.5      # swollen cell to pin tips
PCB_T = 1.6
PCB_BOT_Z = WALL + BATT_T * (1 + BATT_SWELL) + PIN_PROTRUDE + UNDER_CLEAR
PCB_TOP_Z = PCB_BOT_Z + PCB_T

# Board standoffs, with their own M2 screws through the board.
# PCB needs 4 holes here (2.2 mm for M2) with a ~5.5 mm keep-out for the screw head.
SO_D = 5.0             # standoff diameter, mm
SO_HOLE_D = 1.8        # pilot hole for an M2 self-tapper (decided 2026-11-22, John, from the test piece)
SO_X = L / 2 - 14      # +-X position
SO_Y = W / 2 - 7       # +-Y position
SO_H = PCB_BOT_Z - WALL  # floor top to board underside: the board sits on them

# Support pads under the key switches: no screws or board holes, only a keep-out on the
# board's underside. They stop the board flexing when a key is pressed.
PAD_D = 3.5            # fits between a 6 mm tactile switch's pins (6.5 x 4.5 mm pitch)
PAD_SHORT = 0.1        # pads stop this far below the board underside
PAD_H = SO_H - PAD_SHORT

# --- Top-face layout, following the original Hunter Douglas remote (2026-11-13) ---
# Positions are mm from the front end to the centre, and y (y > 0 is the left side when you hold
# the remote pointing away from you). From the front: indicator, CHANNEL A/B slider, one tall oval
# holding the UP/DOWN arrows (two caps, two switches), groups 1/2 beside UP and 3/4 beside DOWN,
# ALL centred below.
SWITCHES = {"UP": (42, 0), "DOWN": (58, 0), "1": (42, 14), "2": (42, -14),
            "3": (58, 14), "4": (58, -14), "ALL": (74, 0)}
KEY_XY = [(L / 2 - d, y) for d, y in SWITCHES.values()]
# Openings in the key face, all fully rounded: name -> (from front, y, length along X, width along Y)
KEY_HOLES = {"ARROWS": (50.15, 0, 33.7, 14), "1": (42, 14, 8, 8), "2": (42, -14, 8, 8),
             "3": (58, 14, 8, 8), "4": (58, -14, 8, 8), "ALL": (74, 0, 8, 16)}
# Corner radius per key (cap outline; the hole adds CAP_CLEAR). Keys not listed are fully rounded
# (stadium). Keys 1-4 are rounded squares, not circles (2026-11-15): a round cap in a round hole
# can turn, and its number with it; the corners stop it (a few degrees of play at CAP_CLEAR).
KEY_CORNER_R = {"1": 2.0, "2": 2.0, "3": 2.0, "4": 2.0}
CAP_CLEAR = 0.3        # gap around each keycap in its hole (placeholder; test print)
SW_H = 13.0            # key switches: 6 mm tactile, plunger top 13 mm above the board
CAP_PROUD = 1.5        # cap tops this far above the key face
CAP_TOP_R = 0.6        # rounding on the cap's top edge
FLANGE_W, FLANGE_T = 1.0, 0.8   # retaining flange under the face: overhang and thickness
CAP_PLAY = 0.3         # gap between the flange underside and the switch plunger at rest. Generous
                       # on purpose (review 2026-11-13): MJF is +-0.3 mm per part, and shims under
                       # the board can only take the gap down, so start large and tune on the print
ARROW_GAP = 0.6        # gap between the UP and DOWN halves of the arrow oval
LEGEND_DEPTH = 1.0     # legends recessed into the cap tops. Test print: 0.8 read best of 0.4/0.6/0.8;
                       # deepest and largest practical (decided 2026-11-21, John; was 0.8, before that 0.4)
LEGEND_SIZE_DIGIT, LEGEND_SIZE_WORD = 5.5, 4.5   # font sizes for 1-4 and ALL (2026-11-21, John; were 4.5, 4.0)
LEGENDS = {"1": "1", "2": "2", "3": "3", "4": "4", "ALL": "ALL"}
IND_FROM_FRONT = 14.0   # bicolor indicator centre, mm from the front
# Indicator (2026-11-15): Kingbright WP59EGW (5 mm red/green) seated flush on the board, seen
# through a Bivar PLP2-250 press-fit light pipe in the key face. The board sets the LED height and
# the face sets the pipe height, so nothing needs adjusting at assembly.
IND_LED_D, IND_LED_H = 5.0, 8.6    # LED lens diameter; flange base to dome tip (WP59EGW datasheet)
PIPE_HOLE_D = 2.92                 # Bivar PLP2 recommended hole: 0.115" +0.003/-0.002 (MJF: test-fit)
PIPE_PANEL_MIN, PIPE_PANEL_MAX = 1.19, 2.36   # PLP2 panel thickness range (0.047"-0.093")
PIPE_D, PIPE_L = 2.8, 6.35         # PLP2 pipe diameter (crush ribs to 3.1) and length under the head (-250)
PIPE_HEAD_D, PIPE_HEAD_H = 3.3, 0.8   # domed head standing on the face
PIPE_GAP_MIN, PIPE_GAP_MAX = 0.5, 1.5   # pipe end to LED dome: clear of tolerances, little light lost
# CHANNEL A/B: a printed slider in a slot across the width. A fork on its stem straddles the
# actuator of a side-actuated SMD slide switch on the board: Shouhan MSK12C02 (LCSC C431540) or
# C&K PCM12, same footprint class (body 6.7 x 2.8 mm, actuator 1.3 wide, sticking out 1.45 mm,
# 1.5 mm travel). The switch's long axis runs across the width; its actuator points to the back.
AB_FROM_FRONT = 26
AB_TRAVEL = 1.5        # = the switch's travel (the slider moves 1:1)
AB_KNOB_W, AB_KNOB_L = 8.0, 3.6   # knob in the slot: across the width, along the length
AB_SW_BODY = (2.8, 6.7, 1.4)      # along X, across Y, height. C&K PCM12SMTR datasheet (I-44): 6.7 long, 1.4 tall
AB_SW_ACT = (1.5, 1.3, 1.0)       # actuator: sticks out 1.5, 1.3 wide (datasheet); thickness 1.0 a guess
# Slider drive (2026-11-14): a solid block with a notch for the actuator, not a two-prong fork.
AB_BLOCK_T, AB_BLOCK_W = 2.4, 4.0  # along X (grows toward the back); across Y (4.6 until 2026-11-21: narrowed
                                   # to clear the switch's mounting tabs now that the foot is on the board)
# (6.7 mm) at both ends of travel, so it misses the switch's mounting legs at the body ends
AB_NOTCH_PLAY = 0.15               # notch clearance each side of the actuator
# Board-riding slider (decided 2026-11-21, John): the drive
# block reaches down and slides on the board's solder mask (crossing only the AB_SW trace), so
# downward pressure on the knob goes into the board, never the switch. One part; nothing else holds
# it. X offsets are from the switch centre, + toward the front.
AB_PLATE_X0, AB_PLATE_X1 = -5.0, 1.5   # slider plate along X (covers the drive block, clears the arrow cap)
AB_PLATE_W, AB_PLATE_T = AB_KNOB_W + 4.0, 1.0   # plate across Y, and thickness
AB_FACE_PLAY = 0.5                 # plate top below the face underside: the case stack-up's tolerance
AB_FOOT_R = 0.4                    # rounding on the foot's bottom edges, to glide over mask and trace
AB_NOTCH_ROOF = 0.6                # notch roof above the actuator's top: never rests on the actuator
AB_TAB_CLEAR = 0.3                 # foot to the switch's mounting-tab pads, minimum (solder stays on
                                   # the pad and wets up the leg; it doesn't spread over the mask)
# Knob, slot and plate centred over the drive block, not the switch body (decided 2026-11-21, John):
# knob, block and foot form one straight column, so a press goes straight down into the
# board instead of rocking the slider. The arrow oval's front end gave up 0.3 mm for the 2 mm web
# (ARROWS was (50, 0, 34, 14)); the slot moved 2.8 mm toward the back.
AB_KNOB_SHIFT = AB_SW_BODY[0] / 2 + 0.2 + AB_BLOCK_T / 2   # knob behind the switch centre (= block centre)
AB_SLOT = (AB_FROM_FRONT + AB_KNOB_SHIFT, 0, AB_KNOB_L + 0.6, AB_KNOB_W + AB_TRAVEL + 0.6)  # from front, y, X, Y
# Support pad under the slider's foot (2026-11-21, John), like the key pads: a press goes knob ->
# foot -> board -> pad -> floor. The board's underside there is clear (nearest: the switch's
# locating-peg holes, 2.7 mm from the pad's centre to their edges; tented vias 3.95 mm).
AB_PAD_XY = (L / 2 - AB_FROM_FRONT - AB_KNOB_SHIFT, 0.0)
# SETUP: pinhole in the floor (as on the original's back), over a 5 mm tactile switch mounted on
# the board's underside, plunger down. Paperclip travels ~4.5 mm. 10 mm right of centre
# (2026-11-13, PCB placement): on the centre line its through-hole pins land on the A/B switch's
# pads (26 mm) or the indicator's leads (14 mm); DRC is clean from 8 mm off-centre outward.
SETUP_FROM_FRONT, SETUP_Y, SETUP_HOLE_D = 22, -10, 1.6

# --- IR LEDs: two 5 mm TSAL6200s lying flat on the PCB, pointing out of the front end ---
LED_HOLE_D = 5.2       # 5 mm LED + clearance: best of 5.2/5.4/5.6 on the test print (decided 2026-11-21, John)
LED_PITCH = 12.0       # centre-to-centre spacing across the width, mm
LED_Z = PCB_TOP_Z + 3.0  # LED axis: KiCad footprint LED_D5.0mm_Horizontal_..._Z3.0mm
LED_PROUD = 1.5        # dome tips stand this far proud of the front face (the dome is 2.5 long)
LED_CSK = 1.0          # 45-degree countersink on the outside, so the wall can't clip the beam

# --- USB-C in the back (-X) end, centred. Receptacle HRO TYPE-C-31-M-12 ---
USB_W, USB_H = 9.6, 4.0  # opening: shell 8.94 x 3.26 + ~0.35 mm clearance each side
USB_CORNER_R = 0.6     # rounded rectangle with straight sides: no undercut at the split
USB_Z = PCB_TOP_Z + 3.26 / 2  # opening centre = shell centre
# The receptacle's face sits this far inside the outer wall face. Flush didn't work: the front
# shell-leg pads then hung 0.5 mm off the board edge. Check that a plug still seats (test print).
USB_RECESS = 1.0
# Pocket round the opening on the outside, so a cable's moulded plug housing can sink in by the
# recess and the plug still seats fully (review 2026-11-13). Check with real cables.
USB_POCKET_W, USB_POCKET_H, USB_POCKET_R = 12.5, 7.0, 1.5   # across, tall, corner radius
USB_POCKET_DEPTH = USB_RECESS

# --- Charge LED window in the back wall (added 2026-11-14) ---
# D101 is a side-view LED (Kingbright APA1606, 0.6 mm tall) at the board's back edge, KiCad
# (28.6, 72.25), lens facing -X: model Y = 80 - 72.25. The hole is centred on its emitting face.
CHG_Y = 80.0 - 72.25
CHG_HOLE_D = 1.6
CHG_Z = PCB_TOP_Z + 0.3

# --- Split, lip and case screws ---
SPLIT = Z0 + LED_Z     # parting plane at the IR LED axis, so each shell holds half of each hole
LIP_H, LIP_W, LIP_GAP = 2.0, 1.5, 0.2   # lip on the top shell, inside the long walls
# Gaps in the lip (mm from the front to, side +1/-1) where parts reach the board edge near the
# split, in the current placeholder layout (assembly.py).
LIP_GAPS = [(88, 108, -1),   # ESP32 antenna end at the -Y board edge (rotated 2026-11-13, PCB routing)
            (127, 134, -1)]  # battery leads crossing to the -Y board-edge notch
BOSS_D = 6.0           # corner posts, touching both walls
BOSS_INSET = WALL + BOSS_D / 2
SCREW_CLEAR_D = 2.2    # M2 through the bottom post
CB_D = 4.0             # counterbore for the head, from underneath. 4.0 (was 4.2) for a 1.0 mm post
                       # wall, JLC's minimum (decided 2026-11-21, John); the M2 heads measure 3.2 x 1.3
CB_TOP_BELOW_SPLIT = 3.0  # counterbore stops this far below the split (screw-head seat)
PILOT_D, PILOT_DEPTH = 1.8, 6.0   # pilot in the top boss (1.8 decided 2026-11-22, John). M2 x 8
# Post centres: with small corners, in the corner touching both walls; with corners big enough to
# matter, on the corner's diagonal, touching the curved inner wall.
_arc = CORNER_R - WALL - BOSS_D / 2          # inner-arc centre to post centre, along the diagonal
if _arc > 0:
    _post_off = CORNER_R - _arc / 2 ** 0.5   # post centre's distance in from each outer face
else:
    _post_off = BOSS_INSET
POST_X, POST_Y = L / 2 - _post_off, W / 2 - _post_off
POSTS = [(sx * POST_X, sy * POST_Y) for sx in (-1, 1) for sy in (-1, 1)]
# Relief flat on the back (-X), -Y corner post (decided 2026-11-17, John). The battery connector J102
# sits beside it, its body hanging 0.7 mm over the board's post notch and 3.3 mm from the post axis
# (0.3 mm from the 6 mm post). A flat on the post's +Y side, POST_RELIEF_D from the axis, runs from
# the board top up past the connector's 6 mm height. 2.3 gives ~1.0 mm clearance and keeps a
# 1.2 mm wall round the screw's clearance hole (1.4 mm round the pilot above the split).
POST_RELIEF_D = 2.3
POST_RELIEF_H = 7.0
# The flat also runs POST_RELIEF_BELOW below the top of the board's standoffs (the board underside),
# so a board seated low can't catch the connector body on the post (John, 2026-11-17: the corner
# posts don't support the board, so clearance wins). Below the board top the screw's counterbore
# (CB_D, which reaches up to the board top) leaves only 0.2 mm of skin between it and the flat. That
# skin carries no load: the screw clamps through the post above the counterbore's ledge.
POST_RELIEF_BELOW = 1.0
# Window instead of skin (decided 2026-11-21, John: cut through): below the screw-head seat, a deeper
# flat POST_WINDOW_D from the axis opens the counterbore's side, from the relief's bottom to
# POST_WINDOW_BELOW_SEAT under the seat, so the seat itself stays whole. Replaces a thin skin that
# might not print.
POST_WINDOW_D, POST_WINDOW_BELOW_SEAT = 1.7, 0.2


def _battery_ribs():
    """Four L-shaped ribs just outside the cell's corners, standing on the floor."""
    ribs = None
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx = BATT_X + sx * (BATT_L / 2 + BATT_CLEAR)       # inside corner of the L
            cy = sy * (BATT_W / 2 + BATT_CLEAR)
            xs = sorted((cx - sx * (BATT_RIB_LEG - BATT_RIB_T), cx + sx * BATT_RIB_T))
            ys = sorted((cy - sy * (BATT_RIB_LEG - BATT_RIB_T), cy + sy * BATT_RIB_T))
            for (x0, x1), (y0, y1) in ((xs, sorted((cy, cy + sy * BATT_RIB_T))),
                                       (sorted((cx, cx + sx * BATT_RIB_T)), ys)):
                leg = Pos((x0 + x1) / 2, (y0 + y1) / 2, FLOOR_TOP) * Box(
                    x1 - x0, y1 - y0, BATT_RIB_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
                ribs = leg if ribs is None else ribs + leg
    return ribs


batt_ribs = _battery_ribs()


def _rounded_rect(length, width, radius):
    """Rounded rectangle; a radius >= half the short side gives a stadium (fully rounded ends)."""
    return RectangleRounded(length, width, min(radius, min(length, width) / 2 - 0.01))


with BuildPart() as body:
    # closed hollow box: outer shape minus the inner cavity
    with BuildSketch(Plane.XY.offset(Z0)):
        RectangleRounded(L, W, CORNER_R)
    extrude(amount=Z_TOP - Z0)
    # round the top and bottom perimeter edges (before hollowing, so the wall follows the curve)
    fillet(body.edges().group_by(Axis.Z)[0] + body.edges().group_by(Axis.Z)[-1], radius=EDGE_R)
    with BuildSketch(Plane.XY.offset(FLOOR_TOP)):
        RectangleRounded(L - 2 * WALL, W - 2 * WALL, CORNER_R - WALL)
    extrude(amount=(Z_TOP - TOP_T) - FLOOR_TOP, mode=Mode.SUBTRACT)

    # corner posts, floor to the underside of the key face (split into bottom post + top boss)
    with Locations(*[(x, y, FLOOR_TOP) for x, y in POSTS]):
        Cylinder(BOSS_D / 2, (Z_TOP - TOP_T) - FLOOR_TOP,
                 align=(Align.CENTER, Align.CENTER, Align.MIN))

    # relief flat on the corner post beside the battery connector (see POST_RELIEF_D)
    _relief_len = BOSS_D / 2 - POST_RELIEF_D + 0.8
    _relief_z0 = Z0 + PCB_BOT_Z - POST_RELIEF_BELOW
    with Locations((-POST_X, -POST_Y + POST_RELIEF_D + _relief_len / 2, _relief_z0)):
        Box(BOSS_D + 1.0, _relief_len, (Z0 + PCB_TOP_Z + POST_RELIEF_H) - _relief_z0,
            align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)

    # board standoffs, with blind pilot holes (Hole() without a depth would pierce the floor)
    so_pts = [(sx * SO_X, sy * SO_Y) for sx in (-1, 1) for sy in (-1, 1)]
    with Locations(*[(x, y, FLOOR_TOP) for x, y in so_pts]):
        Cylinder(SO_D / 2, SO_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    with Locations(*[(x, y, FLOOR_TOP + SO_H) for x, y in so_pts]):
        Hole(SO_HOLE_D / 2, depth=SO_H)

    # battery corner ribs
    add(batt_ribs)

    # support pads under the key switches
    with Locations(*[(x, y, FLOOR_TOP) for x, y in KEY_XY + [AB_PAD_XY]]):
        Cylinder(PAD_D / 2, PAD_H, align=(Align.CENTER, Align.CENTER, Align.MIN))

    # IR LED holes through the front wall, with outside countersinks
    for y in (-LED_PITCH / 2, LED_PITCH / 2):
        with Locations(Location((L / 2 - WALL / 2, y, SPLIT), (0, 90, 0))):
            Cylinder(LED_HOLE_D / 2, WALL * 2, mode=Mode.SUBTRACT)
        with Locations(Location((L / 2 - LED_CSK / 2 + 0.1, y, SPLIT), (0, 90, 0))):
            Cone(LED_HOLE_D / 2, LED_HOLE_D / 2 + LED_CSK + 0.2, LED_CSK + 0.2, mode=Mode.SUBTRACT)

    # USB-C opening through the back wall
    with BuildSketch(Plane(origin=(-L / 2 - 0.5, 0, Z0 + USB_Z), x_dir=(0, 1, 0), z_dir=(1, 0, 0))):
        RectangleRounded(USB_W, USB_H, USB_CORNER_R)
    extrude(amount=WALL + 1.0, mode=Mode.SUBTRACT)
    with BuildSketch(Plane(origin=(-L / 2 - 0.5, 0, Z0 + USB_Z), x_dir=(0, 1, 0), z_dir=(1, 0, 0))):
        RectangleRounded(USB_POCKET_W, USB_POCKET_H, USB_POCKET_R)
    extrude(amount=USB_POCKET_DEPTH + 0.5, mode=Mode.SUBTRACT)

    # charge LED window through the back wall, beside the USB-C pocket
    with Locations(Location((-L / 2 + WALL / 2, CHG_Y, Z0 + CHG_Z), (0, 90, 0))):
        Cylinder(CHG_HOLE_D / 2, WALL * 2, mode=Mode.SUBTRACT)

    # key, A/B slider and indicator openings in the key face
    with BuildSketch(Plane.XY.offset(Z_TOP - TOP_T - 0.1)):
        for name, (d, y, kl, kw) in KEY_HOLES.items():
            with Locations((L / 2 - d, y)):
                _rounded_rect(kl + 2 * CAP_CLEAR, kw + 2 * CAP_CLEAR, KEY_CORNER_R.get(name, 99) + CAP_CLEAR)
        d, y, kl, kw = AB_SLOT
        with Locations((L / 2 - d, y)):
            _rounded_rect(kl, kw, 99)
        with Locations((L / 2 - IND_FROM_FRONT, 0)):
            Circle(PIPE_HOLE_D / 2)       # light pipe press-fit hole
    extrude(amount=TOP_T + 0.2, mode=Mode.SUBTRACT)

    # SETUP pinhole through the floor
    with Locations((L / 2 - SETUP_FROM_FRONT, SETUP_Y, Z0 - 0.1)):
        Cylinder(SETUP_HOLE_D / 2, WALL + 0.2, align=(Align.CENTER, Align.CENTER, Align.MIN),
                 mode=Mode.SUBTRACT)

    # case screws from underneath: counterbore + clearance in the bottom post, pilot in the top boss
    for x, y in POSTS:
        with Locations((x, y, Z0 - 0.1)):
            Cylinder(CB_D / 2, (SPLIT - CB_TOP_BELOW_SPLIT) - Z0 + 0.1,
                     align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)
            Cylinder(SCREW_CLEAR_D / 2, SPLIT - Z0 + 0.2,
                     align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)
    # window through the relief side of the J102 post's counterbore (see POST_WINDOW_D)
    _win_z0 = Z0 + PCB_BOT_Z - POST_RELIEF_BELOW
    _win_z1 = SPLIT - CB_TOP_BELOW_SPLIT - POST_WINDOW_BELOW_SEAT
    _win_x = 2 * (BOSS_D ** 2 / 4 - POST_WINDOW_D ** 2) ** 0.5 + 0.2     # the post's chord there
    with Locations((-POST_X, -POST_Y + POST_WINDOW_D + 1.0, _win_z0)):
        Box(_win_x, 2.0, _win_z1 - _win_z0, align=(Align.CENTER, Align.CENTER, Align.MIN), mode=Mode.SUBTRACT)
        with Locations((x, y, SPLIT)):
            Cylinder(PILOT_D / 2, PILOT_DEPTH, align=(Align.CENTER, Align.CENTER, Align.MIN),
                     mode=Mode.SUBTRACT)

bottom = body.part.split(Plane.XY.offset(SPLIT), keep=Keep.BOTTOM)
top = body.part.split(Plane.XY.offset(SPLIT), keep=Keep.TOP)

# Lip on the top shell, along the long sides between the corner posts. The ends need none: the
# LEDs and USB in their half-holes lock them together.
with BuildPart() as lip:
    # stop before the posts, and before the side walls start to curve into the corners
    lip_x = min(POST_X - BOSS_D / 2 - 0.5, L / 2 - CORNER_R - 0.5)
    for side in (-1, 1):
        y_c = side * (W / 2 - WALL - LIP_GAP - LIP_W / 2)
        with Locations((0, y_c, SPLIT)):
            Box(2 * lip_x, LIP_W, LIP_H, align=(Align.CENTER, Align.CENTER, Align.MAX))
    for d_from, d_to, side in LIP_GAPS:
        with Locations((L / 2 - (d_from + d_to) / 2, side * (W / 2 - WALL - LIP_GAP - LIP_W / 2),
                        SPLIT)):
            Box(d_to - d_from, LIP_W + 0.2, LIP_H + 0.2,
                align=(Align.CENTER, Align.CENTER, Align.MAX), mode=Mode.SUBTRACT)
top = top + lip.part

case = bottom + top    # closed assembly, for clearance checks


# --- Keycaps: separate printed parts, dropped into the top shell from inside before closing ---
FACE_UNDER = Z_TOP - TOP_T
PLUNGER_TOP = Z0 + PCB_TOP_Z + SW_H
NUB_H = FACE_UNDER - FLANGE_T - CAP_PLAY - PLUNGER_TOP   # pad under the flange down to the plunger
# NUB_H < 0: the play doesn't fit under a full-thickness flange, so a relief pocket of that depth
# is cut into the flange's underside over the plunger instead (the flange stays FLANGE_T elsewhere).
RELIEF_D = 3.5 + 1.0   # plunger 3.5 mm + clearance
assert FLANGE_T + NUB_H >= 0.5, f"flange over the plunger would be {FLANGE_T + NUB_H:.2f} mm thick"


def _outline(d, y, lx, wy, grow=0.0, r=None):
    """Key outline grown by `grow`; corner radius `r` (also grown), or fully rounded if None."""
    lx, wy = lx + 2 * grow, wy + 2 * grow
    rad = min(lx, wy) / 2 - 0.01 if r is None else min(r + grow, min(lx, wy) / 2 - 0.01)
    return Pos(L / 2 - d, y) * RectangleRounded(lx, wy, rad)


def _cap(sk_body, sk_flange, at, legend=None, arrow=None):
    """Cap: body through the face, flange under it, contact nub and legend centred on `at`.
    Returns (cap, legend recess); the recess is what gets filled with white paint."""
    cap_top = Z_TOP + CAP_PROUD
    flange_bot = FACE_UNDER - FLANGE_T
    part = extrude(Plane.XY.offset(FACE_UNDER) * sk_body, amount=cap_top - FACE_UNDER)
    part = fillet(part.edges().group_by(Axis.Z)[-1], radius=CAP_TOP_R)
    part += extrude(Plane.XY.offset(flange_bot) * sk_flange, amount=FLANGE_T)
    if NUB_H > 0:
        part += Pos(*at, flange_bot) * Cylinder(1.5, NUB_H, align=(Align.CENTER, Align.CENTER, Align.MAX))
    elif NUB_H < 0:
        part -= Pos(*at, flange_bot) * Cylinder(RELIEF_D / 2, -NUB_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    mark = None
    if legend:
        # rotated 90 deg clockwise (seen from above) so the text reads upright when you hold the
        # remote pointing away from you (+X is "up" for the reader)
        mark = Pos(*at, 0) * Rot(0, 0, -90) * Text(legend, font_size=LEGEND_SIZE_WORD if len(legend) > 1 else LEGEND_SIZE_DIGIT,
                                                    font_style=FontStyle.BOLD)
    if arrow:                          # +1: points to the front (OPEN), -1: to the back (CLOSE)
        # one triangle, rotated for DOWN (mirroring its points reverses the face, and the cut
        # then goes upward and removes nothing)
        mark = Pos(*at, 0) * Rot(0, 0, 0 if arrow > 0 else 180) * Polygon((2.2, 0), (-1.6, 2.4),
                                                                         (-1.6, -2.4))
    recess = None
    if mark is not None:
        recess = extrude(Plane.XY.offset(cap_top) * mark, amount=-LEGEND_DEPTH) & part
        part -= recess
    return part, recess


# Finish (decided 2026-11-21, John; replaces grey shells and black caps of 2026-11-13): dyed-black
# shells, natural grey caps and A/B knob, legends recessed and left unfilled.
caps, legend_fill = {}, {}
ad, ay, al, aw = KEY_HOLES["ARROWS"]
mid_x = L / 2 - ad
oval_body, oval_flange = _outline(ad, ay, al, aw), _outline(ad, ay, al, aw, FLANGE_W + CAP_CLEAR)
for name, sign in (("UP", 1), ("DOWN", -1)):
    # keep the half on the UP (front, +X) or DOWN side of the centre line, minus half the gap
    keep = Pos(mid_x + sign * (25 + ARROW_GAP / 2), ay) * Rectangle(50, 40)
    body_half = oval_body & keep
    flange_half = oval_flange & keep
    # switch sits under the half's centre of travel; the arrow mark is centred on the switch
    sd, sy = SWITCHES[name]
    caps[name], legend_fill[name] = _cap(body_half, flange_half, (L / 2 - sd, sy), arrow=sign)
for name in ("1", "2", "3", "4", "ALL"):
    d, y, lx, wy = KEY_HOLES[name]
    caps[name], legend_fill[name] = _cap(
        _outline(d, y, lx, wy, r=KEY_CORNER_R.get(name)),
        _outline(d, y, lx, wy, FLANGE_W + CAP_CLEAR, r=KEY_CORNER_R.get(name)),
        (L / 2 - SWITCHES[name][0], SWITCHES[name][1]), legend=LEGENDS[name])

if __name__ == "__main__":
    show(bottom, top.moved(Location((0, 0, 20))), *caps.values(),
         names=["bottom shell", "top shell (lifted)", *[f"cap {n}" for n in caps]],
         colors=["lightgray", "lightblue", *["dimgray"] * len(caps)])
