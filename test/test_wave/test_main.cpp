#include <unity.h>
#include "logic/wave.h"

void setUp() {}
void tearDown() {}

// Meldet von t0 an alle 250 ms Schwerpunkte, die zwischen den Punkten pts[] hin- und herlaufen (je 3 Schritte pro Strecke).
// Liefert die Zeit nach der letzten Meldung; zählt erkannte Winken in *waves.
static uint32_t sweep(WaveDetect &w, uint32_t t, const int16_t *pts, int np, int *waves, int16_t n = 20, uint32_t dt = 250) {
  for (int i = 0; i + 1 < np; i++)
    for (int s = 1; s <= 3; s++) {
      wave_feed(w, (int16_t)(pts[i] + (pts[i + 1] - pts[i]) * s / 3), n, t);
      if (wave_take(w, t)) (*waves)++;
      t += dt;
    }
  return t;
}
static const int16_t WAVE5[] = {200, 800, 200, 800, 200};      // rechts, links, rechts, links: 3 Umkehrungen

void test_clean_wave_detected_once() {
  WaveDetect w; int waves = 0;
  sweep(w, 1000, WAVE5, 5, &waves);
  TEST_ASSERT_EQUAL_INT(1, waves);
}
void test_single_sweep_no_wave() {
  WaveDetect w; int waves = 0;
  const int16_t walk[] = {100, 900};
  sweep(w, 1000, walk, 2, &waves);
  TEST_ASSERT_EQUAL_INT(0, waves);
}
void test_small_jitter_no_wave() {
  WaveDetect w; int waves = 0;
  const int16_t jit[] = {450, 550, 450, 550, 450, 550, 450, 550, 450};   // Hub 100 < WAVE_MIN_SWING
  sweep(w, 1000, jit, 9, &waves);
  TEST_ASSERT_EQUAL_INT(0, waves);
}
void test_gap_resets_count() {
  WaveDetect w; int waves = 0;
  const int16_t a[] = {200, 800, 200, 800};                       // 2 Umkehrungen
  uint32_t t = sweep(w, 1000, a, 4, &waves);
  const int16_t b[] = {800, 200, 800};                            // nach Pause weitere 2: zusammen 4, aber getrennt
  sweep(w, t + 1000, b, 3, &waves);
  TEST_ASSERT_EQUAL_INT(0, waves);
}
void test_too_slow_no_wave() {
  WaveDetect w; int waves = 0;
  sweep(w, 1000, WAVE5, 5, &waves, 20, 780);   // Meldungen dicht genug (kein Reset), aber Umkehrungen weiter als 2,5 s auseinander
  TEST_ASSERT_EQUAL_INT(0, waves);
}
void test_cooldown_then_allowed() {
  WaveDetect w; int waves = 0;
  uint32_t t = sweep(w, 1000, WAVE5, 5, &waves);
  TEST_ASSERT_EQUAL_INT(1, waves);
  t = sweep(w, t, WAVE5, 5, &waves);                              // gleich nochmal: gesperrt
  TEST_ASSERT_EQUAL_INT(1, waves);
  sweep(w, t + 9000, WAVE5, 5, &waves);                           // nach Sperrzeit wieder
  TEST_ASSERT_EQUAL_INT(2, waves);
}
void test_few_cells_ignored() {
  WaveDetect w; int waves = 0;
  sweep(w, 1000, WAVE5, 5, &waves, 2);
  TEST_ASSERT_EQUAL_INT(0, waves);
}
void test_millis_wrap() {
  WaveDetect w; int waves = 0;
  uint32_t t = sweep(w, 0xFFFFF000u, WAVE5, 5, &waves);
  TEST_ASSERT_EQUAL_INT(1, waves);
  sweep(w, t + 9000, WAVE5, 5, &waves);
  TEST_ASSERT_EQUAL_INT(2, waves);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clean_wave_detected_once);
  RUN_TEST(test_single_sweep_no_wave);
  RUN_TEST(test_small_jitter_no_wave);
  RUN_TEST(test_gap_resets_count);
  RUN_TEST(test_too_slow_no_wave);
  RUN_TEST(test_cooldown_then_allowed);
  RUN_TEST(test_few_cells_ignored);
  RUN_TEST(test_millis_wrap);
  return UNITY_END();
}
