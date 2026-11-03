#include "app.h"
#include <Arduino.h>
#include "config.h"
#include "logic/mood.h"
#include "logic/gesture.h"
#include "logic/eye_model.h"
#include "logic/weather_mood.h"
#include "logic/look.h"
#include "logic/speech.h"
#include "logic/rituals.h"
#include "logic/daystats.h"
#include "logic/food.h"
#include "logic/character.h"
#include "ui/feed.h"
#include "logic/pomodoro.h"
#include "ui/timer_view.h"
#include "ui/review.h"
#include "hw/led.h"
#include "hw/light.h"
#include "hw/cam_uart.h"
#include "logic/presence.h"
#include "hw/store.h"
#include "ui/eyes.h"
#include "ui/status_bar.h"
#include "net/net.h"
#include "net/claude_api.h"
#include "ui/menu.h"
#include "ui/clock_view.h"
#include "ui/settings.h"
#include "ui/game_catch.h"
#include "ui/game_simon.h"
#include "ui/ota_card.h"
#include "ota/ota_pull.h"
#include "logic/ota_policy.h"
#include "logic/claude_face.h"
#include "logic/deadline.h"

#ifndef CYD_LOG_LDR
#define CYD_LOG_LDR 0   // 1 = Rohwert des Lichtsensors jede Sekunde seriell ausgeben (Einmessen); per build_flags -DCYD_LOG_LDR=1
#endif
#ifndef CYD_DIAG_TOUCH
#define CYD_DIAG_TOUCH 0   // 1 = Touch/Gesten seriell ausgeben (Kalibrierung prüfen); per build_flags -DCYD_DIAG_TOUCH=1
#endif

static LGFX s_lcd;
LGFX &app_lcd() { return s_lcd; }

static constexpr uint32_t kFrameMs = 33;              // ~30 Bilder/s
static constexpr uint32_t kSaveEveryMs = 10UL * 60000UL;
static constexpr uint32_t kLogEveryMs = 60000UL;
static constexpr uint32_t kStormEveryMs = 4UL * 60000UL;
static constexpr uint32_t kStormForMs = 1000;
static constexpr uint8_t kLedPct = 15;
static constexpr uint8_t kSleepBacklight = 26;        // 10 %
static const uint8_t kBacklight[3] = {77, 153, 255};  // 30 / 60 / 100 %

static Persist s_persist;
static ClaudeLogic s_claude;                       // Claude-Modus: Zustand aus den Hook-Ereignissen (claude_api -> claude_event)
// Claude-Modus Darstellung (Spec 2026-10-25-claude-modus-design.md)
static constexpr Rgb kClaudeCol{217, 119, 87};
static constexpr uint32_t kTouchPrioMs = 3000;         // Berührungs-Gefühle haben so lange Vorrang vor Claude-Gesichtern
static constexpr uint32_t kHeyEveryMs = 30000;
static Deadline s_touch_prio;
static uint8_t s_claude_fx = CFX_NONE;                 // Zusatzeffekte des angezeigten Claude-Gesichts
static bool s_claude_busy = false;                     // Zustand mit eigenem Gesicht (nicht OFF/IDLE)
static ClaudeState s_prev_cs = ClaudeState::OFF;
static uint32_t s_next_hmm = 0, s_next_hey = 0;
static bool s_needs_ack = false;                       // Tippen hat das gelbe Blinken beendet ...
static uint32_t s_needs_ack_since = 0;                 // ... für das NEEDS_YOU, das bei L.since begann
static MoodState s_mood;
static Presence s_presence;                         // Kamera: Anwesenheit, Begrüßung, Nähe, Blick
static PresenceOut s_pres;                          // Ergebnis des letzten mood_frame (für speech_frame/eyes_frame)
static bool s_prev_near = false;
static GestureState s_gesture;
static Blinker s_blink;
static Gazer s_gaze;
static FaceShape s_face;
static Rgb s_color;
static uint32_t s_last_frame = 0, s_last_save = 0, s_last_log = 0;
static uint32_t s_frames = 0;
static uint8_t s_backlight = 0;
static Emotion s_emotion = Emotion::CONTENT;

static bool s_elapsed_applied = false;
static LookState s_look{};
static bool s_pressed = false;                     // Finger liegt gerade auf
static Deadline s_hand;                           // High-Five-Hand sichtbar bis
static Deadline s_led_flash;                     // LED kurz weiß
static constexpr uint32_t kHandMs = 1500, kLedFlashMs = 250;

// F2: Sprechblasen, Rituale, Tageszähler
static constexpr uint32_t kBubbleMs = 4000;
static constexpr uint32_t kMorningWeatherDelayMs = 4000;
static constexpr uint8_t kStrokeSayPct = 25;
static SpeechState s_speech;
static RitualState s_rituals{};
static char s_bubble[64] = "";
static Deadline s_bubble_dl;
static char s_pending[64] = "";        // zweite Blase (Morgen: Tageswetter)
static uint32_t s_pending_at = 0;
static bool s_touch_began = false;     // in diesem Bild hat eine Berührung begonnen
static bool s_prev_sleeping = false;
static bool s_prev_storm = false;
static uint32_t s_seen_weather_ms = 0; // fetched_ms des zuletzt ausgewerteten Wetters
static bool s_sky_known = false;
static Sky s_last_sky = Sky::CLOUDY;

// F3: Füttern, Krankheit, Charakter
static constexpr uint32_t kFlyMs = 600, kChewMs = 1200, kChewStepMs = 200, kSneezeMs = 600;
static constexpr int kFoodY0 = 236, kMouthY = 172;
static Character s_char{};
static int s_feed_food = -1;           // gerade gefütterte Speise (Animation), -1 = keine
static uint32_t s_feed_start = 0;
static Deadline s_sneeze;

// F4: Pomodoro
static constexpr Rgb kTimerCol{255, 90, 140}, kBreakCol{60, 220, 120};
static constexpr uint32_t kBlinkStepMs = 150;
static Pomo s_pomo{};
static Rgb s_blink_col{0, 0, 0};       // LED-Signal: Farbe, Anzahl, Start
static uint8_t s_blink_n = 0;
static uint32_t s_blink_start = 0;
static bool focused_work() { return s_pomo.state == PomoState::WORK; }
static void led_blink(Rgb c, uint8_t n, uint32_t now) { s_blink_col = c; s_blink_n = n; s_blink_start = now; }

enum class Screen { EYES, MENU, CLOCK, SETTINGS, GAME_CATCH, GAME_SIMON, OTA, REVIEW, FEED, TIMER };
static Screen s_screen = Screen::EYES;

static int current_hour() { return net_hour(); }

static void set_backlight(uint8_t v) {
  if (v == s_backlight) return;
  s_backlight = v;
  s_lcd.setBrightness(v);
}

static void save_now() {
  s_persist.needs = s_mood.n;
  s_persist.sick = s_mood.sick;
  s_persist.cuddly = s_char.cuddly;
  s_persist.playful = s_char.playful;
  const uint32_t epoch = net_epoch();
  if (epoch) s_persist.epoch = epoch;
  store_save(s_persist);
}

// Frisches Wetter (höchstens 3 h alt) oder false.
static bool fresh_weather(uint32_t now, WeatherMood *wm, NetWeather *w) {
  *w = net_weather();
  if (!w->valid || !weather_fresh(now, w->fetched_ms)) return false;
  *wm = weather_mood(w->code, w->temp_c);
  return true;
}

static SettingsCam settings_cam(uint32_t now) {   // Zustand für Einstellungen Seite 2
  return SettingsCam{s_persist.cam_on, cam_link_conn(cam_uart_link(), now, s_persist.cam_on), cam_link_version(cam_uart_link())};
}

static StatusInfo status_info(uint32_t now) {
  StatusInfo si{};
  struct tm t;
  si.time_ok = net_local_time(&t);
  si.hour = si.time_ok ? t.tm_hour : 0;
  si.minute = si.time_ok ? t.tm_min : 0;
  si.wifi_ok = net_wifi_ok();
  si.portal = net_portal_active();
  WeatherMood wm;
  NetWeather w;
  si.weather_ok = fresh_weather(now, &wm, &w);
  si.sky = si.weather_ok ? wm.sky : Sky::CLOUDY;
  si.is_day = si.weather_ok ? w.is_day : true;
  si.temp_c = si.weather_ok ? w.temp_c : 0;
  si.update_ready = ota_pull::info().state == ota_pull::AVAILABLE;
  si.timer_s = (uint16_t)pomo_remaining_s(s_pomo, now);
  si.timer_break = s_pomo.state == PomoState::BREAK;
  if (s_persist.cam_on) {   // Kamera-Auge: rot ohne Verbindung/bei Fehler, grün bei laufender Kamera mit Gesicht, sonst grau
    const CamConn cc = cam_link_conn(cam_uart_link(), now, true);
    si.cam = (cc == CamConn::NONE || cc == CamConn::ERR) ? 3 : (cc == CamConn::ON && s_pres.present ? 2 : 1);
  }
  si.claude_on = claude_active(s_claude, now);
  si.helpers = claude_helpers(s_claude, now);
  return si;
}

// Einmal, sobald die Uhrzeit bekannt ist: Aus-Zeit seit dem letzten Speichern wie Schlaf nachrechnen.
static void apply_elapsed_once() {
  if (s_elapsed_applied || !net_time_ok()) return;
  s_elapsed_applied = true;
  const uint32_t now_epoch = net_epoch();
  if (s_persist.epoch && now_epoch > s_persist.epoch) {
    const uint32_t secs = now_epoch - s_persist.epoch;
    mood_apply_elapsed(s_mood, secs);
    Serial.printf("mood: %u s aus, Energie jetzt %.1f\n", (unsigned)secs, s_mood.n.energy);
  }
  s_persist.epoch = now_epoch;
}

static void show_bubble(const char *txt, uint32_t now) {
  strlcpy(s_bubble, txt, sizeof s_bubble);
  s_bubble_dl.arm(now, kBubbleMs);
  speech_shown(s_speech, now);
}
// Claude-Blase: während der Pomodoro-Arbeit nur, wenn Claude etwas von dir braucht (even_in_work).
static void claude_say(Say what, uint32_t now, bool even_in_work = false) {
  if (focused_work() && !even_in_work) return;
  show_bubble(speech_pick(s_speech, what, esp_random()), now);
}
static void say(Say what, uint32_t now) {
  const bool pomo_say = what == Say::POMODORO_START || what == Say::POMODORO_PAUSE || what == Say::POMODORO_FERTIG;
  if (s_claude_busy && !pomo_say) return;                      // Claude arbeitet: keine Pet-Blasen
  if (focused_work() && what != Say::POMODORO_START) return;   // während der Arbeit still (außer beim Start)
  show_bubble(speech_pick(s_speech, what, esp_random()), now);
}

// Heutiges Datum als YYYYMMDD, 0 wenn die Zeit unbekannt ist.
static uint32_t today(struct tm *t) {
  if (!net_local_time(t)) return 0;
  return day_key(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

// Touch lesen und Gesten an die Stimmung weitergeben. Liefert die Geste (für Menü usw.).
static Gesture handle_touch(uint32_t now, int16_t &tx, int16_t &ty) {
  int32_t x = 0, y = 0;
  const bool pressed = s_lcd.getTouch(&x, &y) > 0;
  // Ein Tipp wird erst beim Loslassen gemeldet; dann liefert getTouch keine Position mehr.
  // Darum gilt beim Loslassen die letzte gültige Position (vorher kam (0,0) an: Menü traf nichts, "Ja" zählte als "daneben").
  static int16_t s_last_x = 0, s_last_y = 0;
  if (pressed) { s_last_x = (int16_t)x; s_last_y = (int16_t)y; }
  tx = s_last_x;
  ty = s_last_y;
  if (pressed) mood_touch(s_mood, now);
  s_touch_began = pressed && !s_pressed;
  s_pressed = pressed;   // jede Berührung hält ihn nachts wach bzw. weckt ihn
#if CYD_DIAG_TOUCH
  static bool s_was_pressed = false;
  if (pressed && !s_was_pressed) {   // Beginn einer Berührung seriell ausgeben (Touch-Kalibrierung prüfen)
    lgfx::touch_point_t raw;
    s_lcd.getTouchRaw(&raw, 1);
    Serial.printf("touch x=%d y=%d raw=%d,%d\n", (int)x, (int)y, (int)raw.x, (int)raw.y);
  }
  s_was_pressed = pressed;
#endif
#if CYD_DIAG_TOUCH
  // Berührung, die der Chip meldet, die Bibliothek aber verwirft (außerhalb des Bereichs): einmal je Berührung melden
  static bool s_raw_only = false;
  if (!pressed) {
    lgfx::touch_point_t raw;
    const bool raw_down = s_lcd.getTouchRaw(&raw, 1) > 0;
    if (raw_down && !s_raw_only) Serial.printf("touch verworfen raw=%d,%d\n", (int)raw.x, (int)raw.y);
    s_raw_only = raw_down;
  }
#endif
  const Gesture g = gesture_feed(s_gesture, pressed, tx, ty, now);
#if CYD_DIAG_TOUCH
  if (g != Gesture::NONE)   // Diagnose: welche Geste mit welcher Position auf welchem Bildschirm
    Serial.printf("geste %d bei x=%d y=%d screen=%d\n", (int)g, (int)tx, (int)ty, (int)s_screen);
#endif
  return g;
}

static void touch_to_mood(Gesture g, int16_t tx, int16_t ty, uint32_t now) {
  const int hour = current_hour();
  if (g == Gesture::TAP || g == Gesture::STROKE || g == Gesture::POKE) s_touch_prio.arm(now, kTouchPrioMs);
  switch (g) {
    case Gesture::TAP:
      if (focused_work()) break;   // während der Arbeit: kein Überraschen, nur der Blick folgt
      if (highfive_hit(tx, ty)) {   // obere Ecke: High-Five statt Tippen
        mood_event(s_mood, MoodEvent::HIGHFIVE, now, hour);
        char_highfive(s_char);
        say(Say::HIGHFIVE, now);
        eyes_set_hand(tx > 160, now);
        s_hand.arm(now, kHandMs);
        s_led_flash.arm(now, kLedFlashMs);
      } else {
        mood_event(s_mood, MoodEvent::TAP, now, hour);
      }
      break;
    case Gesture::STROKE:
      mood_event(s_mood, MoodEvent::STROKE, now, hour);
      s_persist.day.strokes++;
      char_stroke(s_char);
      if (speech_may(s_speech, now) && esp_random() % 100 < kStrokeSayPct) say(Say::VERLIEBT, now);   // Abstand gilt
      break;
    case Gesture::POKE:
      mood_event(s_mood, MoodEvent::POKE, now, hour);
      s_persist.day.pokes++;
      char_poke(s_char);
      say(Say::GENERVT, now);
      break;
    default: break;
  }
}

// Claude-Blasen bei Zustandswechseln (Spec-Tabelle); läuft jedes Bild.
static void claude_frame(ClaudeState cs, uint32_t now) {
  if (cs != s_prev_cs) {
    const bool was_calm = s_prev_cs == ClaudeState::OFF || s_prev_cs == ClaudeState::IDLE || s_prev_cs == ClaudeState::DONE;
    if (was_calm && claude_busy(cs) && cs != ClaudeState::DONE) s_next_hmm = now + 20000 + esp_random() % 20001;
    switch (cs) {
      case ClaudeState::LISTENING: claude_say(Say::CLAUDE_OKAY, now); break;
      case ClaudeState::FAILED: claude_say(Say::CLAUDE_FEHLER, now); break;
      case ClaudeState::NEEDS_YOU:
        claude_say(Say::CLAUDE_HEY, now, true);
        s_next_hey = now + kHeyEveryMs;
        break;
      case ClaudeState::DONE:
        claude_say(Say::CLAUDE_FERTIG, now);
        eyes_set_hand(true, now);
        break;
      default: break;
    }
    s_prev_cs = cs;
  }
  if (cs == ClaudeState::THINKING && (int32_t)(now - s_next_hmm) >= 0) {   // alle 20-40 s ein „Hmm...“
    claude_say(Say::CLAUDE_HMM, now);
    s_next_hmm = now + 20000 + esp_random() % 20001;
  }
  if (cs == ClaudeState::NEEDS_YOU && (int32_t)(now - s_next_hey) >= 0) {   // „Hey, du!“ alle 30 s
    claude_say(Say::CLAUDE_HEY, now, true);
    s_next_hey = now + kHeyEveryMs;
  }
}

// Kamera zählt nur, wenn eingeschaltet und der XIAO läuft.
static bool cam_active(uint32_t now) {
  return s_persist.cam_on && cam_link_conn(cam_uart_link(), now, s_persist.cam_on) == CamConn::ON;
}

// Diagnose über USB (COM6): einmal pro Sekunde Zahl der F/N/H/M-Zeilen, letztes Gesicht und letzte Bewegung (x,y,n).
static uint16_t s_cam_nf = 0, s_cam_nn = 0, s_cam_nh = 0, s_cam_nm = 0;
static CamMsg s_cam_last, s_cam_motion;
static uint32_t s_cam_log_t = 0;

static void cam_log_tick(uint32_t now) {
  if (!due_since(now, s_cam_log_t, 1000)) return;
  s_cam_log_t = now;
  if (cam_uart_snap_active(now)) return;   // während des Schnappschusses keine Textzeilen zwischen den J-Zeilen
  const CamLink &l = cam_uart_link();
  Serial.printf("cam: user=%d conn=%d xiao=%s %s F=%u N=%u H=%u M=%u last=%d,%d,%d mot=%d,%d,%d\n", s_persist.cam_on ? 1 : 0,
                (int)cam_link_conn(l, now, true), l.version, cam_state_name(l.state), s_cam_nf, s_cam_nn, s_cam_nh, s_cam_nm,
                s_cam_last.x, s_cam_last.y, s_cam_last.w, s_cam_motion.x, s_cam_motion.y, s_cam_motion.n);
  s_cam_nf = s_cam_nn = s_cam_nh = s_cam_nm = 0;
}

// Diagnose der U-Zeile: Länge und Zeichenart der Felder (ohne Inhalt) und ob der CYD sie selbst parsen kann.
static void usb_uinfo() {
  const ota_pull::CamInfo c = ota_pull::cam_info();
  char line[CAM_LINE_MAX + 8];
  const size_t n = c.avail ? net_build_cam_upd(c.url, c.sha, line, sizeof line) : 0;
  Serial.printf("uinfo: avail=%d n=%u", c.avail ? 1 : 0, (unsigned)n);
  if (n) {
    CamMsg m;
    const bool ok = cam_parse(line, n - 1, &m);   // ohne '\n'
    unsigned f = 0, len = 0, odd = 0;
    for (size_t i = 0; i + 4 < n; i++) {           // ohne "*XX\n"
      const uint8_t b = (uint8_t)line[i];
      if (b == '\t') { Serial.printf(" f%u=%u/%u", f, len, odd); f++; len = odd = 0; continue; }
      len++;
      if (b < 0x20 || b >= 0x7f) odd++;
    }
    Serial.printf(" f%u=%u/%u parse=%d", f, len, odd, ok ? 1 : 0);
    volatile char *p = m.pass;
    for (size_t i = 0; i < sizeof m.pass; i++) p[i] = 0;
  }
  Serial.println();
  volatile char *w = line;   // enthält das WLAN-Passwort
  for (size_t i = 0; i < sizeof line; i++) w[i] = 0;
}

// Diagnose: U-Zeile mit Schein-WLAN senden (der XIAO muss mit "U err" antworten); slow = Byte für Byte mit Pausen.
static void usb_utest(bool slow) {
  const ota_pull::CamInfo c = ota_pull::cam_info();
  char line[CAM_LINE_MAX + 8];
  const size_t n = c.avail ? cam_build_upd_cmd(c.url, c.sha, "madbuddy-test", "x", line, sizeof line) : 0;
  Serial.printf("utest: n=%u slow=%d\n", (unsigned)n, slow ? 1 : 0);
  if (!slow) { cam_uart_write_sealed(line, n); return; }
  for (size_t i = 0; i < n; i++) { cam_uart_write_sealed(line + i, 1); delayMicroseconds(400); }
}

// USB-Befehle vom PC, zeilenweise und nichtblockierend: "snap" = Kamerabild über USB holen (tools/cam-snap.py),
// "uinfo"/"utest"/"utestslow" = Diagnose des Kamera-Updates.
static void usb_cmd_poll(uint32_t now) {
  static char buf[16];
  static uint8_t n = 0;
  static bool over = false;
  for (int i = 0; i < 64 && Serial.available(); i++) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[n] = '\0';
      if (!over && strcmp(buf, "snap") == 0) cam_uart_snap(now);
      else if (!over && strcmp(buf, "uinfo") == 0) usb_uinfo();
      else if (!over && strcmp(buf, "utest") == 0) usb_utest(false);
      else if (!over && strcmp(buf, "utestslow") == 0) usb_utest(true);
      n = 0;
      over = false;
    } else if (n < sizeof buf - 1) buf[n++] = c;
    else over = true;
  }
}

static void on_cam_msg(const CamMsg &m, uint32_t now) {
  if (m.kind == CAM_FACE) { s_cam_nf++; s_cam_last = m; }
  else if (m.kind == CAM_NOFACE) s_cam_nn++;
  else if (m.kind == CAM_HEART) s_cam_nh++;
  else if (m.kind == CAM_MOTION) { s_cam_nm++; s_cam_motion = m; }
  presence_msg(s_presence, m, now, CAM_MIRROR_X, CAM_GAZE_Y0);
  if (m.kind == CAM_UPD_PCT || m.kind == CAM_UPD_OK || m.kind == CAM_UPD_ERR) {
    Serial.printf("cam: U kind=%d pct=%u %s\n", (int)m.kind, (unsigned)m.pct, m.text);
    ota_card_cam_msg(m, now);   // Kamera-Update (Karte)
  }
}

// Stimmung läuft auf jedem Bildschirm weiter (Zeit vergeht), Licht und LED auch.
static void mood_frame(uint32_t now, uint32_t dt) {
  const int hour = current_hour();
  const bool work = focused_work();
  const ClaudeState cs = claude_state(s_claude, now);
  s_claude_busy = claude_busy(cs);
  const bool calm = work || s_claude_busy;   // Pomodoro-Arbeit bzw. Claude arbeitet: kein Schlaf, keine Eigenarten
  if (calm) mood_touch(s_mood, now);
  const bool cam = cam_active(now);
  const PresenceOut po = presence_tick(s_presence, cam, now, CAM_NEAR_W);
  s_pres = po;   // für eyes_frame/speech_frame
  const bool asleep = mood_is_sleeping(s_mood, now);
  if (po.present && !(mood_is_night(hour) && asleep)) mood_touch(s_mood, now);   // da = wach halten; nachts weckt ein Gesicht nicht
  const bool ldr_dark = light_update(now);   // läuft immer weiter, damit die Glättung nicht springt
  const bool dark = cam ? po.unattended : ldr_dark;   // Kamera ersetzt den Lichtsensor
  mood_set_dark(s_mood, calm ? false : dark, hour, now);
  if (po.near && !s_prev_near) mood_set_transient(s_mood, Emotion::NEAR, now, PRES_NEAR_FOR_MS);
  s_prev_near = po.near;
  mood_night_check(s_mood, hour, now);
  mood_set_character(s_mood, s_char);
  mood_tick(s_mood, dt, now);
  if (!calm) mood_quirks(s_mood, hour, now);
  switch (pomo_tick(s_pomo, now)) {
    case PomoEvent::BREAK_START:
      mood_set_transient(s_mood, Emotion::EXCITED, now, 3000);
      say(Say::POMODORO_PAUSE, now);
      led_blink(Rgb{0, 255, 0}, 5, now);
      s_persist.day.pomodoros++;
      break;
    case PomoEvent::BREAK_END:
      say(Say::POMODORO_FERTIG, now);
      led_blink(Rgb{0, 0, 255}, 2, now);
      break;
    default: break;
  }
  s_emotion = mood_emotion(s_mood, hour, now);
  if (focused_work() && s_emotion != Emotion::SLEEPING) s_emotion = Emotion::FOCUSED;   // überschreibt alles außer Schlaf
  if (s_emotion == Emotion::BORED) char_bored(s_char, dt);
  // Claude-Gesicht: Berührungs-Gefühl (3 s) und Pomodoro haben Vorrang
  const bool touch_feel = s_touch_prio.active(now) && !mood_is_sleeping(s_mood, now) &&
                          s_emotion != mood_base_emotion(s_mood, hour);
  const ClaudeFace cf = claude_face(cs, work, touch_feel, s_emotion);
  s_emotion = cf.emo;
  s_claude_fx = cf.fx;
  claude_frame(cs, now);
  s_persist.cuddly = s_char.cuddly;   // für die Anzeige in den Einstellungen
  s_persist.playful = s_char.playful;
}

// Neues Wetter mit geändertem Himmel: einmal ein passender Satz.
static void weather_say(uint32_t now) {
  WeatherMood wm;
  NetWeather w;
  if (!fresh_weather(now, &wm, &w) || w.fetched_ms == s_seen_weather_ms) return;
  s_seen_weather_ms = w.fetched_ms;
  const bool changed = s_sky_known && wm.sky != s_last_sky;
  s_sky_known = true;
  s_last_sky = wm.sky;
  if (!changed) return;
  if (wm.hot) say(Say::HITZE, now);
  else if (wm.cold) say(Say::KALT, now);
  else if (wm.sky == Sky::RAIN) say(Say::REGEN, now);
  else if (wm.sky == Sky::SUNNY) say(Say::SONNE, now);
}

// Sprechblasen und Rituale auf dem Augen-Bildschirm (nicht im Schlaf).
static void speech_frame(uint32_t now, bool sleeping) {
  const bool woke = (s_prev_sleeping && !sleeping) || s_touch_began;
  s_prev_sleeping = sleeping;
  if (sleeping) { s_bubble_dl.clear(); s_pending_at = 0; return; }
  struct tm t;
  const uint32_t day = today(&t);
  daystats_roll(s_persist.day, day);
  if (focused_work() || s_claude_busy) return;   // keine Rituale, Gedanken, Hatschi-Blasen während der Arbeit bzw. Claude-Modus
  const int hour = day ? t.tm_hour : -1, minute = day ? t.tm_min : 0;
  if (ritual_morning(s_rituals, woke, hour, day)) {
    mood_set_transient(s_mood, Emotion::GREETING, now, 3000);
    say(Say::MORGEN, now);
    WeatherMood wm;
    NetWeather w;
    if (fresh_weather(now, &wm, &w)) {
      weather_sentence(w.temp_c, wm.sky, s_pending, sizeof s_pending);
      s_pending_at = now + kMorningWeatherDelayMs;
    }
  } else if (s_pres.greet) {   // Kamera: jemand ist wieder da (Morgen hat Vorrang)
    mood_set_transient(s_mood, Emotion::GREETING, now, 3000);
    say(Say::WIEDER, now);
  }
  s_pres.greet = false;
  if (ritual_night(s_rituals, true, hour, minute, day)) {
    mood_set_transient(s_mood, Emotion::YAWN, now, 3000);
    say(Say::NACHT, now);
  }
  if (s_pending_at && (int32_t)(now - s_pending_at) >= 0) {
    s_pending_at = 0;
    show_bubble(s_pending, now);
  }
  weather_say(now);
  Say what;
  if (speech_due(s_speech, mood_base_emotion(s_mood, hour), now, &what, s_char.cuddly)) say(what, now);
  if (mood_sneeze_due(s_mood, now)) {   // krank: Hatschi
    s_sneeze.arm(now, kSneezeMs);
    say(Say::KRANK, now);
  }
}

static void eyes_frame(uint32_t now, uint32_t dt) {
  const bool sleeping = s_emotion == Emotion::SLEEPING;
  WeatherMood wm;
  NetWeather w;
  const bool have_weather = fresh_weather(now, &wm, &w);
  uint32_t ov = overlays_for(s_emotion, have_weather ? &wm : nullptr, have_weather ? w.is_day : true);
  if (s_hand.active(now) && !sleeping) ov |= OV_HAND;
  if (!sleeping && (s_claude_fx & CFX_HAND)) ov |= OV_HAND;
  if (!sleeping && (s_claude_fx & CFX_TYPING)) ov |= OV_TYPING;

  const bool storm = (ov & OV_STORM) && now % kStormEveryMs < kStormForMs && !s_claude_busy;
  if (storm && !s_prev_storm && !sleeping) say(Say::GEWITTER, now);   // Gewitter-Schreck
  s_prev_storm = storm;
  speech_frame(now, sleeping);
  FaceExpr target = face_expression(storm ? Emotion::STARTLED : s_emotion);
  gazer_update(s_gaze, now);
  float gx = s_gaze.gx, gy = s_gaze.gy;
  if (look_active(s_look, now)) look_target(s_look, &gx, &gy);   // Blick folgt dem Finger
  else if (s_pres.gaze) { gx = s_pres.gx; gy = s_pres.gy; }   // Blick folgt dem Gesicht
  if (sleeping || (s_claude_fx & (CFX_STRAIGHT | CFX_JUMP))) gx = gy = 0;   // Claude: Blick gerade bzw. springend, kein Wandern
  target.s.pupil_dx += gx / GAZE_X_MAX * 14.0f;
  target.s.pupil_dy += gy / GAZE_Y_MAX * 10.0f;
  target.s.eye.gaze_x = gx / GAZE_X_MAX * 8.0f;
  target.s.eye.gaze_y = gy / GAZE_Y_MAX * 4.0f;
  if (s_claude_fx & CFX_STRAIGHT) {   // Claude braucht dich: Blick gerade auf den Betrachter, Brauen hoch
    target.s.pupil_dx = 0;
    target.s.pupil_dy = 0;
    target.s.pupil_r = 18;
    target.s.brow_y = -26;
  }
  if (s_claude_fx & CFX_JUMP) target.s.pupil_dx = ((now / 250) & 1) ? 14.0f : -14.0f;   // Lesen: alle 250 ms links <-> rechts
  s_face = face_tween(s_face, target.s, dt);   // switch_acc läuft im s_face mit
  s_color = rgb_tween(s_color, target.color, dt);

  eyes_set_spin((s_claude_fx & CFX_RING) && !sleeping, kClaudeCol);   // Claude führt etwas aus (nur ohne Pomodoro-Ring)
  if (s_pomo.state == PomoState::IDLE) eyes_set_ring(-1, kTimerCol);
  else eyes_set_ring(pomo_progress(s_pomo, now), s_pomo.state == PomoState::WORK ? kTimerCol : kBreakCol);

  FaceShape draw = s_face;
  if (!sleeping) draw.eye.open *= blinker_open(s_blink, now);
  const bool sneezing = s_sneeze.active(now);
  if (sneezing) { ov |= OV_SNEEZE; draw.eye.open *= 0.1f; }   // Augen kurz zu
  // Füttern: Speise fliegt in 600 ms von unten zum Mund, dann 3× kauen, dann Reaktion
  if (s_feed_food >= 0) {
    const uint32_t t = now - s_feed_start;
    if (t < kFlyMs) {
      eyes_set_food(s_feed_food, kFoodY0 + (int)((kMouthY - kFoodY0) * (float)t / kFlyMs));
    } else if (t < kFlyMs + kChewMs) {
      eyes_set_food(-1, 0);
      draw.mouth = Mouth::CURVE;
      draw.mouth_curve = 0.3f;
      draw.mouth_open = ((t - kFlyMs) / kChewStepMs) % 2 ? 0.6f : 0.0f;
    } else {
      eyes_set_food(-1, 0);
      const Food f = (Food)s_feed_food;
      s_feed_food = -1;
      s_persist.day.meals++;
      say(food_feed(s_mood, f, (Food)s_persist.favorite_food, now), now);
    }
  }
  eyes_draw(s_lcd, draw, s_color, ov, now, s_bubble_dl.active(now) ? s_bubble : nullptr);
  status_draw(s_lcd, status_info(now));

  if (s_led_flash.active(now)) led_set(Rgb{255, 255, 255}, kLedPct);   // High-Five
  else led_set(sleeping ? Rgb{0, 0, 0} : s_color, kLedPct);
  set_backlight(sleeping ? kSleepBacklight : kBacklight[s_persist.brightness]);
}

static bool s_manual_ota = false;   // in den Einstellungen "Firmware" getippt: nach der Suche Karte sofort zeigen
static uint32_t s_sprite_try = 0;   // letzter Versuch, das Augen-Sprite wieder anzulegen

static void go_eyes() {
  s_manual_ota = false;
  s_screen = Screen::EYES;
  status_invalidate();   // Statusstreifen sicher neu zeichnen
}

// Zeile „Firmware“ in den Einstellungen.
static int firmware_line(char *out, size_t n, uint32_t now) {
  const ota_pull::Info i = ota_pull::info();
  return ota_status_line((int)i.state, i.pct, ota_pull::check_busy(), i.last_check_ms, i.last_failed, now, out, n);
}

static void open_settings(uint32_t now) {
  s_manual_ota = false;
  s_screen = Screen::SETTINGS;
  settings_open(s_lcd, s_persist, now, settings_cam(now));
}

static void open_ota_card(uint32_t now) {
  s_screen = Screen::OTA;
  set_backlight(kBacklight[s_persist.brightness]);
  ota_card_open(now);
  ota_card_draw(s_lcd, now);
}

static void screen_frame(uint32_t now, uint32_t dt, Gesture g, int16_t tx, int16_t ty) {
  const bool tap = g == Gesture::TAP || (s_screen != Screen::EYES && g == Gesture::POKE);   // außerhalb der Augen zählt auch ein Stups (fünfter Tipp) als Tipp
  switch (s_screen) {
    case Screen::EYES:
      if (g == Gesture::LONG_PRESS && s_emotion != Emotion::SLEEPING) {
        s_screen = Screen::MENU;
        set_backlight(kBacklight[s_persist.brightness]);
        menu_open(s_lcd, now);
        return;
      }
      // Karte nicht mitten in Pomodoro-Arbeit oder während Claude arbeitet (Einstellungen -> Firmware geht immer)
      {
        const ClaudeState cs_now = claude_state(s_claude, now);
        const bool ota_ok_now = !focused_work() && (cs_now == ClaudeState::OFF || cs_now == ClaudeState::IDLE);
        if (ota_ok_now && ota_card_wanted(now, s_emotion != Emotion::SLEEPING)) {   // nur bei wachem Display
          open_ota_card(now);
          return;
        }
      }
      {
        struct tm t;
        const uint32_t day = today(&t);
        const bool awake = s_emotion != Emotion::SLEEPING;
        if (!focused_work() && !s_claude_busy && ritual_review(s_rituals, awake, s_touch_began, day ? t.tm_hour : -1, day ? t.tm_min : 0, day)) {
          daystats_roll(s_persist.day, day);
          s_screen = Screen::REVIEW;
          set_backlight(kBacklight[s_persist.brightness]);
          review_open(s_lcd, s_persist.day, now);
          return;
        }
      }
      if (s_pressed) look_at(s_look, tx, ty, now);   // bei Touch-Beginn und während des Streichelns
      touch_to_mood(g, tx, ty, now);
      if (!eyes_ready() && now - s_sprite_try >= 5000) {   // kein Sprite (Speichermangel): alle 5 s neu versuchen
        s_sprite_try = now;
        eyes_begin(s_lcd);
      }
      eyes_frame(now, dt);
      ota_pull::ui_alive();
      return;

    case Screen::MENU: {
      if (menu_timed_out(now)) { go_eyes(); return; }
      if (!tap) return;
      switch (menu_touch(tx, ty, now)) {
        case MenuChoice::CLOCK: s_screen = Screen::CLOCK; clock_open(s_lcd, now); break;
        case MenuChoice::SETTINGS: open_settings(now); break;
        case MenuChoice::CLOSE: go_eyes(); break;
        case MenuChoice::PLAY_CATCH: s_screen = Screen::GAME_CATCH; game_catch_open(s_lcd, now); break;
        case MenuChoice::PLAY_SIMON: s_screen = Screen::GAME_SIMON; game_simon_open(s_lcd, now); break;
        case MenuChoice::TIMER: s_screen = Screen::TIMER; timer_open(s_lcd, s_pomo.state != PomoState::IDLE, now); break;
        case MenuChoice::FEED: s_screen = Screen::FEED; feed_open(s_lcd, s_mood.n.satiety, s_mood.sick, now); break;
        default: break;
      }
      return;
    }

    case Screen::CLOCK:
      if (tap || !clock_update(s_lcd, now)) go_eyes();
      return;

    case Screen::SETTINGS:
      if (settings_timed_out(now)) { go_eyes(); return; }
      if (tap) {
        switch (settings_touch(tx, ty, now)) {
          case SettingsAction::CLOSE: go_eyes(); return;
          case SettingsAction::BRIGHTNESS:
            s_persist.brightness = (uint8_t)((s_persist.brightness + 1) % 3);
            set_backlight(kBacklight[s_persist.brightness]);
            save_now();
            settings_redraw(s_lcd, s_persist, settings_cam(now));
            break;
          case SettingsAction::PORTAL: claude_api_stop(); net_open_portal(); settings_redraw(s_lcd, s_persist, settings_cam(now)); break;   // claude_api_stop: Port 80 frei für das Portal
          case SettingsAction::OTA_CHECK:
            // Manuelles Update: suchen (hebt auch "Später" auf); liegt eins bereit, kommt die Karte sofort (unten).
            ota_pull::check_now();
            s_manual_ota = true;
            settings_redraw(s_lcd, s_persist, settings_cam(now));
            break;
          case SettingsAction::CAM_TOGGLE:
            s_persist.cam_on = !s_persist.cam_on;
            cam_uart_send_ctrl(s_persist.cam_on);
            save_now();
            settings_redraw(s_lcd, s_persist, settings_cam(now));
            break;
          case SettingsAction::PAGE: settings_redraw(s_lcd, s_persist, settings_cam(now)); break;
          default: break;
        }
      }
      if (s_manual_ota && !ota_pull::check_busy()) {
        s_manual_ota = false;
        ota_card_unsnooze();
        if (ota_pull::info().state == ota_pull::AVAILABLE || ota_card_cam_offer(now)) { open_ota_card(now); return; }
      }
      settings_tick(s_lcd, s_persist, now, settings_cam(now));
      return;

    case Screen::GAME_CATCH:
    case Screen::GAME_SIMON: {
      const GameResult r = s_screen == Screen::GAME_CATCH ? game_catch_update(s_lcd, tap, tx, ty, now)
                                                          : game_simon_update(s_lcd, tap, tx, ty, now);
      if (r == GameResult::RUNNING) return;
      s_persist.day.games++;
      char_game(s_char);
      if (r == GameResult::WON) s_persist.day.wins++;
      mood_event(s_mood, r == GameResult::WON ? MoodEvent::GAME_WON : MoodEvent::GAME_LOST, now, current_hour());
      go_eyes();
      return;
    }

    case Screen::OTA: {
      // Beim Laden das Augen-Sprite freigeben (Puffer, TLS); nach Fehler/Schlummern wieder anlegen.
      if (ota_pull::info().state == ota_pull::DOWNLOADING && eyes_ready()) eyes_free();
      if (tap) ota_card_touch(tx, ty, now);
      if (!ota_card_wanted(now, true)) {
        if (!eyes_ready()) eyes_begin(s_lcd);
        s_lcd.fillScreen(TFT_BLACK);
        go_eyes();
        return;
      }
      ota_card_draw(s_lcd, now);
      return;
    }

    case Screen::FEED: {
      if (feed_timed_out(now)) { go_eyes(); return; }
      if (!tap) return;
      const int f = feed_touch(tx, ty, now);
      if (f == FEED_CLOSE) go_eyes();
      else if (f >= 0) {
        s_feed_food = f;
        s_feed_start = now;
        go_eyes();
      }
      return;
    }

    case Screen::TIMER: {
      if (timer_timed_out(now)) { go_eyes(); return; }
      if (!tap) return;
      const int r = timer_touch(tx, ty, now);
      if (r > 0) {
        if (pomo_start(s_pomo, (uint8_t)r, now)) say(Say::POMODORO_START, now);
        go_eyes();
      } else if (r == TIMER_STOP) {
        pomo_stop(s_pomo);
        go_eyes();
      } else if (r == TIMER_BACK) {
        go_eyes();
      }
      return;
    }

    case Screen::REVIEW:
      if ((tap && review_tap_closes(now)) || !review_update(s_lcd, now)) go_eyes();
      return;

    default:
      go_eyes();
      return;
  }
}

void app_setup() {
  led_begin();
  light_begin();
  cam_uart_begin();
  cam_uart_on_msg(on_cam_msg);
  if (!store_load(s_persist)) Serial.println("store: nichts gespeichert, Startwerte");
  mood_init(s_mood, s_persist.needs);
  s_mood.sick = s_persist.sick;
  s_char = Character{s_persist.cuddly, s_persist.playful};
  if (s_persist.favorite_food == STORE_NO_FAVORITE) {   // erster Start: Lieblingsessen auslosen
    s_persist.favorite_food = (uint8_t)(esp_random() % (uint32_t)Food::COUNT);
    store_save(s_persist);
  }
  ota_pull::begin();
  net_task_hook(ota_pull::loop);   // OTA-Abruf läuft im net_task
  settings_firmware_source(firmware_line);
  s_lcd.fillScreen(TFT_BLACK);
  eyes_begin(s_lcd);   // Sprite vor dem WLAN anlegen, solange der Heap noch zusammenhängend ist
  net_begin(&s_persist);
  claude_api_begin(&s_claude);
  gesture_init(s_gesture);
  const uint32_t now = millis();
  const uint32_t seed = esp_random();
  blinker_init(s_blink, seed, now);
  gazer_init(s_gaze, seed ^ 0x9E3779B9u, now);
  mood_seed(s_mood, esp_random());
  speech_init(s_speech, now, esp_random());
  const FaceExpr start = face_expression(Emotion::CONTENT);
  s_face = start.s;
  s_color = start.color;

  status_invalidate();
  set_backlight(kBacklight[s_persist.brightness]);
  s_last_frame = s_last_save = s_last_log = now;
  Serial.printf("MadBuddy gestartet, Panel %s, heap=%u\n", LGFX::PANEL_NAME, (unsigned)ESP.getFreeHeap());
}

void app_loop() {
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
  if (cam_uart_take_restart()) cam_uart_send_ctrl(s_persist.cam_on);
  {   // XIAO kurz neu gestartet (Lücke unter 5 s) und Kamera-Zustand passt nicht zur Einstellung: C erneut, höchstens alle 5 s
    static uint32_t s_ctrl_t = 0;
    const CamLink &cl = cam_uart_link();
    const bool mismatch = cam_link_connected(cl, now) && cl.state != CAM_ST_ERR && (cl.state == CAM_ST_ON) != s_persist.cam_on;
    if (mismatch && !cl.upd && due_since(now, s_ctrl_t, 5000)) { s_ctrl_t = now; cam_uart_send_ctrl(s_persist.cam_on); }
  }
  apply_elapsed_once();

  int16_t tx, ty;
  const Gesture g = handle_touch(now, tx, ty);
  mood_frame(now, dt);
  if (g == Gesture::TAP && claude_needs_you(s_claude, now)) { s_needs_ack = true; s_needs_ack_since = s_claude.since; }   // Tippen stoppt das Blinken
  screen_frame(now, dt, g, tx, ty);
  if (s_blink_n) {   // LED-Signal (Pomodoro) überschreibt die Gefühlsfarbe: n× an/aus je 150 ms
    const uint32_t step = (now - s_blink_start) / kBlinkStepMs;
    if (step >= 2u * s_blink_n) s_blink_n = 0;
    else led_set((step & 1) ? Rgb{0, 0, 0} : s_blink_col, kLedPct);
  } else if (claude_needs_you(s_claude, now) && !(s_needs_ack && s_needs_ack_since == s_claude.since)) {
    led_set((now / 500) & 1 ? Rgb{0, 0, 0} : Rgb{255, 200, 0}, kLedPct);   // Claude braucht dich: gelb, 1 Hz, bis Tippen oder nächster Zustand
  }

  if (now - s_last_save >= kSaveEveryMs) {
    s_last_save = now;
    save_now();
  }
#if CYD_LOG_LDR   // Einmessen des Lichtsensors: Rohwert jede Sekunde (Build-Flag, danach wieder 0)
  static uint32_t s_last_ldr = 0;
  if (now - s_last_ldr >= 1000) { s_last_ldr = now; Serial.printf("ldr=%d\n", light_raw()); }
#endif
  if (now - s_last_log >= kLogEveryMs) {
    const uint32_t fps = s_frames * 1000UL / (now - s_last_log);
    s_last_log = now;
    s_frames = 0;
    Serial.printf("heap=%u min=%u fps=%u emo=%d\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)fps, (int)s_emotion);
  }
}
