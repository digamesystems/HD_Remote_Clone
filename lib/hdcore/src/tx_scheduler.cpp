// SPDX-License-Identifier: GPL-2.0-or-later
#include "tx_scheduler.h"

namespace hd {
namespace tx {

using ui::Tx;

bool Scheduler::sameAsLast(const ui::Output& o) const {
  return o.tx == last_.tx && o.up == last_.up && o.group == last_.group &&
         o.irCode == last_.irCode;
}

Step Scheduler::next(const ui::Output& o) {
  Step s;
  if (o.tx == Tx::None) {
    active_ = false;
    return s;
  }
  if (!active_ || !sameAsLast(o)) {
    active_ = true;
    last_ = o;
    phase_ = Phase::Rf;
    chan_ = 0;
  }

  switch (o.tx) {
    case Tx::Group:
    case Tx::GroupTail:
      s.kind = StepKind::Rf;
      s.channel = 0;  // rotate packet by packet, like the remote in green mode
      s.ms = cfg_.groupStepMs;
      rf::groupPayload(id_[0], id_[1], o.group, o.up, o.tx == Tx::Group, s.payload);
      return s;

    case Tx::Wake:
    case Tx::Setup: {
      const bool rfOn = o.tx == Tx::Setup || cfg_.wake;  // setup frames are the point of pairing
      if (!rfOn) phase_ = Phase::Ir;
      if (phase_ == Phase::Rf) {
        s.kind = StepKind::Rf;
        s.channel = rf::kChannels[chan_];
        s.ms = cfg_.rfMs;
        if (o.tx == Tx::Wake) rf::wakePayload(id_[0], id_[1], s.payload);
        else rf::setupPayload(id_[0], id_[1], o.group, o.up, true, s.payload);
        phase_ = cfg_.gapMs ? Phase::Gap : Phase::Ir;
      } else if (phase_ == Phase::Gap) {
        s.kind = StepKind::Gap;
        s.ms = cfg_.gapMs;
        phase_ = Phase::Ir;
      } else {
        s.kind = StepKind::Ir;
        s.irCode = o.irCode;
        s.ms = cfg_.irMs;
        phase_ = Phase::Rf;
        chan_ = (chan_ + 1) % 3;
      }
      return s;
    }

    default:
      return s;
  }
}

}  // namespace tx
}  // namespace hd
