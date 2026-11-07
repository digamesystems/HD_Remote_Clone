#include "rf_tools.h"

#include <RF24.h>
#include <SPI.h>

#include "dup_filter.h"
#include "nrf_addr.h"
#include "parse.h"
#include "pins.h"

namespace rf {
namespace {

RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
bool radioOk = false;

struct Config {
  uint8_t channel = 7;  // forum-reported; verify with `scan`
  rf24_datarate_e rate = RF24_1MBPS;
  uint8_t addrWidth = 2;
  uint8_t addrOnAir[5] = {0x00, 0xAA};  // as transmitted, first byte first
  uint8_t crcBytes = 0;
  uint8_t payloadSize = 32;
} cfg;

enum class Mode { Idle, Scan, Sniff } mode = Mode::Idle;

// --- scan state ---
constexpr uint8_t kNumChannels = 126;
constexpr int kSweepsPerRow = 20;
uint8_t hits[kNumChannels];
int sweeps = 0;

// --- sniff state ---
// Noise matches the promiscuous address constantly; see hd::DupFilter.
hd::DupFilter filter;
uint32_t rxTotal = 0;
bool dupFilter = true;

const char* rateName(rf24_datarate_e r) { return hd::rateName(static_cast<hd::Rate>(r)); }

bool ensureRadio() {
  if (radioOk) return true;
  radioOk = radio.begin() && radio.isChipConnected();
  if (!radioOk) Serial.println("nRF24 not responding (check wiring/3.3V/10uF cap).");
  return radioOk;
}

void applyRx() {
  radio.stopListening();
  radio.setAutoAck(false);
  radio.disableDynamicPayloads();
  radio.setDataRate(cfg.rate);
  radio.setChannel(cfg.channel);
  radio.setAddressWidth(cfg.addrWidth);
  if (cfg.crcBytes == 0) {
    radio.disableCRC();
  } else {
    radio.setCRCLength(cfg.crcBytes == 1 ? RF24_CRC_8 : RF24_CRC_16);
  }
  radio.setPayloadSize(cfg.payloadSize);
  // RF24 takes the address in register order (LSByte first); the radio sends
  // the MSByte first, so reverse the on-air bytes.
  uint8_t reg[5] = {0};
  hd::onAirToRegister(cfg.addrOnAir, cfg.addrWidth, reg);
  radio.openReadingPipe(0, reg);
  radio.startListening();
}

void printHex(const uint8_t* p, uint8_t n) {
  for (uint8_t i = 0; i < n; i++) Serial.printf("%02X ", p[i]);
}

void handlePacket(const uint8_t* buf) {
  rxTotal++;
  uint32_t now = millis();
  if (!dupFilter) {
    Serial.printf("%8lu ch%u  ", (unsigned long)now, cfg.channel);
    printHex(buf, cfg.payloadSize);
    Serial.println();
    return;
  }
  uint16_t count = filter.feed(buf, cfg.payloadSize);
  if (count) {
    Serial.printf("%8lu ch%u x%-3u ", (unsigned long)now, cfg.channel, count);
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
    radio.read(buf, cfg.payloadSize);
    handlePacket(buf);
  }
}

}  // namespace

void begin() { ensureRadio(); }

void selfTest(int label) {
  Serial.printf("--- nRF24 self-test, module #%d ---\n", label);
  radioOk = false;
  bool began = radio.begin();
  bool connected = began && radio.isChipConnected();
  Serial.printf("begin(): %s  isChipConnected(): %s  isPVariant(): %s\n",
                began ? "ok" : "FAIL", connected ? "ok" : "FAIL",
                connected && radio.isPVariant() ? "yes" : "no");
  int errors = 0;
  const int kIters = 200;
  if (connected) {
    for (int i = 0; i < kIters; i++) {
      uint8_t ch = random(0, 126);
      radio.setChannel(ch);
      if (radio.getChannel() != ch) errors++;
      rf24_pa_dbm_e pa = static_cast<rf24_pa_dbm_e>(i % 4);
      radio.setPALevel(pa);
      if (radio.getPALevel() != pa) errors++;
      rf24_datarate_e dr = static_cast<rf24_datarate_e>(i % 3);
      radio.setDataRate(dr);
      if (radio.getDataRate() != dr) errors++;
    }
  }
  bool pass = connected && errors == 0;
  Serial.printf("register read-back: %d errors / %d checks\n", errors, kIters * 3);
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
  rxTotal = 0;
  mode = Mode::Sniff;
  printConfig();
  Serial.println("Sniffing. Send any line to stop.");
}

void stop() {
  if (mode == Mode::Idle) return;
  if (mode == Mode::Sniff) Serial.printf("Sniff stopped. %lu raw packets.\n", (unsigned long)rxTotal);
  if (mode == Mode::Scan) Serial.println("Scan stopped.");
  radio.stopListening();
  mode = Mode::Idle;
}

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

void presetPromiscuous(uint8_t preamble) {
  cfg.addrWidth = 2;
  cfg.addrOnAir[0] = 0x00;
  cfg.addrOnAir[1] = preamble;
  cfg.crcBytes = 0;
  cfg.payloadSize = 32;
}

void printConfig() {
  Serial.printf("ch %u (%u MHz)  rate %s  aw %u  addr(on-air) ", cfg.channel,
                2400 + cfg.channel, rateName(cfg.rate), cfg.addrWidth);
  printHex(cfg.addrOnAir, cfg.addrWidth);
  Serial.printf(" crc %u  pl %u  filter %s\n", cfg.crcBytes, cfg.payloadSize,
                dupFilter ? "on" : "off");
}

}  // namespace rf
