#pragma once
#include <stdint.h>

// Reine Logik der XIAO-Firmware (cam/): Gesichtsbox -> Protokollwerte, Entscheidung im Probelauf nach einem Update.
// Ohne Arduino-/IDF-Includes, Host-Test test/test_cam_proto.

struct CamFace { int16_t x, y, w; };   // Mitte x/y und Breite, je 0..1000 bezogen auf das Bild

static inline int cam_clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Box (x1,y1)-(x2,y2) in Bildpixeln. false bei leerer Box oder Bildgröße 0.
static inline bool cam_box_to_face(int x1, int y1, int x2, int y2, int img_w, int img_h, CamFace *f) {
  if (img_w <= 0 || img_h <= 0) return false;
  x1 = cam_clampi(x1, 0, img_w); x2 = cam_clampi(x2, 0, img_w);
  y1 = cam_clampi(y1, 0, img_h); y2 = cam_clampi(y2, 0, img_h);
  if (x2 <= x1 || y2 <= y1) return false;
  f->x = (int16_t)(((long)(x1 + x2) * 500) / img_w);
  f->y = (int16_t)(((long)(y1 + y2) * 500) / img_h);
  f->w = (int16_t)(((long)(x2 - x1) * 1000) / img_w);
  return true;
}

// Probelauf nach einem Update: gilt als gut nach 20 s ohne Kamerafehler mit mindestens 5 Lebenszeichen,
// sonst Rollback nach 60 s. cam_ok = Kamera seit dem Start einmal gelaufen und seitdem kein Fehler.
// up_ms = Zeit seit dem Start des neuen Images (kurz, höchstens ~60 s; kein Langzeitvergleich).
enum CamHealth : uint8_t { CAM_HEALTH_WAIT = 0, CAM_HEALTH_VALID, CAM_HEALTH_ROLLBACK };
constexpr uint32_t CAM_HEALTH_MIN_MS = 20000;
constexpr uint32_t CAM_HEALTH_MAX_MS = 60000;
constexpr uint32_t CAM_HEALTH_BEATS = 5;

static inline CamHealth cam_health(uint32_t up_ms, bool cam_ok, uint32_t beats) {
  if (up_ms >= CAM_HEALTH_MIN_MS && cam_ok && beats >= CAM_HEALTH_BEATS) return CAM_HEALTH_VALID;
  if (up_ms >= CAM_HEALTH_MAX_MS) return CAM_HEALTH_ROLLBACK;
  return CAM_HEALTH_WAIT;
}
