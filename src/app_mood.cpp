// Stimmung, Touch und Pomodoro-Ereignisse; läuft auf jedem Bildschirm.
#include "app_internal.h"
#include <Arduino.h>
#include "hw/light.h"
#include "hw/cam_uart.h"
#include "ui/eyes.h"

static constexpr uint32_t kHandMs = 1500, kLedFlashMs = 250;
static constexpr uint32_t kTouchPrioMs = 3000;   // Berührungs-Gefühle haben so lange Vorrang vor Claude-Gesichtern
static constexpr uint8_t kStrokeSayPct = 25;
static Deadline s_touch_prio;
static bool s_prev_near = false;

// Touch lesen und Gesten an die Stimmung weitergeben. Liefert die Geste (für Menü usw.).
Gesture handle_touch(uint32_t now, int16_t &tx, int16_t &ty) {
  App &a = g_app;
  int32_t x = 0, y = 0;
  const bool pressed = a.lcd.getTouch(&x, &y) > 0;
  // Ein Tipp wird erst beim Loslassen gemeldet; dann liefert getTouch keine Position mehr.
  // Darum gilt beim Loslassen die letzte gültige Position (vorher kam (0,0) an: Menü traf nichts, "Ja" zählte als "daneben").
  static int16_t s_last_x = 0, s_last_y = 0;
  if (pressed) { s_last_x = (int16_t)x; s_last_y = (int16_t)y; }
  tx = s_last_x;
  ty = s_last_y;
  if (pressed) mood_touch(a.mood, now);
  a.touch_began = pressed && !a.pressed;
  a.pressed = pressed;   // jede Berührung hält ihn nachts wach bzw. weckt ihn
  diag_touch(pressed, x, y);
  const Gesture g = gesture_feed(a.gesture, pressed, tx, ty, now);
  diag_gesture(g, tx, ty);
  return g;
}

void touch_to_mood(Gesture g, int16_t tx, int16_t ty, uint32_t now) {
  App &a = g_app;
  const int hour = current_hour();
  if (g == Gesture::TAP || g == Gesture::STROKE || g == Gesture::POKE) s_touch_prio.arm(now, kTouchPrioMs);
  switch (g) {
    case Gesture::TAP:
      if (focused_work()) break;   // während der Arbeit: kein Überraschen, nur der Blick folgt
      if (highfive_hit(tx, ty)) {   // obere Ecke: High-Five statt Tippen
        mood_event(a.mood, MoodEvent::HIGHFIVE, now, hour);
        char_highfive(a.chr);
        say(Say::HIGHFIVE, now);
        eyes_set_hand(tx > 160, now);
        a.hand.arm(now, kHandMs);
        a.led_flash.arm(now, kLedFlashMs);
      } else {
        mood_event(a.mood, MoodEvent::TAP, now, hour);
      }
      break;
    case Gesture::STROKE:
      mood_event(a.mood, MoodEvent::STROKE, now, hour);
      a.persist.day.strokes++;
      char_stroke(a.chr);
      if (speech_may_say(now) && esp_random() % 100 < kStrokeSayPct) say(Say::VERLIEBT, now);   // Abstand gilt
      break;
    case Gesture::POKE:
      mood_event(a.mood, MoodEvent::POKE, now, hour);
      a.persist.day.pokes++;
      char_poke(a.chr);
      say(Say::GENERVT, now);
      break;
    default: break;
  }
}

// Kamera zählt nur, wenn eingeschaltet und der XIAO läuft.
static bool cam_active(uint32_t now) {
  return g_app.persist.cam_on && cam_link_conn(cam_uart_link(), now, g_app.persist.cam_on) == CamConn::ON;
}

static void pomo_events(uint32_t now) {
  App &a = g_app;
  switch (pomo_tick(a.pomo, now)) {
    case PomoEvent::BREAK_START:
      mood_set_transient(a.mood, Emotion::EXCITED, now, 3000);
      say(Say::POMODORO_PAUSE, now);
      led_blink(Rgb{0, 255, 0}, 5, now);
      a.persist.day.pomodoros++;
      break;
    case PomoEvent::BREAK_END:
      say(Say::POMODORO_FERTIG, now);
      led_blink(Rgb{0, 0, 255}, 2, now);
      break;
    default: break;
  }
}

// Stimmung läuft auf jedem Bildschirm weiter (Zeit vergeht), Licht und LED auch.
void mood_frame(uint32_t now, uint32_t dt) {
  App &a = g_app;
  const int hour = current_hour();
  const bool work = focused_work();
  const ClaudeState cs = claude_state(a.claude, now);
  a.claude_busy = claude_busy(cs);
  const bool calm = work || a.claude_busy;   // Pomodoro-Arbeit bzw. Claude arbeitet: kein Schlaf, keine Eigenarten
  if (calm) mood_touch(a.mood, now);
  const bool cam = cam_active(now);
  const PresenceOut po = presence_tick(a.presence, cam, now, CAM_NEAR_W);
  a.pres = po;   // für eyes_frame/speech_frame
  a.wave_hello = false;
  if (wave_take(a.wave, now) && !(mood_is_night(hour) && mood_is_sleeping(a.mood, now))) {   // Winken: nachts im Schlaf nicht wecken
    mood_touch(a.mood, now);   // weckt aus dem Tages-Nickerchen
    a.wave_hello = true;
  }
  const bool asleep = mood_is_sleeping(a.mood, now);
  if (po.present && !(mood_is_night(hour) && asleep)) mood_touch(a.mood, now);   // da = wach halten; nachts weckt ein Gesicht nicht
  const bool ldr_dark = light_update(now);   // läuft immer weiter, damit die Glättung nicht springt
  const bool dark = cam ? po.unattended : ldr_dark;   // Kamera ersetzt den Lichtsensor
  mood_set_dark(a.mood, calm ? false : dark, hour, now);
  if (po.near && !s_prev_near) mood_set_transient(a.mood, Emotion::NEAR, now, PRES_NEAR_FOR_MS);
  s_prev_near = po.near;
  mood_night_check(a.mood, hour, now);
  mood_set_character(a.mood, a.chr);
  mood_tick(a.mood, dt, now);
  if (!calm) mood_quirks(a.mood, hour, now);
  pomo_events(now);
  a.emotion = mood_emotion(a.mood, hour, now);
  if (focused_work() && a.emotion != Emotion::SLEEPING) a.emotion = Emotion::FOCUSED;   // überschreibt alles außer Schlaf
  // Claude-Gesicht: Berührungs-Gefühl (3 s) und Pomodoro haben Vorrang
  const bool touch_feel = s_touch_prio.active(now) && !mood_is_sleeping(a.mood, now) &&
                          a.emotion != mood_base_emotion(a.mood, hour);
  const ClaudeFace cf = claude_face(cs, work, touch_feel, a.emotion);
  a.emotion = cf.emo;
  a.claude_fx = cf.fx;
  claude_frame(cs, now);
  a.persist.cuddly = a.chr.cuddly;   // für die Anzeige in den Einstellungen
  a.persist.playful = a.chr.playful;
}
