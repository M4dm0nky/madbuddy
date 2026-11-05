#include <unity.h>
#include "logic/mood.h"

static MoodState s;
static void fresh(float a, float f, float e) { mood_init(s, Needs{a, f, e}); }
void setUp() {}
void tearDown() {}

void test_decay_awake_one_hour() {
  fresh(100, 100, 50);
  mood_tick(s, 3600000UL, 3600000UL);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 88.0f, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 80.2f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 39.8f, s.n.energy);
}
void test_sleep_regenerates_and_clamps() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 23, 0);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 5000));
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 10001));
  mood_tick(s, 3600000UL, 3610000UL);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, s.n.energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, s.n.affection);   // im Schlaf kein Abbau
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, s.n.fun);
}
void test_stroke_cooldown() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 10000);
  mood_event(s, MoodEvent::STROKE, 11000);   // < 2 s: zählt nicht
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 58.0f, s.n.affection);
  // Spec Gesicht: das 3. Streicheln innerhalb 20 s kichert (GIGGLE) -> drittes erst nach 20 s
  mood_event(s, MoodEvent::STROKE, 40000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 66.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)mood_emotion(s, 12, 40500));
}
void test_affection_clamped_at_100() {
  fresh(99, 50, 50);
  mood_event(s, MoodEvent::STROKE, 5000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, s.n.affection);
}
void test_poke_annoyed_20s() {
  fresh(80, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 77.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::ANNOYED, (int)mood_emotion(s, 12, 20999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 21001));
}
void test_transient_then_back_to_base() {
  fresh(10, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL((int)Emotion::SURPRISED, (int)mood_emotion(s, 12, 2000));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 2600));
}
void test_priorities() {
  fresh(50, 50, 50);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 23, 0));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, -1, 0));
  fresh(10, 10, 50);   // unter der Untergrenze: neutral, nie traurig oder gelangweilt
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 0));
  fresh(80, 70, 50);
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 0));
  fresh(50, 50, 20);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 12, 0));
}
void test_nap_and_wake_by_touch() {
  fresh(50, 50, 5);
  mood_tick(s, 1000, 1000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 1000));
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPING, (int)mood_emotion(s, 12, 1000));
  mood_event(s, MoodEvent::TAP, 2000);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 2000));
}
void test_nap_stays_awake_after_touch() {
  fresh(50, 50, 5);
  mood_tick(s, 20, 20);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 20));
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 1000));
  uint32_t t = 1000;
  for (int i = 0; i < 100; i++) { t += 20; mood_tick(s, 20, t); }
  TEST_ASSERT_FALSE(mood_is_sleeping(s, t));
}
void test_nap_again_after_energy_recovers_and_falls() {
  fresh(50, 50, 5);
  mood_tick(s, 20, 20);
  mood_event(s, MoodEvent::TAP, 1000);
  s.n.energy = 12;
  mood_tick(s, 20, 1020);   // wieder >= 10
  s.n.energy = 9;
  mood_tick(s, 20, 1040);   // erneut darunter -> Flanke
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 1040));
}
void test_nap_again_10min_after_last_touch() {
  fresh(50, 50, 5);
  mood_tick(s, 20, 20);
  mood_event(s, MoodEvent::TAP, 1000);
  mood_tick(s, 20, 1000 + 9UL * 60000UL);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 1000 + 9UL * 60000UL));
  s.n.energy = 5;
  mood_tick(s, 20, 1000 + 10UL * 60000UL + 1);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 1000 + 10UL * 60000UL + 1));
}
void test_night_hours_shared() {
  for (int h = 0; h < 24; h++) {
    const bool night = h >= 22 || h < 7;
    TEST_ASSERT_EQUAL_MESSAGE(night, mood_is_night(h), "mood_is_night");
    TEST_ASSERT_EQUAL_MESSAGE(!night, mood_daytime(h), "mood_daytime");
    fresh(50, 50, 50);
    TEST_ASSERT_EQUAL_MESSAGE(night ? (int)Emotion::SLEEPY : (int)Emotion::CONTENT, (int)mood_base_emotion(s, h), "SLEEPY");
  }
  TEST_ASSERT_FALSE(mood_is_night(-1));
}
void test_nap_ends_above_60() {
  fresh(50, 50, 5);
  mood_tick(s, 1000, 1000);
  mood_tick(s, 56UL * 60000UL, 57UL * 60000UL);   // +56 Energie -> ~61
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 57UL * 60000UL));
}
void test_greeting_when_light_returns_daytime() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 9, 0);
  mood_set_dark(s, false, 9, 20000);
  TEST_ASSERT_EQUAL((int)Emotion::GREETING, (int)mood_emotion(s, 9, 21000));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 9, 23001));
}
void test_no_greeting_at_night() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 2, 0);
  mood_set_dark(s, false, 2, 20000);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 2, 21000));
}
void test_game_won() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 75.0f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(s, 12, 5999));
}
void test_elapsed_counts_as_sleep() {
  fresh(50, 70, 20);
  mood_apply_elapsed(s, 30UL * 60UL);   // ausgeschaltet = geschlafen: nur Energie steigt
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 70.0f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, s.n.energy);
}

// Nachtschlaf: nachts (22-7 Uhr) nach 10 min ohne Berührung einschlafen (der Lichtsensor des CYD taugt dafür nicht)
void test_night_idle_sleeps_after_10min() {
  fresh(50, 50, 50);
  mood_touch(s, 0);
  mood_night_check(s, 23, 599999);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 599999));
  mood_night_check(s, 23, 600001);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 600001));
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPING, (int)mood_emotion(s, 23, 600001));
}
void test_day_idle_stays_awake() {
  fresh(50, 50, 50);
  mood_touch(s, 0);
  mood_night_check(s, 14, 7200000UL);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 7200000UL));
  mood_night_check(s, -1, 7200000UL);   // Uhrzeit unbekannt: nie Nachtschlaf
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 7200000UL));
}
void test_touch_wakes_from_night_sleep() {
  fresh(50, 50, 50);
  mood_night_check(s, 2, 700000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 700000));
  mood_touch(s, 700100);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 700100));
  mood_night_check(s, 2, 700200);   // gerade berührt: bleibt wach
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 700200));
}
void test_morning_wakes_with_greeting() {
  fresh(50, 50, 50);
  mood_night_check(s, 6, 700000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 700000));
  mood_night_check(s, 7, 800000);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 800000));
  TEST_ASSERT_EQUAL((int)Emotion::GREETING, (int)mood_emotion(s, 7, 801000));
}

// ---- Neue Ausloeser (Spec Gesicht) ----
static int E(Emotion e) { return (int)e; }

void test_giggle_third_stroke_in_20s() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);
  mood_event(s, MoodEvent::STROKE, 1500);   // Cooldown: zaehlt trotzdem
  TEST_ASSERT_EQUAL(E(Emotion::LOVE), E(mood_emotion(s, 12, 2000)));
  mood_event(s, MoodEvent::STROKE, 3000);
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 3500)));
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 5999)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 6001)));
}
void test_no_giggle_when_spread_over_20s() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 0);
  mood_event(s, MoodEvent::STROKE, 15000);
  mood_event(s, MoodEvent::STROKE, 30000);
  TEST_ASSERT_EQUAL(E(Emotion::LOVE), E(mood_emotion(s, 12, 30500)));
}
void test_kiss_when_affection_over_90() {
  fresh(85, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);   // 93
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 1500)));
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 3999)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 4001)));
}
void test_giggle_beats_kiss_beats_love() {
  fresh(95, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 1100)));
  mood_event(s, MoodEvent::STROKE, 3500);
  mood_event(s, MoodEvent::STROKE, 6000);
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 6100)));
}
void test_double_tap_confused() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 100);   // erster Tipp: nie verwirrt
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 200)));
  mood_event(s, MoodEvent::TAP, 1000);
  mood_event(s, MoodEvent::TAP, 1400);   // < 500 ms
  TEST_ASSERT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 1500)));
  TEST_ASSERT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 3899)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 3901)));
}
void test_slow_taps_stay_surprised() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  mood_event(s, MoodEvent::TAP, 1500);   // genau 500 ms: noch nicht verwirrt
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 1600)));
}
void test_poke_when_not_sad_still_annoyed() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000, 12);
  TEST_ASSERT_EQUAL(E(Emotion::ANNOYED), E(mood_emotion(s, 12, 2000)));
}
void test_game_won_then_proud() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  TEST_ASSERT_EQUAL(E(Emotion::EXCITED), E(mood_emotion(s, 12, 5999)));
  TEST_ASSERT_EQUAL(E(Emotion::PROUD), E(mood_emotion(s, 12, 6001)));
  TEST_ASSERT_EQUAL(E(Emotion::PROUD), E(mood_emotion(s, 12, 9999)));
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 10001)));
}
void test_new_transient_cancels_proud_follow() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 7000)));
}
void test_game_lost_skeptical() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_LOST, 0);
  TEST_ASSERT_EQUAL(E(Emotion::SKEPTICAL), E(mood_emotion(s, 12, 3999)));
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 4001)));
}
void test_base_emotion_ignores_transient() {
  fresh(10, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 1100)));
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_base_emotion(s, 12)));
}

// ---- Eigenarten ----
// Laeuft Frames im 1-s-Takt; liefert Zeitpunkt des ersten Auftretens von `want` (oder -1) und merkt sich weitere.
static long first_seen(Emotion want, int hour, uint32_t until_ms) {
  for (uint32_t t = 0; t <= until_ms; t += 1000) {
    mood_quirks(s, hour, t);
    if (mood_emotion(s, hour, t) == want) return (long)t;
  }
  return -1;
}
void test_quirk_yawn_when_sleepy_within_5min_for_3s() {
  fresh(50, 50, 20);   // SLEEPY
  mood_seed(s, 12345);
  long t0 = first_seen(Emotion::YAWN, 12, 300000);
  TEST_ASSERT_TRUE(t0 >= 120000);
  TEST_ASSERT_EQUAL(E(Emotion::YAWN), E(mood_emotion(s, 12, t0 + 2999)));
  TEST_ASSERT_EQUAL(E(Emotion::SLEEPY), E(mood_emotion(s, 12, t0 + 3001)));
}
void test_quirk_happy_gives_mischievous_or_cheeky_both_occur() {
  bool seen_m = false, seen_c = false;
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 50);   // HAPPY
    mood_seed(s, seed);
    bool got = false;
    for (uint32_t t = 0; t <= 360000; t += 1000) {
      mood_quirks(s, 12, t);
      const Emotion e = mood_emotion(s, 12, t);
      if (e == Emotion::MISCHIEVOUS) { seen_m = true; got = true; break; }
      if (e == Emotion::CHEEKY) { seen_c = true; got = true; break; }
    }
    TEST_ASSERT_TRUE(got);
  }
  TEST_ASSERT_TRUE(seen_m && seen_c);
}
void test_quirk_none_when_sleeping() {
  fresh(50, 50, 20);
  mood_seed(s, 12345);
  s.napping = true;
  for (uint32_t t = 0; t <= 900000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::SLEEPING), E(mood_emotion(s, 12, t)));
  }
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(s.transient));
}
void test_quirk_none_during_other_transient() {
  fresh(80, 70, 50);   // HAPPY
  mood_seed(s, 777);
  mood_set_transient(s, Emotion::SURPRISED, 0, 1000000);
  for (uint32_t t = 0; t <= 400000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, t)));
  }
}
void test_quirk_none_for_other_base_emotions() {
  fresh(50, 50, 50);   // CONTENT
  mood_seed(s, 5);
  for (uint32_t t = 0; t <= 900000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, t)));
  }
}

void test_highfive() {
  MoodState hs;
  mood_init(hs, Needs{50, 50, 50});
  mood_event(hs, MoodEvent::HIGHFIVE, 1000, 12);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, hs.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 52.0f, hs.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(hs, 12, 2999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(hs, 12, 3001));
}

void test_transient_not_reactivated_after_2_31() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000, 12);   // SURPRISED bis 2500
  TEST_ASSERT_EQUAL((int)Emotion::SURPRISED, (int)mood_emotion(s, 12, 1200));
  TEST_ASSERT_NOT_EQUAL((int)Emotion::SURPRISED, (int)mood_emotion(s, 12, 2500UL + 0x80000000UL + 10));
}
void test_follow_not_reactivated_after_2_31() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 1000, 12);   // EXCITED 6 s, PROUD 4 s
  TEST_ASSERT_EQUAL((int)Emotion::PROUD, (int)mood_emotion(s, 12, 8000));
  TEST_ASSERT_NOT_EQUAL((int)Emotion::PROUD, (int)mood_emotion(s, 12, 11000UL + 0x80000000UL + 10));
  TEST_ASSERT_NOT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(s, 12, 7000UL + 0x80000000UL + 10));
}
void test_transient_cleared_by_tick_survives_49_days() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000, 12);
  mood_tick(s, 100, 3000);   // nach Ablauf
  TEST_ASSERT_NOT_EQUAL((int)Emotion::SURPRISED, (int)mood_emotion(s, 12, 1000UL + 0x100000000ULL + 100));
}
void test_quirk_set_long_ago_fires_after_2_31() {
  fresh(50, 50, 50);
  mood_seed(s, 12345);
  mood_quirks(s, 23, 1000);   // plant (SLEEPY, Nacht) 2-5 min
  mood_quirks(s, 23, 0x80000000UL + 400000);   // nie abgefragt (Ziel hoechstens bei 301000): darf nicht 24,8 Tage warten
  TEST_ASSERT_EQUAL((int)Emotion::YAWN, (int)mood_emotion(s, 23, 0x80000000UL + 400000));
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_night_hours_shared);
  RUN_TEST(test_nap_stays_awake_after_touch);
  RUN_TEST(test_nap_again_after_energy_recovers_and_falls);
  RUN_TEST(test_nap_again_10min_after_last_touch);
  RUN_TEST(test_giggle_third_stroke_in_20s);
  RUN_TEST(test_no_giggle_when_spread_over_20s);
  RUN_TEST(test_kiss_when_affection_over_90);
  RUN_TEST(test_giggle_beats_kiss_beats_love);
  RUN_TEST(test_double_tap_confused);
  RUN_TEST(test_slow_taps_stay_surprised);
  RUN_TEST(test_poke_when_not_sad_still_annoyed);
  RUN_TEST(test_game_won_then_proud);
  RUN_TEST(test_new_transient_cancels_proud_follow);
  RUN_TEST(test_game_lost_skeptical);
  RUN_TEST(test_base_emotion_ignores_transient);
  RUN_TEST(test_quirk_yawn_when_sleepy_within_5min_for_3s);
  RUN_TEST(test_quirk_happy_gives_mischievous_or_cheeky_both_occur);
  RUN_TEST(test_quirk_none_when_sleeping);
  RUN_TEST(test_quirk_none_during_other_transient);
  RUN_TEST(test_quirk_none_for_other_base_emotions);
  RUN_TEST(test_decay_awake_one_hour);
  RUN_TEST(test_sleep_regenerates_and_clamps);
  RUN_TEST(test_stroke_cooldown);
  RUN_TEST(test_affection_clamped_at_100);
  RUN_TEST(test_poke_annoyed_20s);
  RUN_TEST(test_transient_then_back_to_base);
  RUN_TEST(test_priorities);
  RUN_TEST(test_nap_and_wake_by_touch);
  RUN_TEST(test_nap_ends_above_60);
  RUN_TEST(test_greeting_when_light_returns_daytime);
  RUN_TEST(test_no_greeting_at_night);
  RUN_TEST(test_game_won);
  RUN_TEST(test_elapsed_counts_as_sleep);
  RUN_TEST(test_night_idle_sleeps_after_10min);
  RUN_TEST(test_day_idle_stays_awake);
  RUN_TEST(test_touch_wakes_from_night_sleep);
  RUN_TEST(test_morning_wakes_with_greeting);
  RUN_TEST(test_highfive);
  RUN_TEST(test_transient_not_reactivated_after_2_31);
  RUN_TEST(test_follow_not_reactivated_after_2_31);
  RUN_TEST(test_transient_cleared_by_tick_survives_49_days);
  RUN_TEST(test_quirk_set_long_ago_fires_after_2_31);
  return UNITY_END();
}
