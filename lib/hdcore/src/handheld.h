// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Handheld remote behaviour: keys in, "what to transmit and show now" out.
//
// A pure state machine with no hardware: the firmware feeds it debounced key
// presses and releases with millisecond timestamps, calls update() every loop,
// and does what the Output says (the radio and IR timing stay in the firmware).
//
// Behaviour copies the original Platinum remote as observed (see CLAUDE.md and
// docs/RF_PROTOCOL.md):
// - Group keys 1-4 and ALL send nothing. Each press opens (or restarts) a green
//   window, about 6.5 s on our remote (the manual says 5 s).
// - An arrow pressed inside the window sends the group command
//   (ID 20 GROUP A1/A2) while held, then the release tail (21/22, ~330 ms).
// - An arrow pressed outside the window is red mode: wake frames (ID 01 00 80)
//   interleaved with the IR code, for as long as the key is held (decided
//   2026-11-15, John). The A/B slider picks the IR channel.
// - SETUP enters setup mode (green blinking, ~30 s); a group key picks the
//   group; UP adds the shade to it, DOWN removes it (setup frames 30/31 plus
//   the IR code, while held); SETUP again, or the timeout, leaves setup mode.
// - Every arrow transmission stops after maxSendMs even if the key is still
//   held (the 5 s cap, decided 2026-11-18, John; manual p. 21: a 6 s OPEN hold
//   at a fully open shade resets its limits).
//
// Choices not taken from the original or decided by John are marked
// "assumed" in Config below; they are easy to change once observed.

#include <cstdint>

namespace hd {
namespace ui {

enum class Key : uint8_t { Up, Down, G1, G2, G3, G4, All, Setup };

enum class Led : uint8_t { Off, Red, Green, GreenBlink };

enum class Tx : uint8_t {
  None,
  Wake,       // red mode: ID 01 00 80 interleaved with irCode
  Group,      // ID 20 group A1 (up) / A2 (down)
  GroupTail,  // ID 20 group 21 / 22 after the arrow is released
  Setup,      // ID 30 (up, add) / 31 (down, remove) group 80, interleaved with irCode
};

struct Output {
  Tx tx = Tx::None;
  bool up = false;      // arrow direction (Wake, Group, GroupTail, Setup)
  uint8_t group = 0;    // group byte: 01/02/04/08, FF = All (Group, GroupTail, Setup)
  uint8_t irCode = 0;   // 4-bit IR code to send alongside (Wake, Setup); 0 = none
  Led led = Led::Off;
};

struct Config {
  uint32_t windowMs = 6500;        // green window after a group key (measured on our remote)
  uint32_t groupMinHeldMs = 300;   // a tapped arrow still sends A1/A2 this long (assumed: = `grp` default)
  uint32_t groupTailMs = 330;      // release tail length (measured)
  uint32_t maxSendMs = 5000;       // cap on any one arrow transmission (decided 2026-11-18, John)
  uint32_t setupTimeoutMs = 30000; // setup mode lasts this long without a key (manual: ~30 s)
  bool arrowRestartsWindow = false;  // assumed: only group keys restart it (not yet observed)
};

// Group byte for a group key: G1..G4 -> 01, 02, 04, 08; All -> FF; else 0.
uint8_t groupByte(Key k);

class Handheld {
 public:
  explicit Handheld(const Config& cfg = Config()) : cfg_(cfg) {}

  void press(Key k, uint32_t now);
  void release(Key k, uint32_t now);
  // A/B slider: false = channel A (IR codes 0101 / 0110), true = B (1001 / 1010).
  void setChannelB(bool b) { channelB_ = b; }

  // Advances the timers and returns what to send and show now.
  Output update(uint32_t now);

  // Nothing to send and nothing to show: the firmware may go to sleep.
  bool idle(uint32_t now);

  bool inSetup() const { return setup_; }

 private:
  // Spent: capped at maxSendMs while still held; sends nothing until released.
  enum class Arrow : uint8_t { None, Red, Group, Setup, Spent };

  bool windowOpen(uint32_t now) const { return windowGroup_ && now - windowAt_ < cfg_.windowMs; }
  uint8_t irCode(bool up) const;
  void endArrow(uint32_t now);
  void advance(uint32_t now);  // timers only: minimum hold, caps, tail, setup timeout

  Config cfg_;
  bool channelB_ = false;

  // green window
  uint8_t windowGroup_ = 0;
  uint32_t windowAt_ = 0;

  // the arrow being sent (only one at a time; a second arrow is ignored)
  Arrow arrow_ = Arrow::None;
  Key arrowKey_ = Key::Up;
  bool arrowHeld_ = false;   // key still down
  bool arrowUp_ = false;
  uint8_t arrowGroup_ = 0;
  uint32_t arrowAt_ = 0;

  // group release tail
  bool tail_ = false;
  bool tailUp_ = false;
  uint8_t tailGroup_ = 0;
  uint32_t tailAt_ = 0;

  // setup mode
  bool setup_ = false;
  uint8_t setupGroup_ = 0;
  uint32_t setupAt_ = 0;
};

// Debounces one key: the stable state follows the raw input once it has held
// a new level for debounceMs. update() returns true when the stable state
// changes (an edge), and pressed() gives the stable state.
class Debouncer {
 public:
  explicit Debouncer(uint32_t debounceMs = 25) : ms_(debounceMs) {}
  bool update(bool raw, uint32_t now);
  bool pressed() const { return stable_; }

 private:
  uint32_t ms_;
  bool stable_ = false, last_ = false;
  uint32_t changedAt_ = 0;
};

}  // namespace ui
}  // namespace hd
