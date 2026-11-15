// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Tracks how long a PC arrow key is held, from the terminal's key repeats.
// A serial terminal sends no key-up event, only the key again at the
// keyboard's repeat rate (about every 33 ms on Windows defaults) after an
// initial repeat delay (about 500 ms). So the key counts as released once no
// repeat has arrived for longer than the expected gap: firstGapMs before the
// first repeat, repeatGapMs after it.

#include <cstdint>

namespace hd {

class HoldTracker {
 public:
  HoldTracker(uint32_t firstGapMs, uint32_t repeatGapMs)
      : firstGapMs_(firstGapMs), repeatGapMs_(repeatGapMs) {}

  void press(uint32_t now) {
    pressAt_ = lastAt_ = now;
    repeats_ = 0;
  }
  void repeat(uint32_t now) {
    lastAt_ = now;
    repeats_++;
  }
  bool released(uint32_t now) const {
    return now - lastAt_ > (repeats_ ? repeatGapMs_ : firstGapMs_);
  }
  // Press to last repeat: the hold time, to within one repeat interval.
  // 0 for a tap (no repeats): the release time is unknown, under firstGapMs.
  uint32_t heldMs() const { return lastAt_ - pressAt_; }
  uint32_t repeats() const { return repeats_; }

 private:
  uint32_t firstGapMs_, repeatGapMs_;
  uint32_t pressAt_ = 0, lastAt_ = 0, repeats_ = 0;
};

}  // namespace hd
