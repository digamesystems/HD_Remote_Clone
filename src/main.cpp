// SPDX-License-Identifier: GPL-2.0-or-later
// Hunter Douglas Platinum remote — bring-up / reverse-engineering firmware.
// Serial-menu driven (115200 baud, newline line endings). Type `help`.

#include <Arduino.h>

#include "hd_ir.h"
#include "hd_rf.h"
#include "hold_tracker.h"
#include "ir_tools.h"
#include "key_parser.h"
#include "parse.h"
#include "pins.h"
#include "rf_tools.h"

namespace {

// Bring-up firmware version, shown at boot and in `help`. Bump the last digit
// with each improvement.
constexpr const char* kFwVersion = "0.9.9";

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

// Arrow keys with seq on: wake frames interleaved with the IR.
void runSeq(uint8_t code) {
  runInterleaved("Seq", code, [](uint8_t ch) { return rf::sendWakeOn(ch, seq.rfMs); });
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
  Serial.printf("\nHD Platinum remote bring-up firmware v%s\n", kFwVersion);
  Serial.println(F(
      "Commands (any line except a # comment stops a running mode):\n"
      " IR\n"
      "  ir [quiet]              IR capture: HD decode summary + raw (quiet: summary only)\n"
      "  send <up|down|bbbb> [ms]  seq steps (RF wake, IR) repeated for ms (default 1000;\n"
      "                            up/down = channel A; max 5000 ms, either direction);\n"
      "                            with wake off, IR alone for ms\n"
      "  Up/Down arrow keys      send up/down (works at the prompt or during capture); see seq\n"
      "  seq [on|off|rf|gap|ir <ms>]  arrow keys: RF ch33, IR, RF ch52, IR, RF ch71, IR\n"
      "                          (on, default: rf 50, gap 0, ir 50) or wake + one 500 ms\n"
      "                          IR burst (off)\n"
      "  hold [on|off|first <ms>]  arrow keys send seq steps while held and print the\n"
      "                          hold time (first: release timeout before the keyboard's\n"
      "                          first repeat, default 700). Either arrow stops at 5000 ms\n"
      "  wake [ms|on|off]       send the RF wake alone (default 450 ms), or turn the\n"
      "                          automatic wake before send / arrow keys on or off\n"
      "  id [hex|new]            show, set or regenerate (random) the remote ID used by\n"
      "                          wake / grp / pair (saved in flash; generated at first boot)\n"
      "  grp <1-4|all> <up|down> [ms]  RF group command (green mode): held frames for ms\n"
      "                          (default 300, max 5000), then the release tail. No IR\n"
      "  pos <mask> <0-255> [ms50 ms32]  EXPERIMENTAL absolute position (spitcool's bridge\n"
      "                          decode): ID 50 MASK POS for ms50 (default 1490), then\n"
      "                          ID 32 MASK POS for ms32 (default 300); max 2000 each.\n"
      "                          mask: group bits 01-08, combinable. Stored (paired) id\n"
      "  bedtime                 preset: pos 08 255, then pos 02 255\n"
      "  waketime                preset: pos 08 0, pos 02 40, pos 05 255\n"
      "  sunset                  preset: pos 01 100, then pos 04 50\n"
      "  pair <1-4|all> <add|remove>  group assignment with the stored id: setup frames\n"
      "                          (ID 30|31 GROUP 80) interleaved with OPEN|CLOSE IR, seq\n"
      "                          timings. Aim at one shade's eye from a few inches\n"
      "  play [slot|last] [n] [gap_ms]  replay at 40 kHz (default last, 1x, 40 ms)\n"
      "  save <slot>             copy last capture to slot 0-7 (RAM only)\n"
      "  list                    show slot sizes\n"
      "  dump [slot|last]        print timings as C array\n"
      " nRF24 (defaults: Platinum handheld, ch 33 / 2 Mbps / E7E7E7 / CRC16 / 5 B)\n"
      "  test [label]            module self-test per data rate, prints PASS/FAIL\n"
      "  info                    printPrettyDetails()\n"
      "  scan                    RPD carrier sweep over ch 0-125\n"
      "  sniff                   listen with current config\n"
      "  hd                      preset: Platinum handheld (also 'ch 52' / 'ch 71')\n"
      "  promisc <aa|55>         fallback preset: 2-byte addr 00 AA / 00 55, no CRC, 32 B\n"
      "  ch <0-125> | rate <250k|1m|2m> | aw <2-5> | addr <hex, on-air order>\n"
      "  crc <0|1|2> | pl <1-32> | filter <on|off> | cfg\n"
      " Misc\n"
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

// Hold mode: an arrow key sends seq steps for as long as it's held, like the
// real remote, and reports the hold time. For timing how long the shade takes
// to respond: hold until the blind does what you want, then let go.
bool holdMode = false;
uint32_t holdFirstGapMs = 700;          // > keyboard repeat delay (Windows default ~500 ms)
constexpr uint32_t kHoldRepeatGapMs = 200;  // > one step (~100 ms) + repeat interval (~33 ms)

void runHold(uint8_t code) {
  hd::HoldTracker hold(holdFirstGapMs, kHoldRepeatGapMs);
  const hd::KeyEvent mine =code == hd::ir::kCodeUp ? hd::KeyEvent::Up : hd::KeyEvent::Down;
  const uint32_t stepMs = (autoWake ? seq.rfMs + seq.gapMs : 0) + seq.irMs;
  const uint32_t capMs = hd::ir::maxSendMs(code);
  uint32_t t0 = millis();
  hold.press(t0);
  int steps = 0;
  long packets = 0;
  bool otherKey = false, capped = false;
  // Key repeats queue up while a step blocks; check between RF and IR as well
  // as after each step, so a repeat is timed to within ~50 ms.
  auto drain = [&] {
    while (Serial.available()) {
      hd::KeyEvent k = keys.feed(Serial.read());
      if (k == mine) hold.repeat(millis());
      else if (k != hd::KeyEvent::Consumed) otherKey = true;
    }
  };
  for (;;) {
    if (autoWake) {
      long n = rf::sendWakeOn(hd::rf::kChannels[steps % 3], seq.rfMs);
      if (n < 0) return;  // no ID or no radio; reason already printed
      packets += n;
      if (seq.gapMs) delay(seq.gapMs);
      drain();
    }
    ir::sendCode(code, seq.irMs, true);
    steps++;
    drain();
    uint32_t now = millis();
    if (otherKey || hold.released(now)) break;
    if (now - t0 + stepMs > capMs) { capped = true; break; }
  }
  uint32_t total = millis() - t0;
  Serial.printf("Hold %s %s: ", code == hd::ir::kCodeUp ? "UP" : "DOWN",
                hd::ir::codeToBits(code).c_str());
  if (hold.repeats())
    Serial.printf("held %lu ms (%lu key repeats)", (unsigned long)hold.heldMs(),
                  (unsigned long)hold.repeats());
  else
    Serial.printf("tap (no key repeats: released within %lu ms)", (unsigned long)holdFirstGapMs);
  Serial.printf(", sent %d steps in %lu ms, %ld wake packets%s%s\n", steps, (unsigned long)total,
                packets, capped ? ", STOPPED at the send limit" : "",
                otherKey ? ", ended by another key" : "");
}

// One burst per key press. A held key repeats, and those repeats pile up during
// the blocking send, so discard input until it goes quiet. That also drops any
// partial escape sequence, which would otherwise land in the command line.
void sendArrow(uint8_t code) {
  Serial.println();
  if (holdMode) runHold(code);
  else if (seq.on) runSeq(code);
  else sendIr(code, kArrowBurstMs);
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
    {"waketime", {{0x08, 0}, {0x02, 40}, {0x05, 255}}},
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
      Serial.printf("Hold mode %s: arrow keys send seq steps while held (release = no key "
                    "repeat for %lu ms before the first repeat, %lu ms after)\n",
                    holdMode ? "on" : "off", (unsigned long)holdFirstGapMs,
                    (unsigned long)kHoldRepeatGapMs);
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
                  digitalRead(PIN_AB_SW) ? 'B' : 'A',  // which rail = which channel: confirm at bring-up
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
  Serial.printf("\nHD Platinum remote bring-up firmware v%s\n", kFwVersion);
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

  if (mode == Mode::IrCapture) ir::pollCapture();
  else if (mode == Mode::RfBusy) {
    rf::poll();
    if (!rf::busy()) {  // failed to start
      mode = Mode::Idle;
      Serial.print("> ");
    }
  }
}
