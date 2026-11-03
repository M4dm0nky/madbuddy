#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "logic/motion.h"
void setUp() {}
void tearDown() {}

static const int W = 240, H = 320;
static uint8_t img[W * H * 2];

static void fill(uint16_t v) { for (int i = 0; i < W * H; i++) { img[2 * i] = v >> 8; img[2 * i + 1] = v & 255; } }
static void rect(int x0, int y0, int x1, int y1, uint16_t v) {
  for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) { img[2 * (y * W + x)] = v >> 8; img[2 * (y * W + x) + 1] = v & 255; }
}

void test_grid_brightness() {
  uint8_t g[MOT_CELLS];
  fill(0x0000);
  motion_grid(img, W, H, g);
  TEST_ASSERT_EQUAL(0, g[0]);
  fill(0xFFFF);
  motion_grid(img, W, H, g);
  TEST_ASSERT_UINT8_WITHIN(8, 248, g[MOT_CELLS - 1]);   // weiß, RGB565 hat nur 5/6 Bit
  fill(0x0000);
  rect(0, 0, 8, 8, 0xFFFF);                              // genau Zelle (0,0)
  motion_grid(img, W, H, g);
  TEST_ASSERT_TRUE(g[0] > 200);
  TEST_ASSERT_EQUAL(0, g[1]);
}

void test_moved_blob_centroid() {
  uint8_t a[MOT_CELLS], b[MOT_CELLS];
  fill(0x2104);
  motion_grid(img, W, H, a);
  rect(40, 80, 72, 112, 0xFFFF);                         // 4x4 Zellen weiß, Zellen x 5..8, y 10..13
  motion_grid(img, W, H, b);
  MotionResult r = motion_compare(a, b);
  TEST_ASSERT_TRUE(r.found);
  TEST_ASSERT_EQUAL(16, r.n);
  TEST_ASSERT_INT_WITHIN(3, 233, r.x);   // Mitte der Zellen 5..8 = 7,0 von 30 → 233
  TEST_ASSERT_INT_WITHIN(3, 12 * 1000 / 40, r.y);          // Mitte der Zellen 10..13 = 12,0 von 40 → 300
  uint8_t c[MOT_CELLS];                                  // Fleck verschoben: Schwerpunkt wandert mit
  fill(0x2104);
  rect(160, 80, 192, 112, 0xFFFF);
  motion_grid(img, W, H, c);
  r = motion_compare(b, c);
  TEST_ASSERT_TRUE(r.found);
  TEST_ASSERT_EQUAL(32, r.n);                            // alte und neue Stelle
  TEST_ASSERT_INT_WITHIN(3, 483, r.x);                   // Mitte der Zellen 5..8 und 20..23 = 14,5 von 30
}

void test_whole_image_change_is_no_motion() {
  uint8_t a[MOT_CELLS], b[MOT_CELLS];
  fill(0x2104);
  motion_grid(img, W, H, a);
  fill(0xFFFF);
  motion_grid(img, W, H, b);
  TEST_ASSERT_FALSE(motion_compare(a, b).found);         // Lichtwechsel
  fill(0x2104);                                          // knapp unter 60 %: noch Bewegung
  rect(0, 0, W, 190, 0xFFFF);                            // 23 Zeilen von 40 = 57,5 %
  motion_grid(img, W, H, b);
  TEST_ASSERT_TRUE(motion_compare(a, b).found);
  fill(0x2104);
  rect(0, 0, W, 200, 0xFFFF);                            // 25 von 40 = 62,5 %
  motion_grid(img, W, H, b);
  TEST_ASSERT_FALSE(motion_compare(a, b).found);
}

void test_below_threshold_is_nothing() {
  uint8_t a[MOT_CELLS], b[MOT_CELLS];
  fill(0x2104);
  motion_grid(img, W, H, a);
  rect(0, 0, 40, 8, 0xFFFF);                             // 5 Zellen < 6
  motion_grid(img, W, H, b);
  TEST_ASSERT_FALSE(motion_compare(a, b).found);
  rect(0, 0, 48, 8, 0xFFFF);                             // 6 Zellen
  motion_grid(img, W, H, b);
  TEST_ASSERT_TRUE(motion_compare(a, b).found);
  fill(0x2104);                                          // kleiner Helligkeitsunterschied (unter Delta) zählt nicht
  rect(0, 0, W, H, 0x2965);
  motion_grid(img, W, H, b);
  TEST_ASSERT_FALSE(motion_compare(a, b).found);
}

// Referenz: die bisherige Drehung (Bild für Bild mit switch), nur für den Vergleich
static void ref_rotate(const uint8_t *src, int w, int h, uint8_t *dst, int rot) {
  int ow, oh;
  cam_rot_dims(w, h, rot, &ow, &oh);
  for (int y = 0; y < oh; y++) for (int x = 0; x < ow; x++) {
    int sx, sy;
    switch (rot) {
      case 90:  sx = y;         sy = h - 1 - x; break;
      case 180: sx = w - 1 - x; sy = h - 1 - y; break;
      case 270: sx = w - 1 - y; sy = x;         break;
      default:  sx = x;         sy = y;         break;
    }
    memcpy(dst + 2 * ((long)y * ow + x), src + 2 * ((long)sy * w + sx), 2);
  }
}

void test_grid_rot_equals_rotate_then_grid() {
  static uint8_t raw[320 * 240 * 2], rotd[320 * 240 * 2];
  const int sizes[2][2] = {{320, 240}, {240, 176}};
  const int rots[4] = {0, 90, 180, 270};
  uint32_t s = 12345;
  for (const auto &sz : sizes) {
    const int rw = sz[0], rh = sz[1];
    for (int i = 0; i < rw * rh * 2; i++) { s = s * 1664525u + 1013904223u; raw[i] = (uint8_t)(s >> 24); }
    for (int rot : rots) {
      int ow, oh;
      cam_rot_dims(rw, rh, rot, &ow, &oh);
      ref_rotate(raw, rw, rh, rotd, rot);
      uint8_t a[MOT_CELLS], b[MOT_CELLS];
      motion_grid(rotd, ow, oh, a);
      motion_grid_rot(raw, rw, rh, rot, b);
      char msg[40]; snprintf(msg, sizeof msg, "%dx%d rot %d", rw, rh, rot);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(a, b, MOT_CELLS, msg);
    }
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_grid_brightness);
  RUN_TEST(test_moved_blob_centroid);
  RUN_TEST(test_whole_image_change_is_no_motion);
  RUN_TEST(test_below_threshold_is_nothing);
  RUN_TEST(test_grid_rot_equals_rotate_then_grid);
  return UNITY_END();
}
