// Tier 2 (device): nRF24 module checks. Ignored unless HW_NRF24 is true.
#include <Arduino.h>
#include <RF24.h>
#include <unity.h>

#include "hw_config.h"
#include "pins.h"

RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);

void setUp() {
  if (!HW_NRF24) TEST_IGNORE_MESSAGE("HW_NRF24=false in hw_config.h (not verified)");
}
void tearDown() {}

void test_nrf_begin_and_connected() {
  TEST_ASSERT_TRUE_MESSAGE(radio.begin(), "radio.begin() failed");
  TEST_ASSERT_TRUE_MESSAGE(radio.isChipConnected(), "isChipConnected() false");
}

void test_nrf_register_readback() {
  TEST_ASSERT_TRUE(radio.begin());
  int errors = 0;
  for (int i = 0; i < 200; i++) {
    uint8_t ch = random(0, 126);
    radio.setChannel(ch);
    if (radio.getChannel() != ch) errors++;
    auto pa = static_cast<rf24_pa_dbm_e>(i % 4);
    radio.setPALevel(pa);
    if (radio.getPALevel() != pa) errors++;
    auto dr = static_cast<rf24_datarate_e>(i % 3);
    radio.setDataRate(dr);
    if (radio.getDataRate() != dr) errors++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, errors, "register read-back mismatches");
}

void test_nrf_rx_config_applies() {
  // Mirrors rf_tools applyRx() with the promiscuous preset.
  TEST_ASSERT_TRUE(radio.begin());
  radio.setAutoAck(false);
  radio.disableDynamicPayloads();
  radio.setAddressWidth(2);
  radio.disableCRC();
  radio.setPayloadSize(32);
  radio.setChannel(7);
  const uint8_t reg[2] = {0xAA, 0x00};
  radio.openReadingPipe(0, reg);
  radio.startListening();
  TEST_ASSERT_EQUAL(7, radio.getChannel());
  TEST_ASSERT_EQUAL(RF24_CRC_DISABLED, radio.getCRCLength());
  TEST_ASSERT_EQUAL(32, radio.getPayloadSize());
  radio.stopListening();
}

void setup() {
  delay(2000);
  randomSeed(esp_random());
  UNITY_BEGIN();
  RUN_TEST(test_nrf_begin_and_connected);
  RUN_TEST(test_nrf_register_readback);
  RUN_TEST(test_nrf_rx_config_applies);
  UNITY_END();
}

void loop() {}
