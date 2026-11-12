// SPDX-License-Identifier: GPL-2.0-or-later
// Tier 2 (device, board only): confirms we're on the expected ESP32.
#include <Arduino.h>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_chip_is_original_esp32() {
  String model = ESP.getChipModel();
  TEST_ASSERT_TRUE_MESSAGE(model.startsWith("ESP32-D0WD"), model.c_str());
}

void test_no_psram() {
  // WROOM-32D has no PSRAM; if this fails we're on a WROVER and must move CE/IRQ.
  TEST_ASSERT_FALSE(psramFound());
}

void test_flash_size() { TEST_ASSERT_GREATER_OR_EQUAL(4 * 1024 * 1024, ESP.getFlashChipSize()); }

void setup() {
  delay(2000);  // let the test runner attach to serial
  UNITY_BEGIN();
  RUN_TEST(test_chip_is_original_esp32);
  RUN_TEST(test_no_psram);
  RUN_TEST(test_flash_size);
  UNITY_END();
}

void loop() {}
