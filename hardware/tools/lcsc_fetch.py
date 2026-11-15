# SPDX-License-Identifier: GPL-2.0-or-later
"""Fetch manufacturer, MPN, description and datasheet for LCSC part numbers into a JSON cache.

  python -I hardware/tools/lcsc_fetch.py C25804 C14663 ...     fetch these (skips cached ones)
  python -I hardware/tools/lcsc_fetch.py --all                 every LCSC number in the schematic

Reads the public LCSC product page (lcsc.com/product-detail/<C>.html). The cache,
hardware/bom/lcsc_cache.json, is committed so the BOM can be rebuilt offline; refetch with
--refresh. Only the part number is sent.
"""
import html as htmllib
import json
import re
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "hardware" / "bom" / "lcsc_cache.json"


def fetch(code):
    req = urllib.request.Request(f"https://www.lcsc.com/product-detail/{code}.html",
                                 headers={"User-Agent": "Mozilla/5.0 (bom-tool)"})
    html = urllib.request.urlopen(req, timeout=30).read().decode("utf-8", "replace")
    title = re.search(r"<title>([^<]*)</title>", html)
    parts = [htmllib.unescape(p).strip() for p in title.group(1).split("|")] if title else []
    first = lambda key: (re.search(rf'"{key}":"([^"]*)"', html) or [None, ""])[1]
    if len(parts) < 3:
        raise ValueError(f"{code}: unexpected page title {title and title.group(1)!r}")
    return {"mpn": parts[0], "category": parts[1], "manufacturer": parts[2],
            "description": htmllib.unescape(first("productIntroEn")), "datasheet": first("pdfUrl"),
            "url": f"https://www.lcsc.com/product-detail/{code}.html"}


def schematic_codes():
    codes = set()
    for sch in (ROOT / "hardware" / "hd_remote").glob("*.kicad_sch"):
        codes |= set(re.findall(r'\(property "LCSC" "(C\d+)"', sch.read_text(encoding="utf-8")))
    return sorted(codes, key=lambda c: int(c[1:]))


def main():
    args = sys.argv[1:]
    refresh = "--refresh" in args
    codes = schematic_codes() if "--all" in args else [a for a in args if re.fullmatch(r"C\d+", a)]
    cache = json.loads(CACHE.read_text(encoding="utf-8")) if CACHE.exists() else {}
    for code in codes:
        if code in cache and not refresh:
            continue
        try:
            cache[code] = fetch(code)
            print(f"{code}: {cache[code]['manufacturer']} {cache[code]['mpn']}")
        except Exception as e:  # keep going; report at the end
            print(f"{code}: FAILED ({e})")
        time.sleep(1.0)
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_text(json.dumps(dict(sorted(cache.items(), key=lambda kv: int(kv[0][1:]))),
                                indent=1, ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
