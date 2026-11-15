# SPDX-License-Identifier: GPL-2.0-or-later
"""Regenerate the board parts table in docs/BOM.md from the KiCad schematic.

  python -I hardware/tools/make_bom.py            (from the repo root)

The schematic is the source of truth: reference, value, footprint, LCSC number, and (where set)
Manufacturer, MPN, Datasheet and Description fields. Parts with an LCSC number get their
manufacturer, MPN and description from hardware/bom/lcsc_cache.json when the schematic doesn't
set them, and always LCSC's datasheet for that exact part (refresh the cache with lcsc_fetch.py). Only the text between the
GENERATED markers in docs/BOM.md is rewritten; everything else in that file is hand-written.
Exits 1 if an LCSC number is missing from the cache.
"""
import csv
import io
import json
import os
import subprocess
import sys
import tempfile
from collections import OrderedDict
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCH = ROOT / "hardware" / "hd_remote" / "hd_remote.kicad_sch"
CACHE = ROOT / "hardware" / "bom" / "lcsc_cache.json"
DOC = ROOT / "docs" / "BOM.md"
KICAD_CLI = os.environ.get(
    "KICAD_CLI", r"C:\Users\johnp\AppData\Local\Programs\KiCad\10.0\bin\kicad-cli.exe")
BEGIN, END = "<!-- BEGIN GENERATED: hardware/tools/make_bom.py -->", "<!-- END GENERATED -->"
# Footprint libraries whose parts are through-hole (hand-soldered, or JLC's paid THT service).
THT_LIBS = ("LED_THT", "Button_Switch_THT", "Connector_PinSocket", "TestPoint", "Connector_JST")
FIELDS = ["Reference", "Value", "Footprint", "LCSC", "Manufacturer", "MPN", "Datasheet",
          "Description", "${DNP}"]


def export_rows():
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "bom.csv"
        subprocess.run([KICAD_CLI, "sch", "export", "bom", "--fields", ",".join(FIELDS),
                        "--group-by", "", "-o", str(out), str(SCH)],   # "" = one row per part
                       check=True, capture_output=True)
        return list(csv.DictReader(io.StringIO(out.read_text(encoding="utf-8"))))


def ref_key(ref):
    head = ref.rstrip("0123456789")
    return head, int(ref[len(head):] or 0)


def ref_ranges(refs):
    refs = sorted(refs, key=ref_key)
    out, run = [], [refs[0]]
    for r in refs[1:]:
        h, n = ref_key(r)
        ph, pn = ref_key(run[-1])
        if h == ph and n == pn + 1:
            run.append(r)
        else:
            out.append(run); run = [r]
    out.append(run)
    return ", ".join(f"{g[0]}–{g[-1]}" if len(g) > 2 else ", ".join(g) for g in out)


def blank(v):
    return not v or v.strip() in ("~", "")


def main():
    cache = json.loads(CACHE.read_text(encoding="utf-8")) if CACHE.exists() else {}
    rows = export_rows()
    missing = sorted({r["LCSC"] for r in rows if r["LCSC"] and r["LCSC"] not in cache})
    if missing:
        sys.exit("LCSC numbers not in the cache; run hardware/tools/lcsc_fetch.py --all first: "
                 + " ".join(missing))

    groups = OrderedDict()
    for r in rows:
        c = cache.get(r["LCSC"], {})
        mpn = r["MPN"] if not blank(r["MPN"]) else c.get("mpn", "")
        key = (r["LCSC"] or mpn or r["Value"], r["Footprint"])
        g = groups.setdefault(key, {"refs": [], "values": [], "row": r, "c": c, "mpn": mpn})
        g["refs"].append(r["Reference"])
        if r["Value"] not in g["values"]:
            g["values"].append(r["Value"])

    lines = ["| Ref | Qty | Value | Manufacturer | MPN | LCSC | Fit | Datasheet | Description |",
             "|---|---|---|---|---|---|---|---|---|"]
    n_parts = n_jlc = n_hand = 0
    todo = []
    for (lcsc, fp), g in sorted(groups.items(), key=lambda kv: ref_key(sorted(kv[1]["refs"], key=ref_key)[0])):
        r, c = g["row"], g["c"]
        mfr = r["Manufacturer"] if not blank(r["Manufacturer"]) else c.get("manufacturer", "")
        # The datasheet must be the exact part that gets fitted: for an LCSC part that's LCSC's
        # copy for that part number (the schematic's link is often a generic family PDF or another
        # maker's version). The schematic's link is used for hand-fitted parts.
        ds = c.get("datasheet") or ("" if blank(r["Datasheet"]) else r["Datasheet"])
        desc = r["Description"] if not blank(r["Description"]) else c.get("description", "")
        tht = fp.split(":")[0].startswith(THT_LIBS)
        if r["LCSC"]:
            fit = "JLC (THT) or hand" if tht else "JLC"
        else:
            fit = "hand"
        qty = len(g["refs"])
        n_parts += qty
        if fit == "JLC":
            n_jlc += qty
        else:
            n_hand += qty
        if r.get("${DNP}", "").strip():
            fit += ", DNP"
        refs = ref_ranges(g["refs"])
        if blank(g["mpn"]):
            todo.append(refs)
        cells = [refs, str(qty), ", ".join(g["values"]), mfr or "—",
                 g["mpn"] or "**to choose**",
                 f"[{r['LCSC']}](https://www.lcsc.com/product-detail/{r['LCSC']}.html)" if r["LCSC"] else "—",
                 fit, f"[PDF]({ds})" if ds.startswith("http") else "—",
                 desc.replace("|", "/")]
        lines.append("| " + " | ".join(cells) + " |")

    summary = (f"{len(groups)} lines, {n_parts} parts: {n_jlc} SMD parts JLC assembles, "
               f"{n_hand} through-hole or hand-fitted. Generated {date.today().isoformat()} from "
               f"`hardware/hd_remote/hd_remote.kicad_sch`; don't edit this table by hand.")
    if todo:
        summary += " **No part chosen yet:** " + "; ".join(todo) + "."
    block = "\n".join([BEGIN, "", summary, "", *lines, "", END])

    doc = DOC.read_text(encoding="utf-8")
    if BEGIN not in doc or END not in doc:
        sys.exit(f"{DOC} has no GENERATED markers")
    head, rest = doc.split(BEGIN, 1)
    _, tail = rest.split(END, 1)
    DOC.write_text(head + block + tail, encoding="utf-8")
    print(f"{DOC.relative_to(ROOT)}: {summary}")


if __name__ == "__main__":
    main()
