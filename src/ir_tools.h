// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <Arduino.h>

namespace ir {

constexpr int kNumSlots = 8;

void begin();
// Call from loop() while in capture mode; dumps any new capture to Serial.
void pollCapture();
// raw=false prints only the one-line decoded summary per capture.
void startCapture(bool raw = true);
void stopCapture();

// Replay: slot < 0 means "last capture".
void play(int slot, int repeats, int gapMs);
bool save(int slot);
// Transmit an HD Platinum frame burst (4-bit code) lasting ~durationMs.
void sendCode(uint8_t code, uint32_t durationMs);
void list();
void dump(int slot);

}  // namespace ir
