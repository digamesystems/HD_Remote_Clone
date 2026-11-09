// Tier 1 (host): HD Platinum IR frame encode/decode.
// Real-capture fixtures are copied from captures/*.log (Nov 7, 2026).
#include <unity.h>

#include "hd_ir.h"

using namespace hd::ir;

void setUp() {}
void tearDown() {}

// captures/ir_red_open_close_20261107_132807.log, capture #1 (UP), first 40 entries
static const uint16_t kCapUp[] = {2738, 2754, 1022, 1074, 994,  2752, 1018, 1076, 1020, 2754,
                                  2724, 2754, 1020, 1048, 1020, 2780, 996,  1074, 1018, 2754,
                                  2722, 2752, 1026, 1048, 1018, 2778, 996,  1076, 1018, 2754,
                                  2724, 2752, 1018, 1050, 1018, 2758, 1018, 1102, 990,  2754};
// same log, capture #2 (DOWN), first 40 entries
static const uint16_t kCapDown[] = {2722, 2754, 1022, 1072, 1022, 2754, 1016, 2754, 1022, 1050,
                                    2724, 2754, 1020, 1070, 1020, 2756, 1020, 2756, 1016, 1048,
                                    2726, 2756, 1022, 1072, 1020, 2756, 1016, 2758, 1018, 1052,
                                    2718, 2780, 994,  1076, 1020, 2730, 1044, 2754, 1020, 1048};
// captures/ir_red_hold_open_then_close_20261107_133330.log, capture #7: capture resumed
// mid-burst so it starts on a header *space* (mark/space phase shifted by one).
static const uint16_t kCapShifted[] = {2770, 996,  1072, 1022, 2752, 1018, 1076, 996,  2778, 2700,
                                       2778, 996,  1072, 1022, 2758, 1018, 1074, 992,  2778, 2702,
                                       2778, 996,  1072, 1020, 2756, 1018, 1076, 994,  2778, 2700,
                                       2780, 994,  1072, 1022, 2752, 1022, 1074, 996,  2778, 2700};

#define N(a) (sizeof(a) / sizeof((a)[0]))

void test_code_to_bits() {
  TEST_ASSERT_EQUAL_STRING("0101", codeToBits(kCodeUp).c_str());
  TEST_ASSERT_EQUAL_STRING("0110", codeToBits(kCodeDown).c_str());
}

void test_code_names() {
  TEST_ASSERT_EQUAL_STRING("A UP", codeName(0b0101));
  TEST_ASSERT_EQUAL_STRING("A DOWN", codeName(0b0110));
  TEST_ASSERT_EQUAL_STRING("B UP", codeName(0b1001));
  TEST_ASSERT_EQUAL_STRING("B DOWN", codeName(0b1010));
  TEST_ASSERT_EQUAL_STRING("?", codeName(0b0000));
}

void test_max_send_limits() {
  TEST_ASSERT_EQUAL_UINT32(5000, maxSendMs(kCodeUp));
  TEST_ASSERT_EQUAL_UINT32(5000, maxSendMs(kCodeUpB));
  TEST_ASSERT_EQUAL_UINT32(10000, maxSendMs(kCodeDown));
  TEST_ASSERT_EQUAL_UINT32(10000, maxSendMs(kCodeDownB));
  // The longest allowed OPEN must stay clear of the manual's 6 s reset hold.
  uint32_t longestUs = framesForDuration(kMaxOpenSendMs) * kFramePeriodUs;
  TEST_ASSERT_TRUE(longestUs < 6000000UL);
}

void test_parse_code() {
  TEST_ASSERT_EQUAL(kCodeUp, parseCode("up"));
  TEST_ASSERT_EQUAL(kCodeDown, parseCode("DOWN"));
  TEST_ASSERT_EQUAL(0b1001, parseCode("1001"));
  TEST_ASSERT_EQUAL(0x6, parseCode("0x6"));
  TEST_ASSERT_EQUAL(-1, parseCode("0x10"));
  TEST_ASSERT_EQUAL(-1, parseCode("012"));
  TEST_ASSERT_EQUAL(-1, parseCode("sideways"));
}

void test_encode_frame_layout() {
  auto f = encodeFrame(kCodeUp);  // 0101
  TEST_ASSERT_EQUAL(kEntriesPerFrame, f.size());
  const uint16_t expect[] = {kHdrMarkUs, kHdrSpaceUs, kBitMarkUs, kZeroSpaceUs, kBitMarkUs,
                             kOneSpaceUs, kBitMarkUs, kZeroSpaceUs, kBitMarkUs, kOneSpaceUs};
  TEST_ASSERT_EQUAL_UINT16_ARRAY(expect, f.data(), kEntriesPerFrame);
}

void test_encoded_frame_period_matches_measurement() {
  // Measured period 17.19 ms for codes with two 1-bits; allow 1%.
  uint32_t sum = 0;
  for (uint16_t v : encodeFrame(kCodeDown)) sum += v;
  TEST_ASSERT_UINT32_WITHIN(172, kFramePeriodUs, sum);
}

void test_encode_burst_and_frames_for_duration() {
  TEST_ASSERT_EQUAL(5 * kEntriesPerFrame, encodeBurst(kCodeDown, 5).size());
  TEST_ASSERT_EQUAL(kEntriesPerFrame, encodeBurst(kCodeDown, 0).size());
  TEST_ASSERT_EQUAL(1, framesForDuration(0));
  TEST_ASSERT_EQUAL(59, framesForDuration(1000));  // ceil(1000 / 17.19)
}

void test_round_trip_all_codes() {
  for (int c = 0; c < 16; c++) {
    auto burst = encodeBurst(c, 7);
    auto codes = decodeFrames(burst.data(), burst.size());
    TEST_ASSERT_EQUAL(7, codes.size());
    for (auto got : codes) TEST_ASSERT_EQUAL(c, got);
  }
}

void test_decode_real_up_capture() {
  auto s = summarize(kCapUp, N(kCapUp));
  TEST_ASSERT_EQUAL(kCodeUp, s.code);
  TEST_ASSERT_EQUAL(4, s.frames);
  TEST_ASSERT_EQUAL(0, s.otherFrames);
}

void test_decode_real_down_capture() {
  auto s = summarize(kCapDown, N(kCapDown));
  TEST_ASSERT_EQUAL(kCodeDown, s.code);
  TEST_ASSERT_EQUAL(4, s.frames);
}

void test_decode_phase_shifted_capture() {
  // Frames start at index 9, 19, 29; the trailing 2700 is a truncated header.
  auto s = summarize(kCapShifted, N(kCapShifted));
  TEST_ASSERT_EQUAL(kCodeUp, s.code);
  TEST_ASSERT_EQUAL(3, s.frames);
  TEST_ASSERT_EQUAL(0, s.otherFrames);
}

void test_decode_ignores_truncated_last_frame() {
  // Release cuts the last frame short (seen in analyzer captures).
  auto burst = encodeBurst(kCodeUp, 3);
  burst.resize(burst.size() - 5);
  TEST_ASSERT_EQUAL(2, decodeFrames(burst.data(), burst.size()).size());
}

void test_decode_skips_garbled_frame() {
  auto burst = encodeBurst(kCodeDown, 4);
  burst[12] = 2750;  // corrupt a bit mark in frame 2
  auto s = summarize(burst.data(), burst.size());
  TEST_ASSERT_EQUAL(kCodeDown, s.code);
  TEST_ASSERT_EQUAL(3, s.frames);
}

void test_summary_majority_and_others() {
  auto a = encodeBurst(kCodeUp, 5);
  auto b = encodeBurst(kCodeDown, 2);
  a.insert(a.end(), b.begin(), b.end());
  auto s = summarize(a.data(), a.size());
  TEST_ASSERT_EQUAL(kCodeUp, s.code);
  TEST_ASSERT_EQUAL(5, s.frames);
  TEST_ASSERT_EQUAL(2, s.otherFrames);
}

void test_summary_empty() {
  const uint16_t junk[] = {500, 500, 500};
  auto s = summarize(junk, 3);
  TEST_ASSERT_EQUAL(-1, s.code);
  TEST_ASSERT_EQUAL(0, s.frames);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_code_to_bits);
  RUN_TEST(test_parse_code);
  RUN_TEST(test_code_names);
  RUN_TEST(test_max_send_limits);
  RUN_TEST(test_encode_frame_layout);
  RUN_TEST(test_encoded_frame_period_matches_measurement);
  RUN_TEST(test_encode_burst_and_frames_for_duration);
  RUN_TEST(test_round_trip_all_codes);
  RUN_TEST(test_decode_real_up_capture);
  RUN_TEST(test_decode_real_down_capture);
  RUN_TEST(test_decode_phase_shifted_capture);
  RUN_TEST(test_decode_ignores_truncated_last_frame);
  RUN_TEST(test_decode_skips_garbled_frame);
  RUN_TEST(test_summary_majority_and_others);
  RUN_TEST(test_summary_empty);
  return UNITY_END();
}
