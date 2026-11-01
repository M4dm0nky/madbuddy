#include "store.h"
#include <Preferences.h>
#include <string.h>

static constexpr const char *kNs = "buddy";
// Version 2: Tageszähler, Version 3: Sättigung, Krankheit, Charakter, Lieblingsessen, 4: cam_on.
// Ältere Stände werden übernommen, fehlende Schlüssel bekommen Startwerte.
static constexpr uint8_t kVersion = 4;

void store_defaults(Persist &p) {
  p = Persist{};
  p.needs = Needs{60, 60, 80};
  p.epoch = 0;
  p.brightness = 2;
  p.place[0] = '\0';
  p.lat = p.lon = 0;
  p.has_place = false;
  p.day = DayStats{};
  p.sick = false;
  p.cuddly = p.playful = 0;
  p.favorite_food = STORE_NO_FAVORITE;
  p.cam_on = true;
}

bool store_load(Persist &p) {
  store_defaults(p);
  Preferences prefs;
  if (!prefs.begin(kNs, true)) return false;   // Namensraum fehlt = noch nie gespeichert
  const uint8_t ver = prefs.getUChar("ver", 0);
  if (ver == 0 || ver > kVersion) { prefs.end(); return false; }
  p.needs.affection = prefs.getFloat("aff", p.needs.affection);
  p.needs.fun = prefs.getFloat("fun", p.needs.fun);
  p.needs.energy = prefs.getFloat("ene", p.needs.energy);
  p.needs.satiety = prefs.getFloat("sat", p.needs.satiety);
  p.sick = prefs.getBool("sick", false);
  p.cuddly = prefs.getFloat("cud", 0);
  p.playful = prefs.getFloat("pla", 0);
  p.favorite_food = prefs.getUChar("fav", STORE_NO_FAVORITE);
  if (p.favorite_food > 3) p.favorite_food = STORE_NO_FAVORITE;
  p.cam_on = prefs.getBool("cam", true);
  p.epoch = prefs.getULong("epoch", 0);
  p.brightness = prefs.getUChar("bright", 2);
  if (p.brightness > 2) p.brightness = 2;
  prefs.getString("place", p.place, sizeof p.place);
  p.place[sizeof p.place - 1] = '\0';
  p.lat = prefs.getFloat("lat", 0);
  p.lon = prefs.getFloat("lon", 0);
  p.has_place = prefs.getBool("hasp", false);
  p.day.day = prefs.getULong("d_day", 0);
  p.day.strokes = prefs.getUShort("d_str", 0);
  p.day.pokes = prefs.getUShort("d_pok", 0);
  p.day.games = prefs.getUShort("d_gam", 0);
  p.day.wins = prefs.getUShort("d_win", 0);
  p.day.meals = prefs.getUShort("d_mea", 0);
  p.day.pomodoros = prefs.getUShort("d_pom", 0);
  prefs.end();
  mood_clamp(p.needs);
  return true;
}

void store_save(const Persist &p) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.putUChar("ver", kVersion);
  prefs.putFloat("aff", p.needs.affection);
  prefs.putFloat("fun", p.needs.fun);
  prefs.putFloat("ene", p.needs.energy);
  prefs.putFloat("sat", p.needs.satiety);
  prefs.putBool("sick", p.sick);
  prefs.putFloat("cud", p.cuddly);
  prefs.putFloat("pla", p.playful);
  prefs.putUChar("fav", p.favorite_food);
  prefs.putBool("cam", p.cam_on);
  prefs.putULong("epoch", p.epoch);
  prefs.putUChar("bright", p.brightness);
  prefs.putString("place", p.place);
  prefs.putFloat("lat", p.lat);
  prefs.putFloat("lon", p.lon);
  prefs.putBool("hasp", p.has_place);
  prefs.putULong("d_day", p.day.day);
  prefs.putUShort("d_str", p.day.strokes);
  prefs.putUShort("d_pok", p.day.pokes);
  prefs.putUShort("d_gam", p.day.games);
  prefs.putUShort("d_win", p.day.wins);
  prefs.putUShort("d_mea", p.day.meals);
  prefs.putUShort("d_pom", p.day.pomodoros);
  prefs.end();
}

// Touch-Kalibrierung (8 Werte aus LGFX::calibrateTouch), eigener Schlüssel, unabhängig von Persist.
bool store_load_touch(uint16_t cal[8]) {
  Preferences prefs;
  if (!prefs.begin(kNs, true)) return false;
  const size_t n = prefs.getBytes("touchcal", cal, 8 * sizeof(uint16_t));
  prefs.end();
  return n == 8 * sizeof(uint16_t);
}

void store_save_touch(const uint16_t cal[8]) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.putBytes("touchcal", cal, 8 * sizeof(uint16_t));
  prefs.end();
}

// Claude-Modus-Schluessel: 12 Hex-Zeichen, eigener Schluessel "ckey" (bleibt bei Persist-Aenderungen erhalten).
bool store_load_claude_key(char key[13]) {
  Preferences prefs;
  if (!prefs.begin(kNs, true)) return false;
  char buf[13] = "";
  prefs.getString("ckey", buf, sizeof buf);
  prefs.end();
  if (strlen(buf) != 12) return false;
  for (int i = 0; i < 12; i++) {
    const char c = buf[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  memcpy(key, buf, sizeof buf);
  return true;
}

void store_save_claude_key(const char key[13]) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.putString("ckey", key);
  prefs.end();
}