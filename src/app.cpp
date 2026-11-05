#include "app.h"
#include "app_internal.h"
#include <Arduino.h>
#include "config.h"
#include "logic/ota_policy.h"
#include "logic/food.h"
#include "hw/led.h"
#include "hw/light.h"
#include "hw/cam_uart.h"
#include "ota/ota_pull.h"
#include "ui/eyes.h"
#include "ui/status_bar.h"
#include "ui/settings.h"
#include "ui/ota_card.h"
#include "net/claude_api.h"

// Rahmen: app_setup/app_loop, gemeinsame Hilfen und der eine Zustand g_app. Die Teile liegen in app_*.cpp
// (Schnittstellen in app_internal.h): diag, speech, mood, eyes, screens.

App g_app;
LGFX &app_lcd() { return g_app.lcd; }

static constexpr uint32_t kFrameMs = 33;              // ~30 Bilder/s
static constexpr uint32_t kSaveEveryMs = 10UL * 60000UL;
static constexpr uint32_t kLogEveryMs = 60000UL;
static constexpr uint32_t kBlinkStepMs = 150;
static const uint8_t kBacklight[3] = {77, 153, 255};  // 30 / 60 / 100 %

static uint32_t s_last_frame = 0, s_last_save = 0, s_last_log = 0;
static uint32_t s_frames = 0;
static bool s_elapsed_applied = false;
static bool s_needs_ack = false;                       // Tippen hat das gelbe Blinken beendet ...
static uint32_t s_needs_ack_since = 0;                 // ... für das NEEDS_YOU, das bei L.since begann

void set_backlight(uint8_t v) {
  if (v == g_app.backlight) return;
  g_app.backlight = v;
  g_app.lcd.setBrightness(v);
}

void apply_user_backlight() { set_backlight(kBacklight[g_app.persist.brightness]); }

void save_now() {
  App &a = g_app;
  a.persist.needs = a.mood.n;
  a.persist.cuddly = a.chr.cuddly;
  a.persist.playful = a.chr.playful;
  const uint32_t epoch = net_epoch();
  if (epoch) a.persist.epoch = epoch;
  store_save(a.persist);
}

bool fresh_weather(uint32_t now, WeatherMood *wm, NetWeather *w) {
  *w = net_weather();
  if (!w->valid || !weather_fresh(now, w->fetched_ms)) return false;
  *wm = weather_mood(w->code, w->temp_c);
  return true;
}

// Heutiges Datum als YYYYMMDD, 0 wenn die Zeit unbekannt ist.
uint32_t today(struct tm *t) {
  if (!net_local_time(t)) return 0;
  return day_key(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

// Einmal, sobald die Uhrzeit bekannt ist: Aus-Zeit seit dem letzten Speichern wie Schlaf nachrechnen.
static void apply_elapsed_once() {
  App &a = g_app;
  if (s_elapsed_applied || !net_time_ok()) return;
  s_elapsed_applied = true;
  const uint32_t now_epoch = net_epoch();
  if (a.persist.epoch && now_epoch > a.persist.epoch) {
    const uint32_t secs = now_epoch - a.persist.epoch;
    mood_apply_elapsed(a.mood, secs);
    Serial.printf("mood: %u s aus, Energie jetzt %.1f\n", (unsigned)secs, a.mood.n.energy);
  }
  a.persist.epoch = now_epoch;
}

static void on_cam_msg(const CamMsg &m, uint32_t now) {
  diag_cam_count(m);
  presence_msg(g_app.presence, m, now, CAM_MIRROR_X, CAM_GAZE_Y0);
  if (m.kind == CAM_MOTION && g_app.presence.active) wave_feed(g_app.wave, m.x, m.n, now);   // wie Presence: nur bei aktiver Kamera
  if (m.kind == CAM_UPD_PCT || m.kind == CAM_UPD_OK || m.kind == CAM_UPD_ERR) {
    Serial.printf("cam: U kind=%d pct=%u %s\n", (int)m.kind, (unsigned)m.pct, m.text);
    ota_card_cam_msg(m, now);   // Kamera-Update (Karte)
  }
}

void app_setup() {
  App &a = g_app;
  led_begin();
  light_begin();
  cam_uart_begin();
  cam_uart_on_msg(on_cam_msg);
  if (!store_load(a.persist)) Serial.println("store: nichts gespeichert, Startwerte");
  mood_init(a.mood, a.persist.needs);
  a.chr = Character{a.persist.cuddly, a.persist.playful};
  if (a.persist.favorite_food == STORE_NO_FAVORITE) {   // erster Start: Lieblingsessen auslosen
    a.persist.favorite_food = (uint8_t)(esp_random() % (uint32_t)Food::COUNT);
    store_save(a.persist);
  }
  ota_pull::begin();
  net_task_hook(ota_pull::loop);   // OTA-Abruf läuft im net_task
  settings_firmware_source(app_firmware_line);
  a.lcd.fillScreen(TFT_BLACK);
  eyes_begin(a.lcd);   // Sprite vor dem WLAN anlegen, solange der Heap noch zusammenhängend ist
  net_begin(&a.persist);
  claude_api_begin(&a.claude);
  gesture_init(a.gesture);
  const uint32_t now = millis();
  const uint32_t seed = esp_random();
  app_eyes_init(now, seed);
  mood_seed(a.mood, esp_random());
  app_speech_init(now, esp_random());

  status_invalidate();
  apply_user_backlight();
  s_last_frame = s_last_save = s_last_log = now;
  Serial.printf("MadBuddy gestartet, Panel %s, heap=%u\n", LGFX::PANEL_NAME, (unsigned)ESP.getFreeHeap());
}

void app_loop() {
  App &a = g_app;
  const uint32_t now = millis();
  const uint32_t dt = now - s_last_frame;
  if (dt < kFrameMs) {
    delay(kFrameMs - dt);
    return;
  }
  s_last_frame = now;
  s_frames++;

  // OTA-Neustart steht bevor (ota_pull wartet davor 2 s): einmal speichern, hier im Hauptthread (kein Datenrennen mit dem net_task)
  static bool s_saved_for_restart = false;
  if (ota_pull::info().state == ota_pull::RESTARTING) {
    if (!s_saved_for_restart) { s_saved_for_restart = true; save_now(); }
  } else {
    s_saved_for_restart = false;
  }

  net_loop();
  claude_api_loop();
  usb_cmd_poll(now);
  cam_uart_poll(now);
  cam_log_tick(now);
  if (cam_uart_take_restart()) cam_uart_send_ctrl(a.persist.cam_on);
  {   // XIAO kurz neu gestartet (Lücke unter 5 s) und Kamera-Zustand passt nicht zur Einstellung: C erneut, höchstens alle 5 s
    static uint32_t s_ctrl_t = 0;
    const CamLink &cl = cam_uart_link();
    const bool mismatch = cam_link_connected(cl, now) && cl.state != CAM_ST_ERR && (cl.state == CAM_ST_ON) != a.persist.cam_on;
    if (mismatch && !cl.upd && due_since(now, s_ctrl_t, 5000)) { s_ctrl_t = now; cam_uart_send_ctrl(a.persist.cam_on); }
  }
  apply_elapsed_once();

  int16_t tx, ty;
  const Gesture g = handle_touch(now, tx, ty);
  mood_frame(now, dt);
  if (g == Gesture::TAP && claude_needs_you(a.claude, now)) { s_needs_ack = true; s_needs_ack_since = a.claude.since; }   // Tippen stoppt das Blinken
  screen_frame(now, dt, g, tx, ty);
  if (a.blink_n) {   // LED-Signal (Pomodoro) überschreibt die Gefühlsfarbe: n× an/aus je 150 ms
    const uint32_t step = (now - a.blink_start) / kBlinkStepMs;
    if (step >= 2u * a.blink_n) a.blink_n = 0;
    else led_set((step & 1) ? Rgb{0, 0, 0} : a.blink_col, kLedPct);
  } else if (claude_needs_you(a.claude, now) && !(s_needs_ack && s_needs_ack_since == a.claude.since)) {
    led_set((now / 500) & 1 ? Rgb{0, 0, 0} : Rgb{255, 200, 0}, kLedPct);   // Claude braucht dich: gelb, 1 Hz, bis Tippen oder nächster Zustand
  }

  if (now - s_last_save >= kSaveEveryMs) {
    s_last_save = now;
    save_now();
  }
  diag_ldr_tick(now);   // nur mit -DCYD_LOG_LDR=1
  diag_fps_frame(now);  // nur mit -DCYD_LOG_FPS=1
  if (now - s_last_log >= kLogEveryMs) {
    const uint32_t fps = s_frames * 1000UL / (now - s_last_log);
    s_last_log = now;
    s_frames = 0;
    Serial.printf("heap=%u min=%u fps=%u emo=%d\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)fps, (int)a.emotion);
  }
}
