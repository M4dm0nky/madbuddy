#pragma once
// Gemeinsamer Zustand und Schnittstellen der app_*.cpp (nur für diese Dateien, nicht für main.cpp).
// Ein Zustand g_app (definiert in app.cpp); Zustand, den nur eine Datei braucht, bleibt dort als static.
#include <stdint.h>
#include "hw/display.h"
#include "hw/store.h"
#include "net/net.h"
#include "logic/mood.h"
#include "logic/gesture.h"
#include "logic/eye_model.h"
#include "logic/weather_mood.h"
#include "logic/look.h"
#include "logic/speech.h"
#include "logic/rituals.h"
#include "logic/character.h"
#include "logic/pomodoro.h"
#include "logic/presence.h"
#include "logic/claude_state.h"
#include "logic/claude_face.h"
#include "logic/cam_proto.h"
#include "logic/deadline.h"

#ifndef CYD_LOG_LDR
#define CYD_LOG_LDR 0   // 1 = Rohwert des Lichtsensors jede Sekunde seriell ausgeben (Einmessen); per build_flags -DCYD_LOG_LDR=1
#endif
#ifndef CYD_DIAG_TOUCH
#define CYD_DIAG_TOUCH 0   // 1 = Touch/Gesten seriell ausgeben (Kalibrierung prüfen); per build_flags -DCYD_DIAG_TOUCH=1
#endif

enum class Screen { EYES, MENU, CLOCK, SETTINGS, GAME_CATCH, GAME_SIMON, OTA, REVIEW, FEED, TIMER };

constexpr uint8_t kLedPct = 15;

struct App {
  LGFX lcd;
  Persist persist;
  ClaudeLogic claude;            // Claude-Modus: Zustand aus den Hook-Ereignissen (claude_api -> claude_event)
  MoodState mood;
  Presence presence;             // Kamera: Anwesenheit, Begrüßung, Nähe, Blick
  PresenceOut pres;              // Ergebnis des letzten mood_frame (für speech_frame/eyes_frame)
  GestureState gesture;
  Character chr{};
  Pomo pomo{};
  RitualState rituals{};
  LookState look{};
  Screen screen = Screen::EYES;
  Emotion emotion = Emotion::CONTENT;
  uint8_t backlight = 0;
  uint8_t claude_fx = CFX_NONE;  // Zusatzeffekte des angezeigten Claude-Gesichts
  bool claude_busy = false;      // Zustand mit eigenem Gesicht (nicht OFF/IDLE)
  bool pressed = false;          // Finger liegt gerade auf
  bool touch_began = false;      // in diesem Bild hat eine Berührung begonnen
  Deadline hand;                 // High-Five-Hand sichtbar bis
  Deadline led_flash;            // LED kurz weiß
  Rgb blink_col{0, 0, 0};        // LED-Signal (Pomodoro): Farbe, Anzahl, Start
  uint8_t blink_n = 0;
  uint32_t blink_start = 0;
  int feed_food = -1;            // gerade gefütterte Speise (Animation), -1 = keine
  uint32_t feed_start = 0;
};
extern App g_app;

// --- app.cpp ---
void set_backlight(uint8_t v);
void apply_user_backlight();   // Helligkeit aus den Einstellungen (persist.brightness)
void save_now();
uint32_t today(struct tm *t);  // heutiges Datum als YYYYMMDD, 0 wenn die Zeit unbekannt ist
bool fresh_weather(uint32_t now, WeatherMood *wm, NetWeather *w);   // höchstens 3 h altes Wetter oder false
inline int current_hour() { return net_hour(); }
inline bool focused_work() { return g_app.pomo.state == PomoState::WORK; }
inline void led_blink(Rgb c, uint8_t n, uint32_t now) { g_app.blink_col = c; g_app.blink_n = n; g_app.blink_start = now; }

// --- app_diag.cpp ---
void usb_cmd_poll(uint32_t now);
void cam_log_tick(uint32_t now);
void diag_cam_count(const CamMsg &m);   // Zähler/Messwerte der Kamera-Zeilen für cam_log_tick
#if CYD_DIAG_TOUCH
void diag_touch(bool pressed, int32_t x, int32_t y);
void diag_gesture(Gesture g, int16_t tx, int16_t ty);
#else
inline void diag_touch(bool, int32_t, int32_t) {}
inline void diag_gesture(Gesture, int16_t, int16_t) {}
#endif
#if CYD_LOG_LDR
void diag_ldr_tick(uint32_t now);
#else
inline void diag_ldr_tick(uint32_t) {}
#endif

// --- app_speech.cpp ---
void app_speech_init(uint32_t now, uint32_t seed);
void say(Say what, uint32_t now);
bool speech_may_say(uint32_t now);   // Mindestabstand seit der letzten Blase eingehalten
void speech_frame(uint32_t now, bool sleeping);
void claude_frame(ClaudeState cs, uint32_t now);
const char *speech_bubble(uint32_t now);   // aktuelle Blase oder nullptr

// --- app_mood.cpp ---
Gesture handle_touch(uint32_t now, int16_t &tx, int16_t &ty);
void touch_to_mood(Gesture g, int16_t tx, int16_t ty, uint32_t now);
void mood_frame(uint32_t now, uint32_t dt);

// --- app_eyes.cpp ---
void app_eyes_init(uint32_t now, uint32_t seed);
void eyes_frame(uint32_t now, uint32_t dt);

// --- app_screens.cpp ---
void screen_frame(uint32_t now, uint32_t dt, Gesture g, int16_t tx, int16_t ty);
int app_firmware_line(char *out, size_t n, uint32_t now);
