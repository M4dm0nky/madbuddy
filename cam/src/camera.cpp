#include <esp_camera.h>
#include "camera.h"
#include <esp_heap_caps.h>
#include "cam_config.h"
#include "logic/cam_xiao.h"

static bool g_on = false;

bool camera_start() {
#ifdef CAM_TEST_BROKEN
  return false;   // Testbild für den Rollback (Task 5)
#endif
  if (g_on) return true;
  camera_config_t c = {};
  c.pin_pwdn = CAM_PIN_PWDN;   c.pin_reset = CAM_PIN_RESET; c.pin_xclk = CAM_PIN_XCLK;
  c.pin_sccb_sda = CAM_PIN_SIOD; c.pin_sccb_scl = CAM_PIN_SIOC;
  c.pin_d7 = CAM_PIN_D7; c.pin_d6 = CAM_PIN_D6; c.pin_d5 = CAM_PIN_D5; c.pin_d4 = CAM_PIN_D4;
  c.pin_d3 = CAM_PIN_D3; c.pin_d2 = CAM_PIN_D2; c.pin_d1 = CAM_PIN_D1; c.pin_d0 = CAM_PIN_D0;
  c.pin_vsync = CAM_PIN_VSYNC; c.pin_href = CAM_PIN_HREF; c.pin_pclk = CAM_PIN_PCLK;
  c.xclk_freq_hz = 20000000;
  c.ledc_timer = LEDC_TIMER_0; c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_RGB565;
  c.frame_size = FRAMESIZE_QVGA;
  c.fb_count = 2;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  g_on = esp_camera_init(&c) == ESP_OK;
#if CAM_FLIP_180
  if (g_on) {
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  }
#endif
  return g_on;
}

void camera_stop() {
  if (!g_on) return;
  esp_camera_deinit();
  g_on = false;
}

bool camera_running() { return g_on; }

const uint8_t *camera_rotated(const camera_fb_t *fb, int *w, int *h) {
  *w = fb->width; *h = fb->height;
#if CAM_ROTATE_CW
  static uint8_t *buf = nullptr;
  static size_t cap = 0;
  const size_t need = (size_t)fb->width * fb->height * 2;
  if (fb->format != PIXFORMAT_RGB565 || fb->len < need) return fb->buf;   // nur RGB565 (2 Byte je Pixel)
  if (!buf) {
    cap = 320 * 240 * 2;
    buf = (uint8_t *)heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
    if (!buf) cap = 0;
  }
  if (!buf || need > cap) return fb->buf;   // kein Puffer: ungedreht weiterarbeiten
  cam_rotate_px2(fb->buf, fb->width, fb->height, buf, CAM_ROTATE_CW);
  cam_rot_dims(fb->width, fb->height, CAM_ROTATE_CW, w, h);
  return buf;
#else
  return fb->buf;
#endif
}
