// Bildschirme: Wechsel zwischen Augen, Menü, Uhr, Einstellungen, Spielen, Update-Karte, Füttern, Timer, Rückblick.
// Jeder Bildschirm hat eine Funktion in der Tabelle kHandlers (Reihenfolge wie das Enum Screen).
#include "app_internal.h"
#include <Arduino.h>
#include "logic/daystats.h"
#include "logic/ota_policy.h"
#include "hw/cam_uart.h"
#include "ota/ota_pull.h"
#include "net/claude_api.h"
#include "ui/eyes.h"
#include "ui/status_bar.h"
#include "ui/menu.h"
#include "ui/clock_view.h"
#include "ui/settings.h"
#include "ui/game_catch.h"
#include "ui/game_simon.h"
#include "ui/feed.h"
#include "ui/timer_view.h"
#include "ui/review.h"
#include "ui/ota_card.h"

static bool s_manual_ota = false;   // in den Einstellungen "Firmware" getippt: nach der Suche Karte sofort zeigen
static uint32_t s_sprite_try = 0;   // letzter Versuch, das Augen-Sprite wieder anzulegen

struct ScreenCtx {
  uint32_t now, dt;
  Gesture g;
  int16_t tx, ty;
  bool tap;   // außerhalb der Augen zählt auch ein Stups (fünfter Tipp) als Tipp
};

static SettingsCam settings_cam(uint32_t now) {   // Zustand für Einstellungen Seite 2
  const Persist &p = g_app.persist;
  return SettingsCam{p.cam_on, cam_link_conn(cam_uart_link(), now, p.cam_on), cam_link_version(cam_uart_link())};
}

static void go_eyes() {
  s_manual_ota = false;
  g_app.screen = Screen::EYES;
  status_invalidate();   // Statusstreifen sicher neu zeichnen
}

// Zeile „Firmware“ in den Einstellungen.
int app_firmware_line(char *out, size_t n, uint32_t now) {
  const ota_pull::Info i = ota_pull::info();
  return ota_status_line((int)i.state, i.pct, ota_pull::check_busy(), i.last_check_ms, i.last_failed, now, out, n);
}

static void open_settings(uint32_t now) {
  s_manual_ota = false;
  g_app.screen = Screen::SETTINGS;
  settings_open(g_app.lcd, g_app.persist, now, settings_cam(now));
}

static void open_ota_card(uint32_t now) {
  g_app.screen = Screen::OTA;
  apply_user_backlight();
  ota_card_open(now);
  ota_card_draw(g_app.lcd, now);
}

static void scr_eyes(const ScreenCtx &c) {
  App &a = g_app;
  const uint32_t now = c.now;
  if (c.g == Gesture::LONG_PRESS && a.emotion != Emotion::SLEEPING) {
    a.screen = Screen::MENU;
    apply_user_backlight();
    menu_open(a.lcd, now);
    return;
  }
  // Karte nicht mitten in Pomodoro-Arbeit oder während Claude arbeitet (Einstellungen -> Firmware geht immer)
  if (ota_card_allowed(focused_work(), claude_state(a.claude, now)) && ota_card_wanted(now, a.emotion != Emotion::SLEEPING)) {   // nur bei wachem Display
    open_ota_card(now);
    return;
  }
  {
    struct tm t;
    const uint32_t day = today(&t);
    const bool awake = a.emotion != Emotion::SLEEPING;
    if (!focused_work() && !a.claude_busy && ritual_review(a.rituals, awake, a.touch_began, day ? t.tm_hour : -1, day ? t.tm_min : 0, day)) {
      daystats_roll(a.persist.day, day);
      a.screen = Screen::REVIEW;
      apply_user_backlight();
      review_open(a.lcd, a.persist.day, now);
      return;
    }
  }
  if (a.pressed) look_at(a.look, c.tx, c.ty, now);   // bei Touch-Beginn und während des Streichelns
  touch_to_mood(c.g, c.tx, c.ty, now);
  if (!eyes_ready() && now - s_sprite_try >= 5000) {   // kein Sprite (Speichermangel): alle 5 s neu versuchen
    s_sprite_try = now;
    eyes_begin(a.lcd);
  }
  eyes_frame(now, c.dt);
  ota_pull::ui_alive();
}

static void scr_menu(const ScreenCtx &c) {
  App &a = g_app;
  if (menu_timed_out(c.now)) { go_eyes(); return; }
  if (!c.tap) return;
  switch (menu_touch(c.tx, c.ty, c.now)) {
    case MenuChoice::CLOCK: a.screen = Screen::CLOCK; clock_open(a.lcd, c.now); break;
    case MenuChoice::SETTINGS: open_settings(c.now); break;
    case MenuChoice::CLOSE: go_eyes(); break;
    case MenuChoice::PLAY_CATCH: a.screen = Screen::GAME_CATCH; game_catch_open(a.lcd, c.now); break;
    case MenuChoice::PLAY_SIMON: a.screen = Screen::GAME_SIMON; game_simon_open(a.lcd, c.now); break;
    case MenuChoice::TIMER: a.screen = Screen::TIMER; timer_open(a.lcd, a.pomo.state != PomoState::IDLE, c.now); break;
    case MenuChoice::FEED: a.screen = Screen::FEED; feed_open(a.lcd, a.mood.n.satiety, c.now); break;
    default: break;
  }
}

static void scr_clock(const ScreenCtx &c) {
  if (c.tap || !clock_update(g_app.lcd, c.now)) go_eyes();
}

static void scr_settings(const ScreenCtx &c) {
  App &a = g_app;
  const uint32_t now = c.now;
  if (settings_timed_out(now)) { go_eyes(); return; }
  if (c.tap) {
    switch (settings_touch(c.tx, c.ty, now)) {
      case SettingsAction::CLOSE: go_eyes(); return;
      case SettingsAction::BRIGHTNESS:
        a.persist.brightness = (uint8_t)((a.persist.brightness + 1) % 3);
        apply_user_backlight();
        save_now();
        settings_redraw(a.lcd, a.persist, settings_cam(now));
        break;
      case SettingsAction::PORTAL: claude_api_stop(); net_open_portal(); settings_redraw(a.lcd, a.persist, settings_cam(now)); break;   // claude_api_stop: Port 80 frei für das Portal
      case SettingsAction::OTA_CHECK:
        // Manuelles Update: suchen (hebt auch "Später" auf); liegt eins bereit, kommt die Karte sofort (unten).
        ota_pull::check_now();
        s_manual_ota = true;
        settings_redraw(a.lcd, a.persist, settings_cam(now));
        break;
      case SettingsAction::CAM_TOGGLE:
        a.persist.cam_on = !a.persist.cam_on;
        cam_uart_send_ctrl(a.persist.cam_on);
        save_now();
        settings_redraw(a.lcd, a.persist, settings_cam(now));
        break;
      case SettingsAction::PAGE: settings_redraw(a.lcd, a.persist, settings_cam(now)); break;
      default: break;
    }
  }
  if (s_manual_ota && !ota_pull::check_busy()) {
    s_manual_ota = false;
    ota_card_unsnooze();
    if (ota_pull::info().state == ota_pull::AVAILABLE || ota_card_cam_offer(now)) { open_ota_card(now); return; }
  }
  settings_tick(a.lcd, a.persist, now, settings_cam(now));
}

static void scr_game(const ScreenCtx &c) {
  App &a = g_app;
  const GameResult r = a.screen == Screen::GAME_CATCH ? game_catch_update(a.lcd, c.tap, c.tx, c.ty, c.now)
                                                      : game_simon_update(a.lcd, c.tap, c.tx, c.ty, c.now);
  if (r == GameResult::RUNNING) return;
  a.persist.day.games++;
  char_game(a.chr);
  if (r == GameResult::WON) a.persist.day.wins++;
  mood_event(a.mood, r == GameResult::WON ? MoodEvent::GAME_WON : MoodEvent::GAME_LOST, c.now, current_hour());
  go_eyes();
}

static void scr_ota(const ScreenCtx &c) {
  // Beim Laden das Augen-Sprite freigeben (Puffer, TLS); nach Fehler/Schlummern wieder anlegen.
  if (ota_pull::info().state == ota_pull::DOWNLOADING && eyes_ready()) eyes_free();
  if (c.tap) ota_card_touch(c.tx, c.ty, c.now);
  if (!ota_card_wanted(c.now, true)) {
    if (!eyes_ready()) eyes_begin(g_app.lcd);
    g_app.lcd.fillScreen(TFT_BLACK);
    go_eyes();
    return;
  }
  ota_card_draw(g_app.lcd, c.now);
}

static void scr_feed(const ScreenCtx &c) {
  if (feed_timed_out(c.now)) { go_eyes(); return; }
  if (!c.tap) return;
  const int f = feed_touch(c.tx, c.ty, c.now);
  if (f == FEED_CLOSE) go_eyes();
  else if (f >= 0) {
    g_app.feed_food = f;
    g_app.feed_start = c.now;
    go_eyes();
  }
}

static void scr_timer(const ScreenCtx &c) {
  if (timer_timed_out(c.now)) { go_eyes(); return; }
  if (!c.tap) return;
  const int r = timer_touch(c.tx, c.ty, c.now);
  if (r > 0) {
    if (pomo_start(g_app.pomo, (uint8_t)r, c.now)) say(Say::POMODORO_START, c.now);
    go_eyes();
  } else if (r == TIMER_STOP) {
    pomo_stop(g_app.pomo);
    go_eyes();
  } else if (r == TIMER_BACK) {
    go_eyes();
  }
}

static void scr_review(const ScreenCtx &c) {
  if ((c.tap && review_tap_closes(c.now)) || !review_update(g_app.lcd, c.now)) go_eyes();
}

// Reihenfolge wie enum class Screen
static void (*const kHandlers[])(const ScreenCtx &) = {
  scr_eyes, scr_menu, scr_clock, scr_settings, scr_game, scr_game, scr_ota, scr_review, scr_feed, scr_timer,
};
static_assert(sizeof kHandlers / sizeof kHandlers[0] == (size_t)Screen::TIMER + 1, "Handler-Tabelle passt nicht zum Enum Screen");

void screen_frame(uint32_t now, uint32_t dt, Gesture g, int16_t tx, int16_t ty) {
  const Screen s = g_app.screen;
  const bool tap = g == Gesture::TAP || (s != Screen::EYES && g == Gesture::POKE);
  const size_t i = (size_t)s;
  if (i >= sizeof kHandlers / sizeof kHandlers[0]) { go_eyes(); return; }
  kHandlers[i](ScreenCtx{now, dt, g, tx, ty, tap});
}
