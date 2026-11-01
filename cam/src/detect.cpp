#include <Arduino.h>
#include <esp_camera.h>
#include "human_face_detect.hpp"
#include "detect.h"
#include "logic/cam_xiao.h"

static HumanFaceDetect *g_det = nullptr;

bool detect_init() {
  g_det = new HumanFaceDetect();
  return g_det != nullptr;
}

bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms) {
  if (!g_det) return false;  // detect_init() fehlgeschlagen
  const uint32_t t0 = millis();
  dl::image::img_t img = {};
  img.data = fb->buf;
  img.width = fb->width;
  img.height = fb->height;
  img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE;   // esp32-camera liefert RGB565 big-endian
  std::list<dl::detect::result_t> &res = g_det->run(img);
  *infer_ms = millis() - t0;
  bool found = false;
  int best = 0;
  for (auto &r : res) {
    CamFace f;
    if (!cam_box_to_face(r.box[0], r.box[1], r.box[2], r.box[3], fb->width, fb->height, &f)) continue;
    if (!found || f.w > best) { *out = f; best = f.w; found = true; }
  }
  return found;
}
