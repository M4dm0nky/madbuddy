#include <Arduino.h>
#include <esp_camera.h>
#include "human_face_detect.hpp"
#include "detect.h"
#include "cam_config.h"
#include "camera.h"
#include "logic/cam_xiao.h"

static HumanFaceDetect *g_det = nullptr;

bool detect_init() {
  #if CAM_FACE_MODEL == 1
  g_det = new HumanFaceDetect(HumanFaceDetect::ESPDET_PICO_224_224_FACE);
#else
  g_det = new HumanFaceDetect(HumanFaceDetect::MSRMNP_S8_V1);
#endif
  return g_det != nullptr;
}

bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms) {
  if (!g_det) return false;  // detect_init() fehlgeschlagen
  const uint32_t t0 = millis();
  dl::image::img_t img = {};
  int w, h;
  const uint8_t *px = camera_rotated(fb, &w, &h);   // aufrechtes Bild (Einbaulage)
  img.data = (void *)px;
  img.width = w;
  img.height = h;
  img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE;   // esp32-camera liefert RGB565 big-endian
  std::list<dl::detect::result_t> &res = g_det->run(img);
  *infer_ms = millis() - t0;
  bool found = false;
  int best = 0;
  for (auto &r : res) {
    CamFace f;
    if (!cam_box_to_face(r.box[0], r.box[1], r.box[2], r.box[3], w, h, &f)) continue;
    if (!found || f.w > best) { *out = f; best = f.w; found = true; }
  }
  return found;
}
