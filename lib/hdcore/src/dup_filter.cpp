#include "dup_filter.h"

#include <cstring>

namespace hd {

void DupFilter::reset() {
  std::memset(slots_, 0, sizeof(slots_));
  next_ = 0;
}

uint16_t DupFilter::feed(const uint8_t* data, uint8_t len) {
  if (len > kMaxLen) len = kMaxLen;
  uint8_t cmp = len < cmpLen_ ? len : cmpLen_;
  for (auto& s : slots_) {
    if (s.count && s.len >= cmp && std::memcmp(s.data, data, cmp) == 0) {
      s.count++;
      return (s.count == 2 || s.count % 10 == 0) ? s.count : 0;
    }
  }
  Slot& s = slots_[next_];
  next_ = (next_ + 1) % kSlots;
  std::memcpy(s.data, data, len);
  s.len = len;
  s.count = 1;
  return 0;
}

}  // namespace hd
