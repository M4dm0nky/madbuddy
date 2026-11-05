#pragma once
#include <stdint.h>
#include "../hw/display.h"
#include "../logic/eye_model.h"
#include "../logic/face_model.h"

// Augen + Overlays im Bereich 320x200 (oben). Mit Sprite (8 Bit, RGB332, 64 000 B) flimmerfrei,
// ohne Sprite (zu wenig Speicher oder während OTA freigegeben) direkt aufs Display (langsamer).
constexpr int EYES_W = 320;
constexpr int EYES_H = 200;

bool eyes_begin(LGFX &lcd);   // false = kein Speicher -> direkt zeichnen
void eyes_free();             // für OTA
bool eyes_ready();
// bubble: Text der Sprechblase (UTF-8) oder nullptr.
// Liefert false, wenn das Bild unverändert war und nichts gezeichnet/gesendet wurde (nur mit Sprite).
bool eyes_draw(LGFX &lcd, const FaceShape &face, Rgb color, uint32_t overlays, uint32_t now_ms, const char *bubble = nullptr);
// High-Five-Hand (Overlay OV_HAND): Ecke (rechts/links) und Startzeit für das kurze Aufploppen.
void eyes_set_hand(bool right, uint32_t since_ms);
// Speise, die gerade zum Mund fliegt (0..3 wie logic/food.h, -1 = keine), y = Mitte.
void eyes_set_food(int food, int y);
// Pomodoro-Fortschrittsring am Sprite-Rand (Bogen um das Gesicht); progress < 0 = aus.
void eyes_set_ring(float progress, Rgb color);
// Claude-Modus (Ausfuehren): kurzes Segment (1/6 des Umfangs) dreht am Rand, 1 Umdrehung je 1,5 s.
void eyes_set_spin(bool on, Rgb color);
// Zeichnet ein Augenpaar ohne Overlays auf ein beliebiges Ziel (Spiele, OTA); (cx, cy) = Mitte zwischen den Augen.
void eyes_draw_pair(LovyanGFX &d, int cx, int cy, const EyeShape &shape, Rgb color, float scale);
