#include <unity.h>
#include "logic/mood.h"
#include "logic/food.h"
#include "logic/character.h"
#include "logic/speech.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000UL;
static MoodState s;
static void fresh(float a, float f, float e, float sat) { mood_init(s, Needs{a, f, e, sat}); }

// ---- Keine Pflichten: Untergrenze statt Vernachlaessigung ----
void test_satiety_decay_stops_at_floor() {
  mood_init(s, Needs{50, 50, 50});
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 70.0f, s.n.satiety);   // Startwert
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 70.0f - 7.2f, s.n.satiety);   // wach -0,12/min
  fresh(50, 50, 50, 100);
  s.night_sleep = true;
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 100.0f - 2.4f, s.n.satiety);   // schlafend -0,04/min
  fresh(50, 50, 50, 50.1f);
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.satiety);   // nie darunter
}
void test_neglect_48h_is_neutral() {
  fresh(100, 100, 100, 100);
  for (uint32_t t = MIN; t <= 48UL * 60 * MIN; t += MIN) mood_tick(s, MIN, t);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.satiety);
  s.n.energy = 100;
  const Emotion e = mood_base_emotion(s, 12);
  TEST_ASSERT_TRUE(e != Emotion::SAD && e != Emotion::BORED);
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)e);
}
void test_stroke_raises_then_decays_to_floor_not_below() {
  fresh(50, 50, 100, 50);
  mood_event(s, MoodEvent::STROKE, 1000, 12);
  TEST_ASSERT_TRUE(s.n.affection > MOOD_NEUTRAL_FLOOR);
  float prev = s.n.affection;
  for (uint32_t t = 2000 + MIN; t <= 2000 + 600UL * MIN; t += MIN) {
    mood_tick(s, MIN, t);
    s.n.energy = 100;
    TEST_ASSERT_TRUE(s.n.affection <= prev);
    TEST_ASSERT_TRUE(s.n.affection >= MOOD_NEUTRAL_FLOOR);
    prev = s.n.affection;
  }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.affection);
}
void test_poke_and_load_clamp_to_floor() {
  fresh(50, 50, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000, 12);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.affection);
  mood_init(s, Needs{0, 0, 40, 0});   // wie ein alter NVS-Stand mit 0
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, s.n.satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 40, s.n.energy);   // Energie bleibt unberuehrt
  Needs n{-5, 10, 0, 20};
  mood_clamp(n);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, n.energy);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MOOD_NEUTRAL_FLOOR, n.satiety);
}
void test_energy_still_drives_sleepy() {
  fresh(50, 50, 20, 50);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_base_emotion(s, 12));
}

// ---- Füttern ----
void test_food_effects() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15, food_effect(Food::APPLE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 35, food_effect(Food::PIZZA).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 25, food_effect(Food::CAKE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5, food_effect(Food::CAKE).fun);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5, food_effect(Food::COFFEE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15, food_effect(Food::COFFEE).energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0, food_effect(Food::APPLE).fun);
}
void test_feed_normal() {
  fresh(50, 50, 50, 50);
  TEST_ASSERT_EQUAL((int)Say::LECKER, (int)food_feed(s, Food::PIZZA, Food::APPLE, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 85, s.n.satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 2999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 3001));
}
void test_feed_favorite() {
  fresh(50, 50, 50, 50);
  TEST_ASSERT_EQUAL((int)Say::LIEBLINGSESSEN, (int)food_feed(s, Food::CAKE, Food::CAKE, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 75, s.n.satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 60, s.n.fun);   // Kuchen +5, Liebling +5
  TEST_ASSERT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(s, 12, 3999));
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 4001) == Emotion::EXCITED);
}
void test_feed_is_transient_joy_no_lasting_state() {
  fresh(50, 50, 100, 100);   // schon satt: er lehnt nicht ab, kein "zu satt"
  TEST_ASSERT_EQUAL((int)Say::LECKER, (int)food_feed(s, Food::PIZZA, Food::APPLE, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100, s.n.satiety);   // geklemmt
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 2999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 3001));   // danach wieder neutral
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_base_emotion(s, 12));
}
void test_coffee_energy() {
  fresh(50, 50, 50, 50);
  food_feed(s, Food::COFFEE, Food::APPLE, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 65, s.n.energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55, s.n.satiety);
}
void test_speech_for_base_only_happy() {
  TEST_ASSERT_EQUAL((int)Say::FROH, (int)speech_for_base(Emotion::HAPPY));
  TEST_ASSERT_EQUAL((int)Say::COUNT, (int)speech_for_base(Emotion::SAD));
  TEST_ASSERT_EQUAL((int)Say::COUNT, (int)speech_for_base(Emotion::BORED));
}

// ---- Charakter ----
void test_character_changes_and_clamp() {
  Character c{};
  char_stroke(c); char_stroke(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, c.cuddly);
  char_poke(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, c.cuddly);
  char_game(c); char_highfive(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, c.playful);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, c.playful);   // Vernachlaessigen aendert den Charakter nicht
  for (int i = 0; i < 1000; i++) { char_poke(c); char_game(c); }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -100.0f, c.cuddly);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, c.playful);
}
void test_character_effects() {
  Character c{};
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, char_cheeky_prob(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_love_scale(c));
  c.playful = 100; c.cuddly = 100;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, char_cheeky_prob(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, char_love_scale(c));
  c.playful = -100; c.cuddly = -100;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_cheeky_prob(c));
}
// Verspielter Charakter: Eigenarten (HAPPY: 2–6 min) kommen nach höchstens 3 min.
void test_character_quirk_interval() {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 80, 70);
    mood_seed(s, seed);
    Character c{0, 100};
    mood_set_character(s, c);
    uint32_t first = 0;
    for (uint32_t t = 0; t <= 7 * MIN; t += 1000) {
      mood_quirks(s, 12, t);
      const Emotion e = mood_emotion(s, 12, t);
      if (e == Emotion::MISCHIEVOUS || e == Emotion::CHEEKY) { first = t; break; }
    }
    TEST_ASSERT_TRUE(first >= 1 * MIN && first <= 3 * MIN + 1000);
  }
}
// Sehr verschmust: nie CHEEKY, immer MISCHIEVOUS; LOVE dauert 1,5× so lang.
void test_character_cheeky_and_love() {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 80, 70);
    mood_seed(s, seed);
    mood_set_character(s, Character{100, 0});
    for (uint32_t t = 0; t <= 7 * MIN; t += 1000) {
      mood_quirks(s, 12, t);
      TEST_ASSERT_FALSE(mood_emotion(s, 12, t) == Emotion::CHEEKY);
    }
  }
  fresh(50, 50, 50, 70);
  mood_set_character(s, Character{100, 0});
  mood_event(s, MoodEvent::STROKE, 1000, 12);
  TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)mood_emotion(s, 12, 1000 + 5999));
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 1000 + 6001) == Emotion::LOVE);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_satiety_decay_stops_at_floor);
  RUN_TEST(test_neglect_48h_is_neutral);
  RUN_TEST(test_stroke_raises_then_decays_to_floor_not_below);
  RUN_TEST(test_poke_and_load_clamp_to_floor);
  RUN_TEST(test_energy_still_drives_sleepy);
  RUN_TEST(test_food_effects);
  RUN_TEST(test_feed_normal);
  RUN_TEST(test_feed_favorite);
  RUN_TEST(test_feed_is_transient_joy_no_lasting_state);
  RUN_TEST(test_coffee_energy);
  RUN_TEST(test_speech_for_base_only_happy);
  RUN_TEST(test_character_changes_and_clamp);
  RUN_TEST(test_character_effects);
  RUN_TEST(test_character_quirk_interval);
  RUN_TEST(test_character_cheeky_and_love);
  return UNITY_END();
}
