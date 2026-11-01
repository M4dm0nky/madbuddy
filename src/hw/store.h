#pragma once
#include <stdint.h>
#include "../logic/mood.h"
#include "../logic/daystats.h"

// Dauerhafte Werte im NVS (Preferences, Namensraum "buddy").
struct Persist {
  Needs needs;
  uint32_t epoch;        // letzte bekannte Unix-Zeit, 0 = unbekannt
  uint8_t brightness;    // 0..2
  char place[48];
  float lat, lon;
  bool has_place;        // lat/lon passen zu place
  DayStats day;          // Tageszähler (F2); day.day = last_day (YYYYMMDD)
  bool sick;             // F3 (Sättigung steckt in needs.satiety)
  float cuddly, playful; // Charakter -100..100
  uint8_t favorite_food; // 0..3; STORE_NO_FAVORITE = noch nicht gewählt (app zieht beim ersten Start zufällig)
  bool cam_on;           // Kamera eingeschaltet (Einstellung), Standard an
};
constexpr uint8_t STORE_NO_FAVORITE = 255;

void store_defaults(Persist &p);     // needs {60,60,80}, epoch 0, brightness 2, place "", has_place false
bool store_load(Persist &p);         // false = nichts gespeichert (dann defaults)
void store_save(const Persist &p);
bool store_load_touch(uint16_t cal[8]);   // false = noch nicht kalibriert
void store_save_touch(const uint16_t cal[8]);
// Claude-Modus-Schluessel (12 Hex-Zeichen + '\0'), eigener NVS-Schluessel "ckey", unabhaengig von Persist.
bool store_load_claude_key(char key[13]);   // false = noch keiner (oder ungueltig)
void store_save_claude_key(const char key[13]);
