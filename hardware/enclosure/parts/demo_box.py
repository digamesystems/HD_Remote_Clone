# SPDX-License-Identifier: CERN-OHL-S-2.0
# Demo part for checking the toolchain: an open box with rounded corners,
# four screw bosses and an IR LED hole. Not a real design.
# Run with the hd_enclosure interpreter while OCP CAD Viewer is open.
from build123d import *
from ocp_vscode import show

L, W, H, WALL = 60, 35, 20, 2.0  # mm

with BuildPart() as case:
    Box(L, W, H)
    fillet(case.edges().filter_by(Axis.Z), radius=4)
    offset(amount=-WALL, openings=case.faces().sort_by(Axis.Z)[-1])  # shell, open top
    # four screw bosses inside the corners
    with Locations((0, 0, -H / 2 + WALL)):
        with GridLocations(L - 12, W - 12, 2, 2):
            Cylinder(3, H - WALL - 1, align=(Align.CENTER, Align.CENTER, Align.MIN))
            Hole(1.0)  # pilot for an M2 self-tapping screw (placeholder size)
    # 5 mm IR LED hole through the +X end wall
    with Locations(case.faces().sort_by(Axis.X)[-1]):
        Hole(2.6, depth=WALL)  # without depth, Hole() cuts through the far wall too

show(case, names=["demo case"])
