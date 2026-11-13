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
CORNER_R = 4           # vertical corner radius, mm
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

# --- Key layout (agreed 2026-11-13) ---
# name -> (mm from the front end to the key centre, y, cap length, cap width, corner radius)
# y > 0 is the left side when you hold the remote pointing away from you.
KEYS = {"UP":  (32, 0, 13, 26, 3.0), "DOWN": (49, 0, 13, 26, 3.0),
        "1": (68, 14.4, 7, 7, 3.5), "2": (68, 4.8, 7, 7, 3.5),
        "3": (68, -4.8, 7, 7, 3.5), "4": (68, -14.4, 7, 7, 3.5),
        "ALL": (82, 0, 8, 22, 4.0)}
KEY_XY = [(L / 2 - k[0], k[1]) for k in KEYS.values()]
CAP_CLEAR = 0.3        # gap around each keycap in its hole (placeholder; test print)
IND_FROM_FRONT, IND_D = 14.8, 5.0   # bicolor indicator, 5 mm LED standing 2 mm proud

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
POSTS = [(sx * (L / 2 - BOSS_INSET), sy * (W / 2 - BOSS_INSET)) for sx in (-1, 1) for sy in (-1, 1)]


def _rounded_rect(length, width, radius):
    return RectangleRounded(length, width, min(radius, min(length, width) / 2 - 0.01))


with BuildPart() as body:
    # closed hollow box: outer shape minus the inner cavity
    with BuildSketch(Plane.XY.offset(Z0)):
        RectangleRounded(L, W, CORNER_R)
    extrude(amount=Z_TOP - Z0)
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

    # key and indicator holes in the key face
    with BuildSketch(Plane.XY.offset(Z_TOP - TOP_T - 0.1)):
        for (x, y), (_, _, kl, kw, kr) in zip(KEY_XY, KEYS.values()):
            with Locations((x, y)):
                _rounded_rect(kl + 2 * CAP_CLEAR, kw + 2 * CAP_CLEAR, kr + CAP_CLEAR)
        with Locations((L / 2 - IND_FROM_FRONT, 0)):
            Circle(IND_D / 2 + 0.1)
    extrude(amount=TOP_T + 0.2, mode=Mode.SUBTRACT)

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
    lip_x = L / 2 - BOSS_INSET - BOSS_D / 2 - 0.5
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

if __name__ == "__main__":
    show(bottom, top.moved(Location((0, 0, 20))), names=["bottom shell", "top shell (lifted)"],
         colors=["lightgray", "lightblue"])
