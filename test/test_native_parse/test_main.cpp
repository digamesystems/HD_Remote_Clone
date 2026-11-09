// Tier 1 (host): command-line and value parsing.
#include <unity.h>

#include "key_parser.h"
#include "parse.h"

using namespace hd;

void setUp() {}
void tearDown() {}

void test_tokenize_basic() {
  auto t = tokenize("play 3 5 40");
  TEST_ASSERT_EQUAL(4, t.size());
  TEST_ASSERT_EQUAL_STRING("play", t[0].c_str());
  TEST_ASSERT_EQUAL_STRING("40", t[3].c_str());
}

void test_tokenize_extra_whitespace_and_crlf() {
  auto t = tokenize("  ch\t 7  \r\n");
  TEST_ASSERT_EQUAL(2, t.size());
  TEST_ASSERT_EQUAL_STRING("ch", t[0].c_str());
  TEST_ASSERT_EQUAL_STRING("7", t[1].c_str());
}

void test_tokenize_empty() {
  TEST_ASSERT_EQUAL(0, tokenize("").size());
  TEST_ASSERT_EQUAL(0, tokenize("   \r\n").size());
}

void test_tokenize_max_tokens() {
  auto t = tokenize("a b c d e f", 4);
  TEST_ASSERT_EQUAL(4, t.size());
  TEST_ASSERT_EQUAL_STRING("d", t[3].c_str());
}

void test_to_lower() { TEST_ASSERT_EQUAL_STRING("promisc aa", toLower("PROMISC Aa").c_str()); }

void test_parse_hex_valid() {
  uint8_t b[5];
  TEST_ASSERT_EQUAL(3, parseHex("c12001", b, sizeof(b)));
  TEST_ASSERT_EQUAL_HEX8(0xC1, b[0]);
  TEST_ASSERT_EQUAL_HEX8(0x20, b[1]);
  TEST_ASSERT_EQUAL_HEX8(0x01, b[2]);
}

void test_parse_hex_prefix_and_case() {
  uint8_t b[5];
  TEST_ASSERT_EQUAL(2, parseHex("0x00Aa", b, sizeof(b)));
  TEST_ASSERT_EQUAL_HEX8(0x00, b[0]);
  TEST_ASSERT_EQUAL_HEX8(0xAA, b[1]);
}

void test_parse_hex_rejects_bad_input() {
  uint8_t b[5];
  TEST_ASSERT_EQUAL(-1, parseHex("", b, sizeof(b)));
  TEST_ASSERT_EQUAL(-1, parseHex("abc", b, sizeof(b)));          // odd length
  TEST_ASSERT_EQUAL(-1, parseHex("zz", b, sizeof(b)));           // bad chars
  TEST_ASSERT_EQUAL(-1, parseHex("0011223344aa", b, sizeof(b)));  // too long for buffer
  TEST_ASSERT_EQUAL(-1, parseHex("0x", b, sizeof(b)));
}

void test_parse_rate() {
  TEST_ASSERT_EQUAL(static_cast<int>(Rate::k250k), static_cast<int>(parseRate("250k")));
  TEST_ASSERT_EQUAL(static_cast<int>(Rate::k1M), static_cast<int>(parseRate("1M")));
  TEST_ASSERT_EQUAL(static_cast<int>(Rate::k2M), static_cast<int>(parseRate("2m")));
  TEST_ASSERT_EQUAL(static_cast<int>(Rate::kInvalid), static_cast<int>(parseRate("3m")));
}

void test_rate_values_match_rf24_enum() {
  // RF24: RF24_1MBPS=0, RF24_2MBPS=1, RF24_250KBPS=2. rf_tools casts between them.
  TEST_ASSERT_EQUAL(0, static_cast<int>(Rate::k1M));
  TEST_ASSERT_EQUAL(1, static_cast<int>(Rate::k2M));
  TEST_ASSERT_EQUAL(2, static_cast<int>(Rate::k250k));
}

static KeyEvent feedAll(KeyParser& p, const char* s) {
  KeyEvent last = KeyEvent::Pass;
  for (; *s; s++) last = p.feed(*s);
  return last;
}

void test_key_parser_arrows() {
  KeyParser p;
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[A") == KeyEvent::Up);
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[B") == KeyEvent::Down);
  TEST_ASSERT_TRUE(feedAll(p, "\x1bOA") == KeyEvent::Up);  // application cursor mode
  TEST_ASSERT_TRUE(feedAll(p, "\x1bOB") == KeyEvent::Down);
}

void test_key_parser_passes_plain_text() {
  KeyParser p;
  for (const char* s = "send AB [O]\n"; *s; s++) {
    TEST_ASSERT_TRUE(p.feed(*s) == KeyEvent::Pass);
  }
}

void test_key_parser_swallows_other_sequences() {
  KeyParser p;
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[D") == KeyEvent::Consumed);    // left arrow
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[3~") == KeyEvent::Consumed);   // delete
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[1;5C") == KeyEvent::Consumed);  // ctrl+right
  TEST_ASSERT_TRUE(p.feed('x') == KeyEvent::Pass);                 // back to normal
}

void test_key_parser_lone_escape_keeps_next_char() {
  KeyParser p;
  TEST_ASSERT_TRUE(p.feed(0x1b) == KeyEvent::Consumed);
  TEST_ASSERT_TRUE(p.feed('s') == KeyEvent::Pass);
  TEST_ASSERT_TRUE(p.feed('e') == KeyEvent::Pass);
}

void test_key_parser_newline_aborts_sequence() {
  KeyParser p;
  TEST_ASSERT_TRUE(feedAll(p, "\x1b[") == KeyEvent::Consumed);
  TEST_ASSERT_TRUE(p.feed('\n') == KeyEvent::Pass);
  TEST_ASSERT_TRUE(p.feed('A') == KeyEvent::Pass);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_key_parser_arrows);
  RUN_TEST(test_key_parser_passes_plain_text);
  RUN_TEST(test_key_parser_swallows_other_sequences);
  RUN_TEST(test_key_parser_lone_escape_keeps_next_char);
  RUN_TEST(test_key_parser_newline_aborts_sequence);
  RUN_TEST(test_tokenize_basic);
  RUN_TEST(test_tokenize_extra_whitespace_and_crlf);
  RUN_TEST(test_tokenize_empty);
  RUN_TEST(test_tokenize_max_tokens);
  RUN_TEST(test_to_lower);
  RUN_TEST(test_parse_hex_valid);
  RUN_TEST(test_parse_hex_prefix_and_case);
  RUN_TEST(test_parse_hex_rejects_bad_input);
  RUN_TEST(test_parse_rate);
  RUN_TEST(test_rate_values_match_rf24_enum);
  return UNITY_END();
}
