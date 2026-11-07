// Hunter Douglas Platinum remote — bring-up / reverse-engineering firmware.
// Serial-menu driven (115200 baud, newline line endings). Type `help`.

#include <Arduino.h>

#include "hd_ir.h"
#include "ir_tools.h"
#include "parse.h"
#include "pins.h"
#include "rf_tools.h"

namespace {

enum class Mode { Idle, IrCapture, RfBusy } mode = Mode::Idle;
String line;

void help() {
  Serial.println(F(
      "\nCommands (any line stops a running mode):\n"
      " IR\n"
      "  ir [quiet]              IR capture: HD decode summary + raw (quiet: summary only)\n"
      "  send <up|down|bbbb> [ms]  transmit HD frames at 40 kHz (default 1000 ms)\n"
      "  play [slot|last] [n] [gap_ms]  replay at 40 kHz (default last, 1x, 40 ms)\n"
      "  save <slot>             copy last capture to slot 0-7 (RAM only)\n"
      "  list                    show slot sizes\n"
      "  dump [slot|last]        print timings as C array\n"
      " nRF24\n"
      "  test [label]            module self-test, prints PASS/FAIL\n"
      "  info                    printPrettyDetails()\n"
      "  scan                    RPD carrier sweep over ch 0-125\n"
      "  sniff                   listen with current config\n"
      "  promisc <aa|55>         preset: 2-byte addr 00 AA / 00 55, no CRC, 32 B\n"
      "  ch <0-125> | rate <250k|1m|2m> | aw <2-5> | addr <hex, on-air order>\n"
      "  crc <0|1|2> | pl <1-32> | filter <on|off> | cfg\n"
      " Misc\n"
      "  btn                     show UP/DOWN button state\n"
      "  help\n"));
}

void stopAll() {
  if (mode == Mode::IrCapture) ir::stopCapture();
  if (mode == Mode::RfBusy) rf::stop();
  mode = Mode::Idle;
}

// slot argument: "" / "last" -> -1, else number
int parseSlot(const String& s) {
  if (s.length() == 0 || s == "last") return -1;
  return s.toInt();
}

void handle(String cmdline) {
  cmdline.trim();
  if (cmdline.length() == 0) return;

  auto parts = hd::tokenize(cmdline.c_str(), 4);
  String tok[4];
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
    if (code < 0 || ms <= 0 || ms > 10000) ok = false;
    else ir::sendCode(code, ms);
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
  else if (c == "btn") {
    Serial.printf("UP=%d DOWN=%d\n", digitalRead(PIN_BTN_UP), digitalRead(PIN_BTN_DN));
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
  randomSeed(esp_random());
  ir::begin();
  Serial.println(F("\nHD Platinum remote bring-up firmware"));
  rf::begin();  // non-fatal if no module yet
  help();
  Serial.print("> ");
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      bool wasBusy = mode != Mode::Idle;
      stopAll();
      handle(line);
      line = "";
      if (!wasBusy || mode == Mode::Idle) Serial.print("> ");
    } else if (line.length() < 120) {
      line += ch;
    }
  }

  if (mode == Mode::IrCapture) ir::pollCapture();
  else if (mode == Mode::RfBusy) rf::poll();
}
