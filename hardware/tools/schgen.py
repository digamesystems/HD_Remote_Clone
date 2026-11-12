"""Builds a KiCad schematic sheet from a parts list (label-per-pin style).

Every pin gets a short wire stub ending in a net label, a global label or a
power symbol, so connectivity is by name and parts can sit anywhere on the
sheet. Unused pins get a no-connect flag. This makes a first draft that is
easy to check against docs/HARDWARE.md; tidy the drawing in KiCad afterwards.

Once a sheet has been edited in KiCad, the KiCad file is the master: don't
regenerate over it.
"""

import uuid

import sexpr
from sexpr import Sym

G = 2.54  # grid, mm
NS = uuid.UUID("6f7e2a52-3c1e-4d6b-9a57-0b6c1b1d2e01")

POWER_NETS = {
    # net: (lib_id, value)
    "GND": ("power:GND", "GND"),
    "+3V3": ("power:+3V3", "+3V3"),
    "VBUS": ("power:VBUS", "VBUS"),
    "VBAT": ("power:+BATT", "VBAT"),
}

FONT = [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]]]


def _effects(justify=None, hide=False):
    e = [Sym("effects"), [Sym("font"), [Sym("size"), 1.27, 1.27]]]
    if justify:
        e.append([Sym("justify")] + [Sym(j) for j in justify.split()])
    if hide:
        e.append([Sym("hide"), Sym("yes")])
    return e


class Sheet:
    def __init__(self, project, file_name, title, root_uuid, sheet_uuid, paper="A3", ref_base=0):
        """ref_base numbers this sheet's parts from ref_base + 1 (e.g. 100 -> R101),
        so references stay unique across sheets. Power symbols aren't affected."""
        self.ref_base = ref_base
        self.project = project
        self.file_name = file_name
        self.title = title
        self.root_uuid = root_uuid
        self.sheet_uuid = sheet_uuid
        self.paper = paper
        self.items = []
        self.lib = {}
        self.refcount = {}
        self.netlist = []  # (ref, pin, net) for the summary

    def uid(self, *key):
        return str(uuid.uuid5(NS, self.file_name + "/" + "/".join(str(k) for k in key)))

    def path(self):
        return "/%s/%s" % (self.root_uuid, self.sheet_uuid)

    def _lib(self, lib_id):
        if lib_id not in self.lib:
            self.lib[lib_id] = sexpr.library_symbol(lib_id)
        return self.lib[lib_id]

    def _symbol(self, lib_id, ref, value, x, y, rot=0, fields=None, ref_at=None, val_at=None,
                hide_ref=False, power=False):
        sym = self._lib(lib_id)
        props = {p[1]: p[2] for p in sexpr.find_all(sym, "property")}
        fields = dict(fields or {})
        ref_at = ref_at or (x, y - 2 * G, "left")
        val_at = val_at or (x, y + 2 * G, "left")
        node = [Sym("symbol"), [Sym("lib_id"), lib_id], [Sym("at"), x, y, rot], [Sym("unit"), 1],
                [Sym("exclude_from_sim"), Sym("no")], [Sym("in_bom"), Sym("no" if power else "yes")],
                [Sym("on_board"), Sym("no" if power else "yes")], [Sym("dnp"), Sym("no")],
                [Sym("uuid"), self.uid("sym", ref)]]
        node.append([Sym("property"), "Reference", ref, [Sym("at"), ref_at[0], ref_at[1], 0],
                     _effects(ref_at[2], hide=hide_ref)])
        node.append([Sym("property"), "Value", value, [Sym("at"), val_at[0], val_at[1], 0],
                     _effects(val_at[2], hide=power and lib_id == "power:PWR_FLAG" and False)])
        for name in ("Footprint", "Datasheet", "Description"):
            v = fields.pop(name, props.get(name, ""))
            node.append([Sym("property"), name, v, [Sym("at"), x, y, 0], _effects(hide=True)])
        for name, v in fields.items():
            node.append([Sym("property"), name, v, [Sym("at"), x, y, 0], _effects(hide=True)])
        for (num, _n, _x, _y, _a, _u) in sexpr.symbol_pins(sym):
            node.append([Sym("pin"), num, [Sym("uuid"), self.uid("pin", ref, num)]])
        node.append([Sym("instances"), [Sym("project"), self.project,
                                        [Sym("path"), self.path(), [Sym("reference"), ref],
                                         [Sym("unit"), 1]]]])
        self.items.append(node)

    def _next(self, prefix):
        self.refcount[prefix] = self.refcount.get(prefix, 0) + 1
        base = 0 if prefix.startswith("#") else self.ref_base
        return "%s%d" % (prefix, base + self.refcount[prefix])

    def wire(self, x1, y1, x2, y2):
        self.items.append([Sym("wire"), [Sym("pts"), [Sym("xy"), x1, y1], [Sym("xy"), x2, y2]],
                           [Sym("stroke"), [Sym("width"), 0], [Sym("type"), Sym("default")]],
                           [Sym("uuid"), self.uid("wire", x1, y1, x2, y2)]])

    def power(self, net, x, y, outward):
        lib_id, value = POWER_NETS[net]
        if net == "GND":
            rot = {"down": 0, "right": 90, "up": 180, "left": 270}[outward]
        else:
            rot = {"up": 0, "left": 90, "down": 180, "right": 270}[outward]
        ref = self._next("#PWR")
        self._symbol(lib_id, ref, value, x, y, rot, hide_ref=True, power=True,
                     val_at=(x, y + (G if net == "GND" else -G), "left"))

    def label(self, net, x, y, outward, glob=False):
        angle = {"right": 0, "up": 90, "left": 180, "down": 270}[outward]
        just = "left" if outward in ("right", "up") else "right"
        if glob:
            self.items.append([Sym("global_label"), net, [Sym("shape"), Sym("bidirectional")],
                               [Sym("at"), x, y, angle], [Sym("fields_autoplaced"), Sym("yes")],
                               _effects(just), [Sym("uuid"), self.uid("glabel", net, x, y)],
                               [Sym("property"), "Intersheetrefs", "${INTERSHEET_REFS}",
                                [Sym("at"), x, y, 0], _effects(just, hide=True)]])
        else:
            self.items.append([Sym("label"), net, [Sym("at"), x, y, angle],
                               [Sym("fields_autoplaced"), Sym("yes")], _effects(just + " bottom"),
                               [Sym("uuid"), self.uid("label", net, x, y)]])

    def no_connect(self, x, y):
        self.items.append([Sym("no_connect"), [Sym("at"), x, y], [Sym("uuid"), self.uid("nc", x, y)]])

    def flag(self, net, gx, gy):
        """PWR_FLAG on a net: the net's power symbol (or a label) at grid (gx, gy),
        the flag 2 grid right."""
        x, y = gx * G, gy * G
        if net in POWER_NETS:
            self.power(net, x, y, "down" if net == "GND" else "up")
        else:
            self.label(net, x, y, "left")
        fx = x + 2 * G
        self.wire(x, y, fx, y)
        ref = self._next("#FLG")
        self._symbol("power:PWR_FLAG", ref, "PWR_FLAG", fx, y, 0, hide_ref=True, power=True,
                     val_at=(fx, y - 2 * G, "left"))

    def part(self, lib_id, prefix, value, gx, gy, nets, fields=None, global_nets=()):
        """Places a part at grid (gx, gy). nets maps pin number -> net name, or
        None for a no-connect. Every pin of the symbol must be listed."""
        x, y = gx * G, gy * G
        sym = self._lib(lib_id)
        pins = sexpr.symbol_pins(sym)
        missing = [p[0] for p in pins if p[0] not in nets]
        if missing:
            raise ValueError("%s %s: pins not mapped: %s" % (prefix, value, missing))
        ref = self._next(prefix)
        xs = [x + p[2] for p in pins]
        ys = [y - p[3] for p in pins]
        two_pin = len(pins) == 2 and lib_id.startswith("Device:")
        if two_pin and lib_id in ("Device:R_Small", "Device:C_Small"):
            ref_at, val_at = (x + 1.27, y - 1.27, "left"), (x + 1.27, y + 1.27, "left")
        elif two_pin:
            ref_at, val_at = (x - 1.27, y - 1.27, "left"), (x - 1.27, y + 2.54, "left")
        else:
            ref_at = (min(xs) + G, min(ys) - 1.27, "left")
            val_at = (min(xs) + G, max(ys) + 2.54, "left")
        self._symbol(lib_id, ref, value, x, y, 0, fields=fields, ref_at=ref_at, val_at=val_at)
        done = set()
        for (num, _name, px, py, a, _u) in pins:
            sx, sy = round(x + px, 4), round(y - py, 4)
            net = nets[num]
            self.netlist.append((ref, num, net))
            if (sx, sy) in done:
                continue  # stacked pins share one connection
            done.add((sx, sy))
            if net is None:
                self.no_connect(sx, sy)
                continue
            outward = {0: "left", 90: "down", 180: "right", 270: "up"}[a]
            dx, dy = {"left": (-G, 0), "right": (G, 0), "up": (0, -G), "down": (0, G)}[outward]
            ex, ey = round(sx + dx, 4), round(sy + dy, 4)
            self.wire(sx, sy, ex, ey)
            if net in POWER_NETS:
                self.power(net, ex, ey, outward)
            else:
                self.label(net, ex, ey, outward, glob=net in global_nets)
        return ref

    def text(self, s, gx, gy):
        self.items.append([Sym("text"), s, [Sym("exclude_from_sim"), Sym("no")],
                           [Sym("at"), gx * G, gy * G, 0], _effects("left top"),
                           [Sym("uuid"), self.uid("text", gx, gy)]])

    def render(self, title_fields):
        tb = [Sym("title_block"), [Sym("title"), title_fields["title"]],
              [Sym("date"), title_fields["date"]], [Sym("rev"), title_fields["rev"]]]
        for i, c in enumerate(title_fields.get("comments", []), 1):
            tb.append([Sym("comment"), i, c])
        # KiCad 10's format: the symbols come from KiCad 10's libraries, which use
        # syntax (e.g. `(power global)`) that older format versions read differently.
        doc = [Sym("kicad_sch"), [Sym("version"), Sym("20260306")], [Sym("generator"), "eeschema"],
               [Sym("generator_version"), "10.0"], [Sym("uuid"), self.sheet_uuid],
               [Sym("paper"), self.paper], tb, [Sym("lib_symbols")] + list(self.lib.values())]
        doc += self.items
        doc.append([Sym("embedded_fonts"), Sym("no")])
        return sexpr.dump(doc) + "\n"


def add_sheet_to_root(root_path, project, sheet_uuid, name, file_name, page, gx, gy, w=50, h=20):
    """Adds a hierarchical sheet block to the root schematic, if not there yet."""
    with open(root_path, encoding="utf-8") as f:
        root = sexpr.parse(f.read())
    for s in sexpr.find_all(root, "sheet"):
        if sexpr.find(s, "uuid")[1] == sheet_uuid:
            return False
    x, y = gx * G, gy * G
    root_uuid = sexpr.find(root, "uuid")[1]
    block = [Sym("sheet"), [Sym("at"), x, y], [Sym("size"), w, h],
             [Sym("exclude_from_sim"), Sym("no")], [Sym("in_bom"), Sym("yes")],
             [Sym("on_board"), Sym("yes")], [Sym("dnp"), Sym("no")],
             [Sym("fields_autoplaced"), Sym("yes")],
             [Sym("stroke"), [Sym("width"), 0.1524], [Sym("type"), Sym("solid")]],
             [Sym("fill"), [Sym("color"), 0, 0, 0, 0.0]],
             [Sym("uuid"), sheet_uuid],
             [Sym("property"), "Sheetname", name, [Sym("at"), x, y - 0.7116, 0],
              _effects("left bottom")],
             [Sym("property"), "Sheetfile", file_name, [Sym("at"), x, y + h + 0.5884, 0],
              _effects("left top")],
             [Sym("instances"), [Sym("project"), project,
                                 [Sym("path"), "/" + root_uuid, [Sym("page"), str(page)]]]]]
    idx = next(i for i, x_ in enumerate(root) if isinstance(x_, list) and x_[0] == "sheet_instances")
    root.insert(idx, block)
    with open(root_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(sexpr.dump(root) + "\n")
    return True


def root_uuid(root_path):
    with open(root_path, encoding="utf-8") as f:
        return sexpr.find(sexpr.parse(f.read()), "uuid")[1]
