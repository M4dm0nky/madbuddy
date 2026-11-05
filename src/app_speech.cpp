// Sprechblasen, Rituale und Claude-Blasen.
#include "app_internal.h"
#include <Arduino.h>
#include "logic/daystats.h"
#include "ui/eyes.h"

static constexpr uint32_t kBubbleMs = 4000;
static constexpr uint32_t kMorningWeatherDelayMs = 4000;
static constexpr uint32_t kHeyEveryMs = 30000;

static SpeechState s_speech;
static char s_bubble[64] = "";
static Deadline s_bubble_dl;
static char s_pending[64] = "";        // zweite Blase (Morgen: Tageswetter)
static Wait s_pending_at;
static bool s_pending_on = false;      // zweite Blase wartet auf ihre Zeit
static bool s_prev_sleeping = false;
static uint32_t s_seen_weather_ms = 0; // fetched_ms des zuletzt ausgewerteten Wetters
static bool s_sky_known = false;
static Sky s_last_sky = Sky::CLOUDY;
static ClaudeState s_prev_cs = ClaudeState::OFF;
static Wait s_next_hmm, s_next_hey;   // Abstand seit dem Setzen (millis()-Überlauf-sicher)

void app_speech_init(uint32_t now, uint32_t seed) { speech_init(s_speech, now, seed); }

bool speech_may_say(uint32_t now) { return speech_may(s_speech, now); }

const char *speech_bubble(uint32_t now) { return s_bubble_dl.active(now) ? s_bubble : nullptr; }

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
void say(Say what, uint32_t now) {
  const bool pomo_say = what == Say::POMODORO_START || what == Say::POMODORO_PAUSE || what == Say::POMODORO_FERTIG;
  if (g_app.claude_busy && !pomo_say) return;                  // Claude arbeitet: keine Pet-Blasen
  if (focused_work() && what != Say::POMODORO_START) return;   // während der Arbeit still (außer beim Start)
  show_bubble(speech_pick(s_speech, what, esp_random()), now);
}

static uint32_t hmm_interval_ms() { return 20000 + esp_random() % 20001; }   // „Hmm...“ alle 20-40 s

// Claude-Blasen bei Zustandswechseln (Spec-Tabelle); läuft jedes Bild.
void claude_frame(ClaudeState cs, uint32_t now) {
  if (cs != s_prev_cs) {
    const bool was_calm = s_prev_cs == ClaudeState::OFF || s_prev_cs == ClaudeState::IDLE || s_prev_cs == ClaudeState::DONE;
    if (was_calm && claude_busy(cs) && cs != ClaudeState::DONE) s_next_hmm.start(now, hmm_interval_ms());
    switch (cs) {
      case ClaudeState::LISTENING: claude_say(Say::CLAUDE_OKAY, now); break;
      case ClaudeState::FAILED: claude_say(Say::CLAUDE_FEHLER, now); break;
      case ClaudeState::NEEDS_YOU:
        claude_say(Say::CLAUDE_HEY, now, true);
        s_next_hey.start(now, kHeyEveryMs);
        break;
      case ClaudeState::DONE:
        claude_say(Say::CLAUDE_FERTIG, now);
        eyes_set_hand(true, now);
        break;
      default: break;
    }
    s_prev_cs = cs;
  }
  if (cs == ClaudeState::THINKING && s_next_hmm.due(now)) {   // alle 20-40 s ein „Hmm...“
    claude_say(Say::CLAUDE_HMM, now);
    s_next_hmm.start(now, hmm_interval_ms());
  }
  if (cs == ClaudeState::NEEDS_YOU && s_next_hey.due(now)) {   // „Hey, du!“ alle 30 s
    claude_say(Say::CLAUDE_HEY, now, true);
    s_next_hey.start(now, kHeyEveryMs);
  }
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
void speech_frame(uint32_t now, bool sleeping) {
  App &a = g_app;
  const bool woke = (s_prev_sleeping && !sleeping) || a.touch_began;
  s_prev_sleeping = sleeping;
  if (sleeping) { s_bubble_dl.clear(); s_pending_on = false; return; }
  struct tm t;
  const uint32_t day = today(&t);
  daystats_roll(a.persist.day, day);
  if (focused_work() || a.claude_busy) return;   // keine Rituale, Gedanken, Hatschi-Blasen während der Arbeit bzw. Claude-Modus
  const int hour = day ? t.tm_hour : -1, minute = day ? t.tm_min : 0;
  if (ritual_morning(a.rituals, woke, hour, day)) {
    mood_set_transient(a.mood, Emotion::GREETING, now, 3000);
    say(Say::MORGEN, now);
    WeatherMood wm;
    NetWeather w;
    if (fresh_weather(now, &wm, &w)) {
      weather_sentence(w.temp_c, wm.sky, s_pending, sizeof s_pending);
      s_pending_at.start(now, kMorningWeatherDelayMs);
      s_pending_on = true;
    }
  } else if (a.pres.greet) {   // Kamera: jemand ist wieder da (Morgen hat Vorrang)
    mood_set_transient(a.mood, Emotion::GREETING, now, 3000);
    say(Say::WIEDER, now);
  }
  if (a.wave_hello && !a.pres.greet) {   // zurückwinken (nur auf dem Augen-Bildschirm, wie die Begrüßung)
    mood_set_transient(a.mood, Emotion::GREETING, now, 3000);
    say(Say::WINKEN, now);
  }
  a.pres.greet = false;
  if (ritual_night(a.rituals, true, hour, minute, day)) {
    mood_set_transient(a.mood, Emotion::YAWN, now, 3000);
    say(Say::NACHT, now);
  }
  if (s_pending_on && s_pending_at.due(now)) {
    s_pending_on = false;
    show_bubble(s_pending, now);
  }
  weather_say(now);
  Say what;
  if (speech_due(s_speech, mood_base_emotion(a.mood, hour), now, &what, a.chr.cuddly)) say(what, now);
}
