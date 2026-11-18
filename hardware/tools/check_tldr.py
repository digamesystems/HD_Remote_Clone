# SPDX-License-Identifier: GPL-2.0-or-later
"""Flag docs whose TL;DR may be stale.

Each long doc opens with a "> **TL;DR** (summary checked YYYY-MM-DD)" block.
This lists every doc that has been changed after that date: in a commit dated
later, or with uncommitted changes. It is a prompt to reread the summary, not
a verdict: most changes don't touch what a TL;DR says. If a TL;DR does need
new wording, ask John first (CLAUDE.md, TL;DR blocks); if it is still right,
bump its date.

    python -I hardware/tools/check_tldr.py

Exit status: 0 if nothing is flagged, 1 otherwise.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TLDR = re.compile(r"\*\*TL;DR\*\*\s*\(summary checked (\d{4}-\d{2}-\d{2})\)")


def git(*args):
    return subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True,
                          check=True).stdout.strip()


def main():
    flagged = 0
    for path in sorted(ROOT.glob("**/*.md")):
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith((".pio/", ".git/")):
            continue
        m = TLDR.search(path.read_text(encoding="utf-8")[:4000])
        if not m:
            continue
        checked = m.group(1)
        # Commit dates of every commit touching the doc after the checked date
        later = git("log", "--format=%cs %h %s", f"--since={checked} 23:59:59", "--", rel)
        dirty = git("status", "--porcelain", "--", rel)
        if not later and not dirty:
            print(f"ok     {rel} (checked {checked})")
            continue
        flagged += 1
        print(f"LOOK   {rel} (checked {checked})")
        for line in later.splitlines():
            print(f"         {line}")
        if dirty:
            print("         (uncommitted changes)")
    if flagged:
        print(f"\n{flagged} doc(s) changed since their TL;DR was checked: reread the summary; "
              "ask John before rewording it, or bump the date if it still holds.")
    return 1 if flagged else 0


if __name__ == "__main__":
    sys.exit(main())
