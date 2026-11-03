#include <Arduino.h>
#include "motion.h"
#include "camera.h"
#include "cam_config.h"
#include "logic/cam_xiao.h"

static uint8_t g_prev[MOT_CELLS], g_cur[MOT_CELLS];
static bool g_have_prev = false;

void motion_reset() { g_have_prev = false; }

bool motion_detect(camera_fb_t *fb, MotionResult *out) {
  int w, h;
  cam_rot_dims(fb->width, fb->height, CAM_ROTATE_CW, &w, &h);
  if (fb->format != PIXFORMAT_RGB565 || fb->len < (size_t)fb->width * fb->height * 2 || w < MOT_W * 2 || h < MOT_H * 2) return false;
  motion_grid_rot(fb->buf, fb->width, fb->height, CAM_ROTATE_CW, g_cur);
  bool found = false;
  if (g_have_prev) {
    *out = motion_compare(g_prev, g_cur);
    found = out->found;
  }
  memcpy(g_prev, g_cur, sizeof g_prev);
  g_have_prev = true;
  return found;
}
