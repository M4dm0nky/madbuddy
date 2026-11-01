#include <unity.h>
#include "logic/presence.h"

void setUp() {}
void tearDown() {}

static CamMsg face(int16_t x, int16_t y, int16_t w) { CamMsg m; m.kind = CAM_FACE; m.x = x; m.y = y; m.w = w; return m; }
static CamMsg noface() { CamMsg m; m.kind = CAM_NOFACE; return m; }
static const int16_t NEAR = 450;

// Meldet ab t alle 100 ms ein Gesicht bis t_end; liefert die letzte Ausgabe.
static PresenceOut faces(Presence &p, uint32_t t, uint32_t t_end, int16_t w = 300) {
  PresenceOut o{};
  for (; t <= t_end; t += 100) { presence_msg(p, face(500, 500, w), t); o = presence_tick(p, true, t, NEAR); }
  return o;
}

void test_present_needs_two_faces_in_1s() {
  Presence p;
  presence_tick(p, true, 0, NEAR);                      // Kamera ab 0 aktiv, sonst wird das erste F verworfen
  presence_msg(p, face(500, 500, 300), 0);
  TEST_ASSERT_FALSE(presence_tick(p, true, 0, NEAR).present);
  presence_msg(p, face(500, 500, 300), 1500);           // zweites erst nach 1,5 s
  TEST_ASSERT_FALSE(presence_tick(p, true, 1500, NEAR).present);
  presence_msg(p, face(500, 500, 300), 2000);           // 0,5 s nach dem vorigen
  TEST_ASSERT_TRUE(presence_tick(p, true, 2000, NEAR).present);
}

void test_gone_after_30s() {
  Presence p;
  faces(p, 0, 1000);
  presence_msg(p, noface(), 1100);
  TEST_ASSERT_TRUE(presence_tick(p, true, 1000 + 29999, NEAR).present);
  TEST_ASSERT_FALSE(presence_tick(p, true, 1000 + 30000, NEAR).present);
}

void test_greet_after_2min_away_with_cooldown() {
  Presence p;
  PresenceOut o = faces(p, 0, 200);
  TEST_ASSERT_FALSE(o.greet);                           // erster Kontakt nach Start: keine Begrüßung (nie „weg“ gewesen)
  // weg ab 200, gilt nach 30 s als weg; 2 min weg = ab 200 + 120000
  presence_tick(p, true, 60000, NEAR);
  o = faces(p, 200 + 30000 + 60000, 200 + 30000 + 60000 + 200);   // nur 1,5 min weg
  TEST_ASSERT_FALSE(o.greet);
  uint32_t t = 200 + 30000 + 60000 + 200;               // zuletzt gesehen
  presence_tick(p, true, t + 30000, NEAR);              // wieder weg
  bool greeted = false;
  for (uint32_t s = t + 30000 + 120000; s <= t + 30000 + 120000 + 200; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    greeted |= presence_tick(p, true, s, NEAR).greet;
  }
  TEST_ASSERT_TRUE(greeted);                            // nach 2 min weg
  // gleich wieder weg und nach 2 min zurück: Sperrzeit 10 min seit der letzten Begrüßung
  const uint32_t g = t + 30000 + 120000;
  presence_tick(p, true, g + 200 + 30000, NEAR);
  greeted = false;
  for (uint32_t s = g + 200 + 30000 + 120000; s <= g + 200 + 30000 + 120000 + 200; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    greeted |= presence_tick(p, true, s, NEAR).greet;
  }
  TEST_ASSERT_FALSE(greeted);
}

void test_greet_is_one_shot() {
  Presence p;
  faces(p, 0, 200);
  presence_tick(p, true, 200 + 30000, NEAR);
  int n = 0;
  for (uint32_t s = 200 + 30000 + 120000; s <= 200 + 30000 + 120000 + 3000; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    n += presence_tick(p, true, s, NEAR).greet ? 1 : 0;
  }
  TEST_ASSERT_EQUAL(1, n);
}

void test_unattended_after_10min() {
  Presence p;
  faces(p, 0, 200);
  TEST_ASSERT_FALSE(presence_tick(p, true, 200 + 600000 - 1, NEAR).unattended);
  TEST_ASSERT_TRUE(presence_tick(p, true, 200 + 600000, NEAR).unattended);
  PresenceOut o = faces(p, 200 + 700000, 200 + 700200);
  TEST_ASSERT_FALSE(o.unattended);                      // wieder da
  TEST_ASSERT_TRUE(o.present);
}

void test_unattended_from_start_without_faces() {
  Presence p;
  presence_tick(p, true, 0, NEAR);                      // Kamera ab 0 aktiv
  TEST_ASSERT_FALSE(presence_tick(p, true, 600000 - 1, NEAR).unattended);
  TEST_ASSERT_TRUE(presence_tick(p, true, 600000, NEAR).unattended);   // seit Aktivierung nie jemand
}

void test_unattended_counts_from_activation() {
  Presence p;
  presence_tick(p, true, 5000, NEAR);                   // Kamera ab 5 s aktiv, nie jemand gesehen
  TEST_ASSERT_FALSE(presence_tick(p, true, 5000 + 600000 - 1, NEAR).unattended);
  TEST_ASSERT_TRUE(presence_tick(p, true, 5000 + 600000, NEAR).unattended);   // ab Aktivierung, nicht ab 0
}

void test_near_with_cooldown() {
  Presence p;
  PresenceOut o = faces(p, 0, 200, 500);
  TEST_ASSERT_TRUE(o.near);                             // w 500 > 450, gilt 2 s
  TEST_ASSERT_TRUE(presence_tick(p, true, 1999, NEAR).near);
  TEST_ASSERT_FALSE(presence_tick(p, true, 2300, NEAR).near);
  o = faces(p, 5000, 5200, 500);
  TEST_ASSERT_FALSE(o.near);                            // Sperrzeit 30 s
  o = faces(p, 31000, 31200, 500);
  TEST_ASSERT_TRUE(o.near);
  o = faces(p, 70000, 70200, 450);
  TEST_ASSERT_FALSE(o.near);                            // genau 450 ist nicht „größer“
}

void test_inactive_resets() {
  Presence p;
  faces(p, 0, 200);
  PresenceOut o = presence_tick(p, false, 300, NEAR);
  TEST_ASSERT_FALSE(o.present);
  TEST_ASSERT_FALSE(o.unattended);
  TEST_ASSERT_FALSE(o.gaze);
  TEST_ASSERT_FALSE(presence_tick(p, false, 900000, NEAR).unattended);   // aus = nie unbeaufsichtigt
  presence_msg(p, face(500, 500, 300), 900100);         // Meldungen bei inaktiv zählen nicht
  TEST_ASSERT_FALSE(presence_tick(p, false, 900100, NEAR).present);
}

void test_gaze_mapping_and_smoothing() {
  float gx, gy;
  presence_gaze_raw(1000, 500, false, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, PRES_GAZE_X_MAX, gx);    // ganz rechts im Bild
  presence_gaze_raw(1000, 500, true, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -PRES_GAZE_X_MAX, gx);   // gespiegelt
  presence_gaze_raw(500, 0, false, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -PRES_GAZE_Y_MAX, gy);   // oben
  Presence p;
  presence_tick(p, true, 0, NEAR);                      // aktivieren, sonst zählt die Meldung nicht
  presence_msg(p, face(1000, 500, 300), 0);
  PresenceOut o = presence_tick(p, true, 0, NEAR);
  TEST_ASSERT_TRUE(o.gaze);
  TEST_ASSERT_TRUE(o.gx > 0 && o.gx < PRES_GAZE_X_MAX);    // geglättet, nicht sofort am Ziel
  for (uint32_t t = 100; t < 3000; t += 100) { presence_msg(p, face(1000, 500, 300), t); o = presence_tick(p, true, t, NEAR); }
  TEST_ASSERT_FLOAT_WITHIN(0.5f, PRES_GAZE_X_MAX, o.gx);   // läuft hin
  presence_msg(p, noface(), 3000);                      // letztes F war bei 2900
  TEST_ASSERT_TRUE(presence_tick(p, true, 2900 + PRES_GAZE_HOLD_MS - 1, NEAR).gaze);
  TEST_ASSERT_FALSE(presence_tick(p, true, 2900 + PRES_GAZE_HOLD_MS, NEAR).gaze);   // ohne frisches F kein Kamerablick
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_present_needs_two_faces_in_1s);
  RUN_TEST(test_gone_after_30s);
  RUN_TEST(test_greet_after_2min_away_with_cooldown);
  RUN_TEST(test_greet_is_one_shot);
  RUN_TEST(test_unattended_after_10min);
  RUN_TEST(test_unattended_from_start_without_faces);
  RUN_TEST(test_unattended_counts_from_activation);
  RUN_TEST(test_near_with_cooldown);
  RUN_TEST(test_inactive_resets);
  RUN_TEST(test_gaze_mapping_and_smoothing);
  return UNITY_END();
}
