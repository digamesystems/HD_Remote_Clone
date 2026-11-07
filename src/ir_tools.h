#pragma once
#include <Arduino.h>

namespace ir {

constexpr int kNumSlots = 8;

void begin();
// Call from loop() while in capture mode; dumps any new capture to Serial.
void pollCapture();
void startCapture();
void stopCapture();

// Replay: slot < 0 means "last capture".
void play(int slot, int repeats, int gapMs);
bool save(int slot);
void list();
void dump(int slot);

}  // namespace ir
