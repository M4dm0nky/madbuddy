#pragma once
#include <stdint.h>
#include "mood.h"

// Bedürfnis-Hinweis: welche Bedürfnisse sind niedrig (stehende Symbole) und wann kommt eine Blase.
// Reine Logik, Zeit als Parameter (Host-Test test/test_need_hint).
enum class Need : uint8_t { SICK, HUNGRY, SLEEPY, SAD, BORED, COUNT };   // Reihenfolge = Dringlichkeit

constexpr int NEED_COUNT = (int)Need::COUNT;
constexpr uint32_t NEED_REMIND_MS = 15UL * 60UL * 1000UL;
constexpr float NEED_HYST = 5;
constexpr float NEED_HUNGRY_BELOW = 25, NEED_SLEEPY_BELOW = 30, NEED_SAD_BELOW = 20, NEED_BORED_BELOW = 25;

inline uint8_t need_bit(Need n) { return (uint8_t)(1u << (uint8_t)n); }

struct NeedHintState {
  uint8_t mask = 0;       // aktive (niedrige) Bedürfnisse, mit Hysterese
  uint8_t due = 0;        // Blase fällig, bis need_hint_shown() sie quittiert
  uint32_t last_ms[NEED_COUNT] = {0, 0, 0, 0, 0};   // Zeitpunkt der letzten Blase je Bedürfnis
};

struct NeedHintOut {
  uint8_t mask;   // Bits (need_bit) aller niedrigen Bedürfnisse
  Need due;       // dringendstes Bedürfnis mit fälliger Blase, Need::COUNT = keins
};

inline float need_value(const MoodState &s, Need n) {
  switch (n) {
    case Need::HUNGRY: return s.n.satiety;
    case Need::SLEEPY: return s.n.energy;
    case Need::SAD: return s.n.affection;
    case Need::BORED: return s.n.fun;
    default: return 0;
  }
}
inline float need_threshold(Need n) {
  switch (n) {
    case Need::HUNGRY: return NEED_HUNGRY_BELOW;
    case Need::SLEEPY: return NEED_SLEEPY_BELOW;
    case Need::SAD: return NEED_SAD_BELOW;
    case Need::BORED: return NEED_BORED_BELOW;
    default: return 0;
  }
}

inline NeedHintOut need_hint_update(NeedHintState &st, const MoodState &s, uint32_t now_ms) {
  for (int i = 0; i < NEED_COUNT; i++) {
    const Need n = (Need)i;
    const uint8_t b = need_bit(n);
    const bool was = st.mask & b;
    bool low;
    if (n == Need::SICK) low = s.sick;
    else if (was) low = need_value(s, n) <= need_threshold(n) + NEED_HYST;   // erst über Schwelle + 5 erfüllt
    else low = need_value(s, n) < need_threshold(n);
    if (low && !was) { st.mask |= b; st.due |= b; }                          // Flanke
    else if (!low && was) { st.mask &= (uint8_t)~b; st.due &= (uint8_t)~b; }
    else if (low && !(st.due & b) && now_ms - st.last_ms[i] >= NEED_REMIND_MS) st.due |= b;   // Erinnerung
  }
  NeedHintOut out{st.mask, Need::COUNT};
  for (int i = 0; i < NEED_COUNT; i++)
    if (st.due & (1u << i)) { out.due = (Need)i; break; }
  return out;
}

// Blase wurde gezeigt: fällig-Flag löschen, Erinnerungszeit neu starten.
inline void need_hint_shown(NeedHintState &st, Need n, uint32_t now_ms) {
  if (n == Need::COUNT) return;
  st.due &= (uint8_t)~need_bit(n);
  st.last_ms[(int)n] = now_ms;
}

// Texte UTF-8 (Latin-1-Schrift, kein Gedankenstrich/Ellipsen-Zeichen), alle unter SPEECH_CHARACTER_LIMIT = 40.
inline const char *need_text(Need n) {
  switch (n) {
    case Need::SICK: return "Krank! F\xC3\xBCttern + streicheln";
    case Need::HUNGRY: return "Ich hab Hunger!";
    case Need::SLEEPY: return "Bin so m\xC3\xBCde ...";
    case Need::SAD: return "Streichel mich mal!";
    case Need::BORED: return "Spiel mit mir!";
    default: return "";
  }
}
