// Tier 1 (host): RF helper logic — address byte order, sniffer dup filter.
#include <unity.h>

#include <cstring>

#include "dup_filter.h"
#include "nrf_addr.h"

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

int main() {
  UNITY_BEGIN();
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
