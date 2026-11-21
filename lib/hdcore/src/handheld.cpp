// SPDX-License-Identifier: GPL-2.0-or-later
#include "handheld.h"

#include "hd_ir.h"
#include "hd_rf.h"

namespace hd {
namespace ui {

uint8_t groupByte(Key k) {
  switch (k) {
    case Key::G1: return 0x01;
    case Key::G2: return 0x02;
    case Key::G3: return 0x04;
    case Key::G4: return 0x08;
    case Key::All: return rf::kGroupAll;
    default: return 0;
  }
}

uint8_t Handheld::irCode(bool up) const {
  if (channelB_) return up ? ir::kCodeUpB : ir::kCodeDownB;
  return up ? ir::kCodeUp : ir::kCodeDown;
}

void Handheld::press(Key k, uint32_t now) {
  advance(now);  // finish anything due first, so the result doesn't depend on loop timing
  if (k == Key::Setup) {
    if (setup_) {
      setup_ = false;  // SETUP again: save and leave (the shade stores the pairing itself)
    } else {
      setup_ = true;
      setupGroup_ = 0;
      setupAt_ = now;
      windowGroup_ = 0;  // no green window inside setup mode
    }
    return;
  }

  const uint8_t g = groupByte(k);
  if (g) {
    if (setup_) {
      setupGroup_ = g;
      setupAt_ = now;
    } else {
      windowGroup_ = g;
      windowAt_ = now;
    }
    return;
  }

  // An arrow. Only one at a time: a second arrow while one is down is ignored.
  if (arrow_ != Arrow::None) return;
  const bool up = (k == Key::Up);
  if (setup_) {
    if (!setupGroup_) return;  // setup mode needs a group key first
    arrow_ = Arrow::Setup;
    arrowGroup_ = setupGroup_;
    setupAt_ = now;
  } else if (windowOpen(now)) {
    arrow_ = Arrow::Group;
    arrowGroup_ = windowGroup_;
  } else {
    arrow_ = Arrow::Red;
    arrowGroup_ = 0;
  }
  arrowKey_ = k;
  arrowHeld_ = true;
  arrowUp_ = up;
  arrowAt_ = now;
  tail_ = false;  // a new command cuts off an old release tail
}

void Handheld::release(Key k, uint32_t now) {
  advance(now);
  if (arrow_ == Arrow::None || k != arrowKey_ || !arrowHeld_) return;
  arrowHeld_ = false;
  if (arrow_ == Arrow::Group) {
    if (cfg_.arrowRestartsWindow) windowAt_ = now;
    return;  // update() finishes the minimum hold, then starts the tail
  }
  endArrow(now);
}

void Handheld::endArrow(uint32_t now) {
  if (arrow_ == Arrow::Setup) setupAt_ = now;  // setup's timeout runs from the last key
  arrow_ = Arrow::None;
}

void Handheld::advance(uint32_t now) {
  const uint32_t elapsed = now - arrowAt_;

  if (arrow_ == Arrow::Group) {
    const bool released = !arrowHeld_ && elapsed >= cfg_.groupMinHeldMs;
    if (released || elapsed >= cfg_.maxSendMs) {
      tail_ = true;
      tailUp_ = arrowUp_;
      tailGroup_ = arrowGroup_;
      tailAt_ = now;
      // Capped while still held: the key stays "spent" (sends nothing) until released.
      if (arrowHeld_) arrow_ = Arrow::Spent;
      else arrow_ = Arrow::None;
    }
  } else if ((arrow_ == Arrow::Red || arrow_ == Arrow::Setup) && elapsed >= cfg_.maxSendMs) {
    if (arrow_ == Arrow::Setup) setupAt_ = now;
    arrow_ = Arrow::Spent;
  }
  if (arrow_ == Arrow::Spent && !arrowHeld_) arrow_ = Arrow::None;

  if (tail_ && now - tailAt_ >= cfg_.groupTailMs) tail_ = false;
  if (setup_ && arrow_ != Arrow::Setup && now - setupAt_ >= cfg_.setupTimeoutMs) setup_ = false;
}

Output Handheld::update(uint32_t now) {
  advance(now);
  Output o;
  switch (arrow_) {
    case Arrow::Red:
      o.tx = Tx::Wake;
      o.up = arrowUp_;
      o.irCode = irCode(arrowUp_);
      break;
    case Arrow::Group:
      o.tx = Tx::Group;
      o.up = arrowUp_;
      o.group = arrowGroup_;
      break;
    case Arrow::Setup:
      o.tx = Tx::Setup;
      o.up = arrowUp_;
      o.group = arrowGroup_;
      o.irCode = irCode(arrowUp_);
      break;
    default:
      if (tail_) {
        o.tx = Tx::GroupTail;
        o.up = tailUp_;
        o.group = tailGroup_;
      }
      break;
  }

  if (setup_) o.led = Led::GreenBlink;
  else if (arrow_ == Arrow::Red) o.led = Led::Red;
  else if (arrow_ == Arrow::Group || tail_ || windowOpen(now)) o.led = Led::Green;
  return o;
}

bool Handheld::idle(uint32_t now) {
  const Output o = update(now);
  return o.tx == Tx::None && o.led == Led::Off && arrow_ == Arrow::None;
}

bool Debouncer::update(bool raw, uint32_t now) {
  if (raw != last_) {
    last_ = raw;
    changedAt_ = now;
  }
  if (raw != stable_ && now - changedAt_ >= ms_) {
    stable_ = raw;
    return true;
  }
  return false;
}

}  // namespace ui
}  // namespace hd
