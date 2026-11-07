#include "rf_tools.h"

#include <RF24.h>
#include <SPI.h>

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
// Noise matches the promiscuous address constantly. The remote repeats each
// packet ~30x, so by default only payloads seen at least twice are printed.
constexpr uint8_t kCmpLen = 10;
constexpr int kRecent = 32;
struct Recent {
  uint8_t data[32];
  uint16_t count;
  uint32_t firstMs;
} recent[kRecent];
int recentNext = 0;
uint32_t rxTotal = 0;
bool dupFilter = true;

const char* rateName(rf24_datarate_e r) {
  switch (r) {
    case RF24_250KBPS: return "250k";
    case RF24_1MBPS: return "1m";
    default: return "2m";
  }
}

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
  for (uint8_t i = 0; i < cfg.addrWidth; i++) reg[i] = cfg.addrOnAir[cfg.addrWidth - 1 - i];
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
  uint8_t cmp = min(kCmpLen, cfg.payloadSize);
  for (auto& r : recent) {
    if (r.count && memcmp(r.data, buf, cmp) == 0) {
      r.count++;
      if (r.count == 2 || r.count % 10 == 0) {
        Serial.printf("%8lu ch%u x%-3u ", (unsigned long)now, cfg.channel, r.count);
        printHex(buf, cfg.payloadSize);
        Serial.println();
      }
      return;
    }
  }
  Recent& r = recent[recentNext];
  recentNext = (recentNext + 1) % kRecent;
  memcpy(r.data, buf, cfg.payloadSize);
  r.count = 1;
  r.firstMs = now;
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
  memset(recent, 0, sizeof(recent));
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
  if (s == "250k") cfg.rate = RF24_250KBPS;
  else if (s == "1m") cfg.rate = RF24_1MBPS;
  else if (s == "2m") cfg.rate = RF24_2MBPS;
  else return false;
  return true;
}

bool setAddressWidth(uint8_t w) {
  if (w < 2 || w > 5) return false;
  cfg.addrWidth = w;
  return true;
}

bool setAddress(const String& hex) {
  if (hex.length() == 0 || hex.length() % 2 || hex.length() > 10) return false;
  uint8_t n = hex.length() / 2;
  for (uint8_t i = 0; i < n; i++) {
    char tmp[3] = {hex[2 * i], hex[2 * i + 1], 0};
    char* end;
    long v = strtol(tmp, &end, 16);
    if (*end) return false;
    cfg.addrOnAir[i] = v;
  }
  cfg.addrWidth = max<uint8_t>(n, 2);
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
