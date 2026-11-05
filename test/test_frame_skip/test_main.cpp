#include <unity.h>
#include "logic/frame_skip.h"
void setUp() {}
void tearDown() {}

void test_first_frame_pushes() { TEST_ASSERT_TRUE(frame_should_push(1, 1, 0, 0, false)); }
void test_same_signature_skips() { TEST_ASSERT_FALSE(frame_should_push(7, 7, 500, 100, true)); }
void test_changed_signature_pushes() { TEST_ASSERT_TRUE(frame_should_push(8, 7, 120, 100, true)); }
void test_forced_after_one_second() {
  TEST_ASSERT_FALSE(frame_should_push(7, 7, 1099, 100, true));
  TEST_ASSERT_TRUE(frame_should_push(7, 7, 1100, 100, true));
}
void test_millis_wrap() {
  TEST_ASSERT_FALSE(frame_should_push(7, 7, 500, 0xFFFFFF00u, true));    // 756 ms nach Überlauf
  TEST_ASSERT_TRUE(frame_should_push(7, 7, 900, 0xFFFFFF00u, true));     // 1156 ms
}
void test_gap() {
  TEST_ASSERT_FALSE(frame_gap_exceeded(133, 100));
  TEST_ASSERT_TRUE(frame_gap_exceeded(300, 100));
}
void test_hash_reacts_to_each_input() {
  FrameHash a, b, c, d;
  float f = 1.0f, g = 1.0f, h = 1.5f;
  a.add(f); a.add_str("hi");
  b.add(g); b.add_str("hi");
  c.add(h); c.add_str("hi");
  d.add(f); d.add_str("ho");
  TEST_ASSERT_EQUAL_UINT32(a.h, b.h);
  TEST_ASSERT_NOT_EQUAL(a.h, c.h);
  TEST_ASSERT_NOT_EQUAL(a.h, d.h);
}
void test_hash_null_vs_empty_string() {
  FrameHash a, b;
  a.add_str(nullptr);
  b.add_str("x");
  TEST_ASSERT_NOT_EQUAL(a.h, b.h);
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_first_frame_pushes);
  RUN_TEST(test_same_signature_skips);
  RUN_TEST(test_changed_signature_pushes);
  RUN_TEST(test_forced_after_one_second);
  RUN_TEST(test_millis_wrap);
  RUN_TEST(test_gap);
  RUN_TEST(test_hash_reacts_to_each_input);
  RUN_TEST(test_hash_null_vs_empty_string);
  return UNITY_END();
}
