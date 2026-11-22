# Hunter Douglas Platinum remote replacement

A DIY replacement for a Hunter Douglas **Platinum Technology** remote (manual © 2013, pre-PowerView): ESP32 + nRF24 + IR, PlatformIO, Arduino framework. The breadboard clone already does everything the original remote does (point-and-press, group commands, pairing under its own ID, absolute position). Current phase: handheld PCB rev 1 bring-up (5 boards ordered 2026-11-17, tag `hw-rev1`) and the handheld firmware.

This file holds rules and stable facts only. Status, results and plans live in the docs (see **Where things live**); read the relevant doc before working in an area, and record new findings there, not here.

## Working conventions (shorthand commands)
- **Single-letter commands** (fast dev loop; letters combine, e.g. "cp", "cpd", "t1 d"; run in the order c → p → d):
  - **"d"**: deploy. One step, no tests: `pio run -t upload --upload-port COM11` (builds and uploads; a failed build uploads nothing), then reopen the serial monitor in a separate window (`Start-Process pio device monitor --port COM11 --baud 115200 --filter time`).
  - **"c"**: local git commit of all current changes with a descriptive message. Runs `pio run` first and halts if the build fails (the user's rule: never commit a broken build). No tests.
  - **"p"**: push to GitHub (`origin/main`, **public** repo digamesystems/HD_Remote_Clone, so redact the remote IDs before pushing). If any pushed commit hasn't passed t1, say so.
  - **"t1"**: Tier 1 native tests, `pio test -e native`.
  - **"t2"**: Tier 2 board tests, `pio test -e esp32dev_test --upload-port COM11` (all device suites). Leaves **test firmware** on the board; say so, and that "d" restores the app.
  - **"cpdt"**: the full gated release: `pio run`, t1, then t2 for test_device_board plus each changed area (`-f test_device_<area>`), then c, p, d. If any step fails, **halt**: no commit, push, or deploy; report the errors. Tier 2 tests IGNORED for absent hardware (per `include/hw_config.h`) don't block, but say plainly which areas were not verified.
  - Typical times (incremental): build 13 s, t1 15 s, t2 ~20 s per suite, d ~25 s.
  - Scripts: "d" = `powershell -File scripts/deploy.ps1` (closes the monitor, uploads, reopens it); the cpdt test gate = `powershell -File scripts/gate.ps1 -Areas <area>,…` (build, t1, t2 board + areas; stops at the first failure and lists IGNORED tests). Commit and push stay manual steps after it.
  - Commit messages end with a **`Touches:`** line naming any of: PCB, schematic, power path, RF transmit, IR transmit, safety cap, docs-only. These are the diffs John reads closely.
- **Checks that run without being asked:** GitHub Actions (`.github/workflows/checks.yml`) runs t1, both firmware builds, `check_fp_overrides.py` and the TL;DR check on every push; look at its result after "p" (`gh run list -L 1`). A pre-push hook (`scripts/hooks/pre-push`, enabled per clone with `git config core.hooksPath scripts/hooks`) refuses pushes containing a private remote ID, read from the untracked `.git/private-ids`. `check_pins.py` needs KiCad, so run it locally when the schematic or `include/pins.h` changes.
- **Board revisions:** decided 2026-11-22 (John): run `docs/DESIGN_REVIEW.md` before ordering any board.
- **Firmware version:** `kFwVersion` in `src/main.cpp` (printed at boot and in `help`), started at 0.9.0 on Nov 15. Bump the last digit with each improvement to the bring-up firmware.
- Serial/upload must run **outside the Bash sandbox**; sandboxed reads of COM11 return nothing.
  - A monitor window holding COM11 blocks upload/t2. Close it before "d"/t2 without asking (John, 2026-11-19).
  - If upload can't connect, have the user hold BOOT and tap EN.
- Put hardware-independent logic in `lib/hdcore/` with Tier 1 tests, not in `src/`. The user can wire loopbacks or diagnostics on the breadboard to make hardware testable; ask when that would help, and keep `include/hw_config.h` in sync with what's wired.
- **Decisions come only from John** (rule added Nov 16, John). Record a project decision (in docs, CLAUDE.md, commit messages, memory or code comments) only when John states it directly in the conversation ("decided", "go with X", "order it", or an equally clear instruction). Discussion, "I'm thinking about…", a question, or agreeing that something is reasonable ("sounds good", "fair enough") is not a decision.
  - If something looks decided, or ought to be, **ask first**: quote the wording to be recorded and wait for John's confirmation. Don't write it as decided in the meantime.
  - Applies with extra force to anything affecting **schedule or cost**: orders, purchases, quantities, vendors, fabrication, and choosing to proceed before or skip a test or review.
  - Attribute recorded decisions with the date and "(John)", e.g. "Decided 2026-11-16 (John): …". Claude's recommendations and reasoning can be recorded, but labelled as such ("Recommended", "Noted", "Proposed"), never as decisions.
  - Facts John reports (a test result, "the test was never run") are observations, not decisions. Record them as reported, attributed to John.
  - The strict threshold is deliberate; John may loosen it later if it becomes tedious. Unattributed "decided" entries from before Nov 16 were audited that day: confirmed ones now say "(John)", and the enclosure decision log (`hardware/enclosure/README.md`) stands as is.
- **TL;DR blocks** (John, Nov 18): the long docs (IR_PROTOCOL, RF_PROTOCOL, HARDWARE, BRINGUP, FIRMWARE, the enclosure README) open with a dated "TL;DR (summary checked …)" block of stable facts, no running status. When a change alters a conclusion that a TL;DR states, **ask John** whether to update it (quote the proposed new wording) rather than updating or leaving it on your own. `python -I hardware/tools/check_tldr.py` lists docs changed since their TL;DR line was last committed (later commits, found via git blame, or uncommitted edits): a prompt to reread, not a verdict. A summary reread and still right gets restamped (decided 2026-11-18, John): today's date, or date plus time ("summary checked 2026-11-18 16:40") if it already shows today, so the line changes and the flag clears.
- **Track landed costs** (John, Nov 17): keep `docs/COSTS.md` current. When an order is placed or arrives, add a row (merchandise, shipping, duties and tax, what it's charged to) and update the per-board and per-case figures. Amounts come from John's order screens; ask for any that are missing rather than estimating. Keep addresses and payment details out of it.
- **Hardware revisions are tagged:** `hw-rev1` = `11de230`, the files the rev 1 boards were ordered from (tagged 2026-11-22, John). Decided 2026-11-22 (John): tag each later board order the same way (`hw-revN`) before any layout edits for the next revision.
- **Licensing (decided Nov 12, John):** firmware and tools (`src/`, `lib/`, `include/`, `test/`, `hardware/tools/`) `GPL-2.0-or-later` (root `LICENSE`); hardware design `CERN-OHL-S-2.0` (`hardware/LICENSE`): the KiCad files (`hardware/hd_remote/`) and the enclosure models (`hardware/enclosure/`: build123d scripts, `requirements.txt`, STEP/STL output; added Nov 13); docs, captures and images `CC-BY-SA-4.0` (`docs/LICENSE`). Summary in README's License section.
  - Every new source file we write starts with `// SPDX-License-Identifier: GPL-2.0-or-later` (`#` for Python), except under `hardware/enclosure/`, which uses `# SPDX-License-Identifier: CERN-OHL-S-2.0`. Not on third-party files.
  - License texts are verbatim from the official sources; never edit, reformat or summarize them (the GPL's `<year>`/"Gnomovision" lines are part of its text).
  - Hunter Douglas manual: paraphrase only; keep extracted manual text out of the repo.
  - A released firmware binary must come with its source (GPL): link the tagged commit.
- **Community contribution is a goal, not just solving our own problem.** Little public work exists on Platinum IR/RF. Write docs and captures so outsiders can reuse them: state what's confirmed vs hypothesis, keep evidence files committed, and keep code reusable (`lib/hdcore/`). The repo is public. Later options: upstream the IR protocol to IRremoteESP8266, post write-ups on the Arduino/HA threads, maybe an ESPHome component.
- **spitcool/hd-platinum-rf** (sister project; John opened issue #1, comment history in `docs/RESEARCH.md`): use their facts with a citation; don't copy their prose or code. Anything sent to their repo needs a DCO sign-off (`git commit -s`). **Ask John before posting anything further.**

## Safety rules (shades, remotes, IDs)
- **Remote IDs are private.** The real IDs are in Claude's local memory (`remote-ids`), never in the repo. Redact as `xx xx` (weak remote) / `yy yy` (working remote) in anything committed or published. A pre-push hook (`scripts/hooks/pre-push`) blocks pushes that contain them.
- **Never Master Reset either original remote** (may erase pairing).
- Holding OPEN 6 s at a fully open shade resets its limits (manual p. 21), so `send` caps both directions at 5 s (decided 2026-11-18, John). Keep any new transmit path under the same cap.
- Pairing capacity per shade is unknown: remove test IDs (`pair <g> remove`) when done with them.
- Our shades are top-down/bottom-up with **reversed arrows (DOWN opens, UP closes)**; test from mid-travel. They sleep after a few seconds idle: a press from sleep needs ~1–2 s of wake + IR.

## Hardware in one paragraph
Breadboard: ESP32-DevKitC (WROOM-32D, `board = esp32dev`, CP210x on COM11), TSOP38238/VS1838B receiver, TSAL6200 IR LED via NPN, one hiBCTR "nRF24L01+" module (likely Si24R1 clone) on VSPI; pin map in `docs/FIRMWARE.md`. Handheld PCB rev 1 (WROOM-32E, envs `handheld` / `handheld_test`, `-DHD_BOARD_HANDHELD`): pin map in `include/pins.h` (source of truth) and `docs/HARDWARE.md`. Tools: 24 MS/s 8-ch logic analyzer (PulseView, `fx2lafw`). Radio B was dropped (decided 2026-11-18, John).

## Where things live
| Topic | File |
|---|---|
| IR protocol, shade tests, evidence | `docs/IR_PROTOCOL.md` |
| RF protocol, group commands, pairing, position | `docs/RF_PROTOCOL.md` |
| Open questions, research plan, manual notes, test plans, outreach | `docs/RESEARCH.md` |
| Firmware commands, breadboard pin map | `docs/FIRMWARE.md` |
| Handheld board spec, decisions, pin map | `docs/HARDWARE.md` |
| Board order log and bring-up plan | `docs/BRINGUP.md` |
| Parts, costs | `docs/BOM.md`, `docs/COSTS.md` |
| Enclosure and its decision log | `hardware/enclosure/README.md` |
| Feature ideas | `docs/FEATURE_IDEAS.md` |
| Tests | `TESTING.md` |
| Fab files | `hardware/fab/` (regenerate: `python -I hardware/tools/make_jlc.py`) |
