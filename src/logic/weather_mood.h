#pragma once
#include <stdint.h>
#include "mood.h"

// Wetter -> Himmel + Flags, und welche Overlays die Augen bekommen. Reine Logik (Host-Test test/test_weather_mood).
enum class Sky : uint8_t { SUNNY, CLOUDY, FOG, RAIN, SNOW, STORM };
struct WeatherMood { Sky sky; bool hot; bool cold; };

constexpr float WEATHER_HOT_C = 28.0f;
constexpr float WEATHER_COLD_C = 0.0f;
constexpr uint32_t WEATHER_MAX_AGE_MS = 3UL * 3600UL * 1000UL;

// WMO-Wettercode (Open-Meteo `weather_code`) und Temperatur.
inline WeatherMood weather_mood(int wmo_code, float temp_c) {
  Sky sky;
  const int c = wmo_code;
  if (c == 0 || c == 1) sky = Sky::SUNNY;
  else if (c == 2 || c == 3) sky = Sky::CLOUDY;
  else if (c == 45 || c == 48) sky = Sky::FOG;
  else if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) sky = Sky::RAIN;
  else if ((c >= 71 && c <= 77) || c == 85 || c == 86) sky = Sky::SNOW;
  else if (c >= 95 && c <= 99) sky = Sky::STORM;
  else sky = Sky::CLOUDY;
  return WeatherMood{sky, temp_c > WEATHER_HOT_C, temp_c < WEATHER_COLD_C || sky == Sky::SNOW};
}

// fetched_ms 0 = nie abgerufen.
inline bool weather_fresh(uint32_t now_ms, uint32_t fetched_ms) {
  return fetched_ms != 0 && now_ms - fetched_ms < WEATHER_MAX_AGE_MS;
}

enum Overlay : uint32_t {
  OV_HEART = 1, OV_SUNGLASSES = 2, OV_SWEAT = 4, OV_TEAR = 8,
  OV_ZZZ = 16, OV_SHIVER = 32, OV_RAIN_TINT = 64, OV_STORM = 128,
  OV_QUESTION = 256, OV_NOTE = 512,
  OV_HAND = 1024,   // High-Five: gesetzt von app.cpp, nicht von overlays_for
  OV_TYPING = 4096  // Claude-Modus, Schreiben: drei wandernde Punkte unter dem Mund, gesetzt von app.cpp (2048 war OV_SNEEZE, entfernt)
};

// w = nullptr: kein (frisches) Wetter.
inline uint32_t overlays_for(Emotion e, const WeatherMood *w, bool is_day) {
  if (e == Emotion::SLEEPING) return OV_ZZZ;
  uint32_t o = 0;
  if (e == Emotion::LOVE || e == Emotion::KISS) o |= OV_HEART;
  if (e == Emotion::SAD) o |= OV_TEAR;
  if (e == Emotion::CONFUSED) o |= OV_QUESTION;
  if (e == Emotion::WHISTLE) o |= OV_NOTE;
  if (w) {
    if (w->sky == Sky::SUNNY && is_day &&
        (e == Emotion::HAPPY || e == Emotion::CONTENT || e == Emotion::GREETING ||
         e == Emotion::PROUD || e == Emotion::MISCHIEVOUS || e == Emotion::CHEEKY))
      o |= OV_SUNGLASSES;
    if (w->hot) o |= OV_SWEAT;
    if (w->cold) o |= OV_SHIVER;
    if (w->sky == Sky::RAIN) o |= OV_RAIN_TINT;
    if (w->sky == Sky::STORM) o |= OV_STORM;
  }
  return o;
}
