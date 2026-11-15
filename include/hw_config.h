// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// What hardware is present on the board under test. Tier 2 (on-device) tests
// use this: a test whose hardware is declared absent is IGNORED (reported as
// not verified); a test whose hardware is declared present MUST pass.
// Board selection follows pins.h (HD_BOARD_HANDHELD = handheld PCB).

#if defined(HD_BOARD_HANDHELD)

// Handheld PCB rev 1. Set HW_NRF24 false if the nRF24 module isn't in J401.
constexpr bool HW_NRF24       = true;   // nRF24 module in socket J401
constexpr bool HW_NRF24_B     = false;  // no second radio on the PCB
constexpr bool HW_IR_LOOPBACK = false;  // no IR receiver on the PCB
constexpr bool HW_BUTTONS     = true;   // 8 keys with 100k pull-downs (pins.h)

#else

// Breadboard. Update this whenever the breadboard changes.
constexpr bool HW_NRF24       = true;  // nRF24 module on VSPI + CE/CSN per pins.h
constexpr bool HW_NRF24_B     = false;  // second nRF24 ("radio B") on HSPI pins per pins.h, for RF loopback
constexpr bool HW_IR_LOOPBACK = true;  // IR LED (GPIO 4 driver) aimed at IR receiver (GPIO 14), a few cm apart
constexpr bool HW_BUTTONS     = false;  // UP/DOWN buttons with 10k pull-downs on GPIO 32 (UP) / 33 (DOWN)

#endif
