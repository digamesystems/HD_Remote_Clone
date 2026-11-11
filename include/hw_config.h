#pragma once
// What is currently wired on the breadboard. Tier 2 (on-device) tests use this:
// a test whose hardware is declared absent is IGNORED (reported as not
// verified); a test whose hardware is declared present MUST pass.
// Update this whenever the breadboard changes.

constexpr bool HW_NRF24       = true;  // nRF24 module on VSPI + CE/CSN per pins.h
constexpr bool HW_NRF24_B     = false;  // second nRF24 ("radio B") on HSPI pins per pins.h, for RF loopback
constexpr bool HW_IR_LOOPBACK = true;  // IR LED (GPIO 4 driver) aimed at IR receiver (GPIO 14), a few cm apart
constexpr bool HW_BUTTONS     = false;  // UP/DOWN buttons with 10k pull-downs on GPIO 32 (UP) / 33 (DOWN)
