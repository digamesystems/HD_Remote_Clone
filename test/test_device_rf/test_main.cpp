// Tier 2 (device): nRF24 module checks. Ignored unless HW_NRF24 is true; the
// loopback test also needs HW_NRF24_B (a second module on the HSPI pins).
#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>
#include <unity.h>

#include "hd_rf.h"
#include "hw_config.h"
#include "nrf_addr.h"
#include "pins.h"

RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
SPIClass hspi(HSPI);
RF24 radioB(PIN_NRF_B_CE, PIN_NRF_B_CSN);

void setUp() {
  if (!HW_NRF24) TEST_IGNORE_MESSAGE("HW_NRF24=false in hw_config.h (not verified)");
}
void tearDown() {}

// Mirrors rf_tools applyCommon() with the Platinum (hd) preset.
void applyHd(RF24& r, uint8_t channel) {
  r.stopListening();
  r.setAutoAck(false);
  r.disableDynamicPayloads();
  r.setDataRate(RF24_2MBPS);
  r.setChannel(channel);
  r.setAddressWidth(hd::rf::kAddrWidth);
  r.setCRCLength(RF24_CRC_16);
  r.setPayloadSize(hd::rf::kPayloadSize);
}

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
    // 1 and 2 Mbps only: 250 kbps isn't needed for the Platinum remote.
    auto dr = (i % 2) ? RF24_2MBPS : RF24_1MBPS;
    radio.setDataRate(dr);
    if (radio.getDataRate() != dr) errors++;
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, errors, "register read-back mismatches");
}

void test_nrf_hd_rx_config_applies() {
  TEST_ASSERT_TRUE(radio.begin());
  applyHd(radio, hd::rf::kChannels[0]);
  uint8_t reg[3];
  hd::onAirToRegister(hd::rf::kAddress, hd::rf::kAddrWidth, reg);
  radio.openReadingPipe(0, reg);
  radio.startListening();
  TEST_ASSERT_EQUAL(33, radio.getChannel());
  TEST_ASSERT_EQUAL(RF24_2MBPS, radio.getDataRate());
  TEST_ASSERT_EQUAL(RF24_CRC_16, radio.getCRCLength());
  TEST_ASSERT_EQUAL(5, radio.getPayloadSize());
  radio.stopListening();
}

// Radio B sends beacon packets at the HD config on each Platinum channel; the
// main radio must receive nearly all of them. This is what makes an empty
// sniff of the real remote meaningful.
void test_nrf_loopback_hd_config() {
  if (!HW_NRF24_B) TEST_IGNORE_MESSAGE("HW_NRF24_B=false in hw_config.h (not verified)");
  TEST_ASSERT_TRUE_MESSAGE(radio.begin(), "main radio begin() failed");
  hspi.begin(PIN_NRF_B_SCK, PIN_NRF_B_MISO, PIN_NRF_B_MOSI, PIN_NRF_B_CSN);
  TEST_ASSERT_TRUE_MESSAGE(radioB.begin(&hspi) && radioB.isChipConnected(), "radio B not responding");

  uint8_t reg[3];
  hd::onAirToRegister(hd::rf::kAddress, hd::rf::kAddrWidth, reg);
  const int kPackets = 50;
  for (uint8_t ch : hd::rf::kChannels) {
    applyHd(radio, ch);
    radio.openReadingPipe(0, reg);
    radio.startListening();
    applyHd(radioB, ch);
    radioB.setPALevel(RF24_PA_MIN);
    radioB.openWritingPipe(reg);

    int received = 0;
    uint8_t buf[hd::rf::kPayloadSize];
    for (int i = 0; i < kPackets; i++) {
      radioB.write(hd::rf::kBeaconPayload, hd::rf::kPayloadSize);
      uint32_t t0 = millis();
      while (millis() - t0 < 2) {
        while (radio.available()) {
          radio.read(buf, sizeof(buf));
          if (memcmp(buf, hd::rf::kBeaconPayload, sizeof(buf)) == 0) received++;
        }
      }
    }
    radio.stopListening();
    char msg[48];
    snprintf(msg, sizeof(msg), "ch %u: received %d of %d", ch, received, kPackets);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(kPackets * 9 / 10, received, msg);
  }
}

void setup() {
  delay(2000);
  randomSeed(esp_random());
  UNITY_BEGIN();
  RUN_TEST(test_nrf_begin_and_connected);
  RUN_TEST(test_nrf_register_readback);
  RUN_TEST(test_nrf_hd_rx_config_applies);
  RUN_TEST(test_nrf_loopback_hd_config);
  UNITY_END();
}

void loop() {}
