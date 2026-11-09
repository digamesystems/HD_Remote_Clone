#include "run_tracker.h"

#include <cstring>

namespace hd {

void RunTracker::start(const uint8_t* p, uint8_t len, uint32_t tUs) {
  if (len > PacketRun::kMaxLen) len = PacketRun::kMaxLen;
  memcpy(cur_.payload, p, len);
  cur_.len = len;
  cur_.count = 1;
  cur_.firstUs = cur_.lastUs = tUs;
  cur_.minGapUs = cur_.maxGapUs = 0;
  active_ = true;
}

bool RunTracker::feed(const uint8_t* p, uint8_t len, uint32_t tUs, PacketRun* done) {
  if (len > PacketRun::kMaxLen) len = PacketRun::kMaxLen;
  if (active_ && len == cur_.len && memcmp(p, cur_.payload, len) == 0) {
    uint32_t gap = tUs - cur_.lastUs;  // unsigned: survives micros() wrap
    if (cur_.count == 1 || gap < cur_.minGapUs) cur_.minGapUs = gap;
    if (gap > cur_.maxGapUs) cur_.maxGapUs = gap;
    cur_.lastUs = tUs;
    cur_.count++;
    return false;
  }
  bool ended = flush(done);
  start(p, len, tUs);
  return ended;
}

bool RunTracker::flushIfIdle(uint32_t nowUs, uint32_t idleUs, PacketRun* done) {
  if (!active_ || nowUs - cur_.lastUs < idleUs) return false;
  return flush(done);
}

bool RunTracker::flush(PacketRun* done) {
  if (!active_) return false;
  *done = cur_;
  active_ = false;
  return true;
}

}  // namespace hd
