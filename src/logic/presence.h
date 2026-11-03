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
constexpr float PRES_MOTION_SMOOTH = 0.15f;     // Blick zur Bewegung: stärker geglättet als beim Gesicht
constexpr uint32_t PRES_MOTION_HOLD_MS = 1000; // Bewegungsblick gilt so lange nach dem letzten M (M kommt höchstens alle 250 ms)
constexpr float PRES_GAZE_GAIN = 2.5f;         // Verstärkung der Abweichung von der Mitte (Pupillenweg größer)

struct PresenceOut { bool present, unattended, greet, near, gaze; float gx, gy; };

struct Presence {
  bool active = false;
  uint32_t since_ms = 0;        // Aktivierung (Bezug für „nie jemand da“)
  bool seen = false;            // je ein F seit Aktivierung
  uint32_t face_ms = 0;         // letztes F
  uint32_t live_ms = 0;         // letztes F oder M (Anwesenheit)
  bool face_seen = false;       // je ein F seit Aktivierung
  bool motion_seen = false; uint32_t motion_ms = 0;   // letztes M
  float mgx = 0, mgy = 0;       // geglättetes Blickziel zur Bewegung
  int16_t last_w = 0;           // Breite des letzten F
  bool present = false;
  bool was_away = false;        // seit Aktivierung schon einmal weg gewesen
  uint32_t away_ms = 0;         // letztes F vor dem Wegsein
  bool greeted = false; uint32_t greet_ms = 0; bool greet_pending = false;
  bool near_ever = false; uint32_t near_ms = 0; Deadline near;
  float gx = 0, gy = 0;         // geglättetes Blickziel
};

static inline float pres_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Alles vergessen, außer der Begrüßungssperre (sonst umgeht ein Kamera-Aus/An oder ein WLAN-Flackern die 10 min).
static inline void presence_reset(Presence &p) {
  const bool g = p.greeted; const uint32_t gm = p.greet_ms;
  p = Presence();
  p.greeted = g; p.greet_ms = gm;
}

// Bildposition 0..1000 → Blickziel in Gazer-Einheiten (±30/±15), +x rechts, +y unten.
// y0 = Bildhöhe, die "geradeaus" bedeutet (Kamera gekippt: Gesicht der sitzenden Person liegt nicht in der Mitte).
static inline void presence_gaze_raw(int16_t x, int16_t y, bool mirror, float *gx, float *gy, int16_t y0 = 500) {
  const float fx = mirror ? (float)(1000 - x) : (float)x;
  *gx = pres_clampf((fx - 500) / 500.0f * PRES_GAZE_X_MAX * PRES_GAZE_GAIN, -PRES_GAZE_X_MAX, PRES_GAZE_X_MAX);
  *gy = pres_clampf((float)(y - y0) / 500.0f * PRES_GAZE_Y_MAX * PRES_GAZE_GAIN, -PRES_GAZE_Y_MAX, PRES_GAZE_Y_MAX);
}

// Eine Meldung vom XIAO; zählt nur bei aktiver Kamera und nur F und M. Beides zählt für „da“, aber nur F für Nähe und den Vorrang beim Blick.
static inline void presence_msg(Presence &p, const CamMsg &m, uint32_t now, bool mirror = false, int16_t y0 = 500) {
  if (!p.active || (m.kind != CAM_FACE && m.kind != CAM_MOTION)) return;
  const bool pair = p.seen && !due_since(now, p.live_ms, PRES_PAIR_MS);
  float tx, ty;
  presence_gaze_raw(m.x, m.y, mirror, &tx, &ty, y0);
  if (m.kind == CAM_FACE) {
    p.gx += (tx - p.gx) * PRES_SMOOTH;
    p.gy += (ty - p.gy) * PRES_SMOOTH;
    p.face_seen = true;
    p.face_ms = now;
    p.last_w = m.w;
  } else {
    p.mgx += (tx - p.mgx) * PRES_MOTION_SMOOTH;
    p.mgy += (ty - p.mgy) * PRES_MOTION_SMOOTH;
    p.motion_seen = true;
    p.motion_ms = now;
  }
  p.seen = true;
  p.live_ms = now;
  if (!p.present && pair) {
    p.present = true;
    const uint32_t ref = p.was_away ? p.away_ms : p.since_ms;   // weg seit dem letzten F/M, sonst seit Aktivierung
    const bool long_away = due_since(now, ref, PRES_GREET_AWAY_MS);
    const bool gap_ok = !p.greeted || due_since(now, p.greet_ms, PRES_GREET_GAP_MS);
    if (long_away && gap_ok) { p.greet_pending = true; p.greeted = true; p.greet_ms = now; }
  }
}

// Einmal pro Bild. active = Kamera eingeschaltet, verbunden und ohne Fehler.
static inline PresenceOut presence_tick(Presence &p, bool active, uint32_t now, int16_t near_w) {
  PresenceOut o{};
  if (!active) { presence_reset(p); return o; }   // Ausfall: alles vergessen (Begrüßungssperre bleibt), Buddy wie vor der Kamera
  if (!p.active) { presence_reset(p); p.active = true; p.since_ms = now; }
  if (p.present && due_since(now, p.live_ms, PRES_GONE_MS)) { p.present = false; p.was_away = true; p.away_ms = p.live_ms; }
  if (p.present && p.last_w > near_w && !due_since(now, p.face_ms, PRES_GAZE_HOLD_MS) && !p.near.active(now) && (!p.near_ever || due_since(now, p.near_ms, PRES_NEAR_GAP_MS))) {
    p.near.arm(now, PRES_NEAR_FOR_MS);
    p.near_ms = now;
    p.near_ever = true;
  }
  o.present = p.present;
  o.unattended = !p.present && due_since(now, p.seen ? p.live_ms : p.since_ms, PRES_ALONE_MS);
  o.greet = p.greet_pending;
  p.greet_pending = false;
  o.near = p.near.active(now);
  const bool face_fresh = p.face_seen && !due_since(now, p.face_ms, PRES_GAZE_HOLD_MS);
  const bool motion_fresh = p.motion_seen && !due_since(now, p.motion_ms, PRES_MOTION_HOLD_MS);
  o.gaze = face_fresh || motion_fresh;
  const bool use_motion = !face_fresh && motion_fresh;   // frisches Gesicht hat Vorrang
  o.gx = use_motion ? p.mgx : p.gx;
  o.gy = use_motion ? p.mgy : p.gy;
  return o;
}
