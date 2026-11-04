#include <unity.h>
#include <string.h>
#include "logic/need_hint.h"
#include "logic/speech.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000UL;

static MoodState fine() {
  MoodState s;
  mood_init(s, Needs{80, 80, 80, 80});
  return s;
}

void test_edge_one_bubble_then_15min() {
  NeedHintState st; MoodState s = fine();
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, 0).due);
  s.n.satiety = 20;
  NeedHintOut o = need_hint_update(st, s, 1000);
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)o.due);
  need_hint_shown(st, Need::HUNGRY, 1000);
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, 2000).due);
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, 1000 + 15 * MIN - 1).due);
  o = need_hint_update(st, s, 1000 + 15 * MIN);
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)o.due);
  TEST_ASSERT_EQUAL(need_bit(Need::HUNGRY), o.mask);
}

void test_hysteresis() {
  NeedHintState st; MoodState s = fine();
  s.n.satiety = 24;
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)need_hint_update(st, s, 0).due);
  need_hint_shown(st, Need::HUNGRY, 0);
  s.n.satiety = 26;
  NeedHintOut o = need_hint_update(st, s, 1000);
  TEST_ASSERT_EQUAL(need_bit(Need::HUNGRY), o.mask);   // noch aktiv
  s.n.satiety = 24;
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, 2000).due);   // keine zweite Blase
  s.n.satiety = 30;
  TEST_ASSERT_EQUAL(need_bit(Need::HUNGRY), need_hint_update(st, s, 3000).mask);   // 30 ist noch nicht ueber 30
  s.n.satiety = 31;
  TEST_ASSERT_EQUAL(0, need_hint_update(st, s, 4000).mask);
  s.n.satiety = 24;
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)need_hint_update(st, s, 5000).due);   // neue Flanke
}

void test_mask_all_low_bubble_most_urgent() {
  NeedHintState st; MoodState s = fine();
  s.n.satiety = 10; s.n.energy = 10; s.n.affection = 10; s.n.fun = 10;
  NeedHintOut o = need_hint_update(st, s, 0);
  TEST_ASSERT_EQUAL(need_bit(Need::HUNGRY) | need_bit(Need::SLEEPY) | need_bit(Need::SAD) | need_bit(Need::BORED), o.mask);
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)o.due);
  need_hint_shown(st, Need::HUNGRY, 0);
  TEST_ASSERT_EQUAL((int)Need::SLEEPY, (int)need_hint_update(st, s, 10).due);   // naechstes, noch nicht gezeigtes
}

void test_sick_before_hungry() {
  NeedHintState st; MoodState s = fine();
  s.n.satiety = 10; s.sick = true;
  NeedHintOut o = need_hint_update(st, s, 0);
  TEST_ASSERT_EQUAL((int)Need::SICK, (int)o.due);
  TEST_ASSERT_TRUE(o.mask & need_bit(Need::HUNGRY));
  s.sick = false;
  o = need_hint_update(st, s, 10);
  TEST_ASSERT_EQUAL(0, o.mask & need_bit(Need::SICK));
  TEST_ASSERT_EQUAL((int)Need::HUNGRY, (int)o.due);
}

void test_pending_until_acknowledged() {
  NeedHintState st; MoodState s = fine();
  s.n.fun = 5;
  for (uint32_t t = 0; t < 5; t++)
    TEST_ASSERT_EQUAL((int)Need::BORED, (int)need_hint_update(st, s, t * MIN).due);   // bleibt fällig
  need_hint_shown(st, Need::BORED, 5 * MIN);
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, 6 * MIN).due);
}

void test_pending_cleared_when_fulfilled() {
  NeedHintState st; MoodState s = fine();
  s.n.affection = 5;
  need_hint_update(st, s, 0);
  s.n.affection = 90;
  NeedHintOut o = need_hint_update(st, s, 10);
  TEST_ASSERT_EQUAL(0, o.mask);
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)o.due);
}

void test_wraparound() {
  NeedHintState st; MoodState s = fine();
  const uint32_t t0 = 0xFFFFFFFFu - 5 * MIN;
  s.n.energy = 10;
  TEST_ASSERT_EQUAL((int)Need::SLEEPY, (int)need_hint_update(st, s, t0).due);
  need_hint_shown(st, Need::SLEEPY, t0);
  TEST_ASSERT_EQUAL((int)Need::COUNT, (int)need_hint_update(st, s, t0 + 10 * MIN).due);   // ueber den Ueberlauf, 10 min
  TEST_ASSERT_EQUAL((int)Need::SLEEPY, (int)need_hint_update(st, s, t0 + 15 * MIN).due);   // 15 min
}

void test_texts_fit() {
  for (int i = 0; i < NEED_COUNT; i++) {
    TEST_ASSERT_TRUE(strlen(need_text((Need)i)) > 0);
    TEST_ASSERT_TRUE(strlen(need_text((Need)i)) <= SPEECH_CHARACTER_LIMIT);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_edge_one_bubble_then_15min);
  RUN_TEST(test_hysteresis);
  RUN_TEST(test_mask_all_low_bubble_most_urgent);
  RUN_TEST(test_sick_before_hungry);
  RUN_TEST(test_pending_until_acknowledged);
  RUN_TEST(test_pending_cleared_when_fulfilled);
  RUN_TEST(test_wraparound);
  RUN_TEST(test_texts_fit);
  return UNITY_END();
}
