#pragma once
#include <stdint.h>
#include "clamp.h"
#include "rng.h"
#include "deadline.h"

// BedÃ¼rfnisse und GefÃ¼hl des Buddys. Reine Logik, Zeit kommt als Parameter (Host-Test test/test_mood).
enum class Emotion : uint8_t { CONTENT, HAPPY, BORED, SAD, SLEEPY, SLEEPING, SURPRISED, LOVE, ANNOYED, EXCITED, GREETING,
                            CONFUSED, MISCHIEVOUS, SCARED, SKEPTICAL, PROUD, GIGGLE, YAWN, WHISTLE, KISS, CHEEKY, STARTLED,
                            FOCUSED, THINKING, NEAR, COUNT };
enum class MoodEvent : uint8_t { TAP, STROKE, POKE, GAME_WON, GAME_LOST, HIGHFIVE };
struct Needs { float affection, fun, energy; float satiety = 70; };   // satiety = SÃ¤ttigung (F3)
struct MoodState {
  Needs n;
  Emotion transient;          // CONTENT = keins
  uint32_t transient_set_ms;  // Zeitpunkt des Setzens; Alter = now - set (unsigned, 24,8-Tage-sicher)
  uint32_t transient_dur_ms;
  uint32_t last_stroke_ms;
  bool stroked_once;
  bool napping;               // Nickerchen wegen Energie < 10
  bool low_prev;              // Energie war beim letzten Tick < 10 (Flanke fÃ¼rs Nickerchen)
  bool dark;
  uint32_t dark_since;
  bool night_sleep;           // Nachtschlaf: nachts nach MOOD_NIGHT_IDLE_MS ohne BerÃ¼hrung
  uint32_t last_touch_ms;
  // AuslÃ¶ser (Spec Gesicht)
  uint32_t stroke_hist[3];    // Zeitpunkte der letzten 3 Streichler (auch im Cooldown)
  uint8_t stroke_n;
  uint32_t last_tap_ms;
  bool tapped_once;
  Emotion follow;             // Folge-GefÃ¼hl nach dem kurzzeitigen (CONTENT = keins)
  uint32_t follow_ms;
  // Eigenarten
  uint32_t quirk_seed;
  uint32_t quirk_set_ms;      // Abstand seit dem Planen statt Zielzeitpunkt (24,8-Tage-sicher)
  uint32_t quirk_wait_ms;
  bool quirk_scheduled;
  bool quirk_base_known;
  Emotion quirk_base;
  // Charakter (F3, Ã¼ber mood_set_character)
  float quirk_scale;          // Eigenarten-Abstand Ã—
  float cheeky_prob;          // CHEEKY statt MISCHIEVOUS
  float love_scale;           // LOVE-Dauer Ã—
};

constexpr float MOOD_AFFECTION_PER_MIN = -0.2f;
constexpr float MOOD_FUN_PER_MIN = -0.33f;
constexpr float MOOD_ENERGY_AWAKE_PER_MIN = -0.17f;
constexpr float MOOD_ENERGY_SLEEP_PER_MIN = 1.0f;
constexpr uint32_t MOOD_DARK_SLEEP_MS = 10000;
constexpr uint32_t MOOD_STROKE_COOLDOWN_MS = 2000;
constexpr uint32_t MOOD_NIGHT_IDLE_MS = 10UL * 60UL * 1000UL;
constexpr float MOOD_SATIETY_AWAKE_PER_MIN = -0.12f;
constexpr float MOOD_SATIETY_SLEEP_PER_MIN = -0.04f;

// Untergrenze fuer Zuneigung, Unterhaltung und Saettigung: Vernachlaessigen fuehrt nie zu schlechter Stimmung.
constexpr float MOOD_NEUTRAL_FLOOR = 50;
inline void mood_clamp(Needs &n) {
  n.affection = clampf(n.affection, MOOD_NEUTRAL_FLOOR, 100);
  n.fun = clampf(n.fun, MOOD_NEUTRAL_FLOOR, 100);
  n.energy = clampf(n.energy, 0, 100);
  n.satiety = clampf(n.satiety, MOOD_NEUTRAL_FLOOR, 100);
}
inline void mood_init(MoodState &s, Needs start) {
  s = MoodState{};
  s.n = start;
  s.transient = Emotion::CONTENT;
  s.quirk_scale = 1;
  s.cheeky_prob = 0.5f;
  s.love_scale = 1;
  mood_clamp(s.n);
}
// Nacht = 22-7 Uhr (fÃ¼r Nachtschlaf und SLEEPY); hour < 0 = Uhrzeit unbekannt = nie Nacht.
inline bool mood_is_night(int hour) { return hour >= 0 && (hour >= 22 || hour < 7); }
inline bool mood_daytime(int hour) { return !mood_is_night(hour); }
inline bool mood_is_sleeping(const MoodState &s, uint32_t now_ms) {
  return s.napping || s.night_sleep || (s.dark && now_ms - s.dark_since > MOOD_DARK_SLEEP_MS);
}
inline void mood_set_transient(MoodState &s, Emotion e, uint32_t now_ms, uint32_t dur_ms) {
  s.transient = e;
  s.transient_set_ms = now_ms;
  s.transient_dur_ms = dur_ms;
  s.follow = Emotion::CONTENT;   // ein neues GefÃ¼hl bricht die Folge ab
  s.follow_ms = 0;
}
inline void mood_set_dark(MoodState &s, bool dark, int hour, uint32_t now_ms) {
  if (dark == s.dark) return;
  const bool was_sleeping = mood_is_sleeping(s, now_ms);
  s.dark = dark;
  if (dark) s.dark_since = now_ms;
  else if (was_sleeping && mood_daytime(hour)) mood_set_transient(s, Emotion::GREETING, now_ms, 3000);
}
// Jede BerÃ¼hrung (auch MenÃ¼, Spiele): weckt aus Nickerchen und Nachtschlaf und startet die Ruhezeit neu.
inline void mood_touch(MoodState &s, uint32_t now_ms) {
  s.napping = false;
  s.night_sleep = false;
  s.last_touch_ms = now_ms;
}
// Jedes Bild aufrufen. Nachts (22-7 Uhr) nach 10 min Ruhe einschlafen, morgens mit BegrÃ¼ÃŸung aufwachen.
// Der Lichtsensor des CYD erkennt Dunkelheit kaum (Beschaltung, das Display leuchtet ihn an), darum zÃ¤hlt vor allem die Uhrzeit.
inline void mood_night_check(MoodState &s, int hour, uint32_t now_ms) {
  if (!mood_is_night(hour)) {
    if (s.night_sleep) {
      s.night_sleep = false;
      if (!mood_is_sleeping(s, now_ms)) mood_set_transient(s, Emotion::GREETING, now_ms, 3000);
    }
    return;
  }
  if (!s.night_sleep && now_ms - s.last_touch_ms > MOOD_NIGHT_IDLE_MS) s.night_sleep = true;
}
inline void mood_tick(MoodState &s, uint32_t dt_ms, uint32_t now_ms) {
  const float min = dt_ms / 60000.0f;
  if (mood_is_sleeping(s, now_ms)) {
    s.n.energy += MOOD_ENERGY_SLEEP_PER_MIN * min;   // im Schlaf nur Erholung, kein Abbau
    s.n.satiety += MOOD_SATIETY_SLEEP_PER_MIN * min;
  } else {
    s.n.affection += MOOD_AFFECTION_PER_MIN * min;
    s.n.fun += MOOD_FUN_PER_MIN * min;
    s.n.energy += MOOD_ENERGY_AWAKE_PER_MIN * min;
    s.n.satiety += MOOD_SATIETY_AWAKE_PER_MIN * min;
  }
  mood_clamp(s.n);
  // Nickerchen nur beim Unterschreiten von 10 (Flanke) oder 10 min nach der letzten BerÃ¼hrung; eine BerÃ¼hrung beendet es.
  const bool low = s.n.energy < 10;
  if (low && !s.napping && (!s.low_prev || now_ms - s.last_touch_ms > MOOD_NIGHT_IDLE_MS)) s.napping = true;
  else if (s.napping && s.n.energy > 60) s.napping = false;
  s.low_prev = low;
  // Abgelaufenes kurzzeitiges Gefuehl samt Folge loeschen, damit es nach 49,7 Tagen nicht wieder aufleben kann (mood_emotion ist const).
  if (s.transient != Emotion::CONTENT && !(now_ms - s.transient_set_ms < s.transient_dur_ms + (s.follow != Emotion::CONTENT ? s.follow_ms : 0))) {
    s.transient = Emotion::CONTENT;
    s.follow = Emotion::CONTENT;
    s.follow_ms = 0;
  }
}
constexpr uint32_t MOOD_GIGGLE_WINDOW_MS = 20000;
constexpr uint32_t MOOD_DOUBLE_TAP_MS = 500;

// GrundgefÃ¼hl: aus den BedÃ¼rfnissen, ohne Schlaf und ohne kurzzeitige GefÃ¼hle.
inline Emotion mood_base_emotion(const MoodState &s, int hour) {
  if (s.n.energy < 30 || mood_is_night(hour)) return Emotion::SLEEPY;
  if (s.n.affection > 70 && s.n.fun > 60) return Emotion::HAPPY;
  return Emotion::CONTENT;
}
// hour nur fÃ¼r POKE nÃ¶tig; -1 = unbekannt.
inline void mood_event(MoodState &s, MoodEvent e, uint32_t now_ms, int hour = -1) {
  mood_touch(s, now_ms);   // jede BerÃ¼hrung weckt
  switch (e) {
    case MoodEvent::TAP: {
      const bool dbl = s.tapped_once && now_ms - s.last_tap_ms < MOOD_DOUBLE_TAP_MS;
      s.tapped_once = true;
      s.last_tap_ms = now_ms;
      s.n.fun += 1;
      if (dbl) mood_set_transient(s, Emotion::CONFUSED, now_ms, 2500);
      else mood_set_transient(s, Emotion::SURPRISED, now_ms, 1500);
      break;
    }
    case MoodEvent::STROKE: {
      // Die letzten 3 Streichler merken (auch im Cooldown): der 3. innerhalb 20 s lÃ¤sst ihn kichern.
      s.stroke_hist[0] = s.stroke_hist[1];
      s.stroke_hist[1] = s.stroke_hist[2];
      s.stroke_hist[2] = now_ms;
      if (s.stroke_n < 3) s.stroke_n++;
      const bool giggle = s.stroke_n >= 3 && now_ms - s.stroke_hist[0] < MOOD_GIGGLE_WINDOW_MS;
      const bool cooldown = s.stroked_once && now_ms - s.last_stroke_ms < MOOD_STROKE_COOLDOWN_MS;
      if (!cooldown) {
        s.stroked_once = true;
        s.last_stroke_ms = now_ms;
        s.n.affection += 8;
        mood_set_transient(s, s.n.affection > 90 ? Emotion::KISS : Emotion::LOVE, now_ms,
                           s.n.affection > 90 ? 3000 : (uint32_t)(4000 * s.love_scale));
      }
      if (giggle) {
        s.stroke_n = 0;
        mood_set_transient(s, Emotion::GIGGLE, now_ms, 3000);
      }
      break;
    }
    case MoodEvent::POKE: {
      s.n.affection -= 3;
      mood_set_transient(s, Emotion::ANNOYED, now_ms, 20000);
      break;
    }
    case MoodEvent::GAME_WON:
      s.n.fun += 25;
      s.n.affection += 5;
      mood_set_transient(s, Emotion::EXCITED, now_ms, 6000);
      s.follow = Emotion::PROUD;
      s.follow_ms = 4000;
      break;
    case MoodEvent::GAME_LOST: s.n.fun += 10; mood_set_transient(s, Emotion::SKEPTICAL, now_ms, 4000); break;
    case MoodEvent::HIGHFIVE: s.n.fun += 5; s.n.affection += 2; mood_set_transient(s, Emotion::EXCITED, now_ms, 2000); break;
  }
  mood_clamp(s.n);
}
inline Emotion mood_emotion(const MoodState &s, int hour, uint32_t now_ms) {
  if (mood_is_sleeping(s, now_ms)) return Emotion::SLEEPING;
  if (s.transient != Emotion::CONTENT) {
    const uint32_t age = now_ms - s.transient_set_ms;   // unsigned: stimmt auch ueber den millis()-Ueberlauf und nach >24,8 Tagen
    if (age < s.transient_dur_ms) return s.transient;
    if (s.follow != Emotion::CONTENT && age < s.transient_dur_ms + s.follow_ms) return s.follow;
  }
  return mood_base_emotion(s, hour);
}

// Zufallszustand fÃ¼r die Eigenarten (einmal beim Start, z. B. esp_random()).
inline void mood_seed(MoodState &s, uint32_t seed) { s.quirk_seed = seed ? seed : 1; }
// Eigenarten (GÃ¤hnen, Pfeifen, Schelmisch/Frech): jedes Bild aufrufen. Nur wenn er wach ist, kein anderes
// kurzzeitiges GefÃ¼hl lÃ¤uft und das GrundgefÃ¼hl SLEEPY oder HAPPY ist. NÃ¤chster Zeitpunkt wird
// beim Wechsel des GrundgefÃ¼hls und nach jeder Eigenart neu gezogen.
inline void mood_quirks(MoodState &s, int hour, uint32_t now_ms) {
  if (mood_is_sleeping(s, now_ms)) return;
  const Emotion base = mood_base_emotion(s, hour);
  uint32_t lo = 0, hi = 0;
  switch (base) {
    case Emotion::SLEEPY: lo = 120000; hi = 300000; break;
    case Emotion::HAPPY:  lo = 120000; hi = 360000; break;
    default: break;
  }
  lo = (uint32_t)(lo * s.quirk_scale);   // Charakter: verspielt = hÃ¤ufiger
  hi = (uint32_t)(hi * s.quirk_scale);
  if (!s.quirk_base_known || base != s.quirk_base) {
    s.quirk_base_known = true;
    s.quirk_base = base;
    s.quirk_scheduled = hi != 0;
    if (hi) { s.quirk_set_ms = now_ms; s.quirk_wait_ms = rng_range(s.quirk_seed, lo, hi); }
  }
  if (!s.quirk_scheduled) return;
  if (mood_emotion(s, hour, now_ms) != base) return;   // anderes kurzzeitiges GefÃ¼hl lÃ¤uft
  if (!due_since(now_ms, s.quirk_set_ms, s.quirk_wait_ms)) return;
  switch (base) {
    case Emotion::SLEEPY: mood_set_transient(s, Emotion::YAWN, now_ms, 3000); break;
    default:
      mood_set_transient(s, rng_range(s.quirk_seed, 0, 999) < (uint32_t)(s.cheeky_prob * 1000) ? Emotion::CHEEKY : Emotion::MISCHIEVOUS,
                         now_ms, 3000);
      break;
  }
  s.quirk_set_ms = now_ms;
  s.quirk_wait_ms = rng_range(s.quirk_seed, lo, hi);
}
// Zeit, in der das GerÃ¤t aus war, zÃ¤hlt wie Schlaf: Energie steigt, SÃ¤ttigung sinkt langsam.
inline void mood_apply_elapsed(MoodState &s, uint32_t seconds) {
  s.n.energy += MOOD_ENERGY_SLEEP_PER_MIN * (seconds / 60.0f);
  s.n.satiety += MOOD_SATIETY_SLEEP_PER_MIN * (seconds / 60.0f);
  mood_clamp(s.n);
}
