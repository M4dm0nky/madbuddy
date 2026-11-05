#include <unity.h>
#include <string.h>
#include "logic/speech.h"
#include "logic/rituals.h"
#include "logic/daystats.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000UL;

// ---- speech.h ----
void test_pick_never_repeats() {
  SpeechState s; speech_init(s, 0, 7);
  const char *prev = nullptr;
  for (uint32_t i = 0; i < 200; i++) {
    const char *p = speech_pick(s, Say::FROH, i * 2654435761u);
    TEST_ASSERT_NOT_NULL(p);
    if (prev) TEST_ASSERT_FALSE(strcmp(prev, p) == 0);
    prev = p;
  }
}
void test_pick_categories_independent() {
  SpeechState s; speech_init(s, 0, 7);
  const char *a = speech_pick(s, Say::FROH, 1);
  speech_pick(s, Say::VERLIEBT, 1);
  const char *b = speech_pick(s, Say::FROH, 1);   // gleicher Zufall, aber FROH darf nicht gleich bleiben
  TEST_ASSERT_FALSE(strcmp(a, b) == 0);
}
void test_cooldown_90s() {
  SpeechState s; speech_init(s, 0, 7);
  TEST_ASSERT_TRUE(speech_may(s, 0));
  speech_shown(s, 1000);
  TEST_ASSERT_FALSE(speech_may(s, 1000 + 89999));
  TEST_ASSERT_TRUE(speech_may(s, 1000 + 90000));
}
void test_thoughts_every_5_to_10_min() {
  SpeechState s; speech_init(s, 0, 3);
  Say say;
  uint32_t first = 0;
  for (uint32_t t = 0; t <= 11 * MIN; t += 1000) {
    if (speech_due(s, Emotion::CONTENT, t, &say, 0)) { first = t; break; }
  }
  TEST_ASSERT_TRUE(first >= 5 * MIN && first <= 10 * MIN);
  TEST_ASSERT_EQUAL((int)Say::GEDANKEN, (int)say);
  uint32_t second = 0;
  for (uint32_t t = first + 1000; t <= first + 11 * MIN; t += 1000) {
    if (speech_due(s, Emotion::CONTENT, t, &say, 0)) { second = t; break; }
  }
  TEST_ASSERT_TRUE(second - first >= 5 * MIN && second - first <= 10 * MIN);
}
void test_thoughts_by_character() {
  Say say;
  SpeechState s; speech_init(s, 0, 3);
  for (uint32_t t = 0; t <= 11 * MIN && !speech_due(s, Emotion::CONTENT, t, &say, 50); t += 1000) {}
  TEST_ASSERT_EQUAL((int)Say::VERSCHMUST, (int)say);
  speech_init(s, 0, 3);
  for (uint32_t t = 0; t <= 11 * MIN && !speech_due(s, Emotion::CONTENT, t, &say, -50); t += 1000) {}
  TEST_ASSERT_EQUAL((int)Say::FRECH, (int)say);
  speech_init(s, 0, 3);
  for (uint32_t t = 0; t <= 11 * MIN && !speech_due(s, Emotion::CONTENT, t, &say, 40); t += 1000) {}
  TEST_ASSERT_EQUAL((int)Say::GEDANKEN, (int)say);   // genau 40 zählt noch nicht
}
void test_base_change_40_percent() {
  int hits = 0;
  for (uint32_t seed = 1; seed <= 1000; seed++) {
    SpeechState s; speech_init(s, 0, seed);
    Say say = Say::COUNT;
    speech_due(s, Emotion::CONTENT, 1000, &say, 0);
    if (speech_due(s, Emotion::HAPPY, 2000, &say, 0)) {
      TEST_ASSERT_EQUAL((int)Say::FROH, (int)say);
      hits++;
    }
  }
  TEST_ASSERT_TRUE(hits > 330 && hits < 470);
}
void test_base_change_mapping_and_cooldown() {
  // Mit Zufall, der sicher trifft: viele Seeds probieren, bis einer trifft, dann Zuordnung prüfen.
  const Emotion e[] = {Emotion::HAPPY};
  const Say want[] = {Say::FROH};
  for (int k = 0; k < 1; k++) {
    bool ok = false;
    for (uint32_t seed = 1; seed < 100 && !ok; seed++) {
      SpeechState s; speech_init(s, 0, seed);
      Say say;
      speech_due(s, Emotion::CONTENT, 1000, &say, 0);
      if (speech_due(s, e[k], 2000, &say, 0)) { TEST_ASSERT_EQUAL((int)want[k], (int)say); ok = true; }
    }
    TEST_ASSERT_TRUE(ok);
  }
  // Wechsel innerhalb von 90 s nach einer Blase: nichts
  for (uint32_t seed = 1; seed < 200; seed++) {
    SpeechState s; speech_init(s, 0, seed);
    Say say;
    speech_due(s, Emotion::CONTENT, 1000, &say, 0);
    speech_shown(s, 1500);
    TEST_ASSERT_FALSE(speech_due(s, Emotion::HAPPY, 2000, &say, 0));
  }
  // Wechsel zu CONTENT: nie eine Blase
  for (uint32_t seed = 1; seed < 200; seed++) {
    SpeechState s; speech_init(s, 0, seed);
    Say say;
    speech_due(s, Emotion::HAPPY, 1000, &say, 0);
    TEST_ASSERT_FALSE(speech_due(s, Emotion::CONTENT, 200000, &say, 0));
  }
}

// ---- rituals.h ----
void test_morning_once_per_day_5_to_12() {
  RitualState r{};
  TEST_ASSERT_FALSE(ritual_morning(r, true, 4, 20261025));
  TEST_ASSERT_TRUE(ritual_morning(r, true, 5, 20261025));
  TEST_ASSERT_FALSE(ritual_morning(r, true, 6, 20261025));   // schon heute
  TEST_ASSERT_FALSE(ritual_morning(r, false, 8, 20261026));  // kein Aufwachen
  TEST_ASSERT_FALSE(ritual_morning(r, true, 12, 20261026));  // 12 Uhr zu spät
  TEST_ASSERT_TRUE(ritual_morning(r, true, 11, 20261026));
  TEST_ASSERT_FALSE(ritual_morning(r, true, -1, 20261027));  // Zeit unbekannt
}
void test_night_once_at_22() {
  RitualState r{};
  TEST_ASSERT_FALSE(ritual_night(r, true, 21, 59, 20261025));
  TEST_ASSERT_FALSE(ritual_night(r, false, 22, 0, 20261025));  // schläft
  TEST_ASSERT_TRUE(ritual_night(r, true, 22, 0, 20261025));
  TEST_ASSERT_FALSE(ritual_night(r, true, 22, 1, 20261025));
  TEST_ASSERT_FALSE(ritual_night(r, true, 23, 0, 20261026));   // nicht mehr 22 Uhr
  TEST_ASSERT_TRUE(ritual_night(r, true, 22, 30, 20261027));   // 22:xx, erstes Mal wach
}
void test_review_20_or_catch_up_until_23() {
  RitualState r{};
  TEST_ASSERT_FALSE(ritual_review(r, true, false, 19, 59, 20261025));
  TEST_ASSERT_TRUE(ritual_review(r, true, false, 20, 0, 20261025));
  TEST_ASSERT_FALSE(ritual_review(r, true, true, 21, 0, 20261025));   // einmal pro Tag
  // Tag 2: um 20:00 schlafend -> nachholen bei Berührung bis 23 Uhr
  TEST_ASSERT_FALSE(ritual_review(r, false, false, 20, 0, 20261026));
  TEST_ASSERT_FALSE(ritual_review(r, true, false, 21, 10, 20261026));  // wach, aber keine Berührung
  TEST_ASSERT_TRUE(ritual_review(r, true, true, 22, 59, 20261026));
  // Tag 3: Berührung erst um 23 Uhr -> zu spät
  TEST_ASSERT_FALSE(ritual_review(r, true, true, 23, 0, 20261027));
}
void test_weather_sentence() {
  char b[40];
  weather_sentence(14.3f, Sky::RAIN, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute 14\xC2\xB0 und Regen", b);
  weather_sentence(-2.6f, Sky::SNOW, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute -3\xC2\xB0 und Schnee", b);
  weather_sentence(20, Sky::SUNNY, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute 20\xC2\xB0 und Sonne", b);
  weather_sentence(20, Sky::STORM, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute 20\xC2\xB0 und Gewitter", b);
  weather_sentence(20, Sky::FOG, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute 20\xC2\xB0 und Nebel", b);
  weather_sentence(20, Sky::CLOUDY, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Heute 20\xC2\xB0 und Wolken", b);
}
void test_day_key() {
  TEST_ASSERT_EQUAL_UINT32(20261025u, day_key(2026, 10, 25));
}

// ---- daystats.h ----
void test_daystats_roll() {
  DayStats d{};
  TEST_ASSERT_TRUE(daystats_roll(d, 20261025));   // erster Tag: gesetzt
  d.strokes = 5; d.pokes = 2; d.games = 3; d.wins = 1; d.meals = 4; d.pomodoros = 2;
  TEST_ASSERT_FALSE(daystats_roll(d, 20261025));
  TEST_ASSERT_EQUAL_UINT16(5, d.strokes);
  TEST_ASSERT_TRUE(daystats_roll(d, 20261026));
  TEST_ASSERT_EQUAL_UINT16(0, d.strokes);
  TEST_ASSERT_EQUAL_UINT16(0, d.pokes);
  TEST_ASSERT_EQUAL_UINT16(0, d.games);
  TEST_ASSERT_EQUAL_UINT16(0, d.wins);
  TEST_ASSERT_EQUAL_UINT16(0, d.meals);
  TEST_ASSERT_EQUAL_UINT16(0, d.pomodoros);
  TEST_ASSERT_EQUAL_UINT32(20261026u, d.day);
  TEST_ASSERT_FALSE(daystats_roll(d, 0));          // Datum unbekannt: nichts tun
  TEST_ASSERT_EQUAL_UINT32(20261026u, d.day);
}
void test_daystats_mood() {
  DayStats d{};
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)daystats_mood(d));
  d.strokes = 10;
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)daystats_mood(d));
  d.pokes = 11;
  TEST_ASSERT_EQUAL((int)Emotion::ANNOYED, (int)daystats_mood(d));
  d.strokes = 0; d.pokes = 4;
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)daystats_mood(d));
  d.pokes = 5;
  TEST_ASSERT_EQUAL((int)Emotion::ANNOYED, (int)daystats_mood(d));
  d.strokes = 5;   // gleich viel Streicheln wie Stupsen: nicht genervt
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)daystats_mood(d));
}

void test_thought_set_long_ago_fires_after_2_31() {
  SpeechState s; speech_init(s, 1000, 7);
  Say say;
  TEST_ASSERT_TRUE(speech_due(s, Emotion::CONTENT, 0x80000000UL + 11 * MIN, &say, 0));   // 24,8 Tage nie abgefragt
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_pick_never_repeats);
  RUN_TEST(test_pick_categories_independent);
  RUN_TEST(test_cooldown_90s);
  RUN_TEST(test_thoughts_every_5_to_10_min);
  RUN_TEST(test_thoughts_by_character);
  RUN_TEST(test_base_change_40_percent);
  RUN_TEST(test_base_change_mapping_and_cooldown);
  RUN_TEST(test_morning_once_per_day_5_to_12);
  RUN_TEST(test_night_once_at_22);
  RUN_TEST(test_review_20_or_catch_up_until_23);
  RUN_TEST(test_weather_sentence);
  RUN_TEST(test_day_key);
  RUN_TEST(test_daystats_roll);
  RUN_TEST(test_daystats_mood);
  RUN_TEST(test_thought_set_long_ago_fires_after_2_31);
  return UNITY_END();
}
