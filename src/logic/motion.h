#pragma once
#include <stdint.h>

// Bewegungserkennung der XIAO-Firmware (cam/): grobes Helligkeitsraster, Vergleich mit dem vorigen Bild.
// Ohne Arduino-/IDF-Includes, Host-Test test/test_motion.
constexpr int MOT_W = 30, MOT_H = 40;          // Raster (Breite x Höhe) für ein aufrechtes 240x320-Bild, je Zelle 8x8 Pixel
constexpr int MOT_CELLS = MOT_W * MOT_H;
constexpr int MOT_DELTA = 18;                  // Helligkeitsänderung (0..255), ab der eine Zelle als bewegt gilt
constexpr int MOT_MIN_CELLS = 6;               // so viele bewegte Zellen mindestens
constexpr int MOT_FLASH_PCT = 60;              // mehr bewegte Zellen = Lichtwechsel/Bildwechsel, keine Bewegung
constexpr uint32_t MOT_SEND_MS = 250;          // höchstens alle 250 ms eine M-Zeile

struct MotionResult { bool found; int x, y, n; };   // x, y: Schwerpunkt 0..1000, n: bewegte Zellen

// RGB565 big-endian, w x h Pixel, nach grid[MOT_CELLS] (zeilenweise). Je Zelle Mittel der 2x2 Pixel in der Zellmitte.
static inline void motion_grid(const uint8_t *px, int w, int h, uint8_t *grid) {
  for (int gy = 0; gy < MOT_H; gy++) {
    for (int gx = 0; gx < MOT_W; gx++) {
      const int cw = w / MOT_W, ch = h / MOT_H;
      int x0 = gx * cw + cw / 2 - 1, y0 = gy * ch + ch / 2 - 1;
      if (x0 < 0) x0 = 0;
      if (y0 < 0) y0 = 0;
      if (x0 > w - 2) x0 = w - 2;
      if (y0 > h - 2) y0 = h - 2;
      int sum = 0;
      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          const uint8_t *p = px + 2 * ((long)(y0 + dy) * w + (x0 + dx));
          const int v = (p[0] << 8) | p[1];
          const int r = ((v >> 11) & 31) << 3, g = ((v >> 5) & 63) << 2, b = (v & 31) << 3;
          sum += (r * 77 + g * 150 + b * 29) >> 8;
        }
      }
      grid[gy * MOT_W + gx] = (uint8_t)(sum / 4);
    }
  }
}

// Vergleicht zwei Raster. found nur bei mindestens MOT_MIN_CELLS bewegten Zellen und höchstens MOT_FLASH_PCT %.
static inline MotionResult motion_compare(const uint8_t *prev, const uint8_t *cur) {
  MotionResult r = {false, 0, 0, 0};
  int n = 0, sx = 0, sy = 0;
  for (int i = 0; i < MOT_CELLS; i++) {
    const int d = (int)cur[i] - (int)prev[i];
    if (d > MOT_DELTA || d < -MOT_DELTA) { n++; sx += i % MOT_W; sy += i / MOT_W; }
  }
  if (n < MOT_MIN_CELLS || n * 100 > MOT_CELLS * MOT_FLASH_PCT) return r;
  r.found = true;
  r.n = n;
  r.x = (2 * sx + n) * 500 / (n * MOT_W);   // Mitte der Zellen, auf 0..1000
  r.y = (2 * sy + n) * 500 / (n * MOT_H);
  return r;
}
