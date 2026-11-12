// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Pin map for ESP32-DevKitC (ESP32-WROOM-32). See CLAUDE.md.
// On a WROVER module, move NRF_CE / NRF_IRQ off GPIO 16/17 (PSRAM).

#include <stdint.h>

constexpr uint8_t PIN_IR_RX   = 14;  // IR receiver OUT (TSOP38238 / VS1838B)
constexpr uint8_t PIN_IR_TX   = 4;   // NPN base via 1 kOhm (HIGH = LED on)
constexpr uint8_t PIN_BTN_UP  = 32;  // active-HIGH, external 10k pull-down, RTC-capable
constexpr uint8_t PIN_BTN_DN  = 33;  // active-HIGH, external 10k pull-down

// nRF24 on VSPI (SCK 18, MISO 19, MOSI 23 are the SPI defaults)
constexpr uint8_t PIN_NRF_CSN = 5;
constexpr uint8_t PIN_NRF_CE  = 17;
constexpr uint8_t PIN_NRF_IRQ = 16;  // optional, unused for now

// Optional second nRF24 ("radio B") for loopback checks: B transmits, the main
// radio receives. Own SPI bus (HSPI) on remapped pins: HSPI's default SCK is
// GPIO 14 (our IR receiver) and its MISO is GPIO 12, a boot strapping pin.
constexpr uint8_t PIN_NRF_B_SCK  = 25;
constexpr uint8_t PIN_NRF_B_MISO = 26;
constexpr uint8_t PIN_NRF_B_MOSI = 27;
constexpr uint8_t PIN_NRF_B_CSN  = 13;
constexpr uint8_t PIN_NRF_B_CE   = 21;
