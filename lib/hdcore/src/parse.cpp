#include "parse.h"

#include <cctype>

namespace hd {

std::vector<std::string> tokenize(const std::string& line, size_t maxTokens) {
  std::vector<std::string> out;
  size_t i = 0;
  const size_t n = line.size();
  auto isWs = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  while (i < n && out.size() < maxTokens) {
    while (i < n && isWs(line[i])) i++;
    if (i >= n) break;
    size_t start = i;
    while (i < n && !isWs(line[i])) i++;
    out.emplace_back(line.substr(start, i - start));
  }
  return out;
}

std::string toLower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int parseHex(const std::string& in, uint8_t* out, size_t maxOut) {
  std::string s = in;
  if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s = s.substr(2);
  if (s.empty() || s.size() % 2 != 0) return -1;
  size_t count = s.size() / 2;
  if (count > maxOut) return -1;
  for (size_t i = 0; i < count; i++) {
    int hi = hexNibble(s[2 * i]);
    int lo = hexNibble(s[2 * i + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[i] = static_cast<uint8_t>((hi << 4) | lo);
  }
  return static_cast<int>(count);
}

Rate parseRate(const std::string& s) {
  std::string l = toLower(s);
  if (l == "250k") return Rate::k250k;
  if (l == "1m") return Rate::k1M;
  if (l == "2m") return Rate::k2M;
  return Rate::kInvalid;
}

const char* rateName(Rate r) {
  switch (r) {
    case Rate::k250k: return "250k";
    case Rate::k1M: return "1m";
    case Rate::k2M: return "2m";
    default: return "?";
  }
}

}  // namespace hd
