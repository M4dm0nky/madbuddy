#pragma once
#include "clamp.h"
#include "mood.h"

// Charakter (F3): entwickelt sich aus dem Umgang, je -100..100. Reine Logik (Host-Test test/test_care).
struct Character { float cuddly, playful; };   // verschmust (+) / frech (-), verspielt (+) / faul (-)

constexpr float CHAR_LIMIT = 100;
inline void char_clamp(Character &c) {
  c.cuddly = clampf(c.cuddly, -CHAR_LIMIT, CHAR_LIMIT);
  c.playful = clampf(c.playful, -CHAR_LIMIT, CHAR_LIMIT);
}
inline void char_stroke(Character &c) { c.cuddly += 0.5f; char_clamp(c); }
inline void char_poke(Character &c) { c.cuddly -= 1.5f; char_clamp(c); }
inline void char_game(Character &c) { c.playful += 1.0f; char_clamp(c); }
inline void char_highfive(Character &c) { c.playful += 0.3f; char_clamp(c); }

inline float char_quirk_scale(const Character &c) { return 1.0f - c.playful / 200.0f; }   // Eigenarten-Abstand ×
inline float char_cheeky_prob(const Character &c) { return 0.5f - c.cuddly / 200.0f; }    // CHEEKY statt MISCHIEVOUS
inline float char_love_scale(const Character &c) { return 1.0f + c.cuddly / 200.0f; }     // LOVE-Dauer ×

inline void mood_set_character(MoodState &s, const Character &c) {
  s.quirk_scale = char_quirk_scale(c);
  s.cheeky_prob = char_cheeky_prob(c);
  s.love_scale = char_love_scale(c);
}
