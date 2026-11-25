// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// The remote's eight keys: raw levels in, debounced presses and releases out
// to the key logic (hd::ui::Handheld). No hardware here: the firmware reads
// the pins into a bit mask and calls poll() often.

#include <cstdint>

#include "handheld.h"

namespace hd {
namespace ui {

constexpr int kKeyCount = 8;  // Key::Up .. Key::Setup

// The bit that stands for a key in a key mask.
constexpr uint8_t keyBit(Key k) { return static_cast<uint8_t>(1u << static_cast<int>(k)); }

class Keypad {
 public:
  explicit Keypad(uint32_t debounceMs = 25);

  // `raw`: bit keyBit(k) set while key k reads down. Debounces each key and
  // passes the changes to `h`: releases first, then SETUP, the group keys and
  // last the arrows, so a group key and an arrow that land together make a
  // group command. Returns the mask of keys newly pressed by this call.
  uint8_t poll(uint8_t raw, uint32_t now, Handheld& h);

  // Mask of the keys that are down, debounced.
  uint8_t held() const;

 private:
  Debouncer keys_[kKeyCount];
};

}  // namespace ui
}  // namespace hd
