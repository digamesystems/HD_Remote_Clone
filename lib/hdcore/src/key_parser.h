// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Picks arrow keys out of a serial input stream. Terminals send them as escape
// sequences: ESC [ A (up), ESC [ B (down), or ESC O A / ESC O B in application
// cursor mode. Other sequences are swallowed so they don't land in the command line.

namespace hd {

enum class KeyEvent {
  Pass,      // ordinary character: handle it normally
  Consumed,  // part of an escape sequence: ignore it
  Up,        // up arrow completed
  Down,      // down arrow completed
};

class KeyParser {
 public:
  KeyEvent feed(char c) {
    switch (state_) {
      case State::Idle:
        if (c != kEsc) return KeyEvent::Pass;
        state_ = State::Esc;
        return KeyEvent::Consumed;

      case State::Esc:
        if (c == kEsc) return KeyEvent::Consumed;
        if (c == '[' || c == 'O') {
          state_ = State::Seq;
          return KeyEvent::Consumed;
        }
        state_ = State::Idle;  // lone ESC key: keep what follows it
        return KeyEvent::Pass;

      case State::Seq:
        if (c < 0x20) {  // control character: not a sequence after all
          state_ = State::Idle;
          return KeyEvent::Pass;
        }
        if (c < 0x40) return KeyEvent::Consumed;  // parameter bytes, e.g. "1;5"
        state_ = State::Idle;
        if (c == 'A') return KeyEvent::Up;
        if (c == 'B') return KeyEvent::Down;
        return KeyEvent::Consumed;
    }
    return KeyEvent::Pass;
  }

 private:
  static constexpr char kEsc = 0x1B;
  enum class State { Idle, Esc, Seq };
  State state_ = State::Idle;
};

}  // namespace hd
