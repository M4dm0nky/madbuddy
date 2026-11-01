#pragma once
#include <stdint.h>
#include "cam_proto.h"
#include "deadline.h"

// Kamera → Verhalten (Phase B): Anwesenheit, Begrüßung, Unbeaufsichtigt (Schlaf), Nähe, Blickziel. Reine Logik (test/test_presence).
constexpr uint32_t PRES_PAIR_MS = 1000;        // 2 Gesichter innerhalb dieser Zeit → da
constexpr uint32_t PRES_GONE_MS = 30000;       // so lange kein Gesicht → weg
constexpr uint32_t PRES_GREET_AWAY_MS = 120000;// mindestens so lange weg → Begrüßung
constexpr uint32_t PRES_GREET_GAP_MS = 600000; // höchstens alle 10 min
constexpr uint32_t PRES_ALONE_MS = 600000;     // 10 min weg → unbeaufsichtigt
constexpr uint32_t PRES_NEAR_FOR_MS = 2000;    // Nähe-Ausdruck so lange
constexpr uint32_t PRES_NEAR_GAP_MS = 30000;   // höchstens alle 30 s
constexpr uint32_t PRES_GAZE_HOLD_MS = 500;    // Kamerablick gilt so lange nach dem letzten F
constexpr float PRES_GAZE_X_MAX = 30, PRES_GAZE_Y_MAX = 15;   // wie GAZE_X_MAX/GAZE_Y_MAX
constexpr float PRES_SMOOTH = 0.25f;           // Anteil des neuen Ziels je Meldung

struct PresenceOut { bool present, unattended, greet, near, gaze; float gx, gy; };

struct Presence {
  bool active = false;
  uint32_t since_ms = 0;        // Aktivierung (Bezug für „nie jemand da“)
  bool seen = false;            // je ein F seit Aktivierung
  uint32_t face_ms = 0;         // letztes F
  int16_t last_w = 0;           // Breite des letzten F
  bool present = false;
  bool was_away = false;        // seit Aktivierung schon einmal weg gewesen
  uint32_t away_ms = 0;         // letztes F vor dem Wegsein
  bool greeted = false; uint32_t greet_ms = 0; bool greet_pending = false;
  bool near_ever = false; uint32_t near_ms = 0; Deadline near;
  float gx = 0, gy = 0;         // geglättetes Blickziel
};

static inline float pres_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Bildposition 0..1000 → Blickziel in Gazer-Einheiten (±30/±15), +x rechts, +y unten.
static inline void presence_gaze_raw(int16_t x, int16_t y, bool mirror, float *gx, float *gy) {
  const float fx = mirror ? (float)(1000 - x) : (float)x;
  *gx = pres_clampf((fx - 500) / 500.0f * PRES_GAZE_X_MAX, -PRES_GAZE_X_MAX, PRES_GAZE_X_MAX);
  *gy = pres_clampf((y - 500) / 500.0f * PRES_GAZE_Y_MAX, -PRES_GAZE_Y_MAX, PRES_GAZE_Y_MAX);
}

// Eine Meldung vom XIAO; zählt nur bei aktiver Kamera und nur F.
static inline void presence_msg(Presence &p, const CamMsg &m, uint32_t now, bool mirror = false) {
  if (!p.active || m.kind != CAM_FACE) return;
  const bool pair = p.seen && !due_since(now, p.face_ms, PRES_PAIR_MS);
  float tx, ty;
  presence_gaze_raw(m.x, m.y, mirror, &tx, &ty);
  p.gx += (tx - p.gx) * PRES_SMOOTH;
  p.gy += (ty - p.gy) * PRES_SMOOTH;
  p.seen = true;
  p.face_ms = now;
  p.last_w = m.w;
  if (!p.present && pair) {
    p.present = true;
    const bool long_away = p.was_away && due_since(now, p.away_ms, PRES_GREET_AWAY_MS);
    const bool gap_ok = !p.greeted || due_since(now, p.greet_ms, PRES_GREET_GAP_MS);
    if (long_away && gap_ok) { p.greet_pending = true; p.greeted = true; p.greet_ms = now; }
  }
}

// Einmal pro Bild. active = Kamera eingeschaltet, verbunden und ohne Fehler.
static inline PresenceOut presence_tick(Presence &p, bool active, uint32_t now, int16_t near_w) {
  PresenceOut o{};
  if (!active) { p = Presence(); return o; }   // Ausfall: alles vergessen, Buddy wie vor der Kamera
  if (!p.active) { p = Presence(); p.active = true; p.since_ms = now; }
  if (p.present && due_since(now, p.face_ms, PRES_GONE_MS)) { p.present = false; p.was_away = true; p.away_ms = p.face_ms; }
  if (p.present && p.last_w > near_w && !p.near.active(now) && (!p.near_ever || due_since(now, p.near_ms, PRES_NEAR_GAP_MS))) {
    p.near.arm(now, PRES_NEAR_FOR_MS);
    p.near_ms = now;
    p.near_ever = true;
  }
  o.present = p.present;
  o.unattended = !p.present && due_since(now, p.seen ? p.face_ms : p.since_ms, PRES_ALONE_MS);
  o.greet = p.greet_pending;
  p.greet_pending = false;
  o.near = p.near.active(now);
  o.gaze = p.seen && !due_since(now, p.face_ms, PRES_GAZE_HOLD_MS);
  o.gx = p.gx;
  o.gy = p.gy;
  return o;
}
