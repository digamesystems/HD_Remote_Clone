# SPDX-License-Identifier: GPL-2.0-or-later
"""Flag docs whose TL;DR may be stale.

Each long doc opens with a "> **TL;DR** (summary checked YYYY-MM-DD)" block.
This lists every doc changed since its TL;DR line was last touched: git blame
finds the commit that last changed that line, and any later commit to the file
(or uncommitted changes) is flagged. Commit order, not dates, so edits later on
the same day are caught. It is a prompt to reread the summary, not
a verdict: most changes don't touch what a TL;DR says. If a TL;DR does need
new wording, ask John first (CLAUDE.md, TL;DR blocks); if it is still right,
restamp it: today's date, or "YYYY-MM-DD HH:MM" if it already shows today.

    python -I hardware/tools/check_tldr.py

Exit status: 0 if nothing is flagged, 1 otherwise.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# Date, plus an optional time when a summary is reconfirmed on the same day it
# was last stamped (the line must change for blame to see the recheck).
TLDR = re.compile(r"\*\*TL;DR\*\*\s*\(summary checked (\d{4}-\d{2}-\d{2}(?: \d{2}:\d{2})?)\)")


def git(*args):
    return subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True,
                          check=True).stdout.strip()


def main():
    flagged = 0
    for path in sorted(ROOT.glob("**/*.md")):
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith((".pio/", ".git/")):
            continue
        text = path.read_text(encoding="utf-8")
        m = TLDR.search(text[:4000])
        if not m:
            continue
        checked = m.group(1)
        line_no = text.count("\n", 0, m.start()) + 1
        dirty = git("status", "--porcelain", "--", rel)
        # Commit that last touched the TL;DR line (all zeros if uncommitted)
        blame = git("blame", "--porcelain", "-L", f"{line_no},{line_no}", "--", rel)  # working copy
        since = blame.split()[0] if blame else ""
        if not since or set(since) == {"0"}:
            later = ""                    # the TL;DR itself is new or being edited now
        else:
            later = git("log", "--format=%cs %h %s", f"{since}..HEAD", "--", rel)
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
              "ask John before rewording it, or restamp it if it still holds\n"
              "(today's date, or 'YYYY-MM-DD HH:MM' if it already shows today).")
    return 1 if flagged else 0


if __name__ == "__main__":
    sys.exit(main())
