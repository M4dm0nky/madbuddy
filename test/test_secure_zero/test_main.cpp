#include <unity.h>
#include <string.h>
#include "logic/secure_zero.h"
void setUp() {}
void tearDown() {}

void test_wipes_buffer() {
  char b[16];
  memset(b, 'x', sizeof b);
  secure_zero(b, sizeof b);
  for (char c : b) TEST_ASSERT_EQUAL_CHAR(0, c);
}
void test_wipes_only_n_bytes() {
  char b[8];
  memset(b, 'x', sizeof b);
  secure_zero(b, 4);
  TEST_ASSERT_EQUAL_CHAR(0, b[3]);
  TEST_ASSERT_EQUAL_CHAR('x', b[4]);
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_wipes_buffer);
  RUN_TEST(test_wipes_only_n_bytes);
  return UNITY_END();
}
