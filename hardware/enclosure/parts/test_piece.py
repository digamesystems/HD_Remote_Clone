# SPDX-License-Identifier: CERN-OHL-S-2.0
"""MJF tolerance test print for the remote case.

Turns the case's "placeholder; set from the test print" values into measured ones. All sizes come
from remote_box.py (via assembly.load_model), so the samples test what the case actually uses.

Parts:
  plate        2 mm plate (the case floor) with a 2 mm wall (the case walls). Samples:
               IR LED holes 5.2 / 5.4 / 5.6 mm with the outside countersink; the USB-C opening with
               its outside plug pocket; screw bosses with 1.6 / 1.8 / 2.0 mm pilots; SETUP pinholes
               1.4 / 1.6 / 1.8 mm; legends recessed 0.4 / 0.6 / 0.8 mm. Every sample is labelled.
  key_base     holds one real 6 mm x 13 mm tactile switch (legs through the base) at the case's
               board height, with four posts for the face
  key_face     2 mm key face with one key hole, screwed onto the posts (M2 self-tappers)
  cap_00..03   test keycaps with 0.0 / 0.1 / 0.2 / 0.3 mm play over the plunger, marked on top

  python test_piece.py            checks + show in OCP CAD Viewer
  python test_piece.py --export   checks + write STL and STEP to ../out/
"""
import argparse
import sys
from pathlib import Path

from build123d import *
import ocp_vscode

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import assembly as A                                      # noqa: E402

M = A.load_model()
WALL, FLOOR = M["WALL"], M["WALL"]
OUT = Path(__file__).resolve().parents[1] / "out"

LED_HOLES = (5.2, 5.4, 5.6)
PILOTS = (1.6, 1.8, 2.0)
PINHOLES = (1.4, 1.6, 1.8)
LEGEND_DEPTHS = (0.4, 0.6, 0.8)
PLAYS = (0.0, 0.1, 0.2, 0.3)
LABEL_DEPTH, LABEL_SIZE = 0.6, 3.0


def engrave(part, text, x, y, z, size=LABEL_SIZE, depth=LABEL_DEPTH, rot=0):
    """Recess text into the horizontal face at height z."""
    mark = Pos(x, y, 0) * Rot(0, 0, rot) * Text(text, font_size=size, font_style=FontStyle.BOLD)
    return part - extrude(Plane.XY.offset(z) * mark, amount=-depth)


# ---------------- tolerance plate ----------------
PL, PW = 110.0, 46.0
WALL_H, SAMPLE_Z = 16.0, FLOOR + 9.0        # wall height; LED / USB axis height
wall_y = PW / 2 - WALL / 2                  # wall along the back edge; its outer face is +Y


def plate():
    p = Box(PL, PW, FLOOR, align=(Align.CENTER, Align.CENTER, Align.MIN))
    p = fillet(p.edges().filter_by(Axis.Z), radius=3)
    p += Pos(-8, wall_y, FLOOR) * Box(86, WALL, WALL_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    # IR LED holes through the wall, countersunk on the outside (+Y), as on the case's front wall
    for x, d in zip((-44, -32, -20), LED_HOLES):
        axis = Location((x, wall_y, SAMPLE_Z), (90, 0, 0))     # local +Z -> world -Y
        p -= axis * Cylinder(d / 2, WALL * 2)
        csk = Location((x, PW / 2 - M["LED_CSK"] / 2 + 0.1, SAMPLE_Z), (-90, 0, 0))  # wide end at +Y
        p -= csk * Cone(d / 2, d / 2 + M["LED_CSK"] + 0.2, M["LED_CSK"] + 0.2)
        p = engrave(p, f"{d}", x - 3, wall_y - 6, FLOOR)
    # USB-C opening + outside plug pocket, as on the case's back wall
    face = Plane(origin=(18, PW / 2 + 0.5, SAMPLE_Z), x_dir=(1, 0, 0), z_dir=(0, -1, 0))
    p -= extrude(face * RectangleRounded(M["USB_W"], M["USB_H"], M["USB_CORNER_R"]), amount=WALL + 1)
    p -= extrude(face * RectangleRounded(M["USB_POCKET_W"], M["USB_POCKET_H"], M["USB_POCKET_R"]),
                 amount=M["USB_POCKET_DEPTH"] + 0.5)
    p = engrave(p, "USB", 13, wall_y - 6, FLOOR)
    # screw bosses at the board-standoff size, with three pilot sizes
    for x, d in zip((26, 37, 48), PILOTS):
        p += Pos(x, 2, FLOOR) * Cylinder(M["SO_D"] / 2, M["SO_H"], align=(Align.CENTER, Align.CENTER, Align.MIN))
        p -= Pos(x, 2, FLOOR + M["SO_H"]) * Cylinder(d / 2, M["SO_H"], align=(Align.CENTER, Align.CENTER, Align.MAX))
        p = engrave(p, f"{d}", x - 3, -5, FLOOR, size=2.6)
    # SETUP pinholes through the floor
    for x, d in zip((26, 37, 48), PINHOLES):
        p -= Pos(x, -13, 0) * Cylinder(d / 2, FLOOR * 3)
        p = engrave(p, f"{d}", x - 3, -19, FLOOR, size=2.6)
    # legend samples at three depths: ALL, a digit, an arrow
    for x, d in zip((-44, -24, -4), LEGEND_DEPTHS):
        p = engrave(p, "ALL", x - 4, -6, FLOOR, size=M["LEGEND_SIZE_WORD"], depth=d)
        p = engrave(p, "4", x + 6, -6, FLOOR, size=M["LEGEND_SIZE_DIGIT"], depth=d)
        tri = Pos(x + 1, -13, 0) * Polygon((2.2, 0), (-1.6, 2.4), (-1.6, -2.4))
        p -= extrude(Plane.XY.offset(FLOOR) * tri, amount=-d)
        p = engrave(p, f"{d}", x - 3, -20, FLOOR, size=2.6)
    return p


# ---------------- key-stack fixture ----------------
BOARD_TO_FACE = M["FACE_UNDER"] - (M["Z0"] + M["PCB_TOP_Z"])   # board top to key-face underside
KB, BASE_T, POST_D, POST_XY = 34.0, 3.0, 6.0, 12.0
KEY_HOLE_D = 8.0 + 2 * M["CAP_CLEAR"]                          # same as the case's group keys
SW_PIN_X, SW_PIN_Y = A.SW_PIN_X, A.SW_PIN_Y
posts = [(sx * POST_XY, sy * POST_XY) for sx in (-1, 1) for sy in (-1, 1)]


def key_base():
    b = Box(KB, KB, BASE_T, align=(Align.CENTER, Align.CENTER, Align.MIN))
    b = fillet(b.edges().filter_by(Axis.Z), radius=3)
    for x, y in posts:                                         # posts up to the face underside
        b += Pos(x, y, BASE_T) * Cylinder(POST_D / 2, BOARD_TO_FACE, align=(Align.CENTER, Align.CENTER, Align.MIN))
        b -= Pos(x, y, BASE_T + BOARD_TO_FACE) * Cylinder(
            M["SO_HOLE_D"] / 2, 8, align=(Align.CENTER, Align.CENTER, Align.MAX))
    for sx in (-1, 1):                                         # switch leg holes
        for sy in (-1, 1):
            b -= Pos(sx * SW_PIN_X, sy * SW_PIN_Y, 0) * Cylinder(0.7, BASE_T * 3)
    return engrave(b, "KEY", -5, -14, BASE_T, size=3)


def key_face():
    f = Pos(0, 0, BASE_T + BOARD_TO_FACE) * Box(KB, KB, M["TOP_T"], align=(Align.CENTER, Align.CENTER, Align.MIN))
    f = fillet(f.edges().filter_by(Axis.Z), radius=3)
    f -= Cylinder(KEY_HOLE_D / 2, 100)
    for x, y in posts:
        f -= Pos(x, y, 0) * Cylinder(A.PCB_HOLE_D / 2, 100)
    return f


def key_cap(play):
    """Group-key cap (8 mm round) as in the case, with the given play over the plunger at rest."""
    face_under = BASE_T + BOARD_TO_FACE
    plunger = BASE_T + M["SW_H"]
    flange_bot = face_under - M["FLANGE_T"]
    top = face_under + M["TOP_T"] + M["CAP_PROUD"]
    c = Pos(0, 0, face_under) * Cylinder(4.0, top - face_under, align=(Align.CENTER, Align.CENTER, Align.MIN))
    c = fillet(c.edges().group_by(Axis.Z)[-1], radius=M["CAP_TOP_R"])
    c += Pos(0, 0, flange_bot) * Cylinder(4.0 + M["CAP_CLEAR"] + M["FLANGE_W"], M["FLANGE_T"],
                                          align=(Align.CENTER, Align.CENTER, Align.MIN))
    nub = flange_bot - plunger - play                          # >0: nub down to the plunger; <0: relief
    if nub > 0:
        c += Pos(0, 0, flange_bot) * Cylinder(1.5, nub, align=(Align.CENTER, Align.CENTER, Align.MAX))
    elif nub < 0:
        c -= Pos(0, 0, flange_bot) * Cylinder(2.25, -nub, align=(Align.CENTER, Align.CENTER, Align.MIN))
    mark = Pos(0, 0, 0) * Text(f".{int(round(play * 10))}", font_size=3.6, font_style=FontStyle.BOLD)
    return c - extrude(Plane.XY.offset(top) * mark, amount=-M["LEGEND_DEPTH"])


def build():
    parts = {"plate": plate(), "key_base": key_base(), "key_face": key_face()}
    for p in PLAYS:
        parts[f"cap_{int(round(p * 10)):02d}"] = key_cap(p)
    return parts


def check(parts):
    fails, out = [], []

    def ok(cond, msg):
        out.append(("  ok   " if cond else "  FAIL ") + msg)
        if not cond:
            fails.append(msg)

    for n, p in parts.items():
        ok(p.is_valid and len(p.solids()) == 1, f"{n} is one valid solid")
    face, base = parts["key_face"], parts["key_base"]
    ok(A.ov(face, base) < 1e-6, "key face sits on the posts without overlapping them")
    plunger_top = BASE_T + M["SW_H"]
    for p in PLAYS:
        c = parts[f"cap_{int(round(p * 10)):02d}"]
        probe = Pos(0, 0, plunger_top) * Cylinder(1.75, 6, align=(Align.CENTER, Align.CENTER, Align.MIN))
        x = c & probe
        gap = x.bounding_box().min.Z - plunger_top if x is not None else 99
        ok(abs(gap - p) < 0.01, f"cap play {p:.1f}: measured {gap:.2f} mm")
        ok(A.ov(c, face) < 1e-6, f"cap {p:.1f} clears the face")
        ok(A.ov(c.moved(Location((0, 0, 0.05))), face) > 1e-6, f"cap {p:.1f} retained by the face")
    ok(abs((BASE_T + BOARD_TO_FACE) - (BASE_T + M["SW_H"]) - (M["FACE_UNDER"] - M["PLUNGER_TOP"])) < 1e-6,
       f"fixture reproduces the case's plunger-to-face gap ({M['FACE_UNDER'] - M['PLUNGER_TOP']:.2f} mm)")
    ok(WALL - M["USB_POCKET_DEPTH"] >= 0.8, "wall left behind the USB pocket >= 0.8 mm")
    ok(FLOOR - max(LEGEND_DEPTHS) >= 1.0, "floor left under the deepest legend >= 1 mm")
    return fails, out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--export", action="store_true", help="write STL and STEP to ../out/")
    ap.add_argument("--no-show", action="store_true")
    args = ap.parse_args()
    parts = build()
    fails, out = check(parts)
    print("\n".join(out))
    print(f"\n{len(out) - len(fails)} of {len(out)} checks pass")
    if args.export and not fails:
        OUT.mkdir(exist_ok=True)
        for n, p in parts.items():
            export_stl(p, str(OUT / f"test_piece_{n}.stl"))
        export_step(Compound(list(parts.values())), str(OUT / "test_piece.step"))
        print(f"wrote {len(parts)} STL files and test_piece.step to {OUT}")
    if not args.no_show:
        lay = {"plate": (0, 0, 0), "key_base": (0, -60, 0), "key_face": (0, -60, 12)}
        objs, names, colors = [], [], []
        for n, p in parts.items():
            if n.startswith("cap_"):
                i = int(n[-2:])
                loc = (45 + i * 14, -60, 14)
            else:
                loc = lay[n]
            objs.append(p.moved(Location(loc)))
            names.append(n)
            colors.append("dimgray" if n.startswith("cap_") else "lightgray")
        ocp_vscode.show(*objs, names=names, colors=colors, reset_camera=ocp_vscode.Camera.RESET)
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
