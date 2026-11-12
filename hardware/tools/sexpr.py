"""Minimal S-expression reader/writer for KiCad files, plus symbol-library helpers.

Used by the schematic generators in this folder. Parsed lists are Python lists;
atoms are Sym (bare words) or str (quoted strings), so quoting round-trips.
"""

import os
import re

KICAD_SYMBOLS = os.environ.get(
    "KICAD_SYMBOL_DIR",
    r"C:\Users\johnp\AppData\Local\Programs\KiCad\10.0\share\kicad\symbols",
)


class Sym(str):
    """A bare (unquoted) atom."""


_TOKEN = re.compile(r'\s*(?:(\()|(\))|"((?:[^"\\]|\\.)*)"|([^\s()"]+))', re.S)


def parse(text):
    stack = [[]]
    pos = 0
    n = len(text)
    while pos < n:
        m = _TOKEN.match(text, pos)
        if not m:
            if text[pos:].strip() == "":
                break
            raise ValueError("parse error at %d" % pos)
        pos = m.end()
        if m.group(1):
            stack.append([])
        elif m.group(2):
            done = stack.pop()
            stack[-1].append(done)
        elif m.group(3) is not None:
            raw = m.group(3)
            stack[-1].append(re.sub(r"\\(.)", lambda e: "\n" if e.group(1) == "n" else e.group(1), raw))
        elif m.group(4) is not None:
            stack[-1].append(Sym(m.group(4)))
    return stack[0][0]


def _atom(a):
    if isinstance(a, Sym):
        return str(a)
    if isinstance(a, bool):
        return "yes" if a else "no"
    if isinstance(a, (int, float)):
        s = ("%.4f" % a).rstrip("0").rstrip(".")
        return "0" if s in ("-0", "") else s
    s = str(a).replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return '"' + s + '"'


def dump(node, indent=0):
    if not isinstance(node, list):
        return _atom(node)
    pad = "\t" * indent
    simple = all(not isinstance(x, list) for x in node)
    if simple:
        return pad + "(" + " ".join(_atom(x) for x in node) + ")"
    out = pad + "(" + " ".join(_atom(x) for x in node if not isinstance(x, list))
    for x in node:
        if isinstance(x, list):
            out += "\n" + dump(x, indent + 1)
    return out + "\n" + pad + ")"


def find(node, key):
    for x in node:
        if isinstance(x, list) and x and x[0] == key:
            return x
    return None


def find_all(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]


_lib_cache = {}


def load_library(lib):
    if lib not in _lib_cache:
        with open(os.path.join(KICAD_SYMBOLS, lib + ".kicad_sym"), encoding="utf-8") as f:
            _lib_cache[lib] = parse(f.read())
    return _lib_cache[lib]


def _rename_units(node, old, new):
    for x in node:
        if isinstance(x, list) and x[0] == "symbol" and isinstance(x[1], str) and x[1].startswith(old + "_"):
            x[1] = new + x[1][len(old):]


def library_symbol(lib_id):
    """Returns a deep copy of a library symbol, named "Lib:Name", with any
    `extends` resolved into a standalone definition."""
    import copy

    lib, name = lib_id.split(":", 1)
    tree = load_library(lib)
    syms = {x[1]: x for x in find_all(tree, "symbol")}
    sym = copy.deepcopy(syms[name])
    ext = find(sym, "extends")
    if ext:
        parent = copy.deepcopy(syms[ext[1]])
        child_props = {p[1]: p for p in find_all(sym, "property")}
        merged = [Sym("symbol"), name]
        for x in parent[2:]:
            if isinstance(x, list) and x[0] == "property" and x[1] in child_props:
                merged.append(child_props.pop(x[1]))
            else:
                merged.append(x)
        for p in child_props.values():
            merged.append(p)
        _rename_units(merged, ext[1], name)
        sym = merged
    sym[1] = lib_id
    return sym


def symbol_pins(sym):
    """[(number, name, x, y, angle, unit)] in library coordinates (Y up)."""
    pins = []
    for sub in find_all(sym, "symbol"):
        m = re.search(r"_(\d+)_(\d+)$", sub[1])
        unit = int(m.group(1)) if m else 1
        for p in find_all(sub, "pin"):
            at = find(p, "at")
            name = find(p, "name")[1]
            number = find(p, "number")[1]
            pins.append((number, name, float(at[1]), float(at[2]), int(float(at[3])) if len(at) > 3 else 0, unit))
    return pins
