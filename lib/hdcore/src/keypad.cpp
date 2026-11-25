// SPDX-License-Identifier: GPL-2.0-or-later
#include "keypad.h"

namespace hd {
namespace ui {

Keypad::Keypad(uint32_t debounceMs) {
  for (Debouncer& d : keys_) d = Debouncer(debounceMs);
}

uint8_t Keypad::poll(uint8_t raw, uint32_t now, Handheld& h) {
  uint8_t pressed = 0, released = 0;
  for (int i = 0; i < kKeyCount; i++) {
    const uint8_t bit = static_cast<uint8_t>(1u << i);
    if (!keys_[i].update(raw & bit, now)) continue;
    (keys_[i].pressed() ? pressed : released) |= bit;
  }

  static const Key kOrder[kKeyCount] = {Key::Setup, Key::G1, Key::G2, Key::G3,
                                        Key::G4,    Key::All, Key::Up, Key::Down};
  for (Key k : kOrder)
    if (released & keyBit(k)) h.release(k, now);
  for (Key k : kOrder)
    if (pressed & keyBit(k)) h.press(k, now);
  return pressed;
}

uint8_t Keypad::held() const {
  uint8_t mask = 0;
  for (int i = 0; i < kKeyCount; i++)
    if (keys_[i].pressed()) mask |= static_cast<uint8_t>(1u << i);
  return mask;
}

}  // namespace ui
}  // namespace hd
