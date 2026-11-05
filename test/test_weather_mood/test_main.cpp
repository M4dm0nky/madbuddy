#include <unity.h>
#include "logic/weather_mood.h"
void setUp() {}
void tearDown() {}

void test_wmo_mapping() {
  TEST_ASSERT_EQUAL((int)Sky::SUNNY, (int)weather_mood(0, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::SUNNY, (int)weather_mood(1, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::CLOUDY, (int)weather_mood(3, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::FOG, (int)weather_mood(45, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::RAIN, (int)weather_mood(61, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::RAIN, (int)weather_mood(81, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::SNOW, (int)weather_mood(73, -2).sky);
  TEST_ASSERT_EQUAL((int)Sky::SNOW, (int)weather_mood(86, 1).sky);
  TEST_ASSERT_EQUAL((int)Sky::STORM, (int)weather_mood(95, 20).sky);
  TEST_ASSERT_EQUAL((int)Sky::CLOUDY, (int)weather_mood(42, 20).sky);
}
void test_flags() {
  TEST_ASSERT_TRUE(weather_mood(0, 28.5f).hot);
  TEST_ASSERT_FALSE(weather_mood(0, 28.0f).hot);
  TEST_ASSERT_TRUE(weather_mood(3, -0.5f).cold);
  TEST_ASSERT_TRUE(weather_mood(71, 1.0f).cold);
  TEST_ASSERT_FALSE(weather_mood(3, 5.0f).cold);
}
void test_fresh() {
  TEST_ASSERT_FALSE(weather_fresh(1000, 0));
  TEST_ASSERT_TRUE(weather_fresh(1000 + WEATHER_MAX_AGE_MS - 1, 1000));
  TEST_ASSERT_FALSE(weather_fresh(1000 + WEATHER_MAX_AGE_MS + 1, 1000));
}
void test_overlays() {
  WeatherMood sun = weather_mood(0, 30);
  uint32_t o = overlays_for(Emotion::HAPPY, &sun, true);
  TEST_ASSERT_TRUE(o & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(o & OV_SWEAT);
  TEST_ASSERT_FALSE(overlays_for(Emotion::HAPPY, &sun, false) & OV_SUNGLASSES);
  TEST_ASSERT_FALSE(overlays_for(Emotion::SAD, &sun, true) & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(overlays_for(Emotion::SAD, nullptr, true) & OV_TEAR);
  TEST_ASSERT_TRUE(overlays_for(Emotion::LOVE, nullptr, true) & OV_HEART);
  TEST_ASSERT_EQUAL_UINT32(OV_ZZZ, overlays_for(Emotion::SLEEPING, &sun, true));
  WeatherMood rain = weather_mood(63, 10);
  TEST_ASSERT_TRUE(overlays_for(Emotion::CONTENT, &rain, true) & OV_RAIN_TINT);
  WeatherMood storm = weather_mood(96, 10);
  TEST_ASSERT_TRUE(overlays_for(Emotion::CONTENT, &storm, true) & OV_STORM);
}

void test_new_overlays() {
  TEST_ASSERT_TRUE(overlays_for(Emotion::CONFUSED, nullptr, true) & OV_QUESTION);
  TEST_ASSERT_TRUE(overlays_for(Emotion::WHISTLE, nullptr, true) & OV_NOTE);
  TEST_ASSERT_TRUE(overlays_for(Emotion::KISS, nullptr, true) & OV_HEART);
  TEST_ASSERT_FALSE(overlays_for(Emotion::CONTENT, nullptr, true) & (OV_QUESTION | OV_NOTE));
  TEST_ASSERT_FALSE(overlays_for(Emotion::YAWN, nullptr, true) & OV_ZZZ);   // zZ nur beim Schlafen
}
void test_sunglasses_for_proud_and_cheeky() {
  WeatherMood sun = weather_mood(0, 20);
  TEST_ASSERT_TRUE(overlays_for(Emotion::PROUD, &sun, true) & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(overlays_for(Emotion::MISCHIEVOUS, &sun, true) & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(overlays_for(Emotion::CHEEKY, &sun, true) & OV_SUNGLASSES);
  TEST_ASSERT_FALSE(overlays_for(Emotion::PROUD, &sun, false) & OV_SUNGLASSES);
  WeatherMood rain = weather_mood(63, 10);
  TEST_ASSERT_FALSE(overlays_for(Emotion::PROUD, &rain, true) & OV_SUNGLASSES);
}

// OV_HAND (High-Five) und OV_TYPING setzt app.cpp selbst; overlays_for liefert sie nie.
void test_hand_bit() {
  TEST_ASSERT_EQUAL_UINT32(1024u, (uint32_t)OV_HAND);
  TEST_ASSERT_EQUAL_UINT32(4096u, (uint32_t)OV_TYPING);   // Claude-Modus
  for (int i = 0; i < (int)Emotion::COUNT; i++) {
    WeatherMood w = weather_mood(95, 30);
    TEST_ASSERT_FALSE(overlays_for((Emotion)i, &w, true) & (OV_HAND | OV_TYPING));
    TEST_ASSERT_FALSE(overlays_for((Emotion)i, nullptr, true) & (OV_HAND | OV_TYPING));
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_new_overlays);
  RUN_TEST(test_sunglasses_for_proud_and_cheeky);
  RUN_TEST(test_wmo_mapping);
  RUN_TEST(test_flags);
  RUN_TEST(test_fresh);
  RUN_TEST(test_overlays);
  RUN_TEST(test_hand_bit);
  return UNITY_END();
}
