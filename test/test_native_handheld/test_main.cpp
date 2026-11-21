// SPDX-License-Identifier: GPL-2.0-or-later
// Tier 1 (host): handheld key behaviour (hd::ui::Handheld) and the key debouncer.
#include <unity.h>

#include "handheld.h"

using namespace hd::ui;

void setUp() {}
void tearDown() {}

static void assertIdleOut(const Output& o) {
  TEST_ASSERT_EQUAL(Tx::None, o.tx);
  TEST_ASSERT_EQUAL(Led::Off, o.led);
}

void test_group_bytes() {
  TEST_ASSERT_EQUAL_HEX8(0x01, groupByte(Key::G1));
  TEST_ASSERT_EQUAL_HEX8(0x02, groupByte(Key::G2));
  TEST_ASSERT_EQUAL_HEX8(0x04, groupByte(Key::G3));
  TEST_ASSERT_EQUAL_HEX8(0x08, groupByte(Key::G4));
  TEST_ASSERT_EQUAL_HEX8(0xFF, groupByte(Key::All));
  TEST_ASSERT_EQUAL_HEX8(0x00, groupByte(Key::Up));
}

void test_starts_idle() {
  Handheld h;
  assertIdleOut(h.update(0));
  TEST_ASSERT_TRUE(h.idle(0));
}

void test_red_arrow_sends_wake_and_ir_while_held() {
  Handheld h;
  h.press(Key::Up, 1000);
  Output o = h.update(1000);
  TEST_ASSERT_EQUAL(Tx::Wake, o.tx);
  TEST_ASSERT_TRUE(o.up);
  TEST_ASSERT_EQUAL_HEX8(0b0101, o.irCode);
  TEST_ASSERT_EQUAL(Led::Red, o.led);
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(3000).tx);  // still held: still sending
  h.release(Key::Up, 3000);
  assertIdleOut(h.update(3000));
  TEST_ASSERT_TRUE(h.idle(3000));
}

void test_red_down_and_channel_b_codes() {
  Handheld h;
  h.press(Key::Down, 0);
  TEST_ASSERT_EQUAL_HEX8(0b0110, h.update(0).irCode);
  TEST_ASSERT_FALSE(h.update(0).up);
  h.release(Key::Down, 10);
  h.setChannelB(true);
  h.press(Key::Up, 20);
  TEST_ASSERT_EQUAL_HEX8(0b1001, h.update(20).irCode);
  h.release(Key::Up, 30);
  h.press(Key::Down, 40);
  TEST_ASSERT_EQUAL_HEX8(0b1010, h.update(40).irCode);
}

void test_group_key_alone_sends_nothing_and_lights_green() {
  Handheld h;
  h.press(Key::G2, 0);
  Output o = h.update(10);
  TEST_ASSERT_EQUAL(Tx::None, o.tx);
  TEST_ASSERT_EQUAL(Led::Green, o.led);
  TEST_ASSERT_FALSE(h.idle(6499));
  assertIdleOut(h.update(6500));  // window over
  TEST_ASSERT_TRUE(h.idle(6500));
}

void test_arrow_in_window_sends_group_then_tail() {
  Handheld h;
  h.press(Key::G3, 0);
  h.press(Key::Down, 1000);
  Output o = h.update(1000);
  TEST_ASSERT_EQUAL(Tx::Group, o.tx);
  TEST_ASSERT_FALSE(o.up);
  TEST_ASSERT_EQUAL_HEX8(0x04, o.group);
  TEST_ASSERT_EQUAL_HEX8(0, o.irCode);  // green mode sends no IR
  TEST_ASSERT_EQUAL(Led::Green, o.led);
  h.release(Key::Down, 1600);
  o = h.update(1600);
  TEST_ASSERT_EQUAL(Tx::GroupTail, o.tx);
  TEST_ASSERT_EQUAL_HEX8(0x04, o.group);
  TEST_ASSERT_FALSE(o.up);
  TEST_ASSERT_EQUAL(Tx::GroupTail, h.update(1929).tx);
  o = h.update(1930);  // 330 ms tail over; the window is still open
  TEST_ASSERT_EQUAL(Tx::None, o.tx);
  TEST_ASSERT_EQUAL(Led::Green, o.led);
}

void test_tapped_group_arrow_still_sends_minimum() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::Up, 100);
  h.update(100);
  h.release(Key::Up, 150);
  TEST_ASSERT_EQUAL(Tx::Group, h.update(399).tx);   // held frames for 300 ms
  TEST_ASSERT_EQUAL(Tx::GroupTail, h.update(400).tx);
  TEST_ASSERT_EQUAL(Tx::None, h.update(730).tx);
}

void test_group_key_restarts_window_and_changes_group() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::G4, 5000);
  h.press(Key::Up, 11000);  // 6 s after the second key: still open
  Output o = h.update(11000);
  TEST_ASSERT_EQUAL(Tx::Group, o.tx);
  TEST_ASSERT_EQUAL_HEX8(0x08, o.group);
}

void test_all_key_group_ff() {
  Handheld h;
  h.press(Key::All, 0);
  h.press(Key::Up, 10);
  TEST_ASSERT_EQUAL_HEX8(0xFF, h.update(10).group);
}

void test_arrow_after_window_is_red() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::Up, 6500);
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(6500).tx);
}

void test_group_command_outlives_window_while_held() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::Up, 6400);
  TEST_ASSERT_EQUAL(Tx::Group, h.update(6400).tx);
  TEST_ASSERT_EQUAL(Tx::Group, h.update(8000).tx);  // window closed, command carries on
  TEST_ASSERT_EQUAL(Led::Green, h.update(8000).led);
}

void test_arrow_does_not_restart_window_by_default() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::Up, 6000);
  h.release(Key::Up, 6200);
  h.press(Key::Up, 7000);  // after the window, measured from the group key
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(7000).tx);
}

void test_arrow_restarts_window_when_configured() {
  Config c;
  c.arrowRestartsWindow = true;
  Handheld h(c);
  h.press(Key::G1, 0);
  h.press(Key::Up, 6000);
  h.release(Key::Up, 6200);
  h.press(Key::Up, 7000);
  TEST_ASSERT_EQUAL(Tx::Group, h.update(7000).tx);
}

void test_red_arrow_capped_at_5s() {
  Handheld h;
  h.press(Key::Down, 0);
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(4999).tx);
  assertIdleOut(h.update(5000));
  TEST_ASSERT_FALSE(h.idle(8000));  // key still down: not idle, but sending nothing
  h.press(Key::Up, 8000);           // another arrow is still ignored until release
  TEST_ASSERT_EQUAL(Tx::None, h.update(8000).tx);
  h.release(Key::Down, 9000);
  TEST_ASSERT_TRUE(h.idle(9000));
}

void test_group_arrow_capped_at_5s_then_tail() {
  Handheld h;
  h.press(Key::G2, 0);
  h.press(Key::Up, 100);
  TEST_ASSERT_EQUAL(Tx::Group, h.update(5099).tx);
  TEST_ASSERT_EQUAL(Tx::GroupTail, h.update(5100).tx);
  TEST_ASSERT_EQUAL(Tx::None, h.update(5430).tx);
  h.release(Key::Up, 6000);
  TEST_ASSERT_EQUAL(Tx::None, h.update(6000).tx);  // no second tail on release
}

void test_second_arrow_ignored() {
  Handheld h;
  h.press(Key::Up, 0);
  h.press(Key::Down, 100);
  TEST_ASSERT_TRUE(h.update(100).up);
  h.release(Key::Down, 200);
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(200).tx);  // releasing the ignored key changes nothing
  h.release(Key::Up, 300);
  assertIdleOut(h.update(300));
}

void test_new_group_command_cuts_tail() {
  Handheld h;
  h.press(Key::G1, 0);
  h.press(Key::Up, 100);
  h.release(Key::Up, 500);
  TEST_ASSERT_EQUAL(Tx::GroupTail, h.update(500).tx);
  h.press(Key::Down, 600);
  Output o = h.update(600);
  TEST_ASSERT_EQUAL(Tx::Group, o.tx);
  TEST_ASSERT_FALSE(o.up);
}

void test_setup_mode_pairing() {
  Handheld h;
  h.press(Key::G2, 0);  // an open window is closed by entering setup
  h.press(Key::Setup, 100);
  Output o = h.update(100);
  TEST_ASSERT_EQUAL(Led::GreenBlink, o.led);
  TEST_ASSERT_TRUE(h.inSetup());
  h.press(Key::Up, 200);  // no group chosen yet: ignored
  TEST_ASSERT_EQUAL(Tx::None, h.update(200).tx);
  h.release(Key::Up, 300);
  h.press(Key::G4, 400);
  TEST_ASSERT_EQUAL(Tx::None, h.update(400).tx);  // the group key itself sends nothing
  h.press(Key::Up, 500);
  o = h.update(500);
  TEST_ASSERT_EQUAL(Tx::Setup, o.tx);
  TEST_ASSERT_TRUE(o.up);  // UP (OPEN) adds
  TEST_ASSERT_EQUAL_HEX8(0x08, o.group);
  TEST_ASSERT_EQUAL_HEX8(0b0101, o.irCode);
  TEST_ASSERT_EQUAL(Led::GreenBlink, o.led);
  h.release(Key::Up, 1500);
  h.press(Key::Down, 2000);  // DOWN (CLOSE) removes
  o = h.update(2000);
  TEST_ASSERT_EQUAL(Tx::Setup, o.tx);
  TEST_ASSERT_FALSE(o.up);
  TEST_ASSERT_EQUAL_HEX8(0b0110, o.irCode);
  h.release(Key::Down, 2500);
  h.press(Key::Setup, 3000);  // SETUP again: leave
  TEST_ASSERT_FALSE(h.inSetup());
  assertIdleOut(h.update(3000));
}

void test_setup_times_out_from_last_key() {
  Handheld h;
  h.press(Key::Setup, 0);
  h.press(Key::G1, 20000);
  TEST_ASSERT_TRUE(h.inSetup());
  h.update(49999);
  TEST_ASSERT_TRUE(h.inSetup());
  h.update(50000);  // 30 s after the group key
  TEST_ASSERT_FALSE(h.inSetup());
  h.press(Key::Up, 50100);  // back to normal: red mode
  TEST_ASSERT_EQUAL(Tx::Wake, h.update(50100).tx);
}

void test_window_survives_millis_wrap() {
  Handheld h;
  const uint32_t t0 = 0xFFFFF000u;  // 4096 ms before millis() wraps
  h.press(Key::G1, t0);
  h.press(Key::Up, t0 + 6000);  // wraps past zero
  TEST_ASSERT_EQUAL(Tx::Group, h.update(t0 + 6000).tx);
}

void test_debouncer_ignores_bounce() {
  Debouncer d(25);
  TEST_ASSERT_FALSE(d.update(true, 0));
  TEST_ASSERT_FALSE(d.update(false, 5));   // bounce
  TEST_ASSERT_FALSE(d.update(true, 10));
  TEST_ASSERT_FALSE(d.update(true, 34));   // 24 ms since the last change
  TEST_ASSERT_TRUE(d.update(true, 35));    // stable for 25 ms: pressed
  TEST_ASSERT_TRUE(d.pressed());
  TEST_ASSERT_FALSE(d.update(true, 100));  // one edge only
  TEST_ASSERT_FALSE(d.update(false, 200));
  TEST_ASSERT_TRUE(d.update(false, 225));  // released
  TEST_ASSERT_FALSE(d.pressed());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_group_bytes);
  RUN_TEST(test_starts_idle);
  RUN_TEST(test_red_arrow_sends_wake_and_ir_while_held);
  RUN_TEST(test_red_down_and_channel_b_codes);
  RUN_TEST(test_group_key_alone_sends_nothing_and_lights_green);
  RUN_TEST(test_arrow_in_window_sends_group_then_tail);
  RUN_TEST(test_tapped_group_arrow_still_sends_minimum);
  RUN_TEST(test_group_key_restarts_window_and_changes_group);
  RUN_TEST(test_all_key_group_ff);
  RUN_TEST(test_arrow_after_window_is_red);
  RUN_TEST(test_group_command_outlives_window_while_held);
  RUN_TEST(test_arrow_does_not_restart_window_by_default);
  RUN_TEST(test_arrow_restarts_window_when_configured);
  RUN_TEST(test_red_arrow_capped_at_5s);
  RUN_TEST(test_group_arrow_capped_at_5s_then_tail);
  RUN_TEST(test_second_arrow_ignored);
  RUN_TEST(test_new_group_command_cuts_tail);
  RUN_TEST(test_setup_mode_pairing);
  RUN_TEST(test_setup_times_out_from_last_key);
  RUN_TEST(test_window_survives_millis_wrap);
  RUN_TEST(test_debouncer_ignores_bounce);
  return UNITY_END();
}
