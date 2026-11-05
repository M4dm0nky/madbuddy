// Augen-Bildschirm: Gesicht, Blick, Füttern, Statusleiste, LED und Helligkeit.
#include "app_internal.h"
#include <Arduino.h>
#include "logic/food.h"
#include "hw/led.h"
#include "hw/cam_uart.h"
#include "ota/ota_pull.h"
#include "ui/eyes.h"
#include "ui/status_bar.h"

static constexpr uint32_t kStormEveryMs = 4UL * 60000UL;
static constexpr uint32_t kStormForMs = 1000;
static constexpr uint8_t kSleepBacklight = 26;   // 10 %
static constexpr Rgb kClaudeCol{217, 119, 87};
static constexpr Rgb kTimerCol{255, 90, 140}, kBreakCol{60, 220, 120};
// Füttern: Speise fliegt in 600 ms von unten zum Mund, dann 3x kauen, dann Reaktion
static constexpr uint32_t kFlyMs = 600, kChewMs = 1200, kChewStepMs = 200;
static constexpr int kFoodY0 = 236, kMouthY = 172;

static Blinker s_blink;
static Gazer s_gaze;
static FaceShape s_face;
static Rgb s_color;
static bool s_prev_storm = false;

void app_eyes_init(uint32_t now, uint32_t seed) {
  blinker_init(s_blink, seed, now);
  gazer_init(s_gaze, seed ^ 0x9E3779B9u, now);
  const FaceExpr start = face_expression(Emotion::CONTENT);
  s_face = start.s;
  s_color = start.color;
}

static StatusInfo status_info(uint32_t now) {
  App &a = g_app;
  StatusInfo si{};
  struct tm t;
  si.time_ok = net_local_time(&t);
  si.hour = si.time_ok ? t.tm_hour : 0;
  si.minute = si.time_ok ? t.tm_min : 0;
  si.wifi_ok = net_wifi_ok();
  si.portal = net_portal_active();
  if (si.portal) strlcpy(si.portal_pass, net_portal_pass(), sizeof si.portal_pass);
  WeatherMood wm;
  NetWeather w;
  si.weather_ok = fresh_weather(now, &wm, &w);
  si.sky = si.weather_ok ? wm.sky : Sky::CLOUDY;
  si.is_day = si.weather_ok ? w.is_day : true;
  si.temp_c = si.weather_ok ? w.temp_c : 0;
  si.update_ready = ota_pull::info().state == ota_pull::AVAILABLE;
  si.timer_s = (uint16_t)pomo_remaining_s(a.pomo, now);
  si.timer_break = a.pomo.state == PomoState::BREAK;
  if (a.persist.cam_on)   // Kamera-Auge: rot ohne Verbindung/bei Fehler, grün bei laufender Kamera mit Gesicht, sonst grau
    si.cam = cam_status_level(true, cam_link_conn(cam_uart_link(), now, true), a.pres.present);
  si.claude_on = claude_active(a.claude, now);
  si.helpers = claude_helpers(a.claude, now);
  return si;
}

void eyes_frame(uint32_t now, uint32_t dt) {
  App &a = g_app;
  const bool sleeping = a.emotion == Emotion::SLEEPING;
  WeatherMood wm;
  NetWeather w;
  const bool have_weather = fresh_weather(now, &wm, &w);
  uint32_t ov = overlays_for(a.emotion, have_weather ? &wm : nullptr, have_weather ? w.is_day : true);
  if (a.hand.active(now) && !sleeping) ov |= OV_HAND;
  if (!sleeping && (a.claude_fx & CFX_HAND)) ov |= OV_HAND;
  if (!sleeping && (a.claude_fx & CFX_TYPING)) ov |= OV_TYPING;

  const bool storm = (ov & OV_STORM) && now % kStormEveryMs < kStormForMs && !a.claude_busy;
  if (storm && !s_prev_storm && !sleeping) say(Say::GEWITTER, now);   // Gewitter-Schreck
  s_prev_storm = storm;
  speech_frame(now, sleeping);
  FaceExpr target = face_expression(storm ? Emotion::STARTLED : a.emotion);
  gazer_update(s_gaze, now);
  float gx = s_gaze.gx, gy = s_gaze.gy;
  if (look_active(a.look, now)) look_target(a.look, &gx, &gy);   // Blick folgt dem Finger
  else if (a.pres.gaze) { gx = a.pres.gx; gy = a.pres.gy; }   // Blick folgt dem Gesicht
  if (sleeping || (a.claude_fx & (CFX_STRAIGHT | CFX_JUMP))) gx = gy = 0;   // Claude: Blick gerade bzw. springend, kein Wandern
  target.s.pupil_dx += gx / GAZE_X_MAX * 14.0f;
  target.s.pupil_dy += gy / GAZE_Y_MAX * 10.0f;
  target.s.eye.gaze_x = gx / GAZE_X_MAX * 8.0f;
  target.s.eye.gaze_y = gy / GAZE_Y_MAX * 4.0f;
  if (a.claude_fx & CFX_STRAIGHT) {   // Claude braucht dich: Blick gerade auf den Betrachter, Brauen hoch
    target.s.pupil_dx = 0;
    target.s.pupil_dy = 0;
    target.s.pupil_r = 18;
    target.s.brow_y = -26;
  }
  if (a.claude_fx & CFX_JUMP) target.s.pupil_dx = ((now / 250) & 1) ? 14.0f : -14.0f;   // Lesen: alle 250 ms links <-> rechts
  s_face = face_tween(s_face, target.s, dt);   // switch_acc läuft im s_face mit
  s_color = rgb_tween(s_color, target.color, dt);

  eyes_set_spin((a.claude_fx & CFX_RING) && !sleeping, kClaudeCol);   // Claude führt etwas aus (nur ohne Pomodoro-Ring)
  if (a.pomo.state == PomoState::IDLE) eyes_set_ring(-1, kTimerCol);
  else eyes_set_ring(pomo_progress(a.pomo, now), a.pomo.state == PomoState::WORK ? kTimerCol : kBreakCol);

  FaceShape draw = s_face;
  if (!sleeping) draw.eye.open *= blinker_open(s_blink, now);
  if (a.feed_food >= 0) {
    const uint32_t t = now - a.feed_start;
    if (t < kFlyMs) {
      eyes_set_food(a.feed_food, kFoodY0 + (int)((kMouthY - kFoodY0) * (float)t / kFlyMs));
    } else if (t < kFlyMs + kChewMs) {
      eyes_set_food(-1, 0);
      draw.mouth = Mouth::CURVE;
      draw.mouth_curve = 0.3f;
      draw.mouth_open = ((t - kFlyMs) / kChewStepMs) % 2 ? 0.6f : 0.0f;
    } else {
      eyes_set_food(-1, 0);
      const Food f = (Food)a.feed_food;
      a.feed_food = -1;
      a.persist.day.meals++;
      say(food_feed(a.mood, f, (Food)a.persist.favorite_food, now), now);
    }
  }
  if (eyes_draw(a.lcd, draw, s_color, ov, now, speech_bubble(now))) diag_fps_push();
  status_draw(a.lcd, status_info(now));

  if (a.led_flash.active(now)) led_set(Rgb{255, 255, 255}, kLedPct);   // High-Five
  else led_set(sleeping ? Rgb{0, 0, 0} : s_color, kLedPct);
  if (sleeping) set_backlight(kSleepBacklight);
  else apply_user_backlight();
}
