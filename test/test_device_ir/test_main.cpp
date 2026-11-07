// Tier 2 (device): IR TX -> RX loopback. Ignored unless HW_IR_LOOPBACK is true.
// Wiring: IR LED driver on GPIO 4 aimed at the IR receiver (GPIO 32), a few cm apart.
#include <Arduino.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include <unity.h>

#include <vector>

#include "hd_ir.h"
#include "hw_config.h"
#include "pins.h"

IRrecv irrecv(PIN_IR_RX, 1024, 50, true);
IRsend irsend(PIN_IR_TX);
decode_results results;

// Synthetic test frame: 3000/1500 header, 16 bits of 0xA5C3 (500 mark,
// 500/1500 space), trailing mark. Odd length, ends on a mark, as sendRaw expects.
std::vector<uint16_t> makeFrame() {
  std::vector<uint16_t> f = {3000, 1500};
  const uint16_t bits = 0xA5C3;
  for (int i = 15; i >= 0; i--) {
    f.push_back(500);
    f.push_back((bits >> i) & 1 ? 1500 : 500);
  }
  f.push_back(500);
  return f;
}

// TSOP output stretches marks / shortens spaces by ~50-150 us.
bool within(uint16_t sent, uint16_t got) {
  uint16_t tol = max<uint16_t>(200, sent / 4);
  return abs(int(sent) - int(got)) <= tol;
}

// Sends the frame and returns the captured timings (empty if nothing received).
std::vector<uint16_t> roundTrip(const std::vector<uint16_t>& frame) {
  irrecv.enableIRIn();
  delay(20);
  irrecv.resume();
  irsend.sendRaw(frame.data(), frame.size(), 40);
  uint32_t t0 = millis();
  std::vector<uint16_t> out;
  while (millis() - t0 < 300) {
    if (irrecv.decode(&results)) {
      uint16_t len = getCorrectedRawLength(&results);
      uint16_t* raw = resultToRawArray(&results);
      out.assign(raw, raw + len);
      delete[] raw;
      break;
    }
    delay(1);
  }
  irrecv.disableIRIn();
  return out;
}

void setUp() {
  if (!HW_IR_LOOPBACK) TEST_IGNORE_MESSAGE("HW_IR_LOOPBACK=false in hw_config.h (not verified)");
}
void tearDown() {}

void test_ir_loopback_timings_match() {
  auto frame = makeFrame();
  auto got = roundTrip(frame);
  TEST_ASSERT_FALSE_MESSAGE(got.empty(), "nothing received: check LED aim/driver/TSOP");
  TEST_ASSERT_EQUAL_MESSAGE(frame.size(), got.size(), "entry count differs");
  for (size_t i = 0; i < frame.size(); i++) {
    char msg[64];
    snprintf(msg, sizeof(msg), "entry %u: sent %u got %u", (unsigned)i, frame[i], got[i]);
    TEST_ASSERT_TRUE_MESSAGE(within(frame[i], got[i]), msg);
  }
}

void test_ir_loopback_reliable() {
  auto frame = makeFrame();
  int ok = 0;
  for (int i = 0; i < 10; i++) {
    auto got = roundTrip(frame);
    bool match = got.size() == frame.size();
    for (size_t j = 0; match && j < frame.size(); j++) match = within(frame[j], got[j]);
    ok += match;
    delay(100);
  }
  TEST_ASSERT_EQUAL_MESSAGE(10, ok, "not all of 10 round trips matched");
}

void test_hd_frames_loopback_decode() {
  // What `send up` transmits must decode back to UP through the receiver.
  for (uint8_t code : {hd::ir::kCodeUp, hd::ir::kCodeDown}) {
    auto burst = hd::ir::encodeBurst(code, 10);
    auto got = roundTrip(burst);
    TEST_ASSERT_FALSE_MESSAGE(got.empty(), "nothing received");
    auto s = hd::ir::summarize(got.data(), got.size());
    TEST_ASSERT_EQUAL_MESSAGE(code, s.code, "decoded code differs");
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(9, s.frames, "too few frames decoded");
    TEST_ASSERT_EQUAL_MESSAGE(0, s.otherFrames, "frames with wrong code");
    delay(100);
  }
}

void setup() {
  delay(2000);
  irsend.begin();
  UNITY_BEGIN();
  RUN_TEST(test_ir_loopback_timings_match);
  RUN_TEST(test_ir_loopback_reliable);
  RUN_TEST(test_hd_frames_loopback_decode);
  UNITY_END();
}

void loop() {}
