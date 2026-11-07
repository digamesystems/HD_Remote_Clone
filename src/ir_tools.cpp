#include "ir_tools.h"

#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>

#include <vector>

#include "pins.h"

namespace ir {
namespace {

// Large buffer: the protocol is proprietary and frames may be long.
constexpr uint16_t kCaptureBufferSize = 1024;
// Gap (ms) that ends a capture. Shorter than the inter-frame gap splits each
// repeat into its own capture (useful for spotting toggle bits / counters).
// Raise toward 100+ to see a whole burst of repeats as one capture.
constexpr uint8_t kTimeoutMs = 50;
constexpr uint16_t kCarrierKhz = 40;  // per Platinum manual

IRrecv irrecv(PIN_IR_RX, kCaptureBufferSize, kTimeoutMs, true);
IRsend irsend(PIN_IR_TX);
decode_results results;

bool capturing = false;
uint32_t captureCount = 0;
uint32_t lastCaptureMs = 0;

std::vector<uint16_t> last;
std::vector<uint16_t> slots[kNumSlots];

const std::vector<uint16_t>* pick(int slot) {
  if (slot < 0) return &last;
  if (slot >= kNumSlots) return nullptr;
  return &slots[slot];
}

void printTimings(const std::vector<uint16_t>& v) {
  Serial.printf("uint16_t rawData[%u] = {", (unsigned)v.size());
  for (size_t i = 0; i < v.size(); i++) {
    if (i % 16 == 0) Serial.print("\n  ");
    Serial.printf("%u%s", v[i], i + 1 < v.size() ? ", " : "");
  }
  Serial.println("\n};");
}

}  // namespace

void begin() {
  irsend.begin();
  irrecv.setUnknownThreshold(6);  // report even short unknown frames
}

void startCapture() {
  irrecv.enableIRIn();
  capturing = true;
  Serial.printf("IR capture ON (timeout %u ms, buffer %u). Send any line to stop.\n",
                kTimeoutMs, kCaptureBufferSize);
}

void stopCapture() {
  if (!capturing) return;
  irrecv.disableIRIn();
  capturing = false;
  Serial.println("IR capture OFF");
}

void pollCapture() {
  if (!capturing || !irrecv.decode(&results)) return;

  uint32_t now = millis();
  uint32_t sincePrev = captureCount ? now - lastCaptureMs : 0;
  lastCaptureMs = now;
  captureCount++;

  uint16_t len = getCorrectedRawLength(&results);
  uint16_t* raw = resultToRawArray(&results);
  last.assign(raw, raw + len);
  delete[] raw;

  Serial.printf("\n=== IR capture #%lu  t=%lu ms  (+%lu ms since prev)  entries=%u%s ===\n",
                (unsigned long)captureCount, (unsigned long)now,
                (unsigned long)sincePrev, len,
                results.overflow ? "  ** OVERFLOW: raise kCaptureBufferSize **" : "");
  Serial.printf("protocol: %s", typeToString(results.decode_type).c_str());
  if (results.decode_type != decode_type_t::UNKNOWN) {
    Serial.printf("  value: %s  bits: %u", resultToHexidecimal(&results).c_str(),
                  results.bits);
  }
  Serial.println();
  printTimings(last);
  irrecv.resume();
}

void play(int slot, int repeats, int gapMs) {
  const auto* v = pick(slot);
  if (!v || v->empty()) {
    Serial.println("Nothing to play (empty slot / no capture yet).");
    return;
  }
  bool wasCapturing = capturing;
  if (wasCapturing) irrecv.disableIRIn();  // don't receive our own echo
  for (int i = 0; i < repeats; i++) {
    irsend.sendRaw(v->data(), v->size(), kCarrierKhz);
    if (i + 1 < repeats) delay(gapMs);
  }
  if (wasCapturing) irrecv.enableIRIn();
  Serial.printf("Sent %s x%d (%u entries, %u kHz, gap %d ms)\n",
                slot < 0 ? "last" : String(slot).c_str(), repeats,
                (unsigned)v->size(), kCarrierKhz, gapMs);
}

bool save(int slot) {
  if (slot < 0 || slot >= kNumSlots || last.empty()) return false;
  slots[slot] = last;
  Serial.printf("Saved last capture (%u entries) to slot %d\n", (unsigned)last.size(), slot);
  return true;
}

void list() {
  Serial.printf("last: %u entries\n", (unsigned)last.size());
  for (int i = 0; i < kNumSlots; i++) {
    Serial.printf("slot %d: %u entries\n", i, (unsigned)slots[i].size());
  }
}

void dump(int slot) {
  const auto* v = pick(slot);
  if (!v || v->empty()) {
    Serial.println("Empty.");
    return;
  }
  printTimings(*v);
}

}  // namespace ir
