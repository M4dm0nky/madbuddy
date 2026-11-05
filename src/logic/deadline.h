#pragma once
#include <stdint.h>

// Zeitablauf fÃ¼r lange Lebenszeiten (Blase, Hand, LED-Blitz ...). Host-Test test/test_deadline.
// Das Muster (int32_t)(until - now) > 0 wird nach 2^31 ms (24,8 Tage) wieder wahr, wenn nie neu gesetzt wurde.
// Darum: Start + Dauer + "an"-Flag; active() schaltet sich beim Ablauf selbst aus, und selbst ohne Abfrage
// bleibt es bis 2^32 ms (49,7 Tage) nach dem Start aus. Ein nie bewaffnetes Deadline ist nie aktiv.
// "Warte bis": Abstand seit dem Setzen statt Zielzeitpunkt. Ein lange liegengebliebener Zeitpunkt ist faellig (nach 24,8 Tagen
// warnt (int32_t)(now - ziel) < 0 sonst faelschlich "noch nicht" und wuerde bis zu 24,8 Tage Wartezeit erzeugen).
inline bool due_since(uint32_t now, uint32_t set_ms, uint32_t wait_ms) { return now - set_ms >= wait_ms; }

struct Deadline {
  uint32_t start = 0;
  uint32_t dur = 0;
  bool on = false;
  void arm(uint32_t now, uint32_t duration) { start = now; dur = duration; on = true; }
  void clear() { on = false; }
  bool active(uint32_t now) {
    if (!on) return false;
    if (now - start < dur) return true;
    on = false;
    return false;
  }
};

// Wiederkehrende Wartezeit ("in x ms wieder"): Startzeit + Dauer statt Zielzeitpunkt, ueberlaufsicher. Nie gesetzt = sofort faellig.
struct Wait {
  uint32_t set = 0;
  uint32_t wait = 0;
  void start(uint32_t now, uint32_t duration) { set = now; wait = duration; }
  bool due(uint32_t now) const { return due_since(now, set, wait); }
};
