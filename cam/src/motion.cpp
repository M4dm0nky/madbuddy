#include <Arduino.h>
#include "motion.h"
#include "camera.h"

static uint8_t g_prev[MOT_CELLS], g_cur[MOT_CELLS];
static bool g_have_prev = false;

void motion_reset() { g_have_prev = false; }

bool motion_detect(camera_fb_t *fb, MotionResult *out) {
  int w, h;
  const uint8_t *px = camera_rotated(fb, &w, &h);
  if (fb->format != PIXFORMAT_RGB565 || w < MOT_W * 2 || h < MOT_H * 2) return false;
  motion_grid(px, w, h, g_cur);
  bool found = false;
  if (g_have_prev) {
    *out = motion_compare(g_prev, g_cur);
    found = out->found;
  }
  memcpy(g_prev, g_cur, sizeof g_prev);
  g_have_prev = true;
  return found;
}
