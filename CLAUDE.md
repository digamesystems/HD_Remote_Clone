# Hunter Douglas Platinum remote replacement — project handoff

Context for Claude Code. Summarized from a planning conversation in Claude chat (Nov 6–7, 2026).

## Working conventions (shorthand commands)
- **Single-letter commands** (fast dev loop; letters combine, e.g. "cp", "cpd", "t1 d"; run in the order c → p → d):
  - **"d"**: deploy. One step, no tests: `pio run -t upload --upload-port COM11` (builds and uploads; a failed build uploads nothing), then reopen the serial monitor in a separate window (`Start-Process pio device monitor --port COM11 --baud 115200 --filter time`).
  - **"c"**: local git commit of all current changes with a descriptive message. Runs `pio run` first and halts if the build fails (the user's rule: never commit a broken build). No tests.
  - **"p"**: push to GitHub (`origin/main`, **public** repo digamesystems/HD_Remote_Clone, so redact the remote IDs before pushing). If any pushed commit hasn't passed t1, say so.
  - **"t1"**: Tier 1 native tests, `pio test -e native`.
  - **"t2"**: Tier 2 board tests, `pio test -e esp32dev_test --upload-port COM11` (all device suites). Leaves **test firmware** on the board; say so, and that "d" restores the app.
  - **"cpdt"**: the full gated release: `pio run`, t1, then t2 for test_device_board plus each changed area (`-f test_device_<area>`), then c, p, d. If any step fails, **halt**: no commit, push, or deploy; report the errors. Tier 2 tests IGNORED for absent hardware (per `include/hw_config.h`) don't block, but say plainly which areas were not verified.
  - Typical times (incremental): build 13 s, t1 15 s, t2 ~20 s per suite, d ~25 s.
- **Firmware version:** `kFwVersion` in `src/main.cpp` (printed at boot and in `help`), started at 0.9.0 on Nov 15. Bump the last digit with each improvement to the bring-up firmware.
- Serial/upload must run **outside the Bash sandbox**; sandboxed reads of COM11 return nothing.
  - A monitor window holding COM11 blocks upload/t2. Ask the user to close it (Ctrl+C) first, or close it if they say to.
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
- **Licensing (decided Nov 12, John):** firmware and tools (`src/`, `lib/`, `include/`, `test/`, `hardware/tools/`) `GPL-2.0-or-later` (root `LICENSE`); hardware design `CERN-OHL-S-2.0` (`hardware/LICENSE`): the KiCad files (`hardware/hd_remote/`) and the enclosure models (`hardware/enclosure/`: build123d scripts, `requirements.txt`, STEP/STL output; added Nov 13); docs, captures and images `CC-BY-SA-4.0` (`docs/LICENSE`). Summary in README's License section.
  - Every new source file we write starts with `// SPDX-License-Identifier: GPL-2.0-or-later` (`#` for Python), except under `hardware/enclosure/`, which uses `# SPDX-License-Identifier: CERN-OHL-S-2.0`. Not on third-party files.
  - License texts are verbatim from the official sources; never edit, reformat or summarize them (the GPL's `<year>`/"Gnomovision" lines are part of its text).
  - Hunter Douglas manual: paraphrase only; keep extracted manual text out of the repo.
  - A released firmware binary must come with its source (GPL): link the tagged commit.
- **Community contribution is a goal, not just solving our own problem.** Little public work exists on Platinum IR/RF (scattered, mostly dormant forum threads; no published IR protocol found). Write docs and captures so outsiders can reuse them: state what's confirmed vs hypothesis, keep evidence files committed, and keep code reusable (`lib/hdcore/`). The repo is public (checked Nov 13). Later options: upstream the IR protocol to IRremoteESP8266, post write-ups on the Arduino/HA threads where people got stuck, maybe an ESPHome component.

## Goal
Build a DIY replacement for a Hunter Douglas **Platinum Technology** remote (manual © 2013, pre-PowerView). One original remote works; a second is degraded (IR works only within ~30 cm, RF within ~1 m — both paths degraded, so suspect a power problem: batteries, contacts, or regulator; worth cleaning and re-testing). Both remotes date from about 2012 and have been dropped many times; the user suspects drop damage (e.g. a cracked joint in the power path) may be the cause. Neither cause is verified. Design implication: the clone should survive drops. The Nov 13 review suggested flush IR LEDs; the user kept them 1.5 mm proud for version 1 (most IR margin first). Fallback if drops damage them: flush tips with a deeper/wider countersink.

User is comfortable with **ESP32 + PlatformIO** (Arduino framework assumed).

## What the original remote does
- **Dual mode:** IR (40 kHz carrier per manual) and RF (2.4 GHz).
  - Indicator **red** = IR / point-and-press mode (individual shade, aim at its "sensor eye").
  - Indicator **green** = RF group mode. Press a group button (1–4 or All) no more than 5 s before OPEN/CLOSE.
  - **Observed key/mode behaviour (user, Nov 7):** 7 keys: UP, DOWN, 1, 2, 3, 4, ALL.
    - Group keys 1–4 and ALL work **only in RF**; pressing one always flashes the green LED.
    - The arrows are **IR (red) by default**. After a group key, the arrows act in green/RF mode for a time window, then revert to red/IR.
    - In normal use, only the arrows send IR (analyzer-verified; setup-mode presses not captured yet). **Arrows in green mode (right after a group key) send no IR** (verified with `ir quiet`, Nov 7): green = RF only.
    - **A/B slide switch** is the **Infrared Channel Selector** (manual pp. 1, 18, 22). It changes the IR code (captured Nov 8). The manual lists **4 groups**, not 8. Whether it affects RF is unknown.
      - The forum's group byte (1/2/4/8, 255 = All) still needs verifying on our remote.
    - **RF window ≈ 6.5 s** from a group-key press until the arrows send IR again (user stopwatch, Nov 7; the manual says 5 s). **Pressing a group key again during the window restarts it** (it's a retriggerable timer).
    - Unknown: whether arrow presses extend the window, and whether red-mode arrows key the radio (RF-wake question).
    - The clone should copy this UX.
  - Manual describes IR as press-and-release; pressing the opposite button stops movement.
- **IR protocol: OUR CAPTURES (Nov 7, working remote, red mode, VS1838B/TSOP on GPIO 32).** Full write-up: `docs/IR_PROTOCOL.md` (keep it updated with new findings). Log: `captures/ir_red_open_close_20261107_132807.log`.
  - Frame = header mark ~2.75 ms + space ~2.75 ms, then **4 bits**, each a ~1.0 ms mark + space (~1.07 ms = 0, ~2.75 ms = 1). Frame period exactly **17.19 ms**, back to back, no extra gap.
  - One press = one burst of identical frames, lasting as long as the key is held (about 20 frames / 0.35 s for a quick press). No toggle bit, no counter, **no shade address** (fits point-and-press).
  - **Codes:** switch A: UP (OPEN) = `0101`, DOWN (CLOSE) = `0110` (ESP32 log + logic analyzer). Switch B: UP = `1001`, DOWN = `1010` (ESP32 log, `captures/ir_ab_channel_20261108.log`). Inferred layout: bits 3–2 = channel (`01` A, `10` B), bits 1–0 = command (`01` OPEN, `10` CLOSE). Our shade listens on A. **No other button sends IR in normal use** (logic analyzer); stop = press the opposite arrow.
  - On release the remote stops mid-frame (last frame truncated); no release/stop frame. Analyzer files: `captures/la_ir_red_{up,down}_arrow_20261107.sr` (100 kHz, D0 = receiver OUT).
  - Timing measured through a demodulating receiver: marks read slightly short and spaces slightly long. The true carrier and duty cycle still need measuring with the analyzer on the LED drive.
  - Implication: replay can be synthesized from a 4-bit code rather than raw timings. The shade also needs the RF wake (see below).
- **RF uses an nRF24L01+ inside the remote.** Working parameters (Nov 9) come from spitcool/hd-platinum-rf `docs/rf-protocol.md` (2026-10-25), whose evidence includes FCC filings, a community register dump and their own SDR captures. Firmware defaults and `hd` preset use them (`lib/hdcore/src/hd_rf.h`). **Confirmed on both our remotes by sniffing ch 33 (Nov 11)** with one hiBCTR module (no cap). Full write-up: `docs/RF_PROTOCOL.md`; log `captures/rf_sniff_20261111.log` (IDs redacted).
  - Channels 33, 52, 71 (2433/2452/2471 MHz), one packet per channel in rotation, ~185–193 µs per slot (we see ≥556 µs between packets on one channel). The remote transmits **for as long as the key is held**, not a fixed ~30 rotations. 52/71 not sniffed yet.
  - 2 Mbps; 3-byte address `E7 E7 E7`; static 5-byte payload; CRC-16; auto-ack off; TX ~−6 dBm. Frames have no packet-control field (88 bits on air); our clone module receives them fine. Firmware now also sets ARC = 0 (`setRetries(0,0)`) so our TX matches.
  - Payload `ID0 ID1 MODE GROUP ACTION`: MODE `20` group command, `01` red-mode press, `30`/`31` setup-mode add/remove; GROUP one-hot `01/02/04/08`, `FF` All, `00` none; ACTION `A1`/`A2` held up/down, `21`/`22` release tail, **`80` wake held, `00` wake release tail**. No rolling code.
  - **Clone group commands WORK (Nov 11, `grp <1-4|all> <up|down> [ms]`):** `ID 20 <group> A1/A2` for 300 ms then the `21/22` tail for 330 ms, rotating channels, with the working remote's ID. All four household groups move independently, and All works, up and down. So DOWN (`A2/22`) and groups 2–4/All are confirmed by the shades' response (not sniffed). The clone now does everything the remote does, without it.
  - **ID check, revised Nov 12: group commands check it, the red-mode wake doesn't.** `grp 4 up` with `id 1234`: 0/12; with the working ID: 12/12. Wake + IR with `1234`, `yy yy` or `xx xx` all move a group-4 shade, while IR alone (`seq off` + `wake off`) doesn't. Nov 11's apparent `1234` wake + IR failure is put down to hand-aimed IR. This fits manual p. 3 (point-and-press IR is the default and needs no setup), as the user predicted. So point-and-press needs no paired ID; group control does (and the clone can now pair its own, see the manual notes below). A second made-up ID, `c10e`, also works for wake + IR (Nov 12). **Bug:** `runSeq` ignores `wake off` (it always sends RF); fix offered, not yet requested.
  - **Group key alone sends no RF** (only opens the window). Group 1 + UP = `ID 20 01 A1` while held, then `ID 20 01 21` (~0.3 s). Nothing sent when the green window expires.
  - **Red-mode arrow (either) = `ID 01 00 80` while held: the wake. No direction** (the IR carries it). Working remote: `80` only (tap 439 ms, ~2 s hold 1583 ms). Weak remote: `80` then a `00` tail, total ≥ ~345 ms (hypothesis from 9 presses).
  - **Two remotes, two different IDs, both move the shade.** The real IDs are in Claude's local memory (`remote-ids`), never in the repo. Redact as `xx xx` (weak) / `yy yy` (working) in anything committed. The firmware's `id <hex>` stores the ID in the ESP32's NVS.
  - The Arduino forum thread's example `3 193 32 1 161` = `03 C1 20 01 A1` fits this layout, but its "channel 7, 1 Mbps" doesn't; spitcool associates those with the later PowerView Pebble. Ignore channel 7.
  - Licensing (spitcool): use their facts with a citation; don't copy their prose or code (copying would carry their terms). Anything sent to their repo needs a DCO sign-off (`git commit -s`). Our licenses: see **Licensing** under Working conventions. John opened spitcool/hd-platinum-rf#1 ("sister project"); spitcool's Nov 16 commit already cites our `c36fbbb` (wake, ID check, sleep timing). Follow-up comment posted 2026-11-18 (approved by John): answers to their open questions Q1, Q5, Q6/7, Q8, Q9 (setup/pairing frames), Q12 (FCC ID UXURC4U2), with links pinned to `db5d13d`: https://github.com/spitcool/hd-platinum-rf/issues/1#issuecomment-6989642349. Second comment posted 2026-11-18 (approved by John): their bridge position frames work from our clone under a handheld-paired ID (`pos`, fw 0.9.7), links pinned to `8f2dad8`: https://github.com/spitcool/hd-platinum-rf/issues/1#issuecomment-6995420597. Ask John before posting anything further.
  - **Absolute position works (2026-11-18, John's shade test; `docs/RF_PROTOCOL.md`, Absolute position):** `pos <mask> <0-255>` sends `ID 50 MASK POS` 1.49 s then `ID 32 MASK POS` 0.30 s. Mask = handheld group byte (01/02/04/08, bits combine); 0 = fully down, 255 = fully up on our shades, roughly linear; repeat at target = no move; ID checked (made-up ID: no response). Also today: a ~0.63 s `grp` latches end to end; the release tail isn't a stop; travel down ~21 s, up ~27 s; opposite group command as a stop not tested.

### RF "wake-up" for IR: confirmed by two tests (Nov 9)
**Clone IR + remote RF (Nov 9):** clone alone 0/20; remote alone aimed away, ~20 presses, nothing; clone + remote pressed together moves the shade; clone UP + remote DOWN → shade follows the clone. So our IR encoding is correct, and the red-mode RF is an enable, not the command. **One RF enable arms one IR command, briefly:** a second clone command needs another remote arrow press (either arrow), including same-direction repeats and stop-while-moving; the IR must follow the remote press within ~1 s (rough). The clone must send RF immediately before every IR command. The RF's content is now known: `ID 01 00 80` (Nov 11 sniff, see above). Next unknowns: does the clone's own wake + IR work with no remote, does the shade check the ID, and the receptive window (1/2/5 s delay test).

Three independent but **secondhand** sources (AVS forum 2007, Bond forum 2022, an HD automation dealer) say battery-powered Platinum shades ignore IR unless the remote also sends a brief RF burst to wake them, which is why learning remotes fail. No capture or timing details found anywhere.

**Our evidence (foil-tube test, Nov 9, user's design; full write-up in `docs/IR_PROTOCOL.md`, Shade tests):** the remote sealed in a ~5 cm foil tube, back twisted shut, front open. Its IR decodes cleanly on our receiver at 30 cm; group key + arrow from inside the tube does NOT move the shade (RF blocked); a red-mode arrow at 30 cm from the sensor eye does NOT move the shade; the bare remote moves it both ways. Repeated bare/wrapped/bare. So: **IR carries the command (and selects the shade); RF is required alongside it.** Open: is the RF a bare wake-up, or does it carry the command or the pairing ID? That decides what the clone transmits.

Lessons: a hand barely blocks IR (14–18 of 20 frames still decode), and a foil hole ≥1.5 cm leaks RF. Always verify a shield with the receiver (IR) and the green-mode control (RF). These shades are **top-down/bottom-up with reversed arrows (DOWN opens, UP closes)**: test from mid-travel, because the Nov 8 `send down` on an open shade may have had no travel.

Public summary page (private until shared): https://claude.ai/artifact/X8bRVQXfk2P1AV8owhsCF2. Exact trial counts, tube width and the remote's insertion depth weren't recorded.

**Our shades are battery models converted to DC power:** the user replaced the battery pack with an always-on DC brick and a compatible cable, because battery changes meant ladder work. They have the **same electronics and firmware as battery shades**, so any power-saving or wake-up behaviour would still be active; constant power does *not* weaken the hypothesis. The original batteries drained quickly, which weakly suggests a receiver that's often awake (the motor and the packs also matter).

**Shade timing (Nov 15, clone only, LED taped beside the sensor eye; write-up `docs/IR_PROTOCOL.md`, 2026-11-15; log `captures/shade_timing_20261115.log`).** A step = wake on one channel 50 ms + IR 50 ms (3 frames), channels rotating.
- **The shade sleeps.** After a few seconds idle, 3 steps (~0.32 s) worked 8/18 vs 11/11 at ~1 s pace; the first press of a session often fails. From sleep: 0.41 s 0/3, 0.92 s 9/12, **1.94 s 9/9**. The old `send` (450 ms wake, then IR) got 2/11. So the RF must keep coming during the press, as on the real remote.
- **IR per step:** 1 frame unreliable, 2 good, 3 good (now the default), 10 occasional doubles.
- **Awake shade:** a 0.2–0.7 s tap of the opposite arrow stops it (~40 taps, no reversals). **Holding the opposite arrow reversed only after 4.9, 7.2, 9.0 s (one >10 s)**: no consistent timer. The earlier `rf 200 / ir 400` doubles (1.2 s with 200 ms breaks) were likely separate presses. Real remote's hold-to-reverse not timed yet.
- **Handheld arrow UX (decided Nov 15, user):** send while the key is held, no fixed length. Hold until the blind moves (from sleep ~1–2 s); once it's awake, tap to stop or reverse. Like the real remote in IR mode, where you hold and wave until it moves.
- Ideas not yet tested: rotating channels within each step (each channel is quiet 250 of every 300 ms now) might shorten the wake; the time until the shade sleeps; the `seq gap` sweep (first try spoiled by the tape).
- Firmware: v0.9.2 `send <code> [ms]` repeats steps for `ms` (and honours `wake off`); v0.9.3 `hold on` times arrow-key holds. The arrows with `seq on` still ignore `wake off`. `pair` is still 3 steps: wake the shade first or retry.

Next:
1. **Clone-only shade control WORKS (Nov 11):** `id` = working remote's ID; PC arrows send wake (450 ms, ~1500 packets, ch 33/52/71, ~300 µs each) then IR. Intermittent when hand-aimed; **reliable (up/stop/down/down/stop/up, hardly a miss) with the LED on the ~6 ft extension lead about 1 ft from the sensor eye and 1 ft below it.** Misses = IR aim/power. User's impression: works best aimed slightly "cockeyed" (off what looks like straight at the eye); unconfirmed. `wake off` control done Nov 12 (IR alone: no response). Still to run: trial counts, max range. (Made-up ID: the wake accepts it, group commands don't; see the ID-check note above.) IR range ideas: second LED, wider-angle LED, distance baseline.
   - **Interleaved arrow-key sequence (Nov 11, `seq`, default on):** RF ch33 (50 ms) → IR (170 ms) → RF ch52 → IR → RF ch71 → IR, ~660 ms, with `seq rf|gap|ir <ms>` to tune it and `seq off` for the old pattern (450 ms wake on all channels, then 500 ms IR). **Result: worked flawlessly at 6+ ft, and aiming felt much less sensitive.** But that's a hand-held impression (LED held and aimed by eye; hand shake confounds it), not yet a controlled result. Leading hypothesis: the shade opens a fixed listening window at the first RF packet it hears, so in the old pattern most IR arrived late in the window; `seq` puts IR 50 ms after RF and gives three fresh chances. Alternatives: the shade listens on one channel; the IR receiver is powered only briefly after a wake.
   - **Test plan (agreed with John, to continue):**
     1. **Fix the aim:** prop/tape the LED (books, box, chair back) or clamp it (binder clip on a ruler, helping hands, phone tripod). Then nudge it off-axis or back until it misses ~half the time: differences only show at the margin.
     2. **A/B:** `seq off` vs `seq on`, ≥10 presses each (≥20 if hand-held), logging `# y` / `# n` per press. If hand-held, alternate in blocks of 5 (off, on, off, on…) so drift and fatigue hit both equally.
     3. **Gap sweep (needs the fixed mount):** `seq gap 0`, 100, 200, 400, 800, ~10 presses each with `# y` / `# n`. A sharp drop past some gap = the shade's listening window after RF (a key design number). Flat = long window; the gain comes from three chances or per-channel pairing.
     4. Then try `seq rf 20` (shorter RF per channel) and watch for each RF+IR pair acting as a separate command (start then stop, or a stutter).
     5. Ask the user for a `Seq ...` timing line (actual packets per channel, step start times) for the docs.
     6. **Made-up-ID wake with counts** (review, Nov 12): with the fixed mount, wake + IR under a made-up ID (`1234`, `c10e`) vs the working ID, ≥10 presses each, logged as n of n. The "any ID works" result so far is hand-aimed with no counts; a single hand-aimed miss is what produced Nov 11's wrong "ID checked" conclusion.
2. **Handheld emulation (next phase):**
   - **Random ID per unit (decided 2026-11-18, John; fw 0.9.6):** a unit with no remote ID in flash generates a random one at boot (esp_random(), never 0000 or FFFF), saves it and prints it. `id` with no argument shows the current ID; `id <hex>` still overrides it; `id new` draws a fresh random one (replaces `id clear`, John). Why (review, Nov 12): neighbours' clones sharing one ID could control each other's paired shades. Not done: sniffing ch 33 first to avoid IDs already heard. The draw runs with `bootloader_random_enable()`, since `esp_random()` lacks entropy without the radio subsystem; untested on hardware with the handheld's ADC.
   - Physical buttons (all eight keys; pins fixed by the PCB, see "Pin map (handheld PCB rev 1)"), the remote's UX (group key opens a retriggerable green window in which arrows send `grp`; **10 s on the clone** (decided 2026-11-21, John; the original’s is ~6.5 s), and a different group key inside it takes over with a fresh 10 s; otherwise arrows send wake+IR steps **for as long as the key is held**, per the Nov 15 timing notes), and a **red/green bicolor indicator LED** (user request, Nov 11). On the PCB: D501 common-cathode red/green, red on GPIO 22 via 330 Ω, green on GPIO 16 via 270 Ω, LEDC PWM. Asked the user to describe the original's LED behaviour (steady vs flashing during red presses; green through the whole window or only on presses; low-battery or setup patterns). Then deep sleep (ext1 wake), battery, enclosure.
   - **Reset with the case closed (decided Nov 16, John):** no external reset, on the prototype or the planned run of five (all users are developers who know the serial reset). Recovery: USB auto-reset (works from sleep or a hang, only with USB plugged in), watchdogs (**keep the task and RTC watchdogs enabled in the handheld firmware**), or open the case. Test auto-reset first at board bring-up. A RESET pinhole next to SETUP is the later-revision fix. Details: `docs/HARDWARE.md`, Reset and recovery.
3. **nRF24 bring-up, remaining** (one ESP32; the user prefers not to use a second): screen the other modules with `test <n>` (numeric labels: `a1` parses as 0); fit the 10 µF caps; sniff ch 52/71.
4. Measure foil attenuation with a module at the tube's mouth; probe the remote's LED drive; optionally scope its battery current for radio-shaped steps.

**Manual notes (read in full Nov 11; text extracted to the scratchpad, not committed):**
- **No stated limit** on how many remotes or wall switches a shade remembers.
- **Pairing model:** a shade learns a remote through group assignment (p. 5): SETUP → LED flashes green ~30 s → group key → aim at the eye from 2–3" → OPEN adds, CLOSE removes; the shade jogs; SETUP again saves and exits. A shade can be in several groups. So each shade keeps (remote ID, group) entries, which explains our two remotes having different IDs.
- A **duplicate remote** (p. 7: Master Reset ~10 s → flashing red → OPEN on the original within 18") **copies the original's identity** rather than adding a new one; changes made on either then affect both.
- **The clone pairs under its own ID (Nov 12):** the user's priority, done. Sniffed group assignment (working remote, group 4): SETUP, the group key and the save press send nothing; setup OPEN = `ID 30 08 80` while held + normal OPEN IR `0101`; setup CLOSE = `ID 31 08 80` then `ID 31 08 00` + normal CLOSE IR `0110`. CLOSE aimed away from all shades removed nothing. Firmware `pair <1-4|all> <add|remove>` (setup frames interleaved with IR, seq timings, no tail). With `id c10e` at a group-4 shade: `grp 4 up` before, no response; `pair 4 add`, jog; `grp 4` after, 5/5 up and 5/5 down. **`C1 0E` is still paired to group 4 on that shade**: `pair 4 remove` not tested yet (next: test it, then `grp 4 up` should fail). Evidence: `captures/rf_ir_setup_mode_20261112.log`; write-up: `docs/RF_PROTOCOL.md` (Pairing). Capacity is unknown: remove test IDs as you go. Never Master Reset either remote.
- **Resumed Nov 15 (breadboard rewired; NVS id is back to `yy yy`, the working remote's).** Manual coverage is nearly complete. Remaining:
  1. ✅ `pair 4 remove` (Nov 15): jogged, then `grp 4` under `c10e` got no response. The first tries that morning didn't jog because the shade was asleep (see the timing notes below); once it was awake, add and remove both jogged. ✅ **Safety check:** with `id` back to the working remote's (`yy yy`), `grp 4 up` and `down` still worked, so the removal dropped only the clone's entry. The working remote's setup add/remove/add jogs both group-4 shades every time, whether or not anything changed (user, Nov 15).
  2. Express mode: `grp 4 up 2500` should fully open/close.
  3. Intermediate stop (p. 8): holding ALL never made the remote flash green (user, Nov 12), so this remote apparently lacks the feature (the flash is the remote's, not the shades'). Confirm with `sniff` while holding ALL ~8 s: nothing on air = record "not supported by this remote".
  - Channel B on a shade and the IR channel change (p. 22) not tried (all shades are on A). Duplicate remote / limit reset skipped on purpose.
- **LED meanings** (for the clone's red/green LED): red = IR on an OPEN/CLOSE press; steady green = radio press during the group window (off when the window ends, which the manual says is 5 s; we measured ~6.5 s); flashing green ~30 s = setup mode, also after holding ALL 6 s (intermediate stop); flashing red = backup remote waiting after Master Reset; no light = dead batteries.
- **Express mode** (p. 9): in green mode, holding OPEN/CLOSE for 2 s opens/closes fully, bypassing the intermediate stop (PowerRise only). Our `grp` hold can reach that.
- **Intermediate stop** (p. 8): hold ALL 6 s, aim, OPEN, then ALL to save. One per shade; a reset clears it. Changing a shade's IR channel uses the shade's manual button plus OPEN (p. 22).

**Manual notes for the clone:** holding OPEN 6 s at a fully open shade resets its stops and limits (p. 21), so `send` caps every code at 5 s (`play` isn't capped). **Both directions capped (decided 2026-11-18, John; fw 0.9.5)**: on top-down/bottom-up shades DOWN opens; until fw 0.9.5 only OPEN was capped and CLOSE allowed 10 s. Group assignment uses SETUP + group key + aimed arrow (p. 5), so IR selects the shade. A backup remote copies identity over RF (Master Reset, then OPEN on the original within 18", p. 7): a good sniffing target, but don't Master Reset either remote before capturing, since it may erase pairing.

## Hardware

### Parts on hand / arriving
- ESP32 dev board: **ESP32-DevKitC with ESP32-WROOM-32D** — confirmed Nov 7 from the can marking and the ROM boot banner (`ets Jun 8 2016`, original ESP32). No PSRAM; PlatformIO `board = esp32dev`; pin map applies as-is. Enumerates as CP210x on COM11.
- Bridgold kit (arrived Nov 7): 5× TSOP38238 IR receivers (black, pinout facing dome: OUT, GND, VS) + 5× TSAL6200 940 nm IR LEDs (clear blue-tint, long leg = anode)
  - TSOP38238 is a 38 kHz part; fine for capturing a 40 kHz remote at close range
  - TSAL6200: 100 mA continuous, 200 mA peak, ±17° beam
  - Unbranded kit; parts may not be genuine Vishay
- Gikfun EK8460 (arrived Nov 7): 10× **VS1838B** 38 kHz demodulating receivers (metal X shield; pinout facing front: OUT, GND, VCC; 2.7–5.5 V) + 10× 5 mm 940 nm IR LEDs (spares, good for loopback). A second receiver type to cross-check captures. Raw carrier frequency and duty cycle come from the logic analyzer on the remote's LED drive instead.
- 38 kHz "round dot" long-range IR receivers, CH0038K-compatible (arriving Sun Nov 8). A second demodulating receiver to cross-check captures against the unbranded TSOP38238s.
- hiBCTR 10-pack "NRF24L01+" PCB-antenna modules (arrived Mon Nov 9; in use)
  - Spec "+7 dB max TX" suggests **Si24R1 clones** (genuine nRF24L01+ maxes at 0 dBm). Inference, not confirmed.
  - One verified-buyer review: 5 of 10 failed SPI comms on a test bench; genuine units passed the same bench. Single data point.
  - Plan: screen every module on arrival. Fallback for flaky sniffing: a genuine Nordic module from Digi-Key.
- **Logic analyzer (owned):** 24 MS/s, 8-channel, works with sigrok/PulseView (`fx2lafw` driver). USB 0925:3881; WinUSB installed via Zadig and working in PulseView as "Saleae Logic", D0–D7 (Nov 7). Inputs −0.5 to 5.25 V. Primary tool for RF reverse engineering (see firmware plan step 4).
- **Key switches (ordered Nov 13, DigiKey, FedEx Ground):** 100 × Same Sky TS02-66-130-BK-160-SCR-D (6 × 6 mm THT, 13.0 mm tall, 160 gf). For the handheld's keys and the test print's key fixture. The 5 mm SETUP switch (TS02-66-50-BK-160-SCR-D, DigiKey 2223-TS02-66-50-BK-160-SCR-D-ND) is optional for prototype 1; see below.
- **Tactile switch assortment (on hand):** BOJACK 6 × 6 mm THT kit, 20 each of heights 4.3, 5, 6, 7, 8, 9, 10, 11, 12, 13 mm. Use its 5 mm part for SETUP and its 13 mm part as a stand-in in the test fixture. **Measure first** (calipers): leg spacing 6.5 × 4.5 mm, height from the body's seating plane to the plunger top (13.0 / 5.0), plunger diameter ≤ ~3.5 mm, travel ~0.25 mm. Force isn't specified (often ~260 gf): use the TS02 160 gf parts for the key-feel test.
- **Enclosure test piece (ordered Nov 13, JLC3DP):** the 7 parts from `hardware/enclosure/parts/test_piece.py` in MJF PA12 natural grey. Parts $10.79; most of the cost was shipping and import fees. **In production since Nov 13 (order W2026111404441186), estimated finish Nov 17**; screws ordered Nov 15 (HanTof 1200-pc black carbon-steel **pan head** self-tappers for wood/plastic: M2 × 6/8/10/12, M2.5 and M3; Amazon, ~1 day). M2 heads measured 3.2 × 1.3 mm (John, Nov 21); counterbore `CB_D` now 4.0. Use M2 × 6 for board screws and the test piece, M2 × 8 for case screws; M2.5 is the fallback if M2 strips. On arrival, run the procedure in `hardware/enclosure/README.md` (Test piece) and update the model's placeholder values. Needs M2 self-tapping screws (~6 mm), a TSAL6200 and one TS02 13 mm switch.
- Still needed: NPN (2N2222 / PN2222 preferred, 2N3904 OK for 1 LED), resistors (1 kΩ, 33 Ω, 2× 10 kΩ, 100 Ω), caps (100 µF, 10 µF, 2× 100 nF ceramic), 2 momentary N.O. pushbuttons, optional nRF24 adapter bases (onboard 3.3 V regulator). The NPN, 470 Ω, 100 kΩ and 3.3 Ω for the IR sink test are on the Nov 17 DigiKey order (below).
- **Handheld PCB order (JLCPCB, placed Nov 17, John):** 5 assembled boards (standard PCBA, top side), $285.98 paid (merchandise $165.89, shipping $37.92, duties and taxes $58.07, disbursement fee $7.52, sales tax $16.58). OSP finish, ±0.1 mm outline (only offered with JLC's railed panel), depaneling included. Record, files and what the placement tour found: `docs/BRINGUP.md` (Order log); regenerate the files with `python -I hardware/tools/make_jlc.py` (per-part rotation fixes live in `CPL_FIXES`). Hand-fitted: J102, J401, SW501–SW508, D303, D304, D501, TP604–TP606.
- **Hand-assembly parts (Nov 17, John):** DigiKey ($44.67): J401 SLW-104-01-G-D ×10, D501 WP59EGW ×10, PLP2-250 ×15, J102 S2B-PH-K-S ×10, plus the IR sink-test parts (1N4148, PN2222A, 470 Ω, 100 kΩ, 3.3 Ω, 4.7 Ω). Amazon: 2 more Bridgold LED kits (DigiKey's genuine TSAL6200 was out of stock; the kit LEDs are what the breadboard tests used, run at ~88 mA, while the board's sinks are calculated at ~150 mA: run the sink test first). Adafruit #1578 ×5: ordered Nov 17 ($63.40 with shipping and tax). Nothing here is checked in hardware yet.

### Pin map (original ESP32)
| Function | GPIO | Notes |
|---|---|---|
| IR receive (TSOP38238 / VS1838B OUT) | 14 | moved 14→32 on Nov 7, back to 14 on Nov 8 (user wiring choice) |
| IR transmit (NPN base via 1 kΩ) | 4 | |
| Button UP | 32 | active-HIGH, external 10 kΩ pull-down, RTC-capable (RTC_GPIO9); swapped with IR RX (not wired yet) |
| Button DOWN | 33 | active-HIGH, external 10 kΩ pull-down, RTC-capable |
| nRF24 SCK | 18 | VSPI default |
| nRF24 MOSI | 23 | VSPI default |
| nRF24 MISO | 19 | VSPI default |
| nRF24 CSN | 5 | VSPI default CS |
| nRF24 CE | 17 | **move (e.g. 21) on WROVER boards; 16/17 used by PSRAM** |
| nRF24 IRQ (optional) | 16 | **move (e.g. 22) on WROVER boards** |

**Radio B dropped (decided 2026-11-18, John):** radio B (the second nRF24 for loopback) is dropped from the project. The working main radio did the sniffing, and the clone's own group commands confirmed the transmit side. A module in the handheld's socket either works or it doesn't. Removed from the docs and the code on 2026-11-18 (fw 0.9.4: `beacon`, `HAS_RADIO_B`, `HW_NRF24_B`, the radio B pins, `kBeaconPayload` and the loopback tests). GPIO 13, 21, 25, 26 and 27 are free on the breadboard.

Buttons are active-high so deep-sleep wake can use `ext1` with `ESP_EXT1_WAKEUP_ANY_HIGH` (original ESP32's ext1 only supports ANY_HIGH or ALL_LOW). On S3/C3, revisit pins and polarity.

### Pin map (handheld PCB rev 1, `pio run -e handheld`)
Selected by `-DHD_BOARD_HANDHELD` (envs `handheld` / `handheld_test`); source of truth is `include/pins.h`, taken from the PCB netlist (WROOM-32E). Same as the breadboard: IR TX 4 (HIGH = LEDs on, R301 470 Ω → two BC817), nRF24 CSN 5 / CE 17 / SPI 18-19-23. Different:
- Keys (active-HIGH, 100 k pull-down + 100 nF (C501–C508, fitted) at each switch; the firmware still debounces; all RTC GPIOs for ext1 wake): UP 32, DOWN 33, ALL 25, G1 26, G2 34, G3 27, G4 36, SETUP 35 (34/35/36 input-only).
- A/B slider 14 (via 10 k; driven to 3V3 or GND; which position is A is to be confirmed at bring-up). Battery sense 39 (1 M/1 M, reads VBAT/2). LED red 22, green 16 (common cathode, HIGH = on). nRF IRQ 13 (via 0 Ω R401, unused).
- No IR receiver: `HAS_IR_RX` is false, so `ir` capture says so instead of touching pins. `btn` prints all keys, A/B and VBAT; `led r|g|both|off` drives the indicator (plain on/off for bring-up; LEDC PWM comes with the handheld UX).

### Netlist (sheet 1: IR + buttons)
- TSOP38238 (pinout facing dome: OUT, GND, VS): VS ← 100 Ω ← 3V3; 100 nF VS→GND at the TSOP; GND→GND; OUT→GPIO 14.
- IR LED: 5V/VIN → 33 Ω (22 Ω if fed from 3.3 V) → TSAL6200 anode; cathode → NPN collector; emitter → GND. GPIO 4 → 1 kΩ → base. ~100 mA pulses.
  - 100 µF bulk + 100 nF ceramic across 5V–GND; ceramic placed at the top of the 33 Ω, grounded near the emitter.
  - Two LEDs: duplicate the 33 Ω + LED branch, same collector; use a 2N2222 (≈200 mA).
- Buttons: 3V3 → switch → GPIO 32 (UP) / 33 (DOWN); 10 kΩ GPIO → GND each. Debounce in software (~20–30 ms).

### Netlist (sheet 2: nRF24)
- VCC → 3V3 (3.3 V only!). With an adapter base: feed 5 V, omit the cap.
- 10 µF across VCC/GND soldered at the module (stripe to GND).
- SPI per pin map. Keep SPI wires short (< ~15 cm); retest "failed" modules on short wires before discarding.

## Firmware plan (next step)
One PlatformIO project, serial-menu driven, usable from this weekend through Monday:
1. **IR capture/dump.** IRremoteESP8266, IRrecvDumpV2-style. Raise capture buffer and timeout; dump raw mark/space timings (protocol is proprietary). Capture every button × both modes, short press vs 2 s hold, several repeats each, to detect toggle bits or counters and repeat behavior.
2. **IR replay.** `send up|down|bbbb [ms]` synthesizes HD frames from the 4-bit code (`lib/hdcore/src/hd_ir.*`) and sends them at 40 kHz. Raw `play` of captures also exists. Needs the IR LED driver to test on a shade (acceptance A2/A5).
3. **nRF24 module self-test.** RF24 lib: `begin()`, `isChipConnected()`, `printDetails()`; label pass/fail.
4. **RF config via logic analyzer (preferred first step).** Open the *degraded* remote and clip onto its nRF24 SPI bus (SCK, MOSI, MISO, CSN), plus CE, IRQ and the IR LED drive. PulseView's nRF24L01 decoder (on top of SPI) shows RF_CH (channels and hopping), RF_SETUP (rate), SETUP_AW/TX_ADDR (address), EN_AA/CONFIG (CRC), and W_TX_PAYLOAD (packets with timing).
   - **Wake-burst test:** capture CE/SPI together with the IR LED drive during red-mode presses. RF activity before IR confirms the hypothesis and gives its timing.
   - Also gives the exact IR carrier frequency and duty cycle.
   - Caveat: 24 MS/s is fine for SPI ≤ ~4 MHz. QFN-20 nRF24 pins 1–6 = CE, CSN, SCK, MOSI, MISO, IRQ.
5. **nRF24 sniffer.** Exact-match sniffing at the published Platinum settings (the default, `hd` preset); output is one line per run of identical packets (count, decoded payload, span, gaps), so no packet is dropped by printing. Promiscuous mode (`promisc aa|55`) is the fallback if that catches nothing; the SPI piggyback (step 4) confirms the settings directly.
6. Later: RF transmit emulation (group up/down, held-repeat + release packets), button handling, deep sleep with ext1 wake.

## Open questions
- Does the clone's own wake (`ID 01 00 80`) + IR move the shade? Does the shade check the ID?
- RF-to-IR timing on the original remote (during or after the RF?), and the shade's receptive window.
- Why the weak remote sends a `00` tail and the working one doesn't.
- IR repeat/continuation behavior on hold.
- Exact ESP32 board/module (WROOM vs WROVER vs S3/C3).
- Does the degraded remote recover with new batteries and cleaned contacts?

## References
- Arduino forum, "NRF24L01+ hack on Hunter Douglas shades": https://forum.arduino.cc/t/nrf24l01-hack-on-hunter-douglas-shades/676940
- Arduino forum, "ESP8266 and NRF24L01+ Hunter Douglas Platinum Remote": https://forum.arduino.cc/t/esp8266-and-nrf24l01-hunter-douglas-platinum-remote/918887
- hd-platinum-rf (protocol research; treats handheld RF framing as partly hypothesis; intentionally no TX): https://github.com/spitcool/hd-platinum-rf
- Platinum remote guide (IR 40 kHz, RF 2.4 GHz): https://blindparts.com/wp-content/uploads/1970/01/Hunter-Douglas-Platinum-Technology-Remote-Control-Guide.pdf
- Bond forum thread (IR replay failed; RF wake theory): https://forum.bondhome.io/t/hunterdouglas-powerrise-2-0-with-platinum-technology/3050
- AVS forum (RF wake burst, 2007): https://www.avsforum.com/threads/automating-hunter-douglas-powerrise.852487/
- Remote Central PowerRise IR codes (may not match Platinum): https://www.remotecentral.com/cgi-bin/codes/hunter_douglas/power_rise/
- TSAL6200 datasheet (Vishay)
