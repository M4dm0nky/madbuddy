#pragma once
#include "../hw/display.h"
#include "../logic/weather_mood.h"

// Statusstreifen unten (y 200..239), direkt gezeichnet: Uhrzeit, Wetter, WLAN, Update-Hinweis.
constexpr int STATUS_Y = 200;
constexpr int STATUS_H = 40;

struct StatusInfo {
  bool time_ok; int hour, minute;
  bool wifi_ok; bool portal; char portal_pass[12];   // Portal offen + dessen WLAN-Passwort ("" = unbekannt)
  bool weather_ok; Sky sky; bool is_day; float temp_c;
  bool update_ready;
  uint16_t timer_s; bool timer_break;   // Pomodoro-Restzeit (0 = kein Timer), Pause grün
  uint8_t cam;                          // Kamera-Auge: 0 kein Symbol, 1 grau, 2 grün, 3 rot
  bool claude_on; uint8_t helpers;      // Claude-Modus aktiv (Funke), laufende Helfer (Symbol + Zahl, 0 = keins)
};
void status_draw(LGFX &lcd, const StatusInfo &s);   // zeichnet nur neu, wenn sich etwas geändert hat
void status_invalidate();                            // nächstes status_draw zeichnet sicher (nach Menü/Uhr/…)
