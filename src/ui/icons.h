#pragma once
#include <stdint.h>
#include "../hw/display.h"
#include "../logic/weather_mood.h"

// Symbole aus Primitiven; (cx, cy) = Mitte, s = Kantenlänge, col = RGB888 (lgfx::color888).
// Ziel ist LovyanGFX (Basis von LGFX und LGFX_Sprite), damit sie auf Display und Sprite gehen.
void icon_play(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_clock(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_gear(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_close(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_wifi(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_sky(LovyanGFX &d, int cx, int cy, int s, Sky sky, bool is_day);
void icon_sun(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_update(LovyanGFX &d, int cx, int cy, int s, uint32_t col);   // Pfeil nach unten in Kreis
void icon_info(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
// Zusätzliche Symbole für Menü und Spiele.
void icon_target(LovyanGFX &d, int cx, int cy, int s, uint32_t col);    // Kreis mit Fadenkreuz (Fang mich)
void icon_simon(LovyanGFX &d, int cx, int cy, int s);                   // vier farbige Viertel (Simon)
void icon_brightness(LovyanGFX &d, int cx, int cy, int s, uint8_t level, uint32_t col);   // 3 Balken, level 0..2
void icon_back(LovyanGFX &d, int cx, int cy, int s, uint32_t col);      // Pfeil nach links
// Interaktion (Rückblick, Menü, Füttern, Timer, Charakter).
void icon_heart(LovyanGFX &d, int cx, int cy, int s, uint32_t col);
void icon_bolt(LovyanGFX &d, int cx, int cy, int s, uint32_t col);       // Blitz (Stupsen / frech)
void icon_apple(LovyanGFX &d, int cx, int cy, int s);                    // roter Apfel mit Stiel und Blatt
void icon_hourglass(LovyanGFX &d, int cx, int cy, int s, uint32_t col);  // Sanduhr (Timer)
void icon_food(LovyanGFX &d, int cx, int cy, int s, int food);         // 0 Apfel, 1 Pizza, 2 Kuchen, 3 Kaffee (logic/food.h)
void icon_ball(LovyanGFX &d, int cx, int cy, int s, uint32_t col);       // verspielt
void icon_pillow(LovyanGFX &d, int cx, int cy, int s, uint32_t col);     // faul
void icon_claude(LovyanGFX &d, int cx, int cy, int s, uint32_t col);     // Funke/Stern (Claude-Modus)
void icon_helper(LovyanGFX &d, int cx, int cy, int s, uint32_t col);     // Helfer-Männchen (Subagent)
void icon_stop(LovyanGFX &d, int cx, int cy, int s, uint32_t col);       // Quadrat (Timer stoppen)
void icon_eye(LovyanGFX &d, int cx, int cy, int s, uint32_t col);        // Auge (Kamera), ca. 2s x 1,2s
void icon_arrow(LovyanGFX &d, int cx, int cy, int s, uint32_t col, bool right);   // Dreieck nach rechts/links (Seite wechseln)
