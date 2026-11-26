# SPDX-License-Identifier: CERN-OHL-S-2.0
"""Export the case's printed parts for ordering, after every check passes.

Runs assembly.py's checks (case and real board), then writes one STL per printed part, one STEP of
the assembled case and a manifest to out/case/, and checks every STL mesh: closed, consistently
oriented, one body, volume matching the solid. Writes nothing if a check fails.

  python export_case.py                      checks + export (needs a clean, tagged commit)
  python export_case.py --allow-untagged     export from a clean commit with no case tag (trial runs)
  python export_case.py --allow-dirty        export with uncommitted changes (trial runs only)
  python export_case.py --board PATH         board STEP (default out/hd_remote_board.step)

Printed parts per case (10): bottom shell, top shell, 7 keycaps, A/B slider. Also written:
keys_and_slider.stl, the 7 caps and the slider in one file, for trying a combined upload at JLC3DP
(README: about $6 a case less, if JLC accepts several bodies in one file).
"""
import argparse
import datetime
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

from build123d import Compound, export_step, export_stl

import assembly

HERE = Path(__file__).parent
OUT = HERE / "out" / "case"
STL_TOL, STL_ANG = 0.01, 0.1      # chord deviation (mm) and angle (rad): well below MJF's resolution
TAG_PREFIX = "case-rev"


def git(*args):
    return subprocess.run(["git", *args], cwd=HERE, capture_output=True, text=True).stdout.strip()


def read_stl(path):
    """Triangles of a binary STL as tuples of three vertex tuples."""
    data = path.read_bytes()
    n = struct.unpack_from("<I", data, 80)[0]
    if len(data) != 84 + 50 * n:
        raise ValueError(f"{path.name}: size doesn't match {n} triangles (not a binary STL?)")
    tris = []
    for i in range(n):
        v = struct.unpack_from("<9f", data, 84 + 50 * i + 12)
        tris.append((v[0:3], v[3:6], v[6:9]))
    return tris


def mesh_report(path, bodies_expected):
    """Check an STL: every edge shared by exactly two triangles with opposite directions (closed and
    consistently oriented), no degenerate triangles, positive volume, the expected number of bodies.
    Returns (problems, info)."""
    tris = read_stl(path)
    index, faces = {}, []
    for t in tris:
        f = tuple(index.setdefault(tuple(round(c, 5) for c in v), len(index)) for v in t)
        faces.append(f)
    problems = []
    degenerate = sum(1 for a, b, c in faces if a == b or b == c or a == c)
    if degenerate:
        problems.append(f"{degenerate} degenerate triangles")
    directed = {}
    for a, b, c in faces:
        for e in ((a, b), (b, c), (c, a)):
            directed[e] = directed.get(e, 0) + 1
    doubled = sum(1 for k in directed.values() if k > 1)
    open_edges = sum(1 for (a, b) in directed if (b, a) not in directed)
    if doubled:
        problems.append(f"{doubled} edges used twice in the same direction (flipped triangles)")
    if open_edges:
        problems.append(f"{open_edges} open edges (holes in the surface)")
    # signed volume (divergence theorem); positive when the normals point outward
    vol = 0.0
    for (x1, y1, z1), (x2, y2, z2), (x3, y3, z3) in tris:
        vol += (x1 * (y2 * z3 - y3 * z2) - x2 * (y1 * z3 - y3 * z1) + x3 * (y1 * z2 - y2 * z1)) / 6.0
    if vol <= 0:
        problems.append(f"volume {vol:.1f} mm^3 is not positive (normals inward?)")
    # bodies: connected groups of triangles sharing vertices
    parent = list(range(len(index)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for a, b, c in faces:
        for u in (b, c):
            ra, ru = find(a), find(u)
            if ra != ru:
                parent[ru] = ra
    bodies = len({find(a) for a, _, _ in faces})
    if bodies != bodies_expected:
        problems.append(f"{bodies} bodies, expected {bodies_expected}")
    return problems, {"triangles": len(tris), "volume": vol, "bodies": bodies}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--board", type=Path, default=HERE / "out" / "hd_remote_board.step")
    ap.add_argument("--allow-dirty", action="store_true", help="export with uncommitted changes (trial)")
    ap.add_argument("--allow-untagged", action="store_true", help="export from an untagged commit (trial)")
    args = ap.parse_args()

    # where this export comes from
    commit = git("rev-parse", "--short", "HEAD")
    dirty = git("status", "--porcelain", "--", ".")
    tags = [t for t in git("tag", "--points-at", "HEAD").split() if t.startswith(TAG_PREFIX)]
    if dirty and not args.allow_dirty:
        sys.exit("hardware/enclosure has uncommitted changes: commit first (or --allow-dirty for a trial)")
    if not tags and not args.allow_untagged:
        sys.exit(f"HEAD ({commit}) has no {TAG_PREFIX}N tag: tag it first (or --allow-untagged for a trial)")
    if not args.board.exists():
        sys.exit(f"board STEP not found: {args.board} (export it from KiCad, see assembly.py)")

    # every check, case and real board
    m = assembly.load_model()
    p, info = assembly.build_parts(m)
    fails, out = assembly.check(m, p, info)
    bfails, bout = assembly.check_board(m, p, assembly.load_board(args.board, m))
    fails, out = fails + bfails, out + bout
    print("\n".join(out))
    print(f"\n{len(out) - len(fails)} of {len(out)} checks pass")
    warnings = assembly.input_warnings()
    print("\n".join(warnings))
    if fails:
        sys.exit("\nchecks failed: nothing exported")

    parts = {"bottom_shell": m["bottom"], "top_shell": m["top"]}
    parts.update({f"cap_{n.lower()}": c for n, c in m["caps"].items()})
    parts["ab_slider"] = m["ab_slider"]
    bad = [n for n, s in parts.items() if not s.is_valid or len(s.solids()) != 1]
    if bad:
        sys.exit(f"not one valid solid: {', '.join(bad)}: nothing exported")

    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.glob("*"):
        old.unlink()
    rows, problems = [], []
    files = {n: (s, 1) for n, s in parts.items()}
    keys = [s for n, s in parts.items() if n.startswith("cap_") or n == "ab_slider"]
    files["keys_and_slider"] = (Compound(children=keys), len(keys))
    for n, (shape, bodies) in files.items():
        path = OUT / f"{n}.stl"
        export_stl(shape, str(path), tolerance=STL_TOL, angular_tolerance=STL_ANG)
        probs, rep = mesh_report(path, bodies)
        solid_vol = shape.volume
        err = abs(rep["volume"] - solid_vol) / solid_vol
        if err > 0.01:
            probs.append(f"mesh volume {rep['volume']:.1f} vs solid {solid_vol:.1f} mm^3 ({err:.1%})")
        bb = shape.bounding_box()
        rows.append(f"{n + '.stl':24} {bb.size.X:6.1f} x {bb.size.Y:5.1f} x {bb.size.Z:5.1f} mm  "
                    f"{solid_vol / 1000:6.2f} cm^3  {rep['triangles']:7d} triangles  {rep['bodies']} "
                    f"{'bodies' if rep['bodies'] > 1 else 'body'}  " + ("ok" if not probs else "FAIL: " + "; ".join(probs)))
        problems += [f"{n}: {x}" for x in probs]
    export_step(Compound(children=list(parts.values())), str(OUT / "case_assembled.step"))

    stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    origin = f"commit {commit}" + (f", tag {', '.join(tags)}" if tags else ", untagged") + \
             (", WITH UNCOMMITTED CHANGES (trial, don't order from this)" if dirty else "")
    manifest = [f"HD remote case export, {stamp}", f"From {origin}",
                f"Checks: {len(out)} of {len(out)} pass (case and real board {args.board.name})", ""]
    manifest += [w for w in warnings] + [""] if warnings else []
    manifest += ["Printed parts per case: bottom_shell, top_shell, 7 caps (cap_*), ab_slider.",
                 "Finish (decided 2026-11-21, John): shells dyed black; caps and slider natural grey;",
                 "legends unfilled. Material MJF PA12. keys_and_slider.stl = the caps and slider in one",
                 "file, to try as one upload.", "", "Meshes (STL tolerance "
                 f"{STL_TOL} mm, {STL_ANG} rad):"] + rows + ["", "case_assembled.step: all printed parts, assembled."]
    (OUT / "MANIFEST.txt").write_text("\n".join(manifest) + "\n", encoding="utf-8")
    print("\n" + "\n".join(rows))
    if problems:
        sys.exit("\nmesh check failed: " + "; ".join(problems) + "\n(files left in out/case for inspection)")
    zname = OUT / f"HD_remote_case_{tags[0] if tags else commit}.zip"
    with zipfile.ZipFile(zname, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(OUT.glob("*.stl")) + [OUT / "MANIFEST.txt"]:
            z.write(f, f.name)
    print(f"\nwrote {len(files)} STL files, case_assembled.step, MANIFEST.txt and {zname.name} to {OUT}")
    print(f"from {origin}")


if __name__ == "__main__":
    main()
