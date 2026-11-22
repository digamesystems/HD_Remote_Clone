// SPDX-License-Identifier: GPL-2.0-or-later
// Tier 1 (host): the transmit scheduler (hd::tx::Scheduler), alone and driven
// by the handheld key logic.
#include <unity.h>

#include "handheld.h"
#include "tx_scheduler.h"

using namespace hd;
using tx::Scheduler;
using tx::Step;
using tx::StepKind;
using ui::Output;
using ui::Tx;

static const uint8_t kId0 = 0x12, kId1 = 0x34;  // made-up test ID

void setUp() {}
void tearDown() {}

static Output out(Tx t, bool up = true, uint8_t group = 0, uint8_t ir = 0) {
  Output o;
  o.tx = t;
  o.up = up;
  o.group = group;
  o.irCode = ir;
  return o;
}

static void assertPayload(const Step& s, uint8_t mode, uint8_t group, uint8_t action) {
  const uint8_t want[5] = {kId0, kId1, mode, group, action};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(want, s.payload, 5);
}

static Scheduler sched(const tx::Config& c = tx::Config()) {
  Scheduler s(c);
  s.setId(kId0, kId1);
  return s;
}

void test_none_sends_nothing() {
  Scheduler s = sched();
  Step st = s.next(out(Tx::None));
  TEST_ASSERT_EQUAL(StepKind::None, st.kind);
  TEST_ASSERT_EQUAL_UINT32(0, st.ms);
}

void test_wake_interleaves_rf_and_ir_rotating_channels() {
  Scheduler s = sched();
  const Output o = out(Tx::Wake, true, 0, 0b0101);
  const uint8_t chans[] = {33, 52, 71, 33};
  for (uint8_t ch : chans) {
    Step rf = s.next(o);
    TEST_ASSERT_EQUAL(StepKind::Rf, rf.kind);
    TEST_ASSERT_EQUAL_UINT8(ch, rf.channel);
    TEST_ASSERT_EQUAL_UINT32(50, rf.ms);
    assertPayload(rf, 0x01, 0x00, 0x80);  // ID 01 00 80
    Step ir = s.next(o);
    TEST_ASSERT_EQUAL(StepKind::Ir, ir.kind);
    TEST_ASSERT_EQUAL_HEX8(0b0101, ir.irCode);
    TEST_ASSERT_EQUAL_UINT32(50, ir.ms);
  }
}

void test_wake_gap_between_rf_and_ir() {
  tx::Config c;
  c.gapMs = 100;
  Scheduler s = sched(c);
  const Output o = out(Tx::Wake, false, 0, 0b0110);
  TEST_ASSERT_EQUAL(StepKind::Rf, s.next(o).kind);
  Step g = s.next(o);
  TEST_ASSERT_EQUAL(StepKind::Gap, g.kind);
  TEST_ASSERT_EQUAL_UINT32(100, g.ms);
  TEST_ASSERT_EQUAL(StepKind::Ir, s.next(o).kind);
  Step rf = s.next(o);
  TEST_ASSERT_EQUAL(StepKind::Rf, rf.kind);
  TEST_ASSERT_EQUAL_UINT8(52, rf.channel);
}

void test_wake_off_sends_ir_only() {
  tx::Config c;
  c.wake = false;
  Scheduler s = sched(c);
  const Output o = out(Tx::Wake, true, 0, 0b0101);
  for (int i = 0; i < 4; i++) TEST_ASSERT_EQUAL(StepKind::Ir, s.next(o).kind);
}

void test_setup_frames_with_ir() {
  Scheduler s = sched();
  Step add = s.next(out(Tx::Setup, true, 0x08, 0b0101));
  TEST_ASSERT_EQUAL(StepKind::Rf, add.kind);
  TEST_ASSERT_EQUAL_UINT8(33, add.channel);
  assertPayload(add, 0x30, 0x08, 0x80);  // add: 30
  TEST_ASSERT_EQUAL_HEX8(0b0101, s.next(out(Tx::Setup, true, 0x08, 0b0101)).irCode);

  Step rem = s.next(out(Tx::Setup, false, 0x08, 0b0110));  // new command: restarts on ch 33
  TEST_ASSERT_EQUAL_UINT8(33, rem.channel);
  assertPayload(rem, 0x31, 0x08, 0x80);  // remove: 31
}

void test_setup_ignores_wake_off() {
  tx::Config c;
  c.wake = false;
  Scheduler s = sched(c);
  TEST_ASSERT_EQUAL(StepKind::Rf, s.next(out(Tx::Setup, true, 0x01, 0b0101)).kind);
}

void test_group_and_tail_rotate_in_short_chunks() {
  Scheduler s = sched();
  Step held = s.next(out(Tx::Group, true, 0x04));
  TEST_ASSERT_EQUAL(StepKind::Rf, held.kind);
  TEST_ASSERT_EQUAL_UINT8(0, held.channel);  // rotate packet by packet
  TEST_ASSERT_EQUAL_UINT32(20, held.ms);
  assertPayload(held, 0x20, 0x04, 0xA1);
  assertPayload(s.next(out(Tx::Group, false, 0xFF)), 0x20, 0xFF, 0xA2);
  assertPayload(s.next(out(Tx::GroupTail, true, 0x04)), 0x20, 0x04, 0x21);
  assertPayload(s.next(out(Tx::GroupTail, false, 0x04)), 0x20, 0x04, 0x22);
}

void test_new_transmission_restarts_pattern() {
  Scheduler s = sched();
  const Output up = out(Tx::Wake, true, 0, 0b0101);
  s.next(up);  // RF 33
  s.next(up);  // IR
  TEST_ASSERT_EQUAL_UINT8(52, s.next(up).channel);
  TEST_ASSERT_EQUAL(StepKind::None, s.next(out(Tx::None)).kind);
  Step again = s.next(up);  // after None: from the top
  TEST_ASSERT_EQUAL(StepKind::Rf, again.kind);
  TEST_ASSERT_EQUAL_UINT8(33, again.channel);
  s.next(up);
  Step down = s.next(out(Tx::Wake, false, 0, 0b0110));  // other direction: from the top
  TEST_ASSERT_EQUAL(StepKind::Rf, down.kind);
  TEST_ASSERT_EQUAL_UINT8(33, down.channel);
  s.next(out(Tx::Wake, false, 0, 0b0110));
  s.reset();
  TEST_ASSERT_EQUAL(StepKind::Rf, s.next(out(Tx::Wake, false, 0, 0b0110)).kind);
}

// --- driven by the handheld logic, with time advancing by each step's length ---

struct Totals {
  uint32_t rfMs = 0, irMs = 0, heldMs = 0, tailMs = 0, endMs = 0;
};

// Runs from `t` until the scheduler has nothing to send, releasing `key` at
// `releaseAt`. Returns time spent per kind and when sending stopped.
static Totals run(ui::Handheld& h, Scheduler& s, ui::Key key, uint32_t t, uint32_t releaseAt) {
  Totals tot;
  bool released = false;
  for (int guard = 0; guard < 10000; guard++) {
    if (!released && t >= releaseAt) {
      h.release(key, t);
      released = true;
    }
    Step st = s.next(h.update(t));
    if (st.kind == StepKind::None) {
      if (released) break;
      t += 1;  // nothing yet (e.g. capped, key still held): let time pass
      continue;
    }
    if (st.kind == StepKind::Rf) {
      tot.rfMs += st.ms;
      if (st.payload[2] == 0x20) (st.payload[4] & 0x80 ? tot.heldMs : tot.tailMs) += st.ms;
    }
    if (st.kind == StepKind::Ir) tot.irMs += st.ms;
    t += st.ms;
    tot.endMs = t;
  }
  return tot;
}

void test_red_hold_sends_steps_until_release() {
  ui::Handheld h;
  Scheduler s = sched();
  h.press(ui::Key::Up, 0);
  Totals t = run(h, s, ui::Key::Up, 0, 1000);
  // 100 ms per RF + IR cycle: about 10 cycles; the step running at release finishes
  TEST_ASSERT_UINT32_WITHIN(50, 500, t.rfMs);
  TEST_ASSERT_UINT32_WITHIN(50, 500, t.irMs);
  TEST_ASSERT_UINT32_WITHIN(50, 1000, t.endMs);
}

void test_red_hold_capped_at_5s() {
  ui::Handheld h;
  Scheduler s = sched();
  h.press(ui::Key::Down, 0);
  Totals t = run(h, s, ui::Key::Down, 0, 8000);  // held 8 s
  TEST_ASSERT_UINT32_WITHIN(50, 5000, t.endMs);
}

void test_group_tap_sends_minimum_hold_then_tail() {
  ui::Handheld h;
  Scheduler s = sched();
  h.press(ui::Key::G3, 0);
  h.release(ui::Key::G3, 50);
  h.press(ui::Key::Up, 100);
  Totals t = run(h, s, ui::Key::Up, 100, 150);  // a 50 ms tap
  TEST_ASSERT_UINT32_WITHIN(20, 300, t.heldMs);  // A1 for the 300 ms minimum
  TEST_ASSERT_UINT32_WITHIN(20, 330, t.tailMs);  // then the 21 tail
  TEST_ASSERT_EQUAL_UINT32(0, t.irMs);           // green mode: RF only
}

void test_group_hold_sends_until_release_then_tail() {
  ui::Handheld h;
  Scheduler s = sched();
  h.press(ui::Key::G1, 0);
  h.release(ui::Key::G1, 50);
  h.press(ui::Key::Down, 1000);
  Totals t = run(h, s, ui::Key::Down, 1000, 3000);  // held 2 s
  TEST_ASSERT_UINT32_WITHIN(20, 2000, t.heldMs);
  TEST_ASSERT_UINT32_WITHIN(20, 330, t.tailMs);
}

// The 5 s cap (decided 2026-11-18, John) on a group press, through the
// scheduler: A2 stops at 5 s while the key is still held, the tail follows,
// and nothing more goes out until the key is released.
void test_group_hold_capped_at_5s_then_tail() {
  ui::Handheld h;
  Scheduler s = sched();
  h.press(ui::Key::G2, 0);
  h.release(ui::Key::G2, 50);
  h.press(ui::Key::Down, 1000);
  Totals t = run(h, s, ui::Key::Down, 1000, 9000);  // held 8 s
  TEST_ASSERT_UINT32_WITHIN(20, 5000, t.heldMs);
  TEST_ASSERT_UINT32_WITHIN(20, 330, t.tailMs);
  TEST_ASSERT_EQUAL_UINT32(0, t.irMs);
  TEST_ASSERT_UINT32_WITHIN(40, 1000 + 5000 + 330, t.endMs);  // silent from then to release
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_none_sends_nothing);
  RUN_TEST(test_wake_interleaves_rf_and_ir_rotating_channels);
  RUN_TEST(test_wake_gap_between_rf_and_ir);
  RUN_TEST(test_wake_off_sends_ir_only);
  RUN_TEST(test_setup_frames_with_ir);
  RUN_TEST(test_setup_ignores_wake_off);
  RUN_TEST(test_group_and_tail_rotate_in_short_chunks);
  RUN_TEST(test_new_transmission_restarts_pattern);
  RUN_TEST(test_red_hold_sends_steps_until_release);
  RUN_TEST(test_red_hold_capped_at_5s);
  RUN_TEST(test_group_tap_sends_minimum_hold_then_tail);
  RUN_TEST(test_group_hold_sends_until_release_then_tail);
  RUN_TEST(test_group_hold_capped_at_5s_then_tail);
  return UNITY_END();
}
