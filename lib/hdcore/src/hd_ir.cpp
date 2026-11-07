#include "hd_ir.h"

#include <cstdlib>

#include "parse.h"

namespace hd {
namespace ir {

std::string codeToBits(uint8_t code) {
  std::string s;
  for (int i = kBits - 1; i >= 0; i--) s += ((code >> i) & 1) ? '1' : '0';
  return s;
}

int parseCode(const std::string& in) {
  std::string s = toLower(in);
  if (s == "up") return kCodeUp;
  if (s == "down") return kCodeDown;
  if (s.size() == static_cast<size_t>(kBits) && s.find_first_not_of("01") == std::string::npos) {
    int v = 0;
    for (char c : s) v = (v << 1) | (c == '1');
    return v;
  }
  if (s.size() > 2 && s[0] == '0' && s[1] == 'x') {
    char* end = nullptr;
    long v = std::strtol(s.c_str() + 2, &end, 16);
    if (*end == '\0' && v >= 0 && v < (1 << kBits)) return static_cast<int>(v);
  }
  return -1;
}

const char* codeName(uint8_t code) {
  if (code == kCodeUp) return "UP";
  if (code == kCodeDown) return "DOWN";
  return "?";
}

std::vector<uint16_t> encodeFrame(uint8_t code) {
  std::vector<uint16_t> f = {kHdrMarkUs, kHdrSpaceUs};
  for (int i = kBits - 1; i >= 0; i--) {
    f.push_back(kBitMarkUs);
    f.push_back(((code >> i) & 1) ? kOneSpaceUs : kZeroSpaceUs);
  }
  return f;
}

std::vector<uint16_t> encodeBurst(uint8_t code, int frames) {
  if (frames < 1) frames = 1;
  std::vector<uint16_t> one = encodeFrame(code);
  std::vector<uint16_t> out;
  out.reserve(one.size() * frames);
  for (int i = 0; i < frames; i++) out.insert(out.end(), one.begin(), one.end());
  return out;
}

int framesForDuration(uint32_t durationMs) {
  uint32_t n = (durationMs * 1000 + kFramePeriodUs - 1) / kFramePeriodUs;
  return n < 1 ? 1 : static_cast<int>(n);
}

static inline bool isLong(uint16_t v) { return v > kLongThresholdUs; }

std::vector<uint8_t> decodeFrames(const uint16_t* t, size_t n) {
  std::vector<uint8_t> codes;
  size_t i = 0;
  while (i + kEntriesPerFrame <= n) {
    // Header: two long durations followed by four short marks at i+2, +4, +6, +8.
    bool ok = isLong(t[i]) && isLong(t[i + 1]);
    for (int b = 0; ok && b < kBits; b++) ok = !isLong(t[i + 2 + 2 * b]);
    if (!ok) {
      i++;  // resync: also handles phase-shifted captures
      continue;
    }
    uint8_t code = 0;
    for (int b = 0; b < kBits; b++) code = (code << 1) | (isLong(t[i + 3 + 2 * b]) ? 1 : 0);
    codes.push_back(code);
    i += kEntriesPerFrame;
  }
  return codes;
}

Summary summarize(const uint16_t* t, size_t n) {
  Summary s;
  std::vector<uint8_t> codes = decodeFrames(t, n);
  if (codes.empty()) return s;
  int counts[1 << kBits] = {0};
  for (uint8_t c : codes) counts[c]++;
  int best = 0;
  for (int c = 1; c < (1 << kBits); c++)
    if (counts[c] > counts[best]) best = c;
  s.code = best;
  s.frames = counts[best];
  s.otherFrames = static_cast<int>(codes.size()) - counts[best];
  return s;
}

}  // namespace ir
}  // namespace hd
