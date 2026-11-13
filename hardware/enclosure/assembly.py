# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Assembly check for the remote case: placeholder internal parts plus every fit check.

Builds the case from parts/remote_box.py, adds placeholder models of the parts inside it (board,
switches, LEDs, USB-C, ESP32, nRF24, battery, A/B slider, ...), runs the fit checks, and shows the
result in OCP CAD Viewer. Exits non-zero if any check fails.

  python assembly.py                     checks + exploded view (top lifted 20 mm)
  python assembly.py --explode 0 --top-alpha 0.2      assembled, see-through top
  python assembly.py --finish            assembled, in the chosen finish colours
  python assembly.py --no-show           checks only
  python assembly.py --sweep CORNER_R 8 10 12 14      rebuild and check at each value

The internal parts are placeholders for clearance checking, not a board layout. Their positions
follow the layout explored on 2026-11-13 (see README.md) and should be replaced by the board's
STEP export once the PCB is laid out.
"""
import argparse
import re
import sys
from itertools import combinations
from pathlib import Path

from build123d import *
import ocp_vscode

MODEL = Path(__file__).parent / "parts" / "remote_box.py"

# ---- placeholder board layout (mm from the front end unless noted) ----
ESP_FRONT, ESP_L, ESP_W, ESP_H, ESP_ANT = 89.0, 25.5, 18.0, 3.1, 6.0   # WROOM-32E, antenna at +Y edge
NRF_BACK = 136.5                       # nRF24 module's back end; antenna end points forward
NRF_SOCKET_H, NRF_SPACER_H, NRF_PCB_T, NRF_PARTS_H = 8.5, 2.5, 1.0, 1.2
NRF_L, NRF_W, NRF_HDR_FROM_END = 29.0, 15.3, 2.5
PCB_GAP = 0.3                          # board edge to the inside of the walls
POST_NOTCH_CLEAR = 1.0                 # board notch radius beyond the post radius
PCB_HOLE_D, HEAD_D, HEAD_H, SCREW_L = 2.2, 3.8, 1.3, 6.0   # M2 board screws
SW_PIN_X, SW_PIN_Y = 3.25, 2.25        # 6 mm tactile switch pins at (+-6.5/2, +-4.5/2)
MIN_WEB = 2.0                          # thinnest key-face material allowed between openings
MIN_USB_ROUTING = 3.0                  # USB signal pads to the nearest nRF socket pin pad
# USB-C HRO TYPE-C-31-M-12 footprint (KiCad 10): mating face at fy = +3.65
USB_FACE_FY = 3.65
USB_SMD = [(-3.25, .6), (-2.45, .6), (-1.75, .3), (-1.25, .3), (-0.75, .3), (-0.25, .3),
           (0.25, .3), (0.75, .3), (1.25, .3), (1.75, .3), (2.45, .6), (3.25, .6)]
USB_SH = [(-4.32, -3.13, 2.1), (4.32, -3.13, 2.1), (-4.32, 1.05, 1.6), (4.32, 1.05, 1.6)]
# IR LED footprint LED_D5.0mm_Horizontal_O1.27mm_Z3.0mm: pads at fy 0, dome tip at fy 9.87
# Battery connector: JST PH S2B-PH-K side entry (footprint 5.9 x 7.6 mm; height ~6, check the
# datasheet) on top of the board, mouth facing the front. The mating plug's envelope is approximate.
JST_MOUTH_D, JST_Y = 140.0, -9.15          # mouth position (mm from the front), centre y
JST_W, JST_DEPTH, JST_H = 5.9, 7.6, 6.0
PLUG_L, PLUG_H = 6.0, 4.5                  # plug housing in front of the mouth
# Battery leads go from the plug, across to a notch in the -Y (right-hand) board edge, down through
# it and under the board to the cell's back end. Keep-out bundle 2 mm wide x 2 mm tall.
NOTCH_FROM, NOTCH_TO, NOTCH_DEPTH = 128.0, 133.0, 3.0   # board-edge notch, mm from the front
LEAD_W, LEAD_H = 2.0, 2.0

FINISH = {"shell": "#8a8a88", "key": "#2a2a2a", "legend": "#f4f4f0", "indicator": "#d8e8d0",
          "ir": "#9fb4d8", "metal": "#c0c0c0"}


def load_model(overrides=None):
    """Run remote_box.py, optionally replacing top-level constants (NAME = value)."""
    src = MODEL.read_text(encoding="utf-8-sig")
    for name, value in (overrides or {}).items():
        src, n = re.subn(rf"^{name} = [^#\n]+", f"{name} = {value} ", src, count=1, flags=re.M)
        if n != 1:
            raise SystemExit(f"constant {name} not found in {MODEL.name}")
    ns = {"__name__": "remote_box", "__file__": str(MODEL)}
    exec(compile(src, str(MODEL), "exec"), ns)
    return ns


def ov(a, b):
    x = a & b
    return 0.0 if x is None else x.volume


def build_parts(m):
    """Placeholder internal parts, positioned from the model's constants."""
    L, W, WALL, Z0 = m["L"], m["W"], m["WALL"], m["Z0"]
    fx = lambda d: L / 2 - d
    top = Z0 + m["PCB_TOP_Z"]
    bot = Z0 + m["PCB_BOT_Z"]
    p = {}

    # board: outline follows the inner wall; notches round the case posts; holes for the standoffs
    so_pts = [(sx * m["SO_X"], sy * m["SO_Y"]) for sx in (-1, 1) for sy in (-1, 1)]
    with BuildPart() as pcb:
        with BuildSketch(Plane.XY.offset(bot)):
            RectangleRounded(L - 2 * (WALL + PCB_GAP), W - 2 * (WALL + PCB_GAP),
                             max(m["CORNER_R"] - WALL - PCB_GAP, 0.5))
            with Locations(*m["POSTS"]):
                Circle(m["BOSS_D"] / 2 + POST_NOTCH_CLEAR, mode=Mode.SUBTRACT)
            with Locations(*so_pts):
                Circle(PCB_HOLE_D / 2, mode=Mode.SUBTRACT)
            # notch in the -Y edge for the battery leads
            edge = -(W / 2 - WALL - PCB_GAP)
            with Locations((fx((NOTCH_FROM + NOTCH_TO) / 2), edge + NOTCH_DEPTH / 2 - 0.5)):
                Rectangle(NOTCH_TO - NOTCH_FROM, NOTCH_DEPTH + 1.0, mode=Mode.SUBTRACT)
        extrude(amount=m["PCB_T"])
    p["PCB"] = pcb.part

    with BuildPart() as sw:                       # key switches: body + pins through the board
        for x, y in m["KEY_XY"]:
            with Locations((x, y, top)):
                Box(6, 6, 3.5, align=(Align.CENTER, Align.CENTER, Align.MIN))
            with Locations((x, y, bot)):
                with GridLocations(2 * SW_PIN_X, 2 * SW_PIN_Y, 2, 2):
                    Cylinder(0.5, m["PIN_PROTRUDE"] + m["PCB_T"],
                             align=(Align.CENTER, Align.CENTER, Align.MAX))
    p["key switches"] = sw.part

    with BuildPart() as screws:                   # M2 board screws, shank at the pilot diameter
        for x, y in so_pts:
            with Locations((x, y, top)):
                Cylinder(HEAD_D / 2, HEAD_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
                Cylinder(m["SO_HOLE_D"] / 2, SCREW_L, align=(Align.CENTER, Align.CENTER, Align.MAX))
    p["board screws"] = screws.part

    with BuildPart() as cell:
        with Locations((m["BATT_X"], 0, m["FLOOR_TOP"])):
            Box(m["BATT_L"], m["BATT_W"], m["BATT_T"], align=(Align.CENTER, Align.CENTER, Align.MIN))
        fillet(cell.edges().filter_by(Axis.X), radius=1.5)
    p["battery"] = cell.part

    # USB-C: stadium shell (face recessed), signal pads at its rear, shell legs through the board
    face_x = -L / 2 + m["USB_RECESS"]
    uw = lambda fx_, fy_: (face_x + (USB_FACE_FY - fy_), fx_)
    with BuildPart() as usb:
        with BuildSketch(Plane(origin=(face_x, 0, top + 3.26 / 2), x_dir=(0, 1, 0), z_dir=(1, 0, 0))):
            SlotOverall(8.94, 3.26)
        extrude(amount=7.3)
    p["USB-C"] = usb.part
    usb_rear = uw(0, -4.045 - 0.725)[0]
    usb_sh_front = uw(0, 1.05 + 0.8)[0]

    # IR LEDs from the footprint, dome tips LED_PROUD beyond the front face
    tip_x = L / 2 + m["LED_PROUD"]
    leds = None
    for yc in (-m["LED_PITCH"] / 2, m["LED_PITCH"] / 2):
        with BuildPart(Plane(origin=(tip_x, yc, top + 3.0), z_dir=(1, 0, 0))) as led:
            with Locations((0, 0, -2.5)):
                Sphere(2.5)
                Cylinder(2.5, 8.6 - 2.5 - 1.0, align=(Align.CENTER, Align.CENTER, Align.MAX))
            with Locations((0, 0, -8.6 + 1.0)):
                Cylinder(2.9, 1.0, align=(Align.CENTER, Align.CENTER, Align.MAX))
        leds = led.part if leds is None else leds + led.part
    p["IR LEDs"] = leds
    led_pad_d = L / 2 - (tip_x - 9.87)            # pads, mm from the front

    # battery connector, mating plug, and the lead path down through the board notch
    p["battery JST"] = Pos(fx(JST_MOUTH_D + JST_DEPTH / 2), JST_Y, top) * Box(
        JST_DEPTH, JST_W, JST_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    p["battery plug"] = Pos(fx(JST_MOUTH_D - PLUG_L / 2), JST_Y, top) * Box(
        PLUG_L, JST_W, PLUG_H, align=(Align.CENTER, Align.CENTER, Align.MIN))
    edge = -(W / 2 - WALL - PCB_GAP)
    n_mid = (NOTCH_FROM + NOTCH_TO) / 2
    y_in = edge + NOTCH_DEPTH / 2              # lead centre line inside the notch
    plug_front = JST_MOUTH_D - PLUG_L
    batt_back = L / 2 - (m["BATT_X"] - m["BATT_L"] / 2)
    leads = (
        # forward out of the plug, on top of the board
        Pos(fx((plug_front + n_mid) / 2), JST_Y, top) * Box(plug_front - n_mid + LEAD_W, LEAD_W, LEAD_H,
                                                           align=(Align.CENTER, Align.CENTER, Align.MIN))
        # across to the notch
        + Pos(fx(n_mid), (JST_Y + y_in) / 2, top) * Box(LEAD_W, JST_Y - y_in + LEAD_W, LEAD_H,
                                                       align=(Align.CENTER, Align.CENTER, Align.MIN))
        # down through the notch to the floor
        + Pos(fx(n_mid), y_in, m["FLOOR_TOP"]) * Box(LEAD_W, LEAD_W, top + LEAD_H - m["FLOOR_TOP"],
                                                    align=(Align.CENTER, Align.CENTER, Align.MIN))
        # under the board, forward to the cell's back end
        + Pos(fx((n_mid + batt_back) / 2), y_in, m["FLOOR_TOP"]) * Box(n_mid - batt_back, LEAD_W, LEAD_H,
                                                                       align=(Align.CENTER, Align.CENTER, Align.MIN)))
    p["battery leads"] = leads

    # nRF24 on its socket, antenna end forward
    hdr_d = NRF_BACK - NRF_HDR_FROM_END
    z_mod = top + NRF_SOCKET_H + NRF_SPACER_H
    p["nRF socket"] = Pos(fx(hdr_d), 0, top) * Box(5.1, 10.2, NRF_SOCKET_H,
                                                   align=(Align.CENTER, Align.CENTER, Align.MIN))
    p["nRF module"] = (Pos(fx(hdr_d), 0, top + NRF_SOCKET_H) * Box(2.5, 10.2, NRF_SPACER_H,
                                                                   align=(Align.CENTER, Align.CENTER, Align.MIN))
                       + Pos(fx(NRF_BACK - NRF_L / 2), 0, z_mod) * Box(NRF_L, NRF_W, NRF_PCB_T,
                                                                      align=(Align.CENTER, Align.CENTER, Align.MIN))
                       + Pos(fx(NRF_BACK - 9), 0, z_mod + NRF_PCB_T) * Box(8, 10, NRF_PARTS_H,
                                                                          align=(Align.CENTER, Align.CENTER, Align.MIN)))
    nrf_top = z_mod + NRF_PCB_T + NRF_PARTS_H

    # ESP32 across the board, antenna end at the +Y board edge
    edge_y = W / 2 - WALL - PCB_GAP
    p["ESP32"] = Pos(fx(ESP_FRONT + ESP_W / 2), edge_y - ESP_L / 2, top) * Box(
        ESP_W, ESP_L, ESP_H, align=(Align.CENTER, Align.CENTER, Align.MIN))

    # A/B: side-actuated switch + printed slider (knob, retaining plate, blade, fork)
    bx, by, bh = m["AB_SW_BODY"]
    ax_, ay_w, ah = m["AB_SW_ACT"]
    sw_x, pos_a = fx(m["AB_FROM_FRONT"]), -m["AB_TRAVEL"] / 2
    face_under = m["FACE_UNDER"]
    p["A/B switch"] = (Pos(sw_x, 0, top) * Box(bx, by, bh, align=(Align.CENTER, Align.CENTER, Align.MIN))
                       + Pos(sw_x - bx / 2 - ax_ / 2, pos_a, top + bh / 2 - ah / 2)
                       * Box(ax_, ay_w, ah, align=(Align.CENTER, Align.CENTER, Align.MIN)))
    kl, kw = m["AB_KNOB_L"], m["AB_KNOB_W"]
    knob = extrude(Plane.XY.offset(face_under) * Pos(sw_x, pos_a) * RectangleRounded(kl, kw, kl / 2 - 0.01),
                   amount=m["TOP_T"] + 1.0)
    plate = Pos(sw_x, pos_a, face_under) * Box(kl + 4, kw + 4, 1.0, align=(Align.CENTER, Align.CENTER, Align.MAX))
    blade_x = sw_x - bx / 2 - 0.2 - 0.8
    stem = Pos(blade_x, pos_a, face_under - 1.0) * Box(
        1.6, 6.0, (face_under - 1.0) - (top + bh + 0.5), align=(Align.CENTER, Align.CENTER, Align.MAX))
    prong_bot = top + bh / 2 - ah / 2 + 0.2
    prongs = [Pos(blade_x, pos_a + s * (ay_w / 2 + 0.15 + 0.6), top + bh + 0.5)
              * Box(1.6, 1.2, (top + bh + 0.5) - prong_bot, align=(Align.CENTER, Align.CENTER, Align.MAX))
              for s in (-1, 1)]
    p["A/B slider"] = knob + plate + stem + prongs[0] + prongs[1]

    # SETUP: 5 mm tactile switch on the board's underside, plunger down
    p["SETUP switch"] = Pos(fx(m["SETUP_FROM_FRONT"]), m["SETUP_Y"], bot) * Box(
        6, 6, 5, align=(Align.CENTER, Align.CENTER, Align.MAX))

    # indicator: 5 mm LED standing on the board, dome 2 mm proud of the face
    tip = m["Z_TOP"] + 2.0
    c = Pos(fx(m["IND_FROM_FRONT"]), 0, tip - m["IND_D"] / 2)
    p["indicator"] = c * Sphere(m["IND_D"] / 2) + c * Cylinder(
        m["IND_D"] / 2, tip - m["IND_D"] / 2 - top, align=(Align.CENTER, Align.CENTER, Align.MAX))

    caps_all = None
    for cap in m["caps"].values():
        caps_all = cap if caps_all is None else caps_all + cap
    p["keycaps"] = caps_all

    info = {"usb_rear_d": L / 2 - usb_rear, "usb_sh_front_x": usb_sh_front,
            "board_back_x": -L / 2 + WALL + PCB_GAP, "hdr_d": hdr_d, "nrf_top": nrf_top,
            "led_pad_d": led_pad_d, "sw_x": sw_x, "body": (bx, by, bh)}
    return p, info


# parts that touch or pass through each other by design
ALLOWED = [{"key switches", "PCB"}, {"A/B switch", "PCB"}, {"SETUP switch", "PCB"},
           {"indicator", "PCB"}, {"keycaps", "key switches"},
           {"battery leads", "battery plug"}, {"battery leads", "battery"}]


def check(m, p, info):
    """Run every fit check. Returns a list of failure messages (empty = all pass)."""
    fails, out = [], []
    bottom, top = m["bottom"], m["top"]
    shells = bottom + top
    L, W, WALL = m["L"], m["W"], m["WALL"]
    fx = lambda d: L / 2 - d

    def ok(cond, msg):
        out.append(("  ok   " if cond else "  FAIL ") + msg)
        if not cond:
            fails.append(msg)

    ok(bottom.is_valid and top.is_valid, "shells are valid solids")
    ok(ov(bottom, top) < 1e-6, "shells don't overlap")
    for n, part in p.items():
        ok(ov(part, shells) < 1e-6, f"{n} clears both shells")
    bad = [(a, b) for a, b in combinations(p, 2) if {a, b} not in ALLOWED and ov(p[a], p[b]) > 1e-6]
    ok(not bad, f"parts clear each other {bad or ''}")

    # key face
    def stadium(d, y, lx, wy, grow=0.0):
        lx, wy = lx + 2 * grow, wy + 2 * grow
        return Pos(fx(d), y) * RectangleRounded(lx, wy, min(lx, wy) / 2 - 0.01)
    holes = {n: stadium(*v, grow=m["CAP_CLEAR"]) for n, v in m["KEY_HOLES"].items()}
    holes["A/B"] = stadium(*m["AB_SLOT"])
    holes["indicator"] = Pos(fx(m["IND_FROM_FRONT"]), 0) * Circle(m["IND_D"] / 2 + 0.1)
    web, a, b = min((holes[a].distance_to(holes[b]), a, b) for a, b in combinations(holes, 2))
    ok(web >= MIN_WEB, f"key face web >= {MIN_WEB} mm (thinnest {web:.1f}, {a}-{b})")

    # keycaps
    caps = m["caps"]
    travel = m["CAP_PLAY"] + 0.25 + 0.2
    ok(all(c.is_valid for c in caps.values()), "keycaps are valid solids")
    ok(not [1 for a, b in combinations(caps, 2) if ov(caps[a], caps[b]) > 1e-6], "keycaps clear each other")
    blocked = [n for n, c in caps.items() if ov(c.moved(Location((0, 0, -travel))), shells) > 1e-6]
    ok(not blocked, f"keycaps free when pressed {travel:.2f} mm {blocked or ''}")
    loose = [n for n, c in caps.items() if ov(c.moved(Location((0, 0, 0.05))), top) < 1e-6]
    ok(not loose, f"keycaps retained by the face {loose or ''}")
    unfilled = [n for n, f in m["legend_fill"].items() if f is None or f.volume < 0.1]
    ok(not unfilled, f"every keycap legend is cut {unfilled or ''}")

    # A/B slider at both ends of its travel
    bx, by, bh = info["body"]
    body = Pos(info["sw_x"], 0, m["Z0"] + m["PCB_TOP_Z"]) * Box(bx, by, bh, align=(Align.CENTER, Align.CENTER, Align.MIN))
    for dy, end in ((0, "A"), (m["AB_TRAVEL"], "B")):
        s = p["A/B slider"].moved(Location((0, dy, 0)))
        ok(ov(s, body) < 1e-6 and ov(s, top) < 1e-6, f"A/B slider clear at the {end} end")

    # end walls: openings on the flat part, USB pads on the board, routing room, heights
    flat = W / 2 - m["CORNER_R"]
    led_edge = m["LED_PITCH"] / 2 + m["LED_HOLE_D"] / 2 + m["LED_CSK"]
    ok(led_edge <= flat - 0.5, f"IR LED countersinks on the flat front wall (+-{led_edge:.1f} of +-{flat:.1f})")
    ok(m["USB_W"] / 2 <= flat - 0.5, "USB-C opening on the flat back wall")
    ok(info["usb_sh_front_x"] > info["board_back_x"] + 0.3, "USB-C shell-leg pads on the board")
    gap = info["usb_rear_d"] - (info["hdr_d"] + 1.27 + 0.85)
    ok(gap >= MIN_USB_ROUTING, f"USB-C routing room to the nRF socket >= {MIN_USB_ROUTING} mm ({gap:.1f})")
    batt_back = m["L"] / 2 - (m["BATT_X"] - m["BATT_L"] / 2)
    ok(info["hdr_d"] - 2.55 > batt_back, "nRF socket pins clear of the battery")
    ok(JST_MOUTH_D > batt_back + 2, "battery JST pins clear of the battery")
    ribs, cell = m["batt_ribs"], p["battery"]
    ok(ribs.is_valid and ov(ribs, cell) < 1e-6, "battery ribs clear the cell")
    ok(abs(ov(ribs, bottom) - ribs.volume) < 1e-3, "battery ribs are part of the bottom shell")
    play = min(cell.distance_to(r) for r in ribs.solids())
    ok(0.3 <= play <= 1.0, f"battery located by the ribs (play {play:.2f} mm per side)")
    near = min(ribs.distance_to(Pos(x, y, m["FLOOR_TOP"]) * Cylinder(m["PAD_D"] / 2, 1,
               align=(Align.CENTER, Align.CENTER, Align.MIN))) for x, y in m["KEY_XY"])
    ok(near >= 1.0, f"battery ribs clear of the key pads ({near:.1f} mm)")
    so_near = min(ribs.distance_to(Pos(sx * m["SO_X"], sy * m["SO_Y"], m["FLOOR_TOP"]) * Cylinder(
        m["SO_D"] / 2, 1, align=(Align.CENTER, Align.CENTER, Align.MIN))) for sx in (-1, 1) for sy in (-1, 1))
    ok(so_near >= 1.0, f"battery ribs clear of the board standoffs ({so_near:.1f} mm)")
    for n in ("battery leads", "SETUP switch", "key switches", "PCB"):
        ok(ov(ribs, p[n]) < 1e-6, f"battery ribs clear the {n}")
    rib_top = m["FLOOR_TOP"] + m["BATT_RIB_H"]
    ok(rib_top < m["Z0"] + m["PCB_BOT_Z"] - m["PIN_PROTRUDE"] - 0.5,
       "battery ribs below the pin-tip zone")
    lead_in_notch = ov(p["battery leads"], p["PCB"]) < 1e-6
    ok(lead_in_notch, "battery leads pass through the board notch without touching the board")
    head = m["FACE_UNDER"] - info["nrf_top"]
    ok(head >= 0.5, f"nRF24 module under the key face (clearance {head:.1f} mm; want >= 0.5)")
    return fails, out


def _camera(reset):
    """Viewer options: reset to the standard isometric view (for screenshots), else keep."""
    return {"reset_camera": ocp_vscode.Camera.RESET} if reset else {}


def show_assembly(m, p, explode, top_alpha, bottom_alpha, finish, reset_camera=False):
    up = Location((0, 0, explode))
    lifted = {"keycaps", "A/B slider"}
    objs = [m["bottom"], m["top"].moved(up)]
    names = ["bottom shell", "top shell"]
    if finish:
        colors = [FINISH["shell"], FINISH["shell"]]
        alphas = [1.0, 1.0]
        fill = None
        for f in m["legend_fill"].values():
            if f is not None:
                fill = f if fill is None else fill + f
        extra = {"keycaps": FINISH["key"], "A/B slider": FINISH["key"], "indicator": FINISH["indicator"],
                 "IR LEDs": FINISH["ir"], "USB-C": FINISH["metal"]}
        for n, col in extra.items():
            objs.append(p[n].moved(up) if n in lifted else p[n]); names.append(n); colors.append(col)
            alphas.append(1.0)
        objs.append(fill.moved(up)); names.append("legend fill"); colors.append(FINISH["legend"]); alphas.append(1.0)
        ocp_vscode.show(*objs, names=names, colors=colors, alphas=alphas, metalness=0.0, roughness=0.9,
                        **_camera(reset_camera))
        return
    palette = {"PCB": "darkgreen", "key switches": "black", "board screws": "silver", "battery": "royalblue",
               "USB-C": "silver", "IR LEDs": "lightskyblue", "nRF socket": "dimgray", "nRF module": "purple",
               "ESP32": "darkorange", "A/B switch": "orange", "A/B slider": "orange", "SETUP switch": "red",
               "indicator": "limegreen", "keycaps": "dimgray", "battery JST": "white",
               "battery plug": "beige", "battery leads": "red"}
    colors, alphas = ["lightgray", "lightblue"], [bottom_alpha, top_alpha]
    for n, part in p.items():
        objs.append(part.moved(up) if n in lifted else part)
        names.append(n); colors.append(palette[n])
        alphas.append(0.6 if n in ("PCB", "nRF module", "battery") else 1.0)
    ocp_vscode.show(*objs, names=names, colors=colors, alphas=alphas, **_camera(reset_camera))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--explode", type=float, default=20.0, help="lift the top shell by this many mm")
    ap.add_argument("--top-alpha", type=float, default=0.7)
    ap.add_argument("--bottom-alpha", type=float, default=1.0)
    ap.add_argument("--finish", action="store_true", help="assembled, in the finish colours")
    ap.add_argument("--no-show", action="store_true")
    ap.add_argument("--sweep", nargs="+", metavar=("NAME", "VALUE"),
                    help="rebuild and check with a model constant set to each value")
    args = ap.parse_args()

    if args.sweep:
        name, values = args.sweep[0], args.sweep[1:]
        any_fail = False
        for v in values:
            m = load_model({name: v})
            p, info = build_parts(m)
            fails, _ = check(m, p, info)
            any_fail |= bool(fails)
            print(f"{name} = {v}: " + ("all checks pass" if not fails else "FAIL: " + "; ".join(fails)))
        sys.exit(1 if any_fail else 0)

    m = load_model()
    p, info = build_parts(m)
    fails, out = check(m, p, info)
    print("\n".join(out))
    print(f"\n{len(out) - len(fails)} of {len(out)} checks pass")
    if not args.no_show:
        if args.finish:
            show_assembly(m, p, 0.0, 1.0, 1.0, True)
        else:
            show_assembly(m, p, args.explode, args.top_alpha, args.bottom_alpha, False)
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
