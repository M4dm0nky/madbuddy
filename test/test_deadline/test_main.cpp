#include <unity.h>
#include "logic/deadline.h"
void setUp() {}
void tearDown() {}

void test_never_armed_is_never_active() {
  Deadline d{};
  TEST_ASSERT_FALSE(d.active(0));
  TEST_ASSERT_FALSE(d.active(12345));
  TEST_ASSERT_FALSE(d.active(0x80000000UL));
  TEST_ASSERT_FALSE(d.active(0xFFFFFFFFUL));
}
void test_active_until_expiry() {
  Deadline d{};
  d.arm(1000, 500);
  TEST_ASSERT_TRUE(d.active(1000));
  TEST_ASSERT_TRUE(d.active(1499));
  TEST_ASSERT_FALSE(d.active(1500));
  TEST_ASSERT_FALSE(d.active(1501));
}
void test_stays_off_after_expiry_beyond_2_31() {
  Deadline d{};
  d.arm(1000, 500);
  TEST_ASSERT_TRUE(d.active(1200));
  TEST_ASSERT_FALSE(d.active(1500));            // Ablauf bemerkt
  TEST_ASSERT_FALSE(d.active(1500 + 0x7FFFFFFFUL));   // 24,8 Tage danach
  TEST_ASSERT_FALSE(d.active(1500 + 0x80000000UL));   // das alte (int32_t)-Muster wäre jetzt wieder wahr
  TEST_ASSERT_FALSE(d.active(1500 + 0xC0000000UL));
}
void test_expiry_not_polled_for_long_still_off() {
  Deadline d{};
  d.arm(1000, 500);
  TEST_ASSERT_FALSE(d.active(1000UL + 0x80000000UL + 7));   // nie zwischendurch abgefragt
}
void test_wraparound_of_millis() {
  Deadline d{};
  d.arm(0xFFFFFF00UL, 0x200);
  TEST_ASSERT_TRUE(d.active(0xFFFFFFF0UL));
  TEST_ASSERT_TRUE(d.active(0x000000FFUL));
  TEST_ASSERT_FALSE(d.active(0x00000100UL));
}
void test_rearm_and_clear() {
  Deadline d{};
  d.arm(0, 100);
  TEST_ASSERT_FALSE(d.active(100));
  d.arm(5000, 100);
  TEST_ASSERT_TRUE(d.active(5050));
  d.clear();
  TEST_ASSERT_FALSE(d.active(5050));
}
void test_high_now_arm() {
  Deadline d{};
  d.arm(0x90000000UL, 4000);
  TEST_ASSERT_TRUE(d.active(0x90000000UL + 3999));
  TEST_ASSERT_FALSE(d.active(0x90000000UL + 4000));
  TEST_ASSERT_FALSE(d.active(0x90000000UL + 4000 + 0x80000000UL));
}

void test_due_since() {
  TEST_ASSERT_FALSE(due_since(1499, 1000, 500));
  TEST_ASSERT_TRUE(due_since(1500, 1000, 500));
  TEST_ASSERT_TRUE(due_since(1000UL + 0x80000000UL + 5, 1000, 500));   // lange liegengeblieben: faellig, nicht 24,8 Tage warten
  TEST_ASSERT_TRUE(due_since(0x00000100UL, 0xFFFFFF00UL, 0x100));      // millis()-Ueberlauf
  TEST_ASSERT_FALSE(due_since(0x000000FFUL, 0xFFFFFF00UL, 0x200));
}
void test_wait_struct() {
  Wait w;
  TEST_ASSERT_TRUE(w.due(5));                  // nie gesetzt: sofort faellig
  w.start(1000, 500);
  TEST_ASSERT_FALSE(w.due(1499));
  TEST_ASSERT_TRUE(w.due(1500));
  w.start(0xFFFFFF00UL, 0x200);                // ueber den millis()-Ueberlauf
  TEST_ASSERT_FALSE(w.due(0x000000FFUL));
  TEST_ASSERT_TRUE(w.due(0x00000100UL));
  TEST_ASSERT_TRUE(w.due(0xFFFFFF00UL + 0x80000000UL + 5));   // lange liegengeblieben: faellig
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_never_armed_is_never_active);
  RUN_TEST(test_active_until_expiry);
  RUN_TEST(test_stays_off_after_expiry_beyond_2_31);
  RUN_TEST(test_expiry_not_polled_for_long_still_off);
  RUN_TEST(test_wraparound_of_millis);
  RUN_TEST(test_rearm_and_clear);
  RUN_TEST(test_high_now_arm);
  RUN_TEST(test_due_since);
  RUN_TEST(test_wait_struct);
  return UNITY_END();
}
