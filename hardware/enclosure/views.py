# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Documentation views of the remote case, generated from the model.

Writes to docs/images/enclosure/:
  enclosure_pcb_constraints.svg / .dxf   board outline, holes, part positions, keep-outs
  enclosure_section.svg                  lengthwise sections: centre line and an IR LED
  enclosure_views.svg                    top, bottom, front and back views (hidden lines dashed)
  enclosure_finish.png, enclosure_exploded.png   shaded screenshots (OCP CAD Viewer must be open)

  python views.py              all views
  python views.py --no-shots   drawings only (no viewer needed)

The internal parts are placeholders (see assembly.py); the drawings say so.
"""
import argparse
import time
from pathlib import Path

from build123d import *
from build123d import Drawing
import ocp_vscode

import assembly as A

OUT = Path(__file__).resolve().parents[2] / "docs" / "images" / "enclosure"
def rgb(r, g, b):
    """0-255 colour as the normalized tuple ExportSVG expects."""
    return (r / 255, g / 255, b / 255)


NOTE = "Preliminary. Internal parts are placeholders until the PCB is laid out."


def label(exp, text, x, y, size=2.5, layer="text", align=(Align.MIN, Align.CENTER)):
    exp.add_shape(Pos(x, y) * Text(text, font_size=size, align=align), layer=layer)


def flat(face_or_shape, z):
    """Move a horizontal planar shape down to z = 0."""
    return face_or_shape.moved(Location((0, 0, -z)))


def pcb_constraints(m, p, info):
    L, W, WALL = m["L"], m["W"], m["WALL"]
    fx = lambda d: L / 2 - d
    bot = m["Z0"] + m["PCB_BOT_Z"]
    board = p["PCB"].faces().sort_by(Axis.Z)[0]          # underside face: outline + holes + notch
    outline = flat(board, bot)

    keep = []                                            # (shape, label, label xy)
    rect = lambda cx, cy, lx, wy: Pos(cx, cy) * Rectangle(lx, wy)
    circ = lambda cx, cy, r: Pos(cx, cy) * Circle(r)
    bx0 = m["BATT_X"]
    keep.append((rect(bx0, 0, m["BATT_L"] + 1, m["BATT_W"] + 1), "battery under the board:\nno THT pins here",
                 (bx0 - m["BATT_L"] / 2 + 1, 2)))
    for x, y in m["KEY_XY"]:
        keep.append((circ(x, y, m["PAD_D"] / 2 + 0.25), None, None))
    so = [(sx * m["SO_X"], sy * m["SO_Y"]) for sx in (-1, 1) for sy in (-1, 1)]
    for x, y in so:
        keep.append((circ(x, y, 2.75), None, None))

    parts = []                                           # footprints of placed parts (top view)
    for name in ("key switches", "ESP32", "nRF socket", "USB-C", "IR LEDs", "battery JST",
                 "A/B switch", "indicator", "SETUP switch"):
        bb = p[name].bounding_box()
        parts.append((name, bb))

    exp = ExportSVG(margin=6, line_weight=0.15)
    exp.add_layer("outline", line_color=rgb(0, 0, 0), line_weight=0.35)
    exp.add_layer("parts", line_color=rgb(110, 110, 110), line_weight=0.12)
    exp.add_layer("keepout", line_color=rgb(200, 30, 30), line_type=LineType.ISO_DASH, line_weight=0.2)
    exp.add_layer("text", fill_color=rgb(30, 30, 30), line_color=None, line_weight=0)
    exp.add_layer("keeptext", fill_color=rgb(200, 30, 30), line_color=None, line_weight=0)
    exp.add_shape(outline.edges(), layer="outline")
    for shape, text, xy in keep:
        exp.add_shape(shape.edges(), layer="keepout")
        if text:
            for i, line in enumerate(text.split("\n")):
                label(exp, line, xy[0], xy[1] - i * 3.2, 2.3, "keeptext")
    for name, bb in parts:
        if name == "key switches":
            for x, y in m["KEY_XY"]:
                exp.add_shape(rect(x, y, 6, 6).edges(), layer="parts")
            continue
        exp.add_shape(rect((bb.min.X + bb.max.X) / 2, (bb.min.Y + bb.max.Y) / 2,
                           bb.size.X, bb.size.Y).edges(), layer="parts")
    # label text, and where it goes relative to the part's outline
    tags = {"ESP32": ("ESP32-WROOM-32E\n(antenna at edge)", "above"),
            "nRF socket": ("nRF24\nsocket", "above"), "USB-C": ("USB-C\n(1 mm recess)", "above"),
            "IR LEDs": ("IR\nLEDs", "inside"), "battery JST": ("JST", "below"),
            "A/B switch": ("A/B", "above"), "indicator": ("LED", "above"),
            "SETUP switch": ("SETUP\n(underside)", "below")}
    for name, bb in parts:
        if name not in tags:
            continue
        text, where = tags[name]
        lines = text.split("\n")
        for i, line in enumerate(lines):
            if where == "above":
                y = bb.max.Y + 1.6 + (len(lines) - 1 - i) * 2.6
            elif where == "below":
                y = bb.min.Y - 2.2 - i * 2.6
            else:
                y = (bb.min.Y + bb.max.Y) / 2 + (len(lines) / 2 - 0.5 - i) * 2.6
            label(exp, line, bb.min.X + (0.6 if where == "inside" else 0), y, 2.0)
    for name, (d, y) in m["SWITCHES"].items():
        label(exp, name, fx(d), y, 2.0, align=(Align.CENTER, Align.CENTER))
    label(exp, "M2 board standoffs (dashed: screw-head keep-out)", fx(14) - 40, -W / 2 - 4.5, 2.2)
    label(exp, "notch: battery leads", fx(133) - 2, -(W / 2 - WALL) - 3.2, 2.2)
    label(exp, "dashed circles under keys: support-pad keep-outs (board underside)", -L / 2, -W / 2 - 10, 2.2, "keeptext")
    label(exp, "PCB constraints from the enclosure, top view. Front (IR) end at the right.", -L / 2, W / 2 + 10, 3.2)
    label(exp, NOTE, -L / 2, W / 2 + 5.5, 2.4)
    label(exp, "Origin: board centre. Full dimensions: hardware/enclosure/README.md, parts/remote_box.py",
          -L / 2, -W / 2 - 14, 2.2)
    exp.write(OUT / "enclosure_pcb_constraints.svg")

    dxf = ExportDXF()
    dxf.add_layer("Edge.Cuts")
    dxf.add_shape(outline.edges(), layer="Edge.Cuts")
    dxf.write(OUT / "enclosure_pcb_outline.dxf")


SECTION_FILL = {"shells": rgb(205, 205, 200), "keycaps": rgb(60, 60, 60), "PCB": rgb(40, 120, 60),
                "battery": rgb(90, 120, 200), "key switches": rgb(20, 20, 20), "nRF module": rgb(130, 70, 160),
                "nRF socket": rgb(90, 90, 90), "ESP32": rgb(220, 140, 40), "USB-C": rgb(170, 170, 170),
                "IR LEDs": rgb(150, 180, 220), "indicator": rgb(120, 200, 120), "A/B slider": rgb(230, 150, 60),
                "A/B switch": rgb(230, 150, 60), "SETUP switch": rgb(210, 60, 60), "battery JST": rgb(240, 240, 240),
                "battery plug": rgb(220, 210, 180), "board screws": rgb(150, 150, 150)}


def sections(m, p):
    L, Z0 = m["L"], m["Z0"]
    solids = dict(p)
    solids.pop("battery leads")
    solids["shells"] = m["bottom"] + m["top"]
    exp = ExportSVG(margin=6, line_weight=0.1)
    exp.add_layer("text", fill_color=rgb(30, 30, 30), line_color=None, line_weight=0)
    exp.add_layer("ticks", line_color=rgb(30, 30, 30), line_weight=0.15)
    for name, col in SECTION_FILL.items():
        exp.add_layer(name, fill_color=col, line_color=rgb(30, 30, 30), line_weight=0.08)
    cuts = [(0.0, "Section A-A: centre line (y = 0)"),
            (m["LED_PITCH"] / 2, f"Section B-B: through an IR LED (y = {m['LED_PITCH'] / 2:.0f} mm)")]
    offset = 0.0
    for y, title in cuts:
        pl = Plane(origin=(0, y, 0), x_dir=(1, 0, 0), z_dir=(0, -1, 0))   # local x = X, local y = Z
        for name in SECTION_FILL:
            shape = solids.get(name)
            if shape is None:
                continue
            sk = section(shape, section_by=pl)
            if sk is None or not sk.faces():
                continue
            for f in sk.faces():
                exp.add_shape(Pos(0, offset) * pl.to_local_coords(f), layer=name)
        label(exp, title, -L / 2, offset + m["Z_TOP"] + 6, 3.0)
        # height ticks on the right
        ticks = [("outside bottom", Z0), ("floor", m["FLOOR_TOP"]), ("board top", Z0 + m["PCB_TOP_Z"]),
                 ("split", m["SPLIT"]), ("key face underside", m["FACE_UNDER"]), ("top", m["Z_TOP"])]
        for i, (t, z) in enumerate(ticks):
            exp.add_shape(Pos(0, offset) * Edge.make_line((L / 2 + 3, z), (L / 2 + 9, z)), layer="ticks")
            label(exp, f"{t} {z - Z0:.1f}", L / 2 + 10, offset + z, 2.2)
        offset -= (m["Z_TOP"] - Z0) + 16
    label(exp, "Front (IR) end at the right. Heights in mm above the outside bottom.", -L / 2, offset + 8, 2.4)
    label(exp, NOTE, -L / 2, offset + 4, 2.4)
    exp.write(OUT / "enclosure_section.svg")


def outer_views(m):
    L, W, Z0, ZT = m["L"], m["W"], m["Z0"], m["Z_TOP"]
    closed = m["bottom"] + m["top"]
    for c in m["caps"].values():
        closed = closed + c
    views = [("Top", (0, 0, 1), (0, 1, 0), (0, 0)),
             ("Bottom (mirrored left-right)", (0, 0, -1), (0, 1, 0), (0, -(W + 22))),
             ("Front (IR) end", (1, 0, 0), (0, 0, 1), (-L / 2 + W / 2, -2 * (W + 22))),
             ("Back (USB) end", (-1, 0, 0), (0, 0, 1), (-L / 2 + W / 2 + W + 25, -2 * (W + 22)))]
    exp = ExportSVG(margin=6, line_weight=0.15)
    exp.add_layer("visible", line_color=rgb(0, 0, 0), line_weight=0.2)
    exp.add_layer("hidden", line_color=rgb(150, 150, 150), line_type=LineType.ISO_DASH, line_weight=0.1)
    exp.add_layer("text", fill_color=rgb(30, 30, 30), line_color=None, line_weight=0)
    for title, look_from, look_up, (ox, oy) in views:
        ends = not title.startswith(("Top", "Bottom"))      # end views: hidden lines are clutter
        d = Drawing(closed, look_at=(0, 0, 0), look_from=look_from, look_up=look_up, with_hidden=not ends)
        bb = d.visible_lines.bounding_box()
        shift = Pos(ox - (bb.min.X + bb.max.X) / 2 + (0 if title.startswith(("Top", "Bottom")) else bb.size.X / 2 - W / 2),
                    oy - (bb.min.Y + bb.max.Y) / 2)
        exp.add_shape(shift * d.visible_lines, layer="visible")
        if not ends and d.hidden_lines is not None:
            exp.add_shape(shift * d.hidden_lines, layer="hidden")
        top_y = oy + bb.size.Y / 2 + 4
        left_x = ox - (bb.size.X / 2 if title.startswith(("Top", "Bottom")) else W / 2)
        label(exp, title, left_x, top_y, 3.0)
    label(exp, f"Remote case, {L:.0f} x {W:.0f} x {ZT - Z0:.0f} mm. Hidden edges dashed (top and bottom views). {NOTE}",
          -L / 2, W / 2 + 14, 2.4)
    exp.write(OUT / "enclosure_views.svg")


def _crop(path, pad=20):
    """Trim the blank margins around a screenshot."""
    from PIL import Image, ImageChops
    im = Image.open(path)
    if im.mode == "RGBA":                      # transparent background: crop to the opaque pixels
        box = im.getchannel("A").getbbox()
    else:
        im = im.convert("RGB")
        box = ImageChops.difference(im, Image.new("RGB", im.size, im.getpixel((0, 0)))).getbbox()
    if box:
        l, t, r, b = box
        im.crop((max(l - pad, 0), max(t - pad, 0), min(r + pad, im.width), min(b + pad, im.height))).save(path)


def screenshots(m, p):
    """Shaded views from OCP CAD Viewer, camera reset to its standard isometric view."""
    for name, args in (("enclosure_finish.png", (0.0, 1.0, 1.0, True)),        # finished look
                       ("enclosure_exploded.png", (30.0, 0.2, 0.2, False))):   # see-through, exploded
        A.show_assembly(m, p, *args, reset_camera=True)
        time.sleep(4)
        ocp_vscode.save_screenshot(str(OUT / name))
        time.sleep(3)
        _crop(OUT / name)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-shots", action="store_true", help="skip the viewer screenshots")
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    m = A.load_model()
    p, info = A.build_parts(m)
    pcb_constraints(m, p, info)
    print("wrote enclosure_pcb_constraints.svg, enclosure_pcb_outline.dxf")
    sections(m, p)
    print("wrote enclosure_section.svg")
    outer_views(m)
    print("wrote enclosure_views.svg")
    if not args.no_shots:
        screenshots(m, p)
        print("wrote enclosure_finish.png, enclosure_exploded.png")


if __name__ == "__main__":
    main()
