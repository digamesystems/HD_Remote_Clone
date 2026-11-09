#include "hd_rf.h"

#include <cstdio>

namespace hd {
namespace rf {

std::string groupName(uint8_t group) {
  switch (group) {
    case kGroupNone: return "none";
    case kGroupAll: return "All";
    case 0x01: return "1";
    case 0x02: return "2";
    case 0x04: return "3";
    case 0x08: return "4";
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "mask 0x%02X", group);
  return buf;
}

const char* actionName(uint8_t action) {
  switch (action) {
    case kActionUpHeld: return "UP held";
    case kActionDownHeld: return "DOWN held";
    case kActionUpRelease: return "UP release";
    case kActionDownRelease: return "DOWN release";
    case kActionNoGroup: return "no-group state";
  }
  return "?";
}

std::string describePayload(const uint8_t* p, size_t len) {
  if (len != kPayloadSize) return "";
  char buf[64];
  snprintf(buf, sizeof(buf), "ID %02X%02X  mode %02X  group %s  %s", p[0], p[1], p[2],
           groupName(p[3]).c_str(), actionName(p[4]));
  return buf;
}

}  // namespace rf
}  // namespace hd
