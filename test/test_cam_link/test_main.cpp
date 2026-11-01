#include <unity.h>
#include <string.h>
#include "logic/cam_link.h"

void setUp() {}
void tearDown() {}

// Schickt einen Inhalt mit Prüfsumme Byte für Byte; liefert die Anzahl fertiger Meldungen.
static int feed_line(CamLink &l, const char *content, uint32_t now, CamMsg *m) {
  char line[CAM_LINE_MAX + 8];
  const size_t n = cam_seal(content, line, sizeof line);
  int got = 0;
  for (size_t i = 0; i < n; i++) if (cam_link_feed(l, line[i], now, m)) got++;
  return got;
}

void test_line_across_chunks() {
  CamLink l;
  CamMsg m;
  char line[64];
  const size_t n = cam_seal("F 500 400 300", line, sizeof line);
  int got = 0;
  for (size_t i = 0; i < n; i++) got += cam_link_feed(l, line[i], 100, &m) ? 1 : 0;   // einzeln wie aus Serial2
  TEST_ASSERT_EQUAL(1, got);
  TEST_ASSERT_EQUAL(CAM_FACE, m.kind);
  TEST_ASSERT_EQUAL(500, m.x);
  TEST_ASSERT_EQUAL(300, m.w);
}

void test_garbage_dropped() {
  CamLink l;
  CamMsg m;
  static const char junk[] = "\x00\xff rst:0xc boot\nF 1 2 3*00\n";   // ROM-Ausgabe mit NUL, falsche Prüfsumme
  int got = 0;
  for (size_t i = 0; i + 1 < sizeof junk; i++) got += cam_link_feed(l, junk[i], 0, &m) ? 1 : 0;
  TEST_ASSERT_EQUAL(0, got);
  TEST_ASSERT_EQUAL(1, feed_line(l, "N", 0, &m));   // danach geht es normal weiter
  TEST_ASSERT_EQUAL(CAM_NOFACE, m.kind);
}

void test_connection_timeout() {
  CamLink l;
  CamMsg m;
  TEST_ASSERT_FALSE(cam_link_connected(l, 0));        // nie ein H
  feed_line(l, "H 0.1.0 on", 1000, &m);
  TEST_ASSERT_TRUE(cam_link_connected(l, 1000));
  TEST_ASSERT_TRUE(cam_link_connected(l, 5999));
  TEST_ASSERT_FALSE(cam_link_connected(l, 6000));     // 5 s ohne H
  TEST_ASSERT_EQUAL_STRING("0.1.0", cam_link_version(l));
}

void test_conn_states() {
  CamLink l;
  CamMsg m;
  TEST_ASSERT_EQUAL((int)CamConn::OFF_USER, (int)cam_link_conn(l, 0, false));
  TEST_ASSERT_EQUAL((int)CamConn::NONE, (int)cam_link_conn(l, 0, true));
  feed_line(l, "H 0.1.0 err", 0, &m);
  TEST_ASSERT_EQUAL((int)CamConn::ERR, (int)cam_link_conn(l, 10, true));
  feed_line(l, "H 0.1.0 off", 20, &m);
  TEST_ASSERT_EQUAL((int)CamConn::OFF, (int)cam_link_conn(l, 30, true));
  feed_line(l, "H 0.1.0 on", 40, &m);
  TEST_ASSERT_EQUAL((int)CamConn::ON, (int)cam_link_conn(l, 50, true));
  TEST_ASSERT_EQUAL((int)CamConn::NONE, (int)cam_link_conn(l, 40 + 5000, true));
}

void test_restart_detect() {
  CamLink l;
  CamMsg m;
  feed_line(l, "H 0.1.0 on", 0, &m);
  TEST_ASSERT_TRUE(cam_link_take_restart(l));          // erstes H überhaupt
  TEST_ASSERT_FALSE(cam_link_take_restart(l));         // nur einmal
  feed_line(l, "H 0.1.0 on", 2000, &m);
  TEST_ASSERT_FALSE(cam_link_take_restart(l));         // laufend verbunden
  feed_line(l, "H 0.1.0 on", 9000, &m);                // nach Trennung wieder da
  TEST_ASSERT_TRUE(cam_link_take_restart(l));
}

void test_update_watchdog() {
  CamLink l;
  CamMsg m;
  cam_link_upd_start(l, 0);
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 19999));
  TEST_ASSERT_TRUE(cam_link_upd_timed_out(l, 20000));
  cam_link_upd_start(l, 0);
  feed_line(l, "U 40", 15000, &m);                     // jede U- oder H-Zeile stößt den Watchdog an
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 34999));
  TEST_ASSERT_TRUE(cam_link_upd_timed_out(l, 35000));
  feed_line(l, "U ok", 35001, &m);                     // Ergebnis beendet die Überwachung
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 90000));
  cam_link_upd_start(l, 0);                            // Zeitüberschreitung bleibt wahr, bis die Karte upd_end meldet
  TEST_ASSERT_TRUE(cam_link_upd_timed_out(l, 90000));
  cam_link_upd_end(l);
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 90000));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_line_across_chunks);
  RUN_TEST(test_garbage_dropped);
  RUN_TEST(test_connection_timeout);
  RUN_TEST(test_conn_states);
  RUN_TEST(test_restart_detect);
  RUN_TEST(test_update_watchdog);
  return UNITY_END();
}
