#include <unity.h>
#include <string.h>
#include "logic/phrases.h"
#include "logic/ascii.h"
void setUp() {}
void tearDown() {}

void test_each_category_has_6() {
  for (int i = 0; i < (int)Say::COUNT; i++) {
    const SayList l = say_list((Say)i);
    TEST_ASSERT_NOT_NULL(l.items);
    TEST_ASSERT_TRUE_MESSAGE(l.n >= 6, "Kategorie mit weniger als 6 Saetzen");
  }
  TEST_ASSERT_EQUAL_UINT8(0, say_list(Say::COUNT).n);
}
void test_wieder_exists() {
  TEST_ASSERT_EQUAL_UINT8(6, say_list(Say::WIEDER).n);
  TEST_ASSERT_EQUAL_STRING("Da bist du ja!", say_list(Say::WIEDER).items[0]);
}
void test_ascii_max_28() {
  char buf[64];
  for (int i = 0; i < (int)Say::COUNT; i++) {
    const SayList l = say_list((Say)i);
    for (uint8_t j = 0; j < l.n; j++) {
      ascii_de(l.items[j], buf, sizeof buf);
      TEST_ASSERT_TRUE_MESSAGE(strlen(buf) <= 28, l.items[j]);
      // kein Zeichen, das ascii_de nicht kennt (würde zu "?")
      int q_in = 0, q_out = 0;
      for (const char *c = l.items[j]; *c; c++) q_in += *c == '?';
      for (const char *c = buf; *c; c++) q_out += *c == '?';
      TEST_ASSERT_EQUAL_INT_MESSAGE(q_in, q_out, l.items[j]);
    }
  }
}
void test_known_entries() {
  TEST_ASSERT_EQUAL_STRING("Moin! Schon wach?", say_list(Say::MORGEN).items[0]);
  TEST_ASSERT_EQUAL_STRING("High five!", say_list(Say::HIGHFIVE).items[0]);
  TEST_ASSERT_EQUAL_STRING("Warm und kuschelig.", say_list(Say::VERSCHMUST).items[5]);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_each_category_has_6);
  RUN_TEST(test_wieder_exists);
  RUN_TEST(test_ascii_max_28);
  RUN_TEST(test_known_entries);
  return UNITY_END();
}
