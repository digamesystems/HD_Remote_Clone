// SPDX-License-Identifier: GPL-2.0-or-later
// Hunter Douglas Platinum remote — bring-up / reverse-engineering firmware.
// Serial-menu driven (115200 baud, newline line endings). Type `help`.

#include <Arduino.h>

#include "handheld.h"
#include "hd_ir.h"
#include "hd_rf.h"
#include "hold_tracker.h"
#include "ir_tools.h"
#include "key_parser.h"
#include "parse.h"
#include "pins.h"
#include "rf_tools.h"
#include "tx_scheduler.h"

namespace {

// Bring-up firmware version, shown at boot and in `help`. Bump the last digit
// with each improvement.
constexpr const char* kFwVersion = "0.9.13";

enum class Mode { Idle, IrCapture, RfBusy } mode = Mode::Idle;
String line;
hd::KeyParser keys;

constexpr uint32_t kArrowBurstMs = 500;  // IR burst for one PC arrow key press
constexpr uint32_t kArrowQuietMs = 100;  // input silence that ends a held key's repeats
// RF wake before each IR command. The working remote sends ~440 ms for a quick tap.
constexpr uint32_t kWakeMs = 450;
constexpr uint32_t kMaxWakeMs = 5000;
// RF group command: the working remote held A1 for ~250 ms on a tap, and its
// release tail lasted ~330 ms. Hold capped at 5 s, like `send` (manual p. 21:
// OPEN held 6 s at a fully open shade resets its limits).
constexpr uint32_t kGroupHoldMs = 300;
constexpr uint32_t kGroupTailMs = 330;
constexpr uint32_t kGroupMaxHoldMs = 5000;
bool autoWake = true;

// Arrow-key sequence (experiment): RF on ch 33, IR, RF on ch 52, IR, RF on ch 71,
// IR. `seq off` goes back to one wake (all channels) followed by one IR burst.
struct SeqConfig {
  bool on = true;
  uint32_t rfMs = 50;   // wake frames per channel step
  uint32_t gapMs = 0;   // pause between each RF step and the IR after it
  // IR per step: 50 ms (3 frames). Nov 15, hand-aimed: 1 frame unreliable,
  // 2 good, 3 good (20/20), 10 (old default) worked with occasional doubles.
  uint32_t irMs = 50;
} seq;
constexpr uint32_t kSeqMaxMs = 2000;

void printSeq() {
  Serial.printf("Arrow keys: %s. seq rf %lu ms/channel, gap %lu ms, ir %lu ms/step\n",
                seq.on ? "RF ch33, IR, RF ch52, IR, RF ch71, IR (seq on)"
                       : "one wake on all channels, then one IR burst (seq off)",
                (unsigned long)seq.rfMs, (unsigned long)seq.gapMs, (unsigned long)seq.irMs);
}

// Runs an interleaved sequence: rfStep(channel) on ch 33, IR, ch 52, IR, ch 71,
// IR, with seq's timings. rfStep returns its packet count, or -1 on error.
// Prints when each step started (ms from the first RF packet) at the end, so
// printing can't skew the timing.
template <typename RfStep>
void runInterleaved(const String& label, uint8_t code, RfStep rfStep) {
  uint32_t t0 = millis(), rfAt[3], irAt[3];
  long packets[3];
  for (int i = 0; i < 3; i++) {
    rfAt[i] = millis() - t0;
    packets[i] = rfStep(hd::rf::kChannels[i]);
    if (packets[i] < 0) return;  // no ID or no radio; reason already printed
    if (seq.gapMs) delay(seq.gapMs);
    irAt[i] = millis() - t0;
    ir::sendCode(code, seq.irMs);
  }
  Serial.printf("%s %s:", label.c_str(), hd::ir::codeToBits(code).c_str());
  for (int i = 0; i < 3; i++) {
    Serial.printf("  RF ch%u x%ld @%lu ms, IR @%lu ms", hd::rf::kChannels[i], packets[i],
                  (unsigned long)rfAt[i], (unsigned long)irAt[i]);
    if (i < 2) Serial.print(";");
  }
  Serial.printf("  (total %lu ms)\n", (unsigned long)(millis() - t0));
}

// Group assignment, as the remote does it in setup mode (SETUP, group key, then
// OPEN to add or CLOSE to remove, aimed at the shade): setup frames ID 30|31
// GROUP 80 interleaved with the OPEN / CLOSE IR code (channel A). The IR picks
// the shade; aim at its sensor eye from a few inches, as the manual says.
void runPair(uint8_t group, bool add) {
  String label = String("Pair ") + (add ? "add" : "remove") + " group " +
                 hd::rf::groupName(group).c_str() + ", IR";
  runInterleaved(label, add ? hd::ir::kCodeUp : hd::ir::kCodeDown,
                 [=](uint8_t ch) { return rf::sendSetupOn(group, add, ch, seq.rfMs); });
}

void help() {
  Serial.printf("\n----------------------------------------------\n");
  Serial.printf("|HD Platinum remote bring-up firmware v%s|\n", kFwVersion);
  Serial.printf("----------------------------------------------\n");
  
  Serial.println(F(
      "Commands (any line except a # comment stops a running mode):\n\n"
      " USER COMMANDS\n"
      " -------------\n\n"
      "  Up/Down arrow keys      the handheld's arrows, via its key logic and TX scheduler:\n"
      "                          red mode = wake + IR steps (see seq); in a group window,\n"
      "                          the group command + tail. A tap is 300 ms; hold on to\n"
      "                          send while the key repeats. Prints a summary per press\n"
      "  1-4 (empty line)        group key 1-4: opens the green window (10 s); arrow keys\n"
      "                          pressed in it send that group's command\n"
      "  grp <1-4|all> <up|down> [ms]  RF group command (green mode): held frames for ms\n"
      "                          (default 300, max 5000), then the release tail. No IR\n"
      "  pos <mask> <0-255> [ms50 ms32]  EXPERIMENTAL absolute position (spitcool's bridge\n"
      "                          decode): ID 50 MASK POS for ms50 (default 1490), then\n"
      "                          ID 32 MASK POS for ms32 (default 300); max 2000 each.\n"
      "                          mask: group bits 01-08, combinable. Stored (paired) id\n"
      "  id [hex|new]            show, set or regenerate (random) the remote ID used by\n"
      "                          wake / grp / pair (saved in flash; generated at first boot)\n"
      "  pair <1-4|all> <add|remove>  group assignment with the stored id: setup frames\n"
      "                          (ID 30|31 GROUP 80) interleaved with OPEN|CLOSE IR, seq\n"
      "                          timings. Aim at one shade's eye from a few inches\n"
      "  Presets\n"
      "   bedtime                pos 08 255, then pos 02 255\n"
      "   waketime               pos 08 128, pos 02 40, pos 05 255\n"
      "   sunset                 pos 01 100, then pos 04 50\n\n"
      " IR TESTING\n"
      " ----------\n\n"
      "  ir [quiet]              IR capture: HD decode summary + raw (quiet: summary only)\n"
      "  send <up|down|bbbb> [ms]  seq steps (RF wake, IR) repeated for ms (default 1000;\n"
      "                            up/down = channel A; max 5000 ms, either direction);\n"
      "                            with wake off, IR alone for ms\n"
      "  seq [on|off|rf|gap|ir <ms>]  arrow keys: RF ch33, IR, RF ch52, IR, RF ch71, IR\n"
      "                          (on, default: rf 50, gap 0, ir 50) or wake + one 500 ms\n"
      "                          IR burst (off)\n"
      "  hold [on|off|first <ms>]  on: arrow keys send while the key repeats (release =\n"
      "                          repeats stop; first: wait for the keyboard's first repeat,\n"
      "                          default 700). off (default): each press is a 300 ms tap.\n"
      "                          Either way the arrows stop at 5000 ms\n"
      "  wake [ms|on|off]       send the RF wake alone (default 450 ms), or turn the\n"
      "                          automatic wake before send / arrow keys on or off\n"
      "  play [slot|last] [n] [gap_ms]  replay at 40 kHz (default last, 1x, 40 ms)\n"
      "  save <slot>             copy last capture to slot 0-7 (RAM only)\n"
      "  list                    show slot sizes\n"
      "  dump [slot|last]        print timings as C array\n\n"
      " nRF TESTING (defaults: Platinum handheld, ch 33 / 2 Mbps / E7E7E7 / CRC16 / 5 B)\n"
      " -----------\n\n"
      "  test [label]            module self-test per data rate, prints PASS/FAIL\n"
      "  info                    printPrettyDetails()\n"
      "  scan                    RPD carrier sweep over ch 0-125\n"
      "  sniff                   listen with current config\n"
      "  hd                      preset: Platinum handheld (also 'ch 52' / 'ch 71')\n"
      "  promisc <aa|55>         fallback preset: 2-byte addr 00 AA / 00 55, no CRC, 32 B\n"
      "  ch <0-125> | rate <250k|1m|2m> | aw <2-5> | addr <hex, on-air order>\n"
      "  crc <0|1|2> | pl <1-32> | filter <on|off> | cfg\n"
      "  pa [min|low|high|max]   TX power for wake/grp/pair/pos; no argument shows it\n"
      "                          (high at boot = -6 dBm on a genuine nRF24L01+)\n"
      " MISC\n"
      " ----\n\n"
      "  btn                     show button state (PCB: all keys, A/B, VBAT)\n"
      "  led <r|g|both|off>      indicator LED (handheld PCB only)\n"
      "  pin <gpio> [ms]         logic probe: level, edges, % low (default 3000 ms)\n"
      "  # <note>                comment for the log; works during capture\n"
      "  help\n"));
}

void stopAll() {
  if (mode == Mode::IrCapture) ir::stopCapture();
  if (mode == Mode::RfBusy) rf::stop();
  mode = Mode::Idle;
}

// Software logic probe: sets `pin` to INPUT and polls it for `ms`, reporting
// the starting level, edge count and time spent low. An IR receiver idles
// HIGH and pulses LOW while it sees a remote.
void probePin(int pin, uint32_t ms) {
  pinMode(pin, INPUT);
  int start = digitalRead(pin), prev = start;
  uint32_t edges = 0, lowUs = 0, t0 = micros(), last = t0;
  Serial.printf("Probing GPIO %d for %lu ms (start level %d)...\n", pin, (unsigned long)ms, start);
  while (micros() - t0 < ms * 1000UL) {
    int v = digitalRead(pin);
    uint32_t now = micros();
    if (prev == LOW) lowUs += now - last;
    last = now;
    if (v != prev) { edges++; prev = v; }
  }
  Serial.printf("GPIO %d: start=%d end=%d edges=%lu low=%.1f%%\n", pin, start, prev,
                (unsigned long)edges, 100.0 * lowUs / (ms * 1000.0));
}

// The shade ignores IR unless RF from a paired remote arrives just before it,
// so send the wake first (one wake arms one IR command).
void sendIr(uint8_t code, uint32_t ms) {
  if (autoWake) rf::sendWake(kWakeMs);
  ir::sendCode(code, ms);
}

// `send`: repeats the seq step (RF wake on one channel, gap, IR; channels 33,
// 52, 71 in turn) for about `ms`, so the RF keeps coming for the whole press,
// as with the real remote. Only whole steps that fit in `ms` are sent (at
// least one), so the 5 s cap holds. With `wake off`, IR alone for `ms`.
// Nov 15: one 450 ms wake followed by the IR was unreliable, and failed more
// after a few seconds idle (the shade seems to sleep deeper).
void runSend(uint8_t code, uint32_t ms) {
  if (!autoWake) {
    ir::sendCode(code, ms);
    return;
  }
  const uint32_t stepMs = seq.rfMs + seq.gapMs + seq.irMs;
  uint32_t t0 = millis();
  int steps = 0;
  long packets = 0;
  do {
    long n = rf::sendWakeOn(hd::rf::kChannels[steps % 3], seq.rfMs);
    if (n < 0) return;  // no ID or no radio; reason already printed
    packets += n;
    if (seq.gapMs) delay(seq.gapMs);
    ir::sendCode(code, seq.irMs, true);
    steps++;
  } while (millis() - t0 + stepMs <= ms);
  Serial.printf("Send %s (%s): %d steps of RF %lu ms + gap %lu ms + IR %lu ms, ch 33/52/71 "
                "in turn, %ld wake packets, total %lu ms\n",
                hd::ir::codeToBits(code).c_str(), hd::ir::codeName(code), steps,
                (unsigned long)seq.rfMs, (unsigned long)seq.gapMs, (unsigned long)seq.irMs,
                packets, (unsigned long)(millis() - t0));
}

// Hold mode: an arrow key counts as held for as long as the keyboard repeats
// it, like the real remote's key, and the hold time is reported. A terminal
// sends no key-up, so the release is inferred from the repeats stopping. With
// hold off, every arrow press is a tap of kTapMs.
bool holdMode = false;
uint32_t holdFirstGapMs = 700;          // > keyboard repeat delay (Windows default ~500 ms)
constexpr uint32_t kHoldRepeatGapMs = 200;  // > one step (~100 ms) + repeat interval (~33 ms)
constexpr uint32_t kTapMs = 300;  // hold off: released this long after the press (3 RF+IR cycles)

// Handheld behaviour (lib/hdcore handheld.h), driven from the laptop until the
// boards' keys are wired: digits 1-4 on an empty command line are group keys 1-4
// (open the green window), and the arrow keys follow the window like the real
// remote's arrows. On the handheld build the LED shows it too.
hd::ui::Handheld ui;
bool windowShown = false;

// Drives the LED only when the handheld's LED state changes, so the bring-up
// `led` command still works in between.
void showLed(hd::ui::Led led) {
  static hd::ui::Led shown = hd::ui::Led::Off;
  if (led == shown) return;
  shown = led;
#if defined(HD_BOARD_HANDHELD)
  digitalWrite(PIN_LED_RED, led == hd::ui::Led::Red ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, led == hd::ui::Led::Green ? HIGH : LOW);
#endif
}

void groupKey(int n) {
  static const hd::ui::Key kKeys[4] = {hd::ui::Key::G1, hd::ui::Key::G2, hd::ui::Key::G3,
                                       hd::ui::Key::G4};
  ui.press(kKeys[n - 1], millis());
  ui.release(kKeys[n - 1], millis());
  windowShown = true;
  showLed(ui.update(millis()).led);
  Serial.printf("\nGroup %d: green window open %lu ms; arrow keys send group %d commands\n> ", n,
                (unsigned long)hd::ui::Config().windowMs, n);
}

// Called from loop(): reports the window closing and keeps the LED in step.
void pollUi() {
  const hd::ui::Output o = ui.update(millis());
  showLed(o.led);
  const bool green = o.led == hd::ui::Led::Green;
  if (windowShown && !green) {
    Serial.print("\nGroup window closed: arrow keys back to red mode (wake + IR)\n> ");
    Serial.print(line);
  }
  windowShown = green;
}

// Transmit scheduler (lib/hdcore tx_scheduler.h): turns the handheld logic's
// output into short RF / IR steps, as the handheld firmware will.
hd::tx::Scheduler txs;

struct ArrowStats {
  hd::ui::Tx kind = hd::ui::Tx::None;  // what the press started as
  uint8_t group = 0;
  uint32_t rfSteps = 0, irSteps = 0, heldMs = 0, tailMs = 0;
  long packets = 0;
  uint32_t worstOverrunMs = 0;  // longest a step ran past its nominal length
};

// Runs one arrow press through the handheld logic and the scheduler, step by
// step, until there's nothing left to send (the group tail included). The
// release comes from the key repeats (hold on) or after kTapMs (hold off).
void runArrow(uint8_t code) {
  const bool up = code == hd::ir::kCodeUp;
  const hd::ui::Key k = up ? hd::ui::Key::Up : hd::ui::Key::Down;
  const hd::KeyEvent mine = up ? hd::KeyEvent::Up : hd::KeyEvent::Down;
  hd::tx::Config& c = txs.config();
  c.rfMs = seq.rfMs;
  c.gapMs = seq.gapMs;
  c.irMs = seq.irMs;
  c.wake = autoWake;
  uint8_t id[2];
  rf::getRemoteId(id);  // sendStep reports a missing ID
  txs.setId(id[0], id[1]);
  txs.reset();

  hd::HoldTracker hold(holdFirstGapMs, kHoldRepeatGapMs);
  const uint32_t t0 = millis();
  ui.press(k, t0);
  hold.press(t0);
  ArrowStats st;
  bool released = false, otherKey = false, capped = false, failed = false;
  for (;;) {
    while (Serial.available()) {  // key repeats queue up during a step
      hd::KeyEvent e = keys.feed(Serial.read());
      if (e == mine) hold.repeat(millis());
      else if (e != hd::KeyEvent::Consumed) otherKey = true;
    }
    uint32_t now = millis();
    if (!released && (otherKey || (holdMode ? hold.released(now) : now - t0 >= kTapMs))) {
      ui.release(k, now);
      released = true;
    }
    const hd::ui::Output o = ui.update(now);
    showLed(o.led);
    if (st.kind == hd::ui::Tx::None) {
      st.kind = o.tx;
      st.group = o.group;
    }
    const hd::tx::Step s = txs.next(o);
    if (s.kind == hd::tx::StepKind::None) {
      if (released) break;
      capped = true;  // still held, but the 5 s cap has stopped it
      delay(1);
      continue;
    }
    const uint32_t start = millis();
    if (s.kind == hd::tx::StepKind::Rf) {
      long n = rf::sendStep(s.payload, s.channel, s.ms);
      if (n < 0) { failed = true; break; }  // no ID or no radio; reason already printed
      st.packets += n;
      st.rfSteps++;
      if (s.payload[2] == hd::rf::kModeGroup) (s.payload[4] & 0x80 ? st.heldMs : st.tailMs) += s.ms;
    } else if (s.kind == hd::tx::StepKind::Ir) {
      ir::sendCode(s.irCode, s.ms, true);
      st.irSteps++;
    } else {
      delay(s.ms);
    }
    const uint32_t took = millis() - start;
    if (took > s.ms && took - s.ms > st.worstOverrunMs) st.worstOverrunMs = took - s.ms;
  }
  if (failed) {
    if (!released) ui.release(k, millis());
    txs.reset();
    return;
  }
  const uint32_t total = millis() - t0;

  Serial.printf("Arrow %s, ", up ? "UP" : "DOWN");
  if (st.kind == hd::ui::Tx::Group)
    Serial.printf("group %s: A%c %lu ms, tail %lu ms, %ld packets rotating ch 33/52/71",
                  hd::rf::groupName(st.group).c_str(), up ? '1' : '2', (unsigned long)st.heldMs,
                  (unsigned long)st.tailMs, st.packets);
  else
    Serial.printf("red %s%s: %lu RF steps (%ld wake packets), %lu IR steps",
                  hd::ir::codeToBits(code).c_str(), autoWake ? "" : " (wake off)",
                  (unsigned long)st.rfSteps, st.packets, (unsigned long)st.irSteps);
  if (!holdMode) Serial.printf("; tap (%lu ms)", (unsigned long)kTapMs);
  else if (hold.repeats())
    Serial.printf("; held %lu ms (%lu key repeats)", (unsigned long)hold.heldMs(),
                  (unsigned long)hold.repeats());
  else
    Serial.printf("; tap (no key repeats within %lu ms)", (unsigned long)holdFirstGapMs);
  Serial.printf("; total %lu ms, worst step overrun %lu ms%s%s\n", (unsigned long)total,
                (unsigned long)st.worstOverrunMs, capped ? ", STOPPED at the 5 s limit" : "",
                otherKey ? ", ended by another key" : "");
}

// One arrow press. A held key repeats, and the repeats left over at the end are
// discarded until input goes quiet. That also drops any partial escape
// sequence, which would otherwise land in the command line.
void sendArrow(uint8_t code) {
  Serial.println();
  const bool up = code == hd::ir::kCodeUp;
  const hd::ui::Key k = up ? hd::ui::Key::Up : hd::ui::Key::Down;
  if (seq.on || ui.update(millis()).led == hd::ui::Led::Green) {
    runArrow(code);  // the handheld path; the group window ignores `seq off`
  } else {  // seq off: the old pattern, one wake on all channels, then one IR burst
    ui.press(k, millis());
    showLed(ui.update(millis()).led);
    sendIr(code, kArrowBurstMs);
    ui.release(k, millis());
  }
  uint32_t lastInput = millis();
  while (millis() - lastInput < kArrowQuietMs) {
    if (Serial.available()) {
      Serial.read();
      lastInput = millis();
    } else {
      delay(1);
    }
  }
  if (mode == Mode::Idle) {
    Serial.print("> ");
    Serial.print(line);  // restore anything typed before the arrow
  }
}

// slot argument: "" / "last" -> -1, else number
int parseSlot(const String& s) {
  if (s.length() == 0 || s == "last") return -1;
  return s.toInt();
}

// Position presets (John, 2026-11-18): each step is `pos <mask> <pos>` with
// the default timings, sent in order. Group bits: 01, 02, 04, 08 = groups 1-4.
// Up to 4 steps; unused slots are {0, 0} (mask 0 is never valid) and end it.
struct PresetStep {
  uint8_t mask, pos;
};
struct Preset {
  const char* name;
  PresetStep steps[4];
};
constexpr Preset kPresets[] = {
    {"bedtime", {{0x08, 255}, {0x02, 255}}},
    {"waketime", {{0x08, 128}, {0x02, 40}, {0x05, 255}}},
    {"sunset", {{0x01, 100}, {0x04, 50}}},
};

const Preset* findPreset(const String& name) {
  for (const Preset& p : kPresets)
    if (name == p.name) return &p;
  return nullptr;
}

void handle(String cmdline) {
  cmdline.trim();
  if (cmdline.length() == 0) return;

  auto parts = hd::tokenize(cmdline.c_str(), 5);  // pos takes 4 arguments
  String tok[5];
  for (size_t i = 0; i < parts.size(); i++) tok[i] = parts[i].c_str();
  String& c = tok[0];
  c.toLowerCase();
  bool ok = true;

  if (c == "help" || c == "?") help();
  else if (c == "ir") {
    tok[1].toLowerCase();
    ir::startCapture(tok[1] != "quiet");
    mode = Mode::IrCapture;
  }
  else if (c == "send") {
    int code = hd::ir::parseCode(tok[1].c_str());
    long ms = tok[2].length() ? tok[2].toInt() : 1000;
    if (code < 0 || ms <= 0) ok = false;
    else if ((uint32_t)ms > hd::ir::maxSendMs(code))
      Serial.printf("Not sent: limit for %s is %lu ms (held 6 s at the open limit can reset a shade).\n",
                    hd::ir::codeToBits(code).c_str(), (unsigned long)hd::ir::maxSendMs(code));
    else runSend(code, ms);
  }
  else if (c == "wake") {
    tok[1].toLowerCase();
    if (tok[1] == "on" || tok[1] == "off") {
      autoWake = tok[1] == "on";
      Serial.printf("Automatic RF wake before send / arrow keys: %s\n", autoWake ? "on" : "off");
    } else {
      long ms = tok[1].length() ? tok[1].toInt() : kWakeMs;
      if (ms <= 0 || (uint32_t)ms > kMaxWakeMs) ok = false;
      else rf::sendWake(ms);
    }
  }
  else if (c == "grp") {
    int group = hd::rf::parseGroup(tok[1].c_str());
    tok[2].toLowerCase();
    bool up = tok[2] == "up";
    long ms = tok[3].length() ? tok[3].toInt() : kGroupHoldMs;
    if (group < 0 || !(up || tok[2] == "down") || ms <= 0 || (uint32_t)ms > kGroupMaxHoldMs) ok = false;
    else rf::sendGroup(group, up, ms, kGroupTailMs);
  }
  else if (c == "pos") {
    int mask = hd::rf::parsePosMask(tok[1].c_str());
    int pos = hd::rf::parsePosition(tok[2].c_str());
    // Phase lengths: both or neither; digits only (toInt() reads "abc" as 0).
    auto digits = [](const String& s) {
      if (!s.length() || s.length() > 5) return false;
      for (size_t i = 0; i < s.length(); i++)
        if (!isDigit(s[i])) return false;
      return true;
    };
    uint32_t ms1 = hd::rf::kPosPhase1Ms, ms2 = hd::rf::kPosPhase2Ms;
    bool timesOk = true;
    if (tok[3].length() || tok[4].length()) {
      timesOk = digits(tok[3]) && digits(tok[4]);
      if (timesOk) {
        ms1 = tok[3].toInt();
        ms2 = tok[4].toInt();
      }
    }
    if (mask < 0 || pos < 0 || !timesOk || !hd::rf::positionTimingOk(ms1, ms2)) ok = false;
    else rf::sendPosition(mask, pos, ms1, ms2);
  }
  else if (const Preset* pr = findPreset(c)) {
    for (const PresetStep& s : pr->steps)
      if (!s.mask || !rf::sendPosition(s.mask, s.pos, hd::rf::kPosPhase1Ms, hd::rf::kPosPhase2Ms))
        break;
  }
  else if (c == "pair") {
    int group = hd::rf::parseGroup(tok[1].c_str());
    tok[2].toLowerCase();
    bool add = tok[2] == "add";
    if (group < 0 || !(add || tok[2] == "remove")) ok = false;
    else runPair(group, add);
  }
  else if (c == "seq") {
    tok[1].toLowerCase();
    long v = tok[2].toInt();
    bool inRange = tok[2].length() && v >= 0 && v <= (long)kSeqMaxMs;
    if (tok[1] == "on" || tok[1] == "off") seq.on = tok[1] == "on";
    else if (tok[1] == "rf" && inRange) seq.rfMs = v;
    else if (tok[1] == "gap" && inRange) seq.gapMs = v;
    else if (tok[1] == "ir" && inRange && v > 0) seq.irMs = v;
    else if (tok[1].length()) ok = false;
    if (ok) printSeq();
  }
  else if (c == "hold") {
    tok[1].toLowerCase();
    long v = tok[2].toInt();
    if (tok[1] == "on" || tok[1] == "off") holdMode = tok[1] == "on";
    else if (tok[1] == "first" && v >= 100 && v <= 2000) holdFirstGapMs = v;
    else if (tok[1].length()) ok = false;
    if (ok)
      if (holdMode)
        Serial.printf("Hold mode on: arrow keys send while held (release = no key repeat for "
                      "%lu ms before the first repeat, %lu ms after)\n",
                      (unsigned long)holdFirstGapMs, (unsigned long)kHoldRepeatGapMs);
      else
        Serial.printf("Hold mode off: each arrow press is a %lu ms tap\n", (unsigned long)kTapMs);
  }
  else if (c == "id") {
    tok[1].toLowerCase();
    if (tok[1] == "new") ok = rf::newRemoteId();
    else if (tok[1].length()) ok = rf::setRemoteId(tok[1] + tok[2]);  // "12 34" or "1234"
    if (ok) rf::printRemoteId();
  }
  else if (c == "play") {
    int reps = tok[2].length() ? tok[2].toInt() : 1;
    int gap = tok[3].length() ? tok[3].toInt() : 40;
    ir::play(parseSlot(tok[1]), max(reps, 1), gap);
  }
  else if (c == "save") ok = tok[1].length() && ir::save(tok[1].toInt());
  else if (c == "list") ir::list();
  else if (c == "dump") ir::dump(parseSlot(tok[1]));
  else if (c == "test") rf::selfTest(tok[1].length() ? tok[1].toInt() : 0);
  else if (c == "info") rf::printDetails();
  else if (c == "scan") { rf::startScan(); mode = Mode::RfBusy; }
  else if (c == "sniff") { rf::startSniff(); mode = Mode::RfBusy; }
  else if (c == "hd") { rf::presetHd(); rf::printConfig(); }
  else if (c == "promisc") {
    tok[1].toLowerCase();
    if (tok[1] == "55") rf::presetPromiscuous(0x55);
    else rf::presetPromiscuous(0xAA);
    rf::printConfig();
  }
  else if (c == "ch") { rf::setChannel(tok[1].toInt()); rf::printConfig(); }
  else if (c == "rate") { tok[1].toLowerCase(); ok = rf::setRate(tok[1]); }
  else if (c == "aw") ok = rf::setAddressWidth(tok[1].toInt());
  else if (c == "addr") ok = rf::setAddress(tok[1]);
  else if (c == "crc") ok = rf::setCrc(tok[1].toInt());
  else if (c == "pa") {
    if (tok[1].length()) ok = rf::setPa(tok[1]);
    if (ok) rf::printPa();
  }
  else if (c == "pl") ok = rf::setPayloadSize(tok[1].toInt());
  else if (c == "filter") { tok[1].toLowerCase(); rf::setDupFilter(tok[1] != "off"); }
  else if (c == "cfg") rf::printConfig();
  else if (c == "pin") {
    int p = tok[1].length() ? tok[1].toInt() : -1;
    long ms = tok[2].length() ? tok[2].toInt() : 3000;
    // Refuse flash pins (6-11), the IR LED driver and nRF24 outputs.
    bool bad = p < 0 || p > 39 || (p >= 6 && p <= 11) || p == PIN_IR_TX ||
               p == PIN_NRF_CSN || p == PIN_NRF_CE || p == 18 || p == 23;
    if (bad || ms <= 0 || ms > 30000) ok = false;
    else probePin(p, ms);
  }
  else if (c == "btn") {
#if defined(HD_BOARD_HANDHELD)
    Serial.printf("UP=%d DOWN=%d ALL=%d G1=%d G2=%d G3=%d G4=%d SETUP=%d  A/B=%c  VBAT~%u mV\n",
                  digitalRead(PIN_BTN_UP), digitalRead(PIN_BTN_DN), digitalRead(PIN_BTN_ALL),
                  digitalRead(PIN_BTN_G1), digitalRead(PIN_BTN_G2), digitalRead(PIN_BTN_G3),
                  digitalRead(PIN_BTN_G4), digitalRead(PIN_BTN_SETUP),
                  digitalRead(PIN_AB_SW) ? 'B' : 'A',  // high = B, as on the original (rev 1 bring-up)
                  (unsigned)(analogReadMilliVolts(PIN_VBAT_SENSE) * 2));  // 1M/1M divider
#else
    Serial.printf("UP=%d DOWN=%d\n", digitalRead(PIN_BTN_UP), digitalRead(PIN_BTN_DN));
#endif
  }
  else if (c == "led") {
#if defined(HD_BOARD_HANDHELD)
    tok[1].toLowerCase();
    bool r = tok[1] == "r" || tok[1] == "red" || tok[1] == "both";
    bool g = tok[1] == "g" || tok[1] == "green" || tok[1] == "both";
    if (!r && !g && tok[1] != "off") ok = false;
    else {
      digitalWrite(PIN_LED_RED, r ? HIGH : LOW);
      digitalWrite(PIN_LED_GREEN, g ? HIGH : LOW);
      Serial.printf("LED red=%d green=%d\n", r, g);
    }
#else
    Serial.println("No indicator LED on this board.");
#endif
  }
  else { Serial.printf("Unknown command '%s'. Type help.\n", c.c_str()); return; }

  if (!ok) Serial.println("Bad argument.");
  else if (c == "rate" || c == "aw" || c == "addr" || c == "crc" || c == "pl" || c == "filter")
    rf::printConfig();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PIN_BTN_UP, INPUT);  // external pull-downs
  pinMode(PIN_BTN_DN, INPUT);
#if defined(HD_BOARD_HANDHELD)
  for (uint8_t p : {PIN_BTN_ALL, PIN_BTN_G1, PIN_BTN_G2, PIN_BTN_G3, PIN_BTN_G4, PIN_BTN_SETUP,
                    PIN_AB_SW})
    pinMode(p, INPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);
#endif
  randomSeed(esp_random());
  ir::begin();
  rf::begin();  // non-fatal if no module yet
  help();
  Serial.print("> ");
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    hd::KeyEvent key = keys.feed(ch);
    if (key == hd::KeyEvent::Up) { sendArrow(hd::ir::kCodeUp); continue; }
    if (key == hd::KeyEvent::Down) { sendArrow(hd::ir::kCodeDown); continue; }
    if (key == hd::KeyEvent::Consumed) continue;
    if (ch == '\r') continue;
    if (line.length() == 0 && ch >= '1' && ch <= '4') {  // group key; digits inside a command still type
      groupKey(ch - '0');
      continue;
    }
    // Echo input: terminals like `pio device monitor` don't echo locally, and
    // the echo also records each command in serial logs.
    if (ch == '\b' || ch == 0x7F) {
      if (line.length()) {
        line.remove(line.length() - 1);
        Serial.print("\b \b");
      }
      continue;
    }
    if (ch == '\n') {
      Serial.println();
      if (hd::isComment(line.c_str())) {  // log note: leave any running mode alone
        line = "";
        if (mode == Mode::Idle) Serial.print("> ");
        continue;
      }
      bool wasBusy = mode != Mode::Idle;
      stopAll();
      handle(line);
      line = "";
      if (!wasBusy || mode == Mode::Idle) Serial.print("> ");
    } else if (line.length() < 120 && isprint((unsigned char)ch)) {
      line += ch;
      Serial.write(ch);
    }
  }

  pollUi();
  if (mode == Mode::IrCapture) ir::pollCapture();
  else if (mode == Mode::RfBusy) {
    rf::poll();
    if (!rf::busy()) {  // failed to start
      mode = Mode::Idle;
      Serial.print("> ");
    }
  }
}
