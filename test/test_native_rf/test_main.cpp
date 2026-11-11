// Tier 1 (host): RF helper logic — address byte order, sniffer dup filter,
// Platinum payload decoding, packet runs.
#include <unity.h>

#include <cstring>

#include "dup_filter.h"
#include "hd_rf.h"
#include "nrf_addr.h"
#include "run_tracker.h"

using namespace hd;

void setUp() {}
void tearDown() {}

void test_on_air_to_register_reverses() {
  const uint8_t onAir[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  uint8_t reg[5] = {0};
  onAirToRegister(onAir, 5, reg);
  const uint8_t expect[5] = {0x55, 0x44, 0x33, 0x22, 0x11};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, reg, 5);
}

void test_on_air_to_register_promiscuous_preset() {
  // Promiscuous trick: on-air 00 AA == RF24 uint64 address 0x00AA (reg bytes AA 00).
  const uint8_t onAir[2] = {0x00, 0xAA};
  uint8_t reg[2];
  onAirToRegister(onAir, 2, reg);
  TEST_ASSERT_EQUAL_HEX8(0xAA, reg[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, reg[1]);
}

void test_dup_filter_single_packet_not_reported() {
  DupFilter f;
  const uint8_t p[5] = {3, 193, 32, 1, 161};
  TEST_ASSERT_EQUAL(0, f.feed(p, 5));
}

void test_dup_filter_reports_2nd_then_every_10th() {
  DupFilter f;
  const uint8_t p[5] = {3, 193, 32, 1, 161};
  int reported = 0;
  uint16_t last = 0;
  for (int i = 1; i <= 30; i++) {
    uint16_t c = f.feed(p, 5);
    if (c) {
      reported++;
      last = c;
    }
    if (i == 2) TEST_ASSERT_EQUAL(2, c);
    if (i == 3) TEST_ASSERT_EQUAL(0, c);
    if (i == 10 || i == 20 || i == 30) TEST_ASSERT_EQUAL(i, c);
  }
  TEST_ASSERT_EQUAL(4, reported);  // 2, 10, 20, 30
  TEST_ASSERT_EQUAL(30, last);
}

void test_dup_filter_distinguishes_payloads() {
  DupFilter f;
  const uint8_t up[5] = {3, 193, 32, 1, 161};
  const uint8_t rel[5] = {3, 193, 32, 1, 33};
  f.feed(up, 5);
  TEST_ASSERT_EQUAL(0, f.feed(rel, 5));  // first sighting of release
  TEST_ASSERT_EQUAL(2, f.feed(up, 5));
  TEST_ASSERT_EQUAL(2, f.feed(rel, 5));
}

void test_dup_filter_compares_only_prefix() {
  DupFilter f(4);
  uint8_t a[32], b[32];
  memset(a, 0x5A, sizeof(a));
  memcpy(b, a, sizeof(b));
  b[20] = 0x00;  // differs beyond compare length (noise tail)
  f.feed(a, 32);
  TEST_ASSERT_EQUAL(2, f.feed(b, 32));
}

void test_dup_filter_evicts_oldest() {
  DupFilter f;
  uint8_t p[5] = {0};
  for (int i = 0; i < DupFilter::kSlots + 1; i++) {
    p[0] = static_cast<uint8_t>(i);
    f.feed(p, 5);
  }
  p[0] = 0;  // slot for packet 0 was overwritten
  TEST_ASSERT_EQUAL(0, f.feed(p, 5));
  p[0] = DupFilter::kSlots;  // most recent still present
  TEST_ASSERT_EQUAL(2, f.feed(p, 5));
}

void test_dup_filter_reset() {
  DupFilter f;
  const uint8_t p[3] = {1, 2, 3};
  f.feed(p, 3);
  f.reset();
  TEST_ASSERT_EQUAL(0, f.feed(p, 3));
}

// --- Platinum payload decoding ---

void test_hd_address_same_in_either_byte_order() {
  uint8_t reg[3];
  onAirToRegister(rf::kAddress, rf::kAddrWidth, reg);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(rf::kAddress, reg, 3);
}

void test_describe_forum_example() {
  // Arduino forum example (a different remote's ID): group 1, UP held.
  const uint8_t p[5] = {0x03, 0xC1, 0x20, 0x01, 0xA1};
  TEST_ASSERT_EQUAL_STRING("ID 03C1  mode 20  group 1  UP held", rf::describePayload(p, 5).c_str());
}

void test_describe_wake_frames() {
  const uint8_t held[5] = {0x12, 0x34, 0x01, 0x00, 0x80};
  TEST_ASSERT_EQUAL_STRING("ID 1234  mode 01  group none  wake held",
                           rf::describePayload(held, 5).c_str());
  const uint8_t tail[5] = {0x12, 0x34, 0x01, 0x00, 0x00};
  TEST_ASSERT_EQUAL_STRING("ID 1234  mode 01  group none  wake release",
                           rf::describePayload(tail, 5).c_str());
}

void test_wake_payload_layout() {
  uint8_t p[5];
  rf::wakePayload(0x12, 0x34, p);
  const uint8_t expect[5] = {0x12, 0x34, 0x01, 0x00, 0x80};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, p, 5);
}

void test_group_names() {
  TEST_ASSERT_EQUAL_STRING("1", rf::groupName(0x01).c_str());
  TEST_ASSERT_EQUAL_STRING("3", rf::groupName(0x04).c_str());
  TEST_ASSERT_EQUAL_STRING("4", rf::groupName(0x08).c_str());
  TEST_ASSERT_EQUAL_STRING("All", rf::groupName(0xFF).c_str());
  TEST_ASSERT_EQUAL_STRING("mask 0x03", rf::groupName(0x03).c_str());
}

void test_action_names() {
  TEST_ASSERT_EQUAL_STRING("DOWN held", rf::actionName(0xA2));
  TEST_ASSERT_EQUAL_STRING("UP release", rf::actionName(0x21));
  TEST_ASSERT_EQUAL_STRING("DOWN release", rf::actionName(0x22));
  TEST_ASSERT_EQUAL_STRING("?", rf::actionName(0x42));
}

void test_group_payload_matches_sniff() {
  // Group 1 + UP as sniffed (ID made up): A1 while held, then the 21 tail.
  uint8_t p[5];
  rf::groupPayload(0x12, 0x34, 0x01, true, true, p);
  const uint8_t held[5] = {0x12, 0x34, 0x20, 0x01, 0xA1};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(held, p, 5);
  rf::groupPayload(0x12, 0x34, 0x01, true, false, p);
  const uint8_t tail[5] = {0x12, 0x34, 0x20, 0x01, 0x21};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(tail, p, 5);
}

void test_group_payload_down_and_all() {
  uint8_t p[5];
  rf::groupPayload(0x12, 0x34, rf::kGroupAll, false, true, p);
  TEST_ASSERT_EQUAL_STRING("ID 1234  mode 20  group All  DOWN held", rf::describePayload(p, 5).c_str());
  rf::groupPayload(0x12, 0x34, 0x08, false, false, p);
  TEST_ASSERT_EQUAL_STRING("ID 1234  mode 20  group 4  DOWN release", rf::describePayload(p, 5).c_str());
}

void test_parse_group() {
  TEST_ASSERT_EQUAL(0x01, rf::parseGroup("1"));
  TEST_ASSERT_EQUAL(0x02, rf::parseGroup("2"));
  TEST_ASSERT_EQUAL(0x04, rf::parseGroup("3"));
  TEST_ASSERT_EQUAL(0x08, rf::parseGroup("4"));
  TEST_ASSERT_EQUAL(0xFF, rf::parseGroup("all"));
  TEST_ASSERT_EQUAL(0xFF, rf::parseGroup("ALL"));
  TEST_ASSERT_EQUAL(-1, rf::parseGroup("0"));
  TEST_ASSERT_EQUAL(-1, rf::parseGroup("5"));
  TEST_ASSERT_EQUAL(-1, rf::parseGroup(""));
  TEST_ASSERT_EQUAL(-1, rf::parseGroup("al"));
}

void test_describe_needs_five_bytes() {
  const uint8_t p[6] = {0};
  TEST_ASSERT_EQUAL_STRING("", rf::describePayload(p, 4).c_str());
  TEST_ASSERT_EQUAL_STRING("", rf::describePayload(p, 6).c_str());
}

void test_beacon_is_not_a_known_command() {
  TEST_ASSERT_EQUAL_STRING("?", rf::actionName(rf::kBeaconPayload[4]));
  TEST_ASSERT_EQUAL_STRING("none", rf::groupName(rf::kBeaconPayload[3]).c_str());
  TEST_ASSERT_NOT_EQUAL(rf::kModeGroup, rf::kBeaconPayload[2]);
  TEST_ASSERT_NOT_EQUAL(rf::kModeNoGroup, rf::kBeaconPayload[2]);
}

// --- packet runs ---

void test_runs_count_identical_packets() {
  RunTracker t;
  PacketRun r;
  const uint8_t p[5] = {3, 193, 32, 1, 161};
  TEST_ASSERT_FALSE(t.feed(p, 5, 1000, &r));
  TEST_ASSERT_FALSE(t.feed(p, 5, 1580, &r));
  TEST_ASSERT_FALSE(t.feed(p, 5, 2200, &r));
  TEST_ASSERT_TRUE(t.flush(&r));
  TEST_ASSERT_EQUAL(3, r.count);
  TEST_ASSERT_EQUAL(1000, r.firstUs);
  TEST_ASSERT_EQUAL(2200, r.lastUs);
  TEST_ASSERT_EQUAL(580, r.minGapUs);
  TEST_ASSERT_EQUAL(620, r.maxGapUs);
  TEST_ASSERT_FALSE(t.flush(&r));  // nothing left
}

void test_runs_split_on_new_payload() {
  RunTracker t;
  PacketRun r;
  const uint8_t held[5] = {3, 193, 32, 1, 0xA1};
  const uint8_t rel[5] = {3, 193, 32, 1, 0x21};
  t.feed(held, 5, 0, &r);
  t.feed(held, 5, 600, &r);
  TEST_ASSERT_TRUE(t.feed(rel, 5, 1200, &r));
  TEST_ASSERT_EQUAL(2, r.count);
  TEST_ASSERT_EQUAL_HEX8(0xA1, r.payload[4]);
  TEST_ASSERT_TRUE(t.flush(&r));
  TEST_ASSERT_EQUAL(1, r.count);
  TEST_ASSERT_EQUAL_HEX8(0x21, r.payload[4]);
  TEST_ASSERT_EQUAL(0, r.minGapUs);
}

void test_runs_flush_when_idle() {
  RunTracker t;
  PacketRun r;
  const uint8_t p[3] = {1, 2, 3};
  t.feed(p, 3, 100, &r);
  TEST_ASSERT_FALSE(t.flushIfIdle(20000, 30000, &r));
  TEST_ASSERT_TRUE(t.flushIfIdle(30100, 30000, &r));
  TEST_ASSERT_EQUAL(1, r.count);
}

void test_runs_survive_micros_wrap() {
  RunTracker t;
  PacketRun r;
  const uint8_t p[1] = {7};
  t.feed(p, 1, 0xFFFFFF00u, &r);
  t.feed(p, 1, 0x00000100u, &r);  // 512 us later, after the wrap
  TEST_ASSERT_TRUE(t.flush(&r));
  TEST_ASSERT_EQUAL(512, r.maxGapUs);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_hd_address_same_in_either_byte_order);
  RUN_TEST(test_describe_forum_example);
  RUN_TEST(test_describe_wake_frames);
  RUN_TEST(test_wake_payload_layout);
  RUN_TEST(test_group_payload_matches_sniff);
  RUN_TEST(test_group_payload_down_and_all);
  RUN_TEST(test_parse_group);
  RUN_TEST(test_group_names);
  RUN_TEST(test_action_names);
  RUN_TEST(test_describe_needs_five_bytes);
  RUN_TEST(test_beacon_is_not_a_known_command);
  RUN_TEST(test_runs_count_identical_packets);
  RUN_TEST(test_runs_split_on_new_payload);
  RUN_TEST(test_runs_flush_when_idle);
  RUN_TEST(test_runs_survive_micros_wrap);
  RUN_TEST(test_on_air_to_register_reverses);
  RUN_TEST(test_on_air_to_register_promiscuous_preset);
  RUN_TEST(test_dup_filter_single_packet_not_reported);
  RUN_TEST(test_dup_filter_reports_2nd_then_every_10th);
  RUN_TEST(test_dup_filter_distinguishes_payloads);
  RUN_TEST(test_dup_filter_compares_only_prefix);
  RUN_TEST(test_dup_filter_evicts_oldest);
  RUN_TEST(test_dup_filter_reset);
  return UNITY_END();
}
