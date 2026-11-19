// SPDX-License-Identifier: GPL-2.0-or-later
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
    case kActionWakeHeld: return "wake held";
    case kActionWakeRelease: return "wake release";
  }
  return "?";
}

bool isUsableRemoteId(uint8_t id0, uint8_t id1) {
  return !(id0 == 0x00 && id1 == 0x00) && !(id0 == 0xFF && id1 == 0xFF);
}

bool remoteIdFromRandom(uint32_t r, uint8_t out[2]) {
  for (int shift : {0, 16}) {
    uint8_t lo = (r >> shift) & 0xFF, hi = (r >> (shift + 8)) & 0xFF;
    if (isUsableRemoteId(lo, hi)) {
      out[0] = lo;
      out[1] = hi;
      return true;
    }
  }
  return false;
}

void wakePayload(uint8_t id0, uint8_t id1, uint8_t out[kPayloadSize]) {
  out[0] = id0;
  out[1] = id1;
  out[2] = kModeNoGroup;
  out[3] = kGroupNone;
  out[4] = kActionWakeHeld;
}

void groupPayload(uint8_t id0, uint8_t id1, uint8_t group, bool up, bool held,
                  uint8_t out[kPayloadSize]) {
  out[0] = id0;
  out[1] = id1;
  out[2] = kModeGroup;
  out[3] = group;
  if (up) out[4] = held ? kActionUpHeld : kActionUpRelease;
  else out[4] = held ? kActionDownHeld : kActionDownRelease;
}

void setupPayload(uint8_t id0, uint8_t id1, uint8_t group, bool add, bool held,
                  uint8_t out[kPayloadSize]) {
  out[0] = id0;
  out[1] = id1;
  out[2] = add ? kModeSetupAdd : kModeSetupRemove;
  out[3] = group;
  out[4] = held ? kActionWakeHeld : kActionWakeRelease;
}

void positionPayload(uint8_t id0, uint8_t id1, uint8_t mask, uint8_t pos, bool phase1,
                     uint8_t out[kPayloadSize]) {
  out[0] = id0;
  out[1] = id1;
  out[2] = phase1 ? kModePosPhase1 : kModePosPhase2;
  out[3] = mask;
  out[4] = pos;
}

int parsePosMask(const std::string& in) {
  std::string s = in;
  if (s.size() > 2 && s[0] == '0' && (s[1] | 0x20) == 'x') s = s.substr(2);
  if (s.empty() || s.size() > 2) return -1;
  int v = 0;
  for (char c : s) {
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if ((c | 0x20) >= 'a' && (c | 0x20) <= 'f') d = (c | 0x20) - 'a' + 10;
    else return -1;
    v = v * 16 + d;
  }
  return v == 0 ? -1 : v;
}

int parsePosition(const std::string& s) {
  if (s.empty() || s.size() > 3) return -1;
  int v = 0;
  for (char c : s) {
    if (c < '0' || c > '9') return -1;
    v = v * 10 + (c - '0');
  }
  return v <= 255 ? v : -1;
}

bool positionTimingOk(uint32_t phase1Ms, uint32_t phase2Ms) {
  return phase1Ms <= kPosMaxPhaseMs && phase2Ms <= kPosMaxPhaseMs && phase1Ms + phase2Ms > 0;
}

namespace {
const char* const kPaNames[4] = {"min", "low", "high", "max"};
}  // namespace

int parsePaLevel(const std::string& s) {
  for (int i = 0; i < 4; i++) {
    const char* n = kPaNames[i];
    size_t len = 0;
    while (n[len]) len++;
    if (s.size() != len) continue;
    bool same = true;
    for (size_t k = 0; k < len; k++)
      if ((s[k] | 0x20) != n[k]) same = false;
    if (same) return i;
  }
  return -1;
}

const char* paLevelName(int level) {
  return level >= 0 && level < 4 ? kPaNames[level] : "?";
}

int paLevelGenuineDbm(int level) {
  return level >= 0 && level < 4 ? -18 + 6 * level : 99;
}

int parseGroup(const std::string& s) {
  if (s.size() == 1 && s[0] >= '1' && s[0] <= '4') return 1 << (s[0] - '1');
  if (s.size() == 3 && (s[0] | 0x20) == 'a' && (s[1] | 0x20) == 'l' && (s[2] | 0x20) == 'l')
    return kGroupAll;
  return -1;
}

std::string describePayload(const uint8_t* p, size_t len) {
  if (len != kPayloadSize) return "";
  char buf[64];
  if (p[2] == kModePosPhase1 || p[2] == kModePosPhase2) {
    snprintf(buf, sizeof(buf), "ID %02X%02X  mode %02X  mask %02X  pos %u", p[0], p[1], p[2],
             p[3], (unsigned)p[4]);
    return buf;
  }
  const char* action = actionName(p[4]);
  bool setup = p[2] == kModeSetupAdd || p[2] == kModeSetupRemove;
  if (setup && (p[4] == kActionWakeHeld || p[4] == kActionWakeRelease)) {
    bool add = p[2] == kModeSetupAdd, held = p[4] == kActionWakeHeld;
    action = add ? (held ? "pair add held" : "pair add release")
                 : (held ? "pair remove held" : "pair remove release");
  }
  snprintf(buf, sizeof(buf), "ID %02X%02X  mode %02X  group %s  %s", p[0], p[1], p[2],
           groupName(p[3]).c_str(), action);
  return buf;
}

}  // namespace rf
}  // namespace hd
