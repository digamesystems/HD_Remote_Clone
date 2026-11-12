// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Hunter Douglas Platinum IR frame format (point-and-press / red mode).
// Reverse engineered Nov 7, 2026 from the original remote; see CLAUDE.md.
//
//   header: mark ~2.75 ms, space ~2.75 ms
//   4 bits, MSB first: mark ~1.03 ms, then space ~1.03 ms (0) or ~2.75 ms (1)
//   frame period 17.19 ms; frames repeat back to back while the button is held
//   switch A: UP arrow = 0b0101, DOWN arrow = 0b0110
//   switch B: UP arrow = 0b1001, DOWN arrow = 0b1010
//   inferred layout: bits 3-2 = IR channel (01 A, 10 B), bits 1-0 = command (01 OPEN, 10 CLOSE)
//
// Timings below are derived from receiver-side captures (TSOP/VS1838B output),
// which distort marks/spaces slightly. Refine from the analyzer on the remote's
// LED drive when available. Carrier: 40 kHz per manual (not yet measured).

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hd {
namespace ir {

constexpr uint16_t kHdrMarkUs = 2750;
constexpr uint16_t kHdrSpaceUs = 2750;
constexpr uint16_t kBitMarkUs = 1030;
constexpr uint16_t kZeroSpaceUs = 1030;
constexpr uint16_t kOneSpaceUs = 2750;
constexpr uint16_t kCarrierKhz = 40;
constexpr int kBits = 4;
constexpr int kEntriesPerFrame = 2 + 2 * kBits;  // 10
constexpr uint32_t kFramePeriodUs = 17190;       // measured (codes seen have two 1-bits)

// Durations above this are "long" (header or 1-bit space).
constexpr uint16_t kLongThresholdUs = 1900;

constexpr uint8_t kCodeUp = 0b0101;
constexpr uint8_t kCodeDown = 0b0110;
constexpr uint8_t kCodeUpB = 0b1001;
constexpr uint8_t kCodeDownB = 0b1010;

// Holding OPEN for 6 s at a fully open shade clears its stops and motor limits
// (Platinum manual p. 21), so OPEN sends stop short of that.
constexpr uint32_t kMaxOpenSendMs = 5000;
constexpr uint32_t kMaxSendMs = 10000;

// "0101" etc.
std::string codeToBits(uint8_t code);
// Accepts "up", "down" (channel A), a 4-char binary string ("0110"), or hex ("0x6"). Returns -1 if invalid.
int parseCode(const std::string& s);
// "A UP", "A DOWN", "B UP", "B DOWN" or "?"
const char* codeName(uint8_t code);
// Longest allowed send for this code: kMaxOpenSendMs if its command bits are OPEN, else kMaxSendMs.
uint32_t maxSendMs(uint8_t code);

// One frame: 10 entries, alternating mark/space, starting with the header mark.
std::vector<uint16_t> encodeFrame(uint8_t code);
// Back-to-back frames; frames >= 1.
std::vector<uint16_t> encodeBurst(uint8_t code, int frames);
// Number of frames needed to cover durationMs (at least 1).
int framesForDuration(uint32_t durationMs);

// Decodes every complete, well-formed frame in a raw timing capture
// (alternating durations in microseconds). Tolerates captures that start
// mid-frame or on a space (phase-shifted), and ignores truncated/garbled frames.
std::vector<uint8_t> decodeFrames(const uint16_t* t, size_t n);

struct Summary {
  int code = -1;       // most common code, or -1 if no frames
  int frames = 0;      // frames with that code
  int otherFrames = 0; // well-formed frames with a different code
};
Summary summarize(const uint16_t* t, size_t n);

}  // namespace ir
}  // namespace hd
