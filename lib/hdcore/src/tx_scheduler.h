// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Transmit scheduler: turns the handheld logic's "send this now" (hd::ui::Output)
// into a stream of short steps the firmware can run one at a time.
//
// The firmware loop is: o = handheld.update(now); s = scheduler.next(o); run s
// (blocking for s.ms, at most ~50 ms); repeat. Because each step is short, a
// key release, the 5 s cap or a new key takes effect within one step, and the
// radio and IR drivers only ever do one small thing at a time. No hardware
// here: the firmware maps each Step onto the nRF24 and the IR LED.
//
// Step patterns (timings from the shade tests, docs/IR_PROTOCOL.md 2026-11-15):
// - Wake (red-mode arrow) and Setup (pairing): RF on one channel for rfMs, an
//   optional idle gap, then the IR code for irMs; the channel advances
//   33 -> 52 -> 71 each cycle. This is the breadboard's `seq` / `hold` pattern.
//   With wake off, Wake sends the IR steps alone (breadboard tests only).
// - Group and GroupTail: RF only, rotating channels packet by packet as the
//   original remote does, in chunks of groupStepMs. The handheld logic decides
//   how long each lasts (minimum hold, cap, ~330 ms tail).
// A new transmission (different kind, direction, group or IR code, or after
// None) starts its pattern from the beginning (RF on ch 33).

#include <cstdint>

#include "handheld.h"
#include "hd_rf.h"

namespace hd {
namespace tx {

enum class StepKind : uint8_t {
  None,  // nothing to send
  Rf,    // send `payload` for `ms`: on `channel`, or rotating 33/52/71 packet by packet if 0
  Ir,    // send IR code `irCode` (HD frames at 40 kHz) for `ms`
  Gap,   // stay idle for `ms`
};

struct Step {
  StepKind kind = StepKind::None;
  uint8_t channel = 0;  // Rf: 33, 52 or 71; 0 = rotate every packet
  uint8_t payload[rf::kPayloadSize] = {0, 0, 0, 0, 0};
  uint8_t irCode = 0;
  uint32_t ms = 0;
};

struct Config {
  uint32_t rfMs = 50;         // RF per cycle in Wake / Setup (the `seq rf` default)
  uint32_t gapMs = 0;         // idle between that RF and the IR (`seq gap`)
  uint32_t irMs = 50;         // IR per cycle: three frames (`seq ir`; 1 frame was unreliable)
  uint32_t groupStepMs = 20;  // chunk size for Group / GroupTail, so a release lands within ~20 ms
  bool wake = true;           // false: Wake sends IR alone (`wake off`, for tests)
};

class Scheduler {
 public:
  explicit Scheduler(const Config& cfg = Config()) : cfg_(cfg) {}

  // The remote ID carried in every RF payload.
  void setId(uint8_t id0, uint8_t id1) {
    id_[0] = id0;
    id_[1] = id1;
  }
  Config& config() { return cfg_; }

  // The next step for what the handheld wants now. Call again after running it.
  Step next(const ui::Output& o);

  // Forget the pattern position: the next transmission starts from the top.
  void reset() { active_ = false; }

 private:
  enum class Phase : uint8_t { Rf, Gap, Ir };
  bool sameAsLast(const ui::Output& o) const;

  Config cfg_;
  uint8_t id_[2] = {0, 0};

  bool active_ = false;
  ui::Output last_;
  Phase phase_ = Phase::Rf;
  uint8_t chan_ = 0;  // index into rf::kChannels
};

}  // namespace tx
}  // namespace hd
