# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate the JLCPCB fabrication and assembly files from the KiCad project.

  python -I hardware/tools/make_jlc.py [--tht J102,...]      (from the repo root)

Writes hardware/fab/ :
  gerbers/        Gerber layers (Protel extensions) and Excellon drill files (PTH and NPTH separate)
  hd_remote_gerbers.zip   the folder above, ready to upload
  hd_remote_bom_jlc.csv   JLC BOM: Comment, Designator, Footprint, LCSC Part #
  hd_remote_cpl_jlc.csv   JLC component placement: Designator, Mid X, Mid Y, Layer, Rotation

The BOM lists only parts JLC assembles: an LCSC number, not DNP, and not through-hole. Through-hole
parts that have an LCSC number (J102) are hand-fitted unless named in --tht. The CPL lists exactly
the BOM's designators. Rotations are KiCad's, uncorrected: check every polarized part in JLC's
placement preview and rotate there (BRINGUP.md, Before ordering).

KiCad's Y axis is flipped in position files so that it matches the Gerbers, which is what JLC wants.
"""
import argparse
import csv
import io
import shutil
import subprocess
import sys
import tempfile
import zipfile
from collections import OrderedDict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_bom as mb  # noqa: E402  (reuses its schematic export and fit rules)

ROOT = mb.ROOT
PCB = ROOT / "hardware" / "hd_remote" / "hd_remote.kicad_pcb"
FAB = ROOT / "hardware" / "fab"
KICAD_CLI = mb.KICAD_CLI
LAYERS = "F.Cu,B.Cu,F.Paste,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts"

# Per-part corrections for how JLC's library places a part, found in JLC's placement preview.
# ref -> (rotation added in degrees, counter-clockwise; X shift in mm; Y shift in mm; Y is up).
# U201 (ESP32-WROOM-32E, 2026-11-16): JLC drew it 90 degrees clockwise of the design, and 3.8 mm
# (three pin pitches) along its axis toward the antenna. Rotation -90 and Y +3.81 are an estimate
# from a screenshot; if the preview is still off, adjust here and re-upload the CPL.
CPL_FIXES = {"U201": (-90, 0.0, 3.81)}


def cli(*args):
    r = subprocess.run([KICAD_CLI, *args], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"kicad-cli {' '.join(args[:3])} failed:\n{r.stdout}\n{r.stderr}")
    return r.stdout


def make_gerbers():
    gdir = FAB / "gerbers"
    gdir.mkdir(parents=True, exist_ok=True)
    for old in gdir.iterdir():   # clear stale files; keep the folder (Dropbox can hold it open)
        try:
            old.unlink()
        except OSError:
            pass   # kicad-cli overwrites files of the same name
    cli("pcb", "export", "gerbers", "--layers", LAYERS, "--check-zones", "--subtract-soldermask",
        "--output", str(gdir), str(PCB))
    cli("pcb", "export", "drill", "--format", "excellon", "--drill-origin", "absolute",
        "--excellon-units", "mm", "--excellon-zeros-format", "decimal",
        "--excellon-separate-th", "--output", str(gdir), str(PCB))
    zpath = FAB / "hd_remote_gerbers.zip"
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(gdir.iterdir()):
            if f.suffix != ".gbrjob":   # the job file isn't needed by JLC
                z.write(f, f.name)   # JLC wants the files at the top level of the zip
    return sorted(p.name for p in gdir.iterdir()), zpath


def jlc_parts(tht_ok):
    """One entry per assembled part: refdes -> (value, footprint, lcsc)."""
    parts = OrderedDict()
    skipped = []
    for r in mb.export_rows():
        dnp = bool(r.get("DNP", "").strip())   # kicad-cli labels the ${DNP} column "DNP"
        fp = r["Footprint"].split(":")[-1]
        lib = r["Footprint"].split(":")[0]
        is_tht = lib.startswith(mb.THT_LIBS)
        if dnp or not r["LCSC"] or (is_tht and r["Reference"] not in tht_ok):
            skipped.append((r["Reference"], "DNP" if dnp else "no LCSC" if not r["LCSC"] else "THT"))
            continue
        parts[r["Reference"]] = (r["Value"], fp, r["LCSC"])
    return parts, skipped


def write_bom(parts):
    # One line per LCSC part and footprint: JLC doesn't auto-select a part that two lines match
    # (SW201 RESET and SW202 BOOT are the same switch), so lines are merged and the comments joined.
    groups = OrderedDict()
    for ref, (val, fp, lcsc) in parts.items():
        groups.setdefault((lcsc, fp), []).append((ref, val))
    path = FAB / "hd_remote_bom_jlc.csv"
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Comment", "Designator", "Footprint", "LCSC Part #"])
        for (lcsc, fp), items in sorted(groups.items(), key=lambda kv: mb.ref_key(
                sorted(i[0] for i in kv[1])[0])):
            items.sort(key=lambda i: mb.ref_key(i[0]))
            vals = list(OrderedDict.fromkeys(v for _, v in items))
            w.writerow(["/".join(vals), ",".join(r for r, _ in items), fp, lcsc])
    return path, len(groups)


def write_cpl(parts):
    with tempfile.TemporaryDirectory() as d:
        pos = Path(d) / "pos.csv"
        cli("pcb", "export", "pos", "--format", "csv", "--units", "mm", "--side", "both",
            "--exclude-dnp", "--output", str(pos), str(PCB))
        rows = list(csv.DictReader(io.StringIO(pos.read_text(encoding="utf-8"))))
    by_ref = {r["Ref"]: r for r in rows}
    missing = [ref for ref in parts if ref not in by_ref]
    path = FAB / "hd_remote_cpl_jlc.csv"
    bottom = []
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
        for ref in sorted(parts, key=mb.ref_key):
            r = by_ref.get(ref)
            if not r:
                continue
            layer = "Top" if r["Side"] == "top" else "Bottom"
            if layer == "Bottom":
                bottom.append(ref)
            drot, dx, dy = CPL_FIXES.get(ref, (0, 0.0, 0.0))
            w.writerow([ref, f"{float(r['PosX']) + dx:.4f}mm", f"{float(r['PosY']) + dy:.4f}mm",
                        layer, f"{(float(r['Rot']) + drot) % 360:.0f}"])
    return path, missing, bottom


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tht", default="", help="comma-separated through-hole refs JLC should place")
    args = ap.parse_args()
    tht_ok = {r for r in args.tht.split(",") if r}
    FAB.mkdir(parents=True, exist_ok=True)

    files, zpath = make_gerbers()
    parts, skipped = jlc_parts(tht_ok)
    bom, lines = write_bom(parts)
    cpl, missing, bottom = write_cpl(parts)

    print(f"Gerbers + drill: {len(files)} files -> {zpath.relative_to(ROOT)}")
    for n in files:
        print("   ", n)
    print(f"BOM: {len(parts)} parts on {lines} lines -> {bom.relative_to(ROOT)}")
    print(f"CPL: {len(parts) - len(missing)} placements -> {cpl.relative_to(ROOT)}")
    if bottom:
        print("Bottom-side parts (check the assembly side in JLC's options):", ", ".join(bottom))
    if missing:
        sys.exit("ERROR: in the BOM but not in the position file: " + ", ".join(missing))
    not_jlc = OrderedDict()
    for ref, why in skipped:
        not_jlc.setdefault(why, []).append(ref)
    for why, refs in not_jlc.items():
        print(f"Not assembled by JLC ({why}): " + ", ".join(sorted(refs, key=mb.ref_key)))


if __name__ == "__main__":
    main()
