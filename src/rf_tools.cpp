// SPDX-License-Identifier: GPL-2.0-or-later
#include "rf_tools.h"

#include <Preferences.h>
#include <RF24.h>
#include <SPI.h>
#include <bootloader_random.h>
#include <esp_random.h>

#include "dup_filter.h"
#include "hd_rf.h"
#include "nrf_addr.h"
#include "parse.h"
#include "pins.h"
#include "run_tracker.h"

namespace rf {
namespace {

RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
bool radioOk = false;
// Transmit power for wake, grp, pair and pos (`pa`). RAM only: high at boot.
// PA_HIGH is -6 dBm on a genuine part, about what the original remote sends.
rf24_pa_dbm_e txPa = RF24_PA_HIGH;

// Defaults: the Platinum handheld (see hd_rf.h). `promisc` switches to the
// address-less fallback.
struct Config {
  uint8_t channel = hd::rf::kChannels[0];
  rf24_datarate_e rate = RF24_2MBPS;
  uint8_t addrWidth = hd::rf::kAddrWidth;
  uint8_t addrOnAir[5] = {0xE7, 0xE7, 0xE7};  // as transmitted, first byte first
  uint8_t crcBytes = hd::rf::kCrcBytes;
  uint8_t payloadSize = hd::rf::kPayloadSize;
} cfg;

enum class Mode { Idle, Scan, Sniff } mode = Mode::Idle;

// --- scan state ---
constexpr uint8_t kNumChannels = 126;
constexpr int kSweepsPerRow = 20;
uint8_t hits[kNumChannels];
int sweeps = 0;

// --- sniff state ---
// filter on: hd::DupFilter, for promiscuous mode, where noise matches the
// address constantly. filter off: every packet is counted and printed as runs
// of identical payloads (hd::RunTracker), for exact-match sniffing.
hd::DupFilter filter;
hd::RunTracker runs;
constexpr uint32_t kRunIdleUs = 30000;  // a run ends after 30 ms of silence
uint32_t rxTotal = 0;
bool dupFilter = false;

// --- remote ID for wake, group and setup frames ---
// Kept in flash (NVS), not in the source. A unit with none generates a random
// one at boot (decided 2026-11-18, John), so each builder's clone pairs under
// its own ID; `id <hex>` overrides it (e.g. an original remote's, from a sniff).
constexpr const char* kPrefsNs = "hd";
constexpr const char* kPrefsIdKey = "rid";
uint8_t remoteId[2] = {0, 0};
bool haveRemoteId = false;

void loadRemoteId() {
  Preferences prefs;
  prefs.begin(kPrefsNs, false);
  haveRemoteId = prefs.getBytes(kPrefsIdKey, remoteId, 2) == 2;
  prefs.end();
}

const char* rateName(rf24_datarate_e r) { return hd::rateName(static_cast<hd::Rate>(r)); }

bool ensureRadio() {
  if (radioOk) return true;
  radioOk = radio.begin() && radio.isChipConnected();
  if (!radioOk) Serial.println("nRF24 not responding (check wiring/3.3V/10uF cap).");
  return radioOk;
}

// Applies cfg to the radio, except listening/writing.
void applyCommon(RF24& r) {
  r.stopListening();
  r.setAutoAck(false);
  // Auto-ack off and no retries: ShockBurst-compatible frames with no packet
  // control field, which is how the remote's frames arrive (88 bits on air).
  r.setRetries(0, 0);
  r.disableDynamicPayloads();
  r.setDataRate(cfg.rate);
  r.setChannel(cfg.channel);
  r.setAddressWidth(cfg.addrWidth);
  if (cfg.crcBytes == 0) {
    r.disableCRC();
  } else {
    r.setCRCLength(cfg.crcBytes == 1 ? RF24_CRC_8 : RF24_CRC_16);
  }
  r.setPayloadSize(cfg.payloadSize);
}

// RF24 takes the address in register order (LSByte first); the radio sends
// the MSByte first, so reverse the on-air bytes.
void registerAddress(uint8_t* reg) {
  memset(reg, 0, 5);
  hd::onAirToRegister(cfg.addrOnAir, cfg.addrWidth, reg);
}

void applyRx() {
  applyCommon(radio);
  uint8_t reg[5];
  registerAddress(reg);
  radio.openReadingPipe(0, reg);
  radio.startListening();
}

void printHex(const uint8_t* p, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) Serial.printf("%02X ", p[i]);
}

void printRun(const hd::PacketRun& r) {
  Serial.printf("%10.6f s  ch%u  x%-5lu ", r.firstUs / 1e6, cfg.channel, (unsigned long)r.count);
  printHex(r.payload, r.len);
  std::string desc = hd::rf::describePayload(r.payload, r.len);
  if (!desc.empty()) Serial.printf(" %s", desc.c_str());
  if (r.count > 1) {
    Serial.printf("  span %.1f ms  gap %lu-%lu us", (r.lastUs - r.firstUs) / 1000.0,
                  (unsigned long)r.minGapUs, (unsigned long)r.maxGapUs);
  }
  Serial.println();
}

void handlePacket(const uint8_t* buf, uint32_t tUs) {
  rxTotal++;
  if (!dupFilter) {
    hd::PacketRun done;
    if (runs.feed(buf, cfg.payloadSize, tUs, &done)) printRun(done);
    return;
  }
  uint16_t count = filter.feed(buf, cfg.payloadSize);
  if (count) {
    Serial.printf("%8lu ch%u x%-3u ", (unsigned long)(tUs / 1000), cfg.channel, count);
    printHex(buf, cfg.payloadSize);
    Serial.println();
  }
}

void pollScan() {
  for (uint8_t ch = 0; ch < kNumChannels; ch++) {
    radio.setChannel(ch);
    radio.startListening();
    delayMicroseconds(200);
    radio.stopListening();
    if (radio.testRPD() && hits[ch] < 255) hits[ch]++;
  }
  if (++sweeps < kSweepsPerRow) return;
  for (uint8_t ch = 0; ch < kNumChannels; ch++) {
    uint8_t h = hits[ch];
    Serial.print(h == 0 ? '.' : (h < 10 ? char('0' + h) : '*'));
    hits[ch] = 0;
  }
  Serial.println();
  sweeps = 0;
}

void pollSniff() {
  uint8_t buf[32];
  while (radio.available()) {
    uint32_t t = micros();
    radio.read(buf, cfg.payloadSize);
    handlePacket(buf, t);
  }
  hd::PacketRun done;
  if (!dupFilter && runs.flushIfIdle(micros(), kRunIdleUs, &done)) printRun(done);
}

}  // namespace

namespace {

bool saveRemoteId(const uint8_t id[2]) {
  Preferences prefs;
  prefs.begin(kPrefsNs, false);
  bool ok = prefs.putBytes(kPrefsIdKey, id, 2) == 2;
  prefs.end();
  if (ok) {
    memcpy(remoteId, id, 2);
    haveRemoteId = true;
  }
  return ok;
}

}  // namespace

void begin() {
  loadRemoteId();
  if (!haveRemoteId && newRemoteId()) {
    Serial.printf("No remote ID in flash: generated %02X %02X.\n", remoteId[0], remoteId[1]);
  }
  ensureRadio();
}

bool hasRemoteId() { return haveRemoteId; }

bool setRemoteId(const String& hex) {
  uint8_t tmp[5];
  if (hd::parseHex(hex.c_str(), tmp, sizeof(tmp)) != 2) return false;
  return saveRemoteId(tmp);
}

bool newRemoteId() {
  // esp_random() is only truly random while an entropy source runs (the radio
  // subsystem, which this firmware doesn't start, or this one). Not while the
  // ADC is in use; the serial loop is single-threaded, so nothing else is.
  uint8_t id[2];
  bootloader_random_enable();
  while (!hd::rf::remoteIdFromRandom(esp_random(), id)) {
  }
  bootloader_random_disable();
  return saveRemoteId(id);
}

void printRemoteId() {
  if (haveRemoteId) Serial.printf("Remote ID: %02X %02X (saved in flash)\n", remoteId[0], remoteId[1]);
  else Serial.println("Remote ID: not set (flash write failed?). Try: id new");
}

namespace {

// Sets up radio A to transmit wake frames at the Platinum settings. Fills `p`.
bool prepareWake(uint8_t* p) {
  if (!haveRemoteId) {
    Serial.println("No RF wake: remote ID not set (id <hex>).");
    return false;
  }
  if (!ensureRadio()) return false;
  // Always the Platinum settings, whatever the sniffer is set to.
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.disableDynamicPayloads();
  radio.setDataRate(RF24_2MBPS);
  radio.setAddressWidth(hd::rf::kAddrWidth);
  radio.setCRCLength(RF24_CRC_16);
  radio.setPayloadSize(hd::rf::kPayloadSize);
  radio.setPALevel(txPa);  // see `pa`
  uint8_t reg[5] = {0};
  hd::onAirToRegister(hd::rf::kAddress, hd::rf::kAddrWidth, reg);
  radio.openWritingPipe(reg);
  hd::rf::wakePayload(remoteId[0], remoteId[1], p);
  return true;
}

// Sends `p` for `ms`, one packet per channel in rotation like the remote. Each
// write() waits for the PLL to settle and the packet to go out (~0.3 ms with
// SPI). Returns the packet count.
uint32_t rotateBurst(const uint8_t* p, uint32_t ms) {
  uint32_t sent = 0, t0 = millis();
  while (millis() - t0 < ms) {
    radio.setChannel(hd::rf::kChannels[sent % 3]);
    radio.write(p, hd::rf::kPayloadSize);
    sent++;
  }
  return sent;
}

}  // namespace

bool sendWake(uint32_t ms) {
  uint8_t p[hd::rf::kPayloadSize];
  if (!prepareWake(p)) return false;
  uint32_t t0 = millis();
  uint32_t sent = rotateBurst(p, ms);
  Serial.printf("RF wake: %lu packets in %lu ms (ch 33/52/71).\n", (unsigned long)sent,
                (unsigned long)(millis() - t0));
  return true;
}

bool sendGroup(uint8_t group, bool up, uint32_t holdMs, uint32_t tailMs) {
  uint8_t p[hd::rf::kPayloadSize];
  if (!prepareWake(p)) return false;  // radio setup; the payload is replaced below
  hd::rf::groupPayload(remoteId[0], remoteId[1], group, up, true, p);
  uint32_t held = rotateBurst(p, holdMs);
  hd::rf::groupPayload(remoteId[0], remoteId[1], group, up, false, p);
  uint32_t tail = rotateBurst(p, tailMs);
  Serial.printf("RF group %s %s: %lu x %02X %02X %02X %02X %02X (%lu ms), then %lu x tail %02X (%lu ms)\n",
                hd::rf::groupName(group).c_str(), up ? "UP" : "DOWN", (unsigned long)held,
                remoteId[0], remoteId[1], hd::rf::kModeGroup, group,
                up ? hd::rf::kActionUpHeld : hd::rf::kActionDownHeld, (unsigned long)holdMs,
                (unsigned long)tail, up ? hd::rf::kActionUpRelease : hd::rf::kActionDownRelease,
                (unsigned long)tailMs);
  return true;
}

void printPa() {
  Serial.printf("TX power: %s (%d dBm on a genuine nRF24L01+; clones such as the Si24R1 "
                "map it differently)\n",
                hd::rf::paLevelName(txPa), hd::rf::paLevelGenuineDbm(txPa));
}

bool setPa(const String& name) {
  int level = hd::rf::parsePaLevel(name.c_str());
  if (level < 0) return false;
  txPa = static_cast<rf24_pa_dbm_e>(level);
  return true;
}

bool sendPosition(uint8_t mask, uint8_t pos, uint32_t phase1Ms, uint32_t phase2Ms) {
  uint8_t p[hd::rf::kPayloadSize];
  if (!prepareWake(p)) return false;  // radio setup; the payload is replaced below
  hd::rf::positionPayload(remoteId[0], remoteId[1], mask, pos, true, p);
  uint32_t n1 = rotateBurst(p, phase1Ms);
  hd::rf::positionPayload(remoteId[0], remoteId[1], mask, pos, false, p);
  uint32_t n2 = rotateBurst(p, phase2Ms);
  Serial.printf("RF pos mask %02X -> %u: %lu x %02X %02X %02X %02X %02X (%lu ms), "
                "then %lu x %02X %02X %02X %02X %02X (%lu ms)\n",
                mask, (unsigned)pos, (unsigned long)n1, remoteId[0], remoteId[1],
                hd::rf::kModePosPhase1, mask, pos, (unsigned long)phase1Ms, (unsigned long)n2,
                remoteId[0], remoteId[1], hd::rf::kModePosPhase2, mask, pos,
                (unsigned long)phase2Ms);
  return true;
}

namespace {

// Sends `p` on one channel for `ms`, at least one packet. Returns the count.
long burstOn(const uint8_t* p, uint8_t channel, uint32_t ms) {
  radio.setChannel(channel);
  long sent = 0;
  uint32_t t0 = millis();
  do {  // at least one packet, even for ms = 0
    radio.write(p, hd::rf::kPayloadSize);
    sent++;
  } while (millis() - t0 < ms);
  return sent;
}

}  // namespace

long sendWakeOn(uint8_t channel, uint32_t ms) {
  uint8_t p[hd::rf::kPayloadSize];
  if (!prepareWake(p)) return -1;
  return burstOn(p, channel, ms);
}

long sendSetupOn(uint8_t group, bool add, uint8_t channel, uint32_t ms) {
  uint8_t p[hd::rf::kPayloadSize];
  if (!prepareWake(p)) return -1;  // radio setup; the payload is replaced below
  hd::rf::setupPayload(remoteId[0], remoteId[1], group, add, true, p);
  return burstOn(p, channel, ms);
}

void selfTest(int label) {
  Serial.printf("--- nRF24 self-test, module #%d ---\n", label);
  radioOk = false;
  bool began = radio.begin();
  bool connected = began && radio.isChipConnected();
  Serial.printf("begin(): %s  isChipConnected(): %s  isPVariant(): %s\n",
                began ? "ok" : "FAIL", connected ? "ok" : "FAIL",
                connected && radio.isPVariant() ? "yes" : "no");
  // RF24 rate enum: 0 = 1 Mbps, 1 = 2 Mbps, 2 = 250 kbps.
  int regErrors = 0, rateErrors[3] = {0, 0, 0}, rateChecks[3] = {0, 0, 0};
  const int kIters = 201;  // a multiple of 3, so each rate gets the same number of checks
  if (connected) {
    for (int i = 0; i < kIters; i++) {
      uint8_t ch = random(0, 126);
      radio.setChannel(ch);
      if (radio.getChannel() != ch) regErrors++;
      rf24_pa_dbm_e pa = static_cast<rf24_pa_dbm_e>(i % 4);
      radio.setPALevel(pa);
      if (radio.getPALevel() != pa) regErrors++;
      int r = i % 3;
      rateChecks[r]++;
      radio.setDataRate(static_cast<rf24_datarate_e>(r));
      if (radio.getDataRate() != static_cast<rf24_datarate_e>(r)) rateErrors[r]++;
    }
  }
  Serial.printf("channel/PA read-back: %d errors / %d checks\n", regErrors, kIters * 2);
  for (int r = 0; r < 3; r++) {
    Serial.printf("rate %-4s: %s (%d errors / %d)%s\n", rateName(static_cast<rf24_datarate_e>(r)),
                  rateErrors[r] ? "FAIL" : "ok", rateErrors[r], rateChecks[r],
                  r == RF24_2MBPS ? "  <- used by the Platinum remote" : "");
  }
  // 2 Mbps is what the Platinum remote uses; 250 kbps isn't needed.
  bool pass = connected && regErrors == 0 && rateErrors[RF24_2MBPS] == 0;
  Serial.printf("RESULT module #%d: %s\n", label, pass ? "PASS" : "FAIL");
  radioOk = pass;
}

void printDetails() {
  if (!ensureRadio()) return;
  radio.printPrettyDetails();
}

void startScan() {
  if (!ensureRadio()) return;
  applyRx();  // any address; RPD is carrier detect only
  memset(hits, 0, sizeof(hits));
  sweeps = 0;
  mode = Mode::Scan;
  Serial.printf("RPD scan (>-64 dBm), %d sweeps/row, rate %s. Send any line to stop.\n",
                kSweepsPerRow, rateName(cfg.rate));
  Serial.println("0         1         2         3         4         5         6         7         8         9         10        11        12");
  Serial.println("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345");
}

void startSniff() {
  if (!ensureRadio()) return;
  applyRx();
  filter.reset();
  runs.reset();
  rxTotal = 0;
  mode = Mode::Sniff;
  printConfig();
  Serial.println(dupFilter ? "Sniffing (dup filter on). Send any line to stop."
                           : "Sniffing. One line per run of identical packets. Send any line to stop.");
}

void stop() {
  if (mode == Mode::Idle) return;
  if (mode == Mode::Sniff) {
    hd::PacketRun done;
    if (!dupFilter && runs.flush(&done)) printRun(done);
    Serial.printf("Sniff stopped. %lu packets.\n", (unsigned long)rxTotal);
  }
  if (mode == Mode::Scan) Serial.println("Scan stopped.");
  radio.stopListening();
  mode = Mode::Idle;
}

bool busy() { return mode != Mode::Idle; }

void poll() {
  if (mode == Mode::Scan) pollScan();
  else if (mode == Mode::Sniff) pollSniff();
}

void setChannel(uint8_t ch) { cfg.channel = min<uint8_t>(ch, 125); }

bool setRate(const String& s) {
  hd::Rate r = hd::parseRate(s.c_str());
  if (r == hd::Rate::kInvalid) return false;
  cfg.rate = static_cast<rf24_datarate_e>(r);
  return true;
}

bool setAddressWidth(uint8_t w) {
  if (w < 2 || w > 5) return false;
  cfg.addrWidth = w;
  return true;
}

bool setAddress(const String& hex) {
  uint8_t tmp[5];
  int n = hd::parseHex(hex.c_str(), tmp, sizeof(tmp));
  if (n < 2) return false;  // nRF24 minimum address width is 2
  memcpy(cfg.addrOnAir, tmp, n);
  cfg.addrWidth = n;
  return true;
}

bool setCrc(uint8_t b) {
  if (b > 2) return false;
  cfg.crcBytes = b;
  return true;
}

bool setPayloadSize(uint8_t n) {
  if (n < 1 || n > 32) return false;
  cfg.payloadSize = n;
  return true;
}

void setDupFilter(bool on) { dupFilter = on; }

void presetHd() {
  cfg.channel = hd::rf::kChannels[0];
  cfg.rate = RF24_2MBPS;
  cfg.addrWidth = hd::rf::kAddrWidth;
  memcpy(cfg.addrOnAir, hd::rf::kAddress, hd::rf::kAddrWidth);
  cfg.crcBytes = hd::rf::kCrcBytes;
  cfg.payloadSize = hd::rf::kPayloadSize;
  dupFilter = false;
}

void presetPromiscuous(uint8_t preamble) {
  cfg.addrWidth = 2;
  cfg.addrOnAir[0] = 0x00;
  cfg.addrOnAir[1] = preamble;
  cfg.crcBytes = 0;
  cfg.payloadSize = 32;
  dupFilter = true;
}

void printConfig() {
  Serial.printf("ch %u (%u MHz)  rate %s  aw %u  addr(on-air) ", cfg.channel,
                2400 + cfg.channel, rateName(cfg.rate), cfg.addrWidth);
  printHex(cfg.addrOnAir, cfg.addrWidth);
  Serial.printf(" crc %u  pl %u  filter %s\n", cfg.crcBytes, cfg.payloadSize,
                dupFilter ? "on" : "off (runs)");
}

}  // namespace rf
