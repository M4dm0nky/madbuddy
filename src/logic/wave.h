#pragma once
#include <stdint.h>
#include "deadline.h"

// Winken erkennen aus den Bewegungsmeldungen der Kamera (M: Schwerpunkt x 0..1000, n bewegte Zellen). Reine Logik (test/test_wave).
// Winken = Schwerpunkt wandert mehrmals hin und her. Eine Umkehrung zählt erst, wenn x um WAVE_MIN_SWING gegen die Richtung zurückgeht.
// VORLÄUFIG: alle Schwellen sind Startwerte und müssen am Gerät eingestellt werden.
constexpr int16_t WAVE_MIN_SWING = 120;         // VORLÄUFIG: Mindesthub je Richtung (von 1000 Bildbreite)
constexpr int16_t WAVE_MIN_CELLS = 8;           // VORLÄUFIG: weniger bewegte Zellen (von 30x40) ignorieren; die Kamera meldet erst ab 6
constexpr int WAVE_MIN_REVERSALS = 3;           // VORLÄUFIG: links-rechts-links-rechts
constexpr uint32_t WAVE_WINDOW_MS = 2500;       // VORLÄUFIG: so schnell müssen die Umkehrungen aufeinander folgen
constexpr uint32_t WAVE_GAP_MS = 800;           // VORLÄUFIG: länger keine Meldung = von vorn (M kommt höchstens alle 250 ms)
constexpr uint32_t WAVE_COOLDOWN_MS = 8000;     // VORLÄUFIG: nach einem Winken so lange kein neues
constexpr uint32_t WAVE_TAKE_MS = 1000;         // ein erkanntes Winken verfällt, wenn es so lange niemand abholt

struct WaveDetect {
  bool have = false; uint32_t last_ms = 0;      // letzte gezählte Meldung
  int16_t lo = 0, hi = 0;                       // vor der ersten Richtung: Spanne der bisherigen x
  int8_t dir = 0;                               // 0 = noch keine Richtung, +1 = rechts, -1 = links
  int16_t ext = 0;                              // äußerster x in der aktuellen Richtung
  uint32_t rev_ms[WAVE_MIN_REVERSALS] = {};     // Zeiten der letzten Umkehrungen (Ring)
  int rev_n = 0;                                // Zahl der Umkehrungen seit Reset
  bool waved = false; uint32_t wave_ms = 0;     // letztes erkanntes Winken (Sperrzeit)
  bool pending = false;                         // erkannt, noch nicht abgeholt
};

static inline void wave_reset_count(WaveDetect &w) { w.have = false; w.dir = 0; w.rev_n = 0; }

static inline void wave_feed(WaveDetect &w, int16_t x, int16_t n, uint32_t now) {
  if (n < WAVE_MIN_CELLS) return;
  if (w.have && due_since(now, w.last_ms, WAVE_GAP_MS)) wave_reset_count(w);
  w.last_ms = now;
  if (w.waved && !due_since(now, w.wave_ms, WAVE_COOLDOWN_MS)) { wave_reset_count(w); w.last_ms = now; return; }   // Sperrzeit
  if (!w.have) { w.have = true; w.lo = w.hi = x; return; }
  if (w.dir == 0) {                              // erste Richtung: Hub von WAVE_MIN_SWING seit dem Anfang
    if (x < w.lo) w.lo = x;
    if (x > w.hi) w.hi = x;
    if (x - w.lo >= WAVE_MIN_SWING) { w.dir = 1; w.ext = x; }
    else if (w.hi - x >= WAVE_MIN_SWING) { w.dir = -1; w.ext = x; }
    return;
  }
  if (w.dir > 0 ? x > w.ext : x < w.ext) { w.ext = x; return; }
  const int16_t back = w.dir > 0 ? (int16_t)(w.ext - x) : (int16_t)(x - w.ext);
  if (back < WAVE_MIN_SWING) return;
  w.dir = (int8_t)-w.dir;                        // Umkehrung
  w.ext = x;
  w.rev_ms[w.rev_n % WAVE_MIN_REVERSALS] = now;
  w.rev_n++;
  if (w.rev_n >= WAVE_MIN_REVERSALS) {
    const uint32_t oldest = w.rev_ms[w.rev_n % WAVE_MIN_REVERSALS];   // ältester der letzten WAVE_MIN_REVERSALS
    if (!due_since(now, oldest, WAVE_WINDOW_MS)) {
      w.pending = true; w.waved = true; w.wave_ms = now;
      wave_reset_count(w);
    }
  }
}

// Einmal pro erkanntem Winken true.
static inline bool wave_take(WaveDetect &w, uint32_t now) {
  const bool fresh = w.pending && !due_since(now, w.wave_ms, WAVE_TAKE_MS);
  w.pending = false;
  return fresh;
}
