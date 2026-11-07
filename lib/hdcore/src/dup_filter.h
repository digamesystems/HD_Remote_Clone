#pragma once
// Suppresses one-off noise packets when sniffing promiscuously. The original
// remote repeats each packet ~30x, so a payload is reported on its 2nd
// sighting and then every 10th.

#include <cstdint>

namespace hd {

class DupFilter {
 public:
  static constexpr int kSlots = 32;
  static constexpr uint8_t kMaxLen = 32;

  explicit DupFilter(uint8_t cmpLen = 10) : cmpLen_(cmpLen) { reset(); }

  void reset();

  // Returns the sighting count if this packet should be printed, else 0.
  // Only the first min(cmpLen, len) bytes are compared.
  uint16_t feed(const uint8_t* data, uint8_t len);

 private:
  struct Slot {
    uint8_t data[kMaxLen];
    uint8_t len;
    uint16_t count;
  };
  Slot slots_[kSlots];
  int next_ = 0;
  uint8_t cmpLen_;
};

}  // namespace hd
