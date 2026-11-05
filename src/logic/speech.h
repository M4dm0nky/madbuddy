#pragma once
#include <stdint.h>
#include "rng.h"
#include "deadline.h"
#include "mood.h"
#include "phrases.h"

// Wann der Buddy etwas sagt und welchen Satz (F2). Reine Logik (Host-Test test/test_speech).
constexpr uint32_t SPEECH_GAP_MS = 90000;                 // höchstens eine Blase alle 90 s (Rituale/Ereignisse ausgenommen)
constexpr uint32_t SPEECH_THOUGHT_MIN_MS = 5UL * 60000UL;
constexpr uint32_t SPEECH_THOUGHT_MAX_MS = 10UL * 60000UL;
constexpr uint8_t SPEECH_BASE_CHANCE_PCT = 40;            // beim Wechsel des Grundgefühls
constexpr float SPEECH_CHARACTER_LIMIT = 40;              // |cuddly| darüber: frech bzw. verschmust statt Gedanken

struct SpeechState {
  uint8_t last[(int)Say::COUNT];   // zuletzt gesagter Satz je Kategorie + 1 (0 = noch keiner)
  uint32_t rng;
  uint32_t last_ms;                // letzte Blase
  bool any;                        // überhaupt schon eine Blase
  uint32_t thought_set_ms;         // Abstand seit dem Planen statt Zielzeitpunkt (24,8-Tage-sicher)
  uint32_t thought_wait_ms;
  Emotion base;
  bool base_known;
};

inline void speech_schedule_thought(SpeechState &s, uint32_t now_ms) {
  s.thought_set_ms = now_ms;
  s.thought_wait_ms = rng_range(s.rng, SPEECH_THOUGHT_MIN_MS, SPEECH_THOUGHT_MAX_MS);
}
inline void speech_init(SpeechState &s, uint32_t now_ms, uint32_t seed) {
  s = SpeechState{};
  s.rng = seed ? seed : 1;
  speech_schedule_thought(s, now_ms);
}
// Satz aus der Kategorie; nie zweimal hintereinander derselbe (je Kategorie).
inline const char *speech_pick(SpeechState &s, Say say, uint32_t seed) {
  const SayList l = say_list(say);
  if (!l.items || l.n == 0) return "";
  s.rng ^= seed;
  uint8_t i = (uint8_t)rng_range(s.rng, 0, l.n - 1);
  uint8_t &last = s.last[(int)say];
  if (l.n > 1 && last == i + 1) i = (uint8_t)((i + 1) % l.n);
  last = (uint8_t)(i + 1);
  return l.items[i];
}
inline bool speech_may(const SpeechState &s, uint32_t now_ms) { return !s.any || now_ms - s.last_ms >= SPEECH_GAP_MS; }
// Jede gezeigte Blase melden (auch Rituale/Ereignisse): startet die 90 s und den Gedanken-Abstand neu.
inline void speech_shown(SpeechState &s, uint32_t now_ms) {
  s.any = true;
  s.last_ms = now_ms;
  speech_schedule_thought(s, now_ms);
}
// Kategorie für ein neues Grundgefühl oder COUNT (keine).
inline Say speech_for_base(Emotion e) {
  switch (e) {
    case Emotion::HAPPY: return Say::FROH;
    default: return Say::COUNT;
  }
}
// Jedes Bild aufrufen (Grundgefühl = mood_base_emotion). true = jetzt eine Blase mit *out zeigen (bereits als gezeigt gezählt).
// cuddly: Charakter -100..100 (F3), > 40 verschmust, < -40 frech statt Gedanken.
inline bool speech_due(SpeechState &s, Emotion base, uint32_t now_ms, Say *out, float cuddly) {
  const bool changed = s.base_known && base != s.base;
  s.base = base;
  s.base_known = true;
  if (changed) {
    const Say say = speech_for_base(base);
    if (say != Say::COUNT && speech_may(s, now_ms) && rng_range(s.rng, 0, 99) < SPEECH_BASE_CHANCE_PCT) {
      *out = say;
      speech_shown(s, now_ms);
      return true;
    }
  }
  if (!due_since(now_ms, s.thought_set_ms, s.thought_wait_ms)) return false;
  if (!speech_may(s, now_ms)) { speech_schedule_thought(s, now_ms); return false; }
  *out = cuddly > SPEECH_CHARACTER_LIMIT ? Say::VERSCHMUST : (cuddly < -SPEECH_CHARACTER_LIMIT ? Say::FRECH : Say::GEDANKEN);
  speech_shown(s, now_ms);
  return true;
}
