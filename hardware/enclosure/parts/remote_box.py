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
# Placeholder cell 503035 (5.0 x 30 x 35 mm, ~500 mAh); not chosen yet.
BATT_T, BATT_W, BATT_L = 5.0, 30.0, 35.0
BATT_SWELL = 0.10      # allow ~10 % thickness growth over the cell's life
BATT_FRONT_FROM_FRONT = 92.0  # cell's front edge, mm from the front end (behind the ALL pad)
BATT_X = L / 2 - BATT_FRONT_FROM_FRONT - BATT_L / 2

# --- PCB: its height is set by the battery underneath ---
PIN_PROTRUDE = 1.5     # THT pins below the board (trim, or keep THT parts off the cell area)
UNDER_CLEAR = 0.5      # swollen cell to pin tips
PCB_T = 1.6
PCB_BOT_Z = WALL + BATT_T * (1 + BATT_SWELL) + PIN_PROTRUDE + UNDER_CLEAR
PCB_TOP_Z = PCB_BOT_Z + PCB_T

# Board standoffs, with their own M2 screws through the board.
# PCB needs 4 holes here (2.2 mm for M2) with a ~5.5 mm keep-out for the screw head.
SO_D = 5.0             # standoff diameter, mm
SO_HOLE_D = 1.8        # pilot hole (placeholder for an M2 self-tapper; set from the test print)
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
KEY_HOLES = {"ARROWS": (50, 0, 34, 14), "1": (42, 14, 8, 8), "2": (42, -14, 8, 8),
             "3": (58, 14, 8, 8), "4": (58, -14, 8, 8), "ALL": (74, 0, 8, 16)}
CAP_CLEAR = 0.3        # gap around each keycap in its hole (placeholder; test print)
SW_H = 13.0            # key switches: 6 mm tactile, plunger top 13 mm above the board
CAP_PROUD = 1.5        # cap tops this far above the key face
CAP_TOP_R = 0.6        # rounding on the cap's top edge
FLANGE_W, FLANGE_T = 1.0, 0.8   # retaining flange under the face: overhang and thickness
CAP_PLAY = 0.1         # gap between the flange underside and the switch plunger at rest
ARROW_GAP = 0.6        # gap between the UP and DOWN halves of the arrow oval
LEGEND_DEPTH = 0.4     # legends recessed into the cap tops
LEGENDS = {"1": "1", "2": "2", "3": "3", "4": "4", "ALL": "ALL"}
IND_FROM_FRONT, IND_D = 14.0, 5.0   # bicolor indicator, 5 mm LED standing 2 mm proud
# CHANNEL A/B: a printed slider in a slot across the width. A fork on its stem straddles the
# actuator of a side-actuated SMD slide switch on the board: Shouhan MSK12C02 (LCSC C431540) or
# C&K PCM12, same footprint class (body 6.7 x 2.8 mm, actuator 1.3 wide, sticking out 1.45 mm,
# 1.5 mm travel). The switch's long axis runs across the width; its actuator points to the back.
AB_FROM_FRONT = 26
AB_TRAVEL = 1.5        # = the switch's travel (the slider moves 1:1)
AB_KNOB_W, AB_KNOB_L = 8.0, 3.6   # knob in the slot: across the width, along the length
AB_SLOT = (AB_FROM_FRONT, 0, AB_KNOB_L + 0.6, AB_KNOB_W + AB_TRAVEL + 0.6)  # from front, y, X, Y
AB_SW_BODY = (2.8, 6.7, 1.5)      # along X, across Y, height (height: check the datasheet)
AB_SW_ACT = (1.45, 1.3, 1.0)      # actuator: sticks out (X), width (Y), height
# SETUP: pinhole in the floor (as on the original's back), over a 5 mm tactile switch mounted on
# the board's underside, plunger down. Paperclip travels ~4.5 mm.
SETUP_FROM_FRONT, SETUP_Y, SETUP_HOLE_D = 22, 0, 1.6

# --- IR LEDs: two 5 mm TSAL6200s lying flat on the PCB, pointing out of the front end ---
LED_HOLE_D = 5.4       # 5 mm LED + clearance (placeholder; set from the test print)
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

# --- Split, lip and case screws ---
SPLIT = Z0 + LED_Z     # parting plane at the IR LED axis, so each shell holds half of each hole
LIP_H, LIP_W, LIP_GAP = 2.0, 1.5, 0.2   # lip on the top shell, inside the long walls
# Gaps in the lip (mm from the front, side +1/-1): the ESP32's antenna end sits at the left board
# edge in the current layout preview (89-107 mm from the front).
LIP_GAPS = [(88, 108, +1)]
BOSS_D = 6.0           # corner posts, touching both walls
BOSS_INSET = WALL + BOSS_D / 2
SCREW_CLEAR_D = 2.2    # M2 through the bottom post
CB_D = 4.2             # counterbore for the head, from underneath
CB_TOP_BELOW_SPLIT = 3.0  # counterbore stops this far below the split (screw-head seat)
PILOT_D, PILOT_DEPTH = 1.8, 6.0   # pilot in the top boss (placeholder; test print). M2 x 8
# Post centres: with small corners, in the corner touching both walls; with corners big enough to
# matter, on the corner's diagonal, touching the curved inner wall.
_arc = CORNER_R - WALL - BOSS_D / 2          # inner-arc centre to post centre, along the diagonal
if _arc > 0:
    _post_off = CORNER_R - _arc / 2 ** 0.5   # post centre's distance in from each outer face
else:
    _post_off = BOSS_INSET
POST_X, POST_Y = L / 2 - _post_off, W / 2 - _post_off
POSTS = [(sx * POST_X, sy * POST_Y) for sx in (-1, 1) for sy in (-1, 1)]


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

    # board standoffs, with blind pilot holes (Hole() without a depth would pierce the floor)
    so_pts = [(sx * SO_X, sy * SO_Y) for sx in (-1, 1) for sy in (-1, 1)]
    with Locations(*[(x, y, FLOOR_TOP) for x, y in so_pts]):
        Cylinder(SO_D / 2, SO_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    with Locations(*[(x, y, FLOOR_TOP + SO_H) for x, y in so_pts]):
        Hole(SO_HOLE_D / 2, depth=SO_H)

    # support pads under the key switches
    with Locations(*[(x, y, FLOOR_TOP) for x, y in KEY_XY]):
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

    # key, A/B slider and indicator openings in the key face
    with BuildSketch(Plane.XY.offset(Z_TOP - TOP_T - 0.1)):
        for d, y, kl, kw in KEY_HOLES.values():
            with Locations((L / 2 - d, y)):
                _rounded_rect(kl + 2 * CAP_CLEAR, kw + 2 * CAP_CLEAR, 99)
        d, y, kl, kw = AB_SLOT
        with Locations((L / 2 - d, y)):
            _rounded_rect(kl, kw, 99)
        with Locations((L / 2 - IND_FROM_FRONT, 0)):
            Circle(IND_D / 2 + 0.1)
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
assert NUB_H >= 0, f"flange would press the switch by {-NUB_H:.2f} mm: lower FLANGE_T or the board"


def _outline(d, y, lx, wy, grow=0.0):
    lx, wy = lx + 2 * grow, wy + 2 * grow
    return Pos(L / 2 - d, y) * RectangleRounded(lx, wy, min(lx, wy) / 2 - 0.01)


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
    mark = None
    if legend:
        # rotated 90 deg clockwise (seen from above) so the text reads upright when you hold the
        # remote pointing away from you (+X is "up" for the reader)
        mark = Pos(*at, 0) * Rot(0, 0, -90) * Text(legend, font_size=3.2 if len(legend) > 1 else 4.0,
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


# Finish (agreed 2026-11-13): grey shells, dyed-black caps and A/B knob, legends recessed and
# filled with white paint (wipe in, scrape the cap top clean).
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
        _outline(d, y, lx, wy), _outline(d, y, lx, wy, FLANGE_W + CAP_CLEAR),
        (L / 2 - SWITCHES[name][0], SWITCHES[name][1]), legend=LEGENDS[name])

if __name__ == "__main__":
    show(bottom, top.moved(Location((0, 0, 20))), *caps.values(),
         names=["bottom shell", "top shell (lifted)", *[f"cap {n}" for n in caps]],
         colors=["lightgray", "lightblue", *["dimgray"] * len(caps)])
