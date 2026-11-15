# SPDX-License-Identifier: GPL-2.0-or-later
"""Catch silent changes to per-footprint settings on the board.

KiCad's Update PCB from Schematic (F8) can quietly reset settings that live on the board's
footprints: on 2026-11-15 it turned SW202's "duplicate pad numbers are jumpers" off. This script
records those settings for every footprint in hardware/hd_remote/fp_overrides.json (committed)
and reports any difference, so a reset shows up the next time it runs.

  python -I hardware/tools/check_fp_overrides.py            compare the board with the baseline
  python -I hardware/tools/check_fp_overrides.py --update   accept the board's current settings

Run it after every F8 or footprint swap. Exits 1 on a difference. When a change is intended,
check the report, then --update and commit the baseline with the board.

Per footprint it records: library footprint, side, locked, attributes (SMD/THT, exclude from
BOM or position files, DNP, board-only, ...), the jumper and net-tie settings, footprint-level
clearance / mask / paste / zone-connection overrides, and any pad with its own overrides.
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from sexpr import parse  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PCB = ROOT / "hardware" / "hd_remote" / "hd_remote.kicad_pcb"
BASELINE = ROOT / "hardware" / "hd_remote" / "fp_overrides.json"

FP_KEYS = ("locked", "duplicate_pad_numbers_are_jumpers", "net_tie_pad_groups", "clearance",
           "solder_mask_margin", "solder_paste_margin", "solder_paste_margin_ratio",
           "solder_paste_ratio", "zone_connect", "thermal_width", "thermal_gap", "private_layers",
           "dnp", "board_only", "exclude_from_pos_files", "exclude_from_bom")
PAD_KEYS = ("clearance", "solder_mask_margin", "solder_paste_margin", "solder_paste_margin_ratio",
            "zone_connect", "thermal_bridge_width", "thermal_gap", "thermal_bridge_angle")


def kids(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]


def val(node):
    """Value of a (key value...) node as a string; bare (key) counts as 'yes'."""
    rest = [str(v) for v in node[1:] if not isinstance(v, list)]
    return " ".join(rest) if rest else "yes"


def footprint_settings():
    tree = parse(PCB.read_text(encoding="utf-8"))
    root = tree[0] if isinstance(tree[0], list) else tree
    out = {}
    for fp in kids(root, "footprint"):
        ref = next((str(p[2]) for p in kids(fp, "property") if len(p) > 2 and p[1] == "Reference"), "?")
        d = {"footprint": str(fp[1]),
             "side": next((val(x) for x in kids(fp, "layer")), "?")}
        if "locked" in [str(x) for x in fp[1:] if not isinstance(x, list)]:
            d["locked"] = "yes"
        for k in FP_KEYS:
            for x in kids(fp, k):
                d[k] = val(x)
        for a in kids(fp, "attr"):
            d["attr"] = " ".join(str(v) for v in a[1:] if not isinstance(v, list))
        pads = {}
        for p in kids(fp, "pad"):
            ov = {k: val(x) for k in PAD_KEYS for x in kids(p, k)}
            if ov:
                key = str(p[1]) or "(unnumbered)"
                pads.setdefault(key, ov)
        if pads:
            d["pad_overrides"] = pads
        if ref in out:
            ref = f"{ref} ({d['footprint']})"
        out[ref] = d
    return dict(sorted(out.items()))


def main():
    current = footprint_settings()
    if "--update" in sys.argv[1:]:
        BASELINE.write_text(json.dumps(current, indent=1) + "\n", encoding="utf-8")
        print(f"baseline written: {len(current)} footprints -> {BASELINE.relative_to(ROOT)}")
        return
    if not BASELINE.exists():
        sys.exit(f"no baseline yet: run with --update to create {BASELINE.relative_to(ROOT)}")
    base = json.loads(BASELINE.read_text(encoding="utf-8"))
    diffs = []
    for ref in sorted(set(base) | set(current)):
        if ref not in current:
            diffs.append(f"{ref}: removed from the board")
            continue
        if ref not in base:
            diffs.append(f"{ref}: new on the board ({current[ref]['footprint']})")
            continue
        b, c = base[ref], current[ref]
        for k in sorted(set(b) | set(c)):
            if b.get(k) != c.get(k):
                diffs.append(f"{ref}: {k} {b.get(k, '(none)')!r} -> {c.get(k, '(none)')!r}")
    if diffs:
        print("Footprint settings differ from the baseline:\n  " + "\n  ".join(diffs))
        print("\nIf these are intended: python -I hardware/tools/check_fp_overrides.py --update")
        sys.exit(1)
    print(f"ok: {len(current)} footprints match the baseline")


if __name__ == "__main__":
    main()
