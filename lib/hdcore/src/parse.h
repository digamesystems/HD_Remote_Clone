// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
// Pure C++ parsing helpers (no Arduino) — shared by firmware and host tests.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hd {

// Split on spaces/tabs, ignoring leading/trailing whitespace and CR/LF.
// At most maxTokens are returned; the rest of the line is dropped.
std::vector<std::string> tokenize(const std::string& line, size_t maxTokens = 8);

std::string toLower(std::string s);

// True if the first non-blank character is '#': a note for the serial log.
inline bool isComment(const std::string& line) {
  size_t i = line.find_first_not_of(" \t");
  return i != std::string::npos && line[i] == '#';
}

// Parse an even-length hex string (no separators, case-insensitive, optional
// leading "0x") into bytes. Returns the byte count, or -1 on error.
int parseHex(const std::string& s, uint8_t* out, size_t maxOut);

// nRF24 data rates, numbered to match RF24's rf24_datarate_e.
enum class Rate : int { k1M = 0, k2M = 1, k250k = 2, kInvalid = -1 };
Rate parseRate(const std::string& s);  // "250k" | "1m" | "2m" (case-insensitive)
const char* rateName(Rate r);

}  // namespace hd
