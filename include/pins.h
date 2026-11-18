// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Pin maps. Two boards share one firmware:
//   - default:            breadboard, ESP32-DevKitC (ESP32-WROOM-32D). See CLAUDE.md.
//   - HD_BOARD_HANDHELD:  handheld PCB rev 1 (hardware/hd_remote, ESP32-WROOM-32E),
//                         built by `pio run -e handheld`.
// On a WROVER module, move NRF_CE / NRF_IRQ off GPIO 16/17 (PSRAM).

#include <stdint.h>

constexpr uint8_t PIN_NONE = 0xFF;  // feature not present on this board

#if defined(HD_BOARD_HANDHELD)

// --- Handheld PCB rev 1 (module pin -> GPIO, from hd_remote.kicad_pcb) ---
constexpr bool HAS_IR_RX = false;  // no IR receiver on the PCB

constexpr uint8_t PIN_IR_RX = PIN_NONE;
constexpr uint8_t PIN_IR_TX = 4;   // module pin 26; R301 470R -> Q301/Q302 bases (HIGH = LEDs on)

// Keys: active-HIGH (switch to 3V3), 100k pull-down + 100nF (C501-C508) at each
// switch: ~10 ms release filter; the firmware still debounces. All on RTC GPIOs, so ext1 ANY_HIGH can wake from
// deep sleep. GPIO 34/35/36 are input-only with no internal pulls; the
// external pull-downs cover them.
constexpr uint8_t PIN_BTN_UP    = 32;  // module pin 8,  SW501
constexpr uint8_t PIN_BTN_DN    = 33;  // module pin 9,  SW502
constexpr uint8_t PIN_BTN_ALL   = 25;  // module pin 10, SW503
constexpr uint8_t PIN_BTN_G1    = 26;  // module pin 11, SW505
constexpr uint8_t PIN_BTN_G2    = 34;  // module pin 6,  SW506 (input-only)
constexpr uint8_t PIN_BTN_G3    = 27;  // module pin 12, SW507
constexpr uint8_t PIN_BTN_G4    = 36;  // module pin 4,  SW508 (input-only, SENSOR_VP)
constexpr uint8_t PIN_BTN_SETUP = 35;  // module pin 7,  SW504 (input-only)

// A/B IR channel slider (SW509): common pin to 3V3 or GND, via R202 10k.
// No pull needed; the slider always drives one rail.
constexpr uint8_t PIN_AB_SW = 14;  // module pin 13

// Battery sense: VBAT -> 1M / 1M -> GPIO 39 (ADC1_CH3), 100nF at the pin.
// Reads VBAT/2. ADC1, so it still works with the radio/WiFi active.
constexpr uint8_t PIN_VBAT_SENSE = 39;  // module pin 5, SENSOR_VN (input-only)

// Bicolor indicator D501 (common cathode): HIGH = on.
constexpr uint8_t PIN_LED_RED   = 22;  // module pin 36, R509 330R
constexpr uint8_t PIN_LED_GREEN = 16;  // module pin 27, R510 270R

// nRF24 on VSPI (SCK 18, MISO 19, MOSI 23 are the SPI defaults), socket J401.
constexpr uint8_t PIN_NRF_CSN = 5;   // module pin 29
constexpr uint8_t PIN_NRF_CE  = 17;  // module pin 28
constexpr uint8_t PIN_NRF_IRQ = 13;  // module pin 16, via R401 (0R, optional), unused for now

#else

// --- Breadboard (ESP32-DevKitC) ---
constexpr bool HAS_IR_RX = true;

constexpr uint8_t PIN_IR_RX   = 14;  // IR receiver OUT (TSOP38238 / VS1838B)
constexpr uint8_t PIN_IR_TX   = 4;   // NPN base via 1 kOhm (HIGH = LED on)
constexpr uint8_t PIN_BTN_UP  = 32;  // active-HIGH, external 10k pull-down, RTC-capable
constexpr uint8_t PIN_BTN_DN  = 33;  // active-HIGH, external 10k pull-down

// nRF24 on VSPI (SCK 18, MISO 19, MOSI 23 are the SPI defaults)
constexpr uint8_t PIN_NRF_CSN = 5;
constexpr uint8_t PIN_NRF_CE  = 17;
constexpr uint8_t PIN_NRF_IRQ = 16;  // optional, unused for now

#endif
