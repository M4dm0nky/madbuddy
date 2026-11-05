#include "claude_api.h"
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>
#include <atomic>
#include "net.h"
#include "../hw/store.h"

static constexpr const char *kHost = "madbuddy";
static constexpr size_t kMaxBody = 256;
static constexpr size_t kKeyLen = 12;

// Der WebServer läuft in einem eigenen Task (core 3.x: _parseRequest kann pro Lesezugriff bis zu 5 s blockieren, das
// darf die Oberfläche nicht einfrieren). Die Handler fassen keinen UI-/Logik-Zustand an: Sie prüfen den Schlüssel und
// legen ein kleines Ereignis in eine Queue; claude_api_loop() (UI-Task) leert sie und ruft claude_event().
struct ApiEvent {
  uint32_t ms;
  char e[24], t[24], n[24], sid[12];
  bool interrupt;
};
static constexpr int kQueueLen = 8;
static constexpr uint32_t kTaskStack = 8192;

static WebServer s_srv(80);                 // nur vom Server-Task benutzt
static ClaudeLogic *s_logic = nullptr;      // nur vom UI-Task benutzt
static char s_key[kKeyLen + 1] = "";        // vor dem Task-Start geschrieben, danach nur gelesen
static bool s_mdns = false;                 // nur Server-Task
static QueueHandle_t s_queue = nullptr;
static SemaphoreHandle_t s_life = nullptr;  // schützt Start/Stop gegen die Freigabe-Prüfung des UI-Tasks
static std::atomic<bool> s_want{false};     // UI-Task: Server soll laufen
static std::atomic<bool> s_up{false};       // Server-Task: Port 80 ist belegt
static std::atomic<uint8_t> s_state_cache{0};   // Claude-Zustand für /claude/ping (vom UI-Task gepflegt)

static void make_key() {
  snprintf(s_key, sizeof s_key, "%08x%04x", (unsigned)esp_random(), (unsigned)(esp_random() & 0xFFFF));
}

// Laenge + Inhalt vergleichen, ohne beim ersten Unterschied abzubrechen.
static bool key_ok(const String &got) {
  if (got.length() != kKeyLen) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < kKeyLen; i++) diff |= (uint8_t)(got[i] ^ s_key[i]);
  return diff == 0;
}

// POST /claude als eigener RequestHandler mit "raw"-Modus des WebServers: Der Kern parst Anfragezeile und Header, ruft dann
// raw(RAW_START) auf, BEVOR irgendein Körperbyte gelesen wird (WebServer/Parsing.cpp: canRaw/raw-Zweig). Dort prüfen wir
// Schlüssel und Content-Length; bei Fehler antworten wir sofort (401/413) und schließen die Verbindung, so dass der Kern
// den Körper nie liest. Der erlaubte Körper (<= 256 B) landet in einem statischen Puffer, nicht auf dem Heap.
static char s_body[kMaxBody];
static size_t s_blen = 0;
static int s_rej = 0;            // 0 = angenommen, sonst bereits gesendeter Fehlercode
static bool s_raw_seen = false;  // raw() wurde aufgerufen (nicht bei multipart/form-data: dort liest der Kern den Körper vorab)
static bool s_raw_done = false;  // RAW_END erreicht, Körper vollständig

static void reject_early(WebServer &s, int code, const char *msg) {
  s_rej = code;
  s.setContentLength(CONTENT_LENGTH_NOT_SET);   // der Kern setzt das sonst erst NACH dem Parsen; beim ersten Aufruf stünde 0 darin
  s.send(code, "text/plain", msg);   // "Connection: close"
  s.client().clear();                // schon eingetroffene Bytes verwerfen (kein Heap)
  s.client().stop();                 // Kern bricht danach die Körperschleife ab (read() < 0 -> RAW_ABORTED)
}

class ClaudePostHandler : public RequestHandler {
 public:
  using RequestHandler::canHandle;
  bool canHandle(WebServer &, HTTPMethod m, const String &uri) override {
    s_blen = 0; s_rej = 0; s_raw_seen = false; s_raw_done = false;   // Beginn einer neuen Anfrage
    return m == HTTP_POST && uri == "/claude";
  }
  using RequestHandler::canRaw;
  bool canRaw(WebServer &, const String &uri) override { return uri == "/claude"; }
  void raw(WebServer &s, const String &, HTTPRaw &r) override {
    switch (r.status) {
      case RAW_START: {
        s_raw_seen = true;
        const int len = s.clientContentLength();
        if (!key_ok(s.header("X-Buddy-Key"))) reject_early(s, 401, "unauthorized");
        else if (len < 0 || len > (int)kMaxBody) reject_early(s, 413, "too large");
        break;
      }
      case RAW_WRITE:
        if (s_rej == 0 && s_blen + r.currentSize <= kMaxBody) { memcpy(s_body + s_blen, r.buf, r.currentSize); s_blen += r.currentSize; }
        break;
      case RAW_END: s_raw_done = (s_rej == 0); break;
      default: break;
    }
  }
  using RequestHandler::handle;
  bool handle(WebServer &s, HTTPMethod, const String &) override {
    if (s_rej) return true;   // schon beantwortet, Verbindung zu
    if (!s_raw_seen) {        // multipart/form-data: der Kern hat den Körper schon gelesen; Schlüssel trotzdem prüfen, Inhalt nicht annehmen
      if (!key_ok(s.header("X-Buddy-Key"))) s.send(401, "text/plain", "unauthorized");
      else s.send(415, "text/plain", "unsupported content type");
      return true;
    }
    if (!s_raw_done || s_blen == 0) { s.send(400, "text/plain", "no json body"); return true; }
    JsonDocument doc;
    if (deserializeJson(doc, s_body, s_blen) || !doc.is<JsonObject>()) { s.send(400, "text/plain", "bad json"); return true; }
    const char *e = doc["e"] | "";
    const char *t = doc["t"] | "";
    const char *n = doc["n"] | "";
    const char *sid = doc["sid"] | "";
    ApiEvent ev{};
    ev.ms = millis();
    ev.interrupt = doc["i"].as<int>() != 0;
    strlcpy(ev.e, e, sizeof ev.e);
    strlcpy(ev.t, t, sizeof ev.t);
    strlcpy(ev.n, n, sizeof ev.n);
    strlcpy(ev.sid, sid, sizeof ev.sid);
    if (s_queue) xQueueSend(s_queue, &ev, 0);   // voll -> verwerfen
    s.send(204);
    return true;
  }
};
static ClaudePostHandler s_post_handler;

static void on_ping() {
  if (!key_ok(s_srv.header("X-Buddy-Key"))) { s_srv.send(401, "text/plain", "unauthorized"); return; }
  char buf[64];
  snprintf(buf, sizeof buf, "{\"ok\":true,\"state\":\"%s\"}", claude_state_name((ClaudeState)s_state_cache.load()));
  s_srv.send(200, "application/json", buf);
}

static void start_server() {
  // Handler und Header-Liste nur beim ersten Start: collectHeaders() hängt bei jedem Aufruf an (_headerKeysCount += n),
  // on()/addHandler() hängen Handler an; stop()/begin() lässt beides stehen.
  static bool registered = false;
  if (!registered) {
    static const char *kHeaders[] = {"X-Buddy-Key"};
    s_srv.collectHeaders(kHeaders, 1);
    s_srv.addHandler(&s_post_handler);
    s_srv.on("/claude/ping", HTTP_GET, on_ping);
    registered = true;
  }
  s_srv.begin();
  s_mdns = MDNS.begin(kHost);
  if (s_mdns) MDNS.addService("http", "tcp", 80);
  s_up = true;
  Serial.printf("claude: Empfaenger offen (%s.local, Port 80, mDNS %s)\n", kHost, s_mdns ? "ok" : "Fehler");
}

static void stop_server() {
  s_srv.stop();
  if (s_mdns) MDNS.end();
  s_mdns = false;
  s_up = false;
}

static void server_task(void *) {
  for (;;) {
    const bool want = s_want.load();
    if (want != s_up.load()) {
      xSemaphoreTake(s_life, portMAX_DELAY);
      if (s_want.load() && !s_up.load()) start_server();   // erneut prüfen: der UI-Task kann inzwischen widerrufen haben
      else if (!s_want.load() && s_up.load()) stop_server();
      xSemaphoreGive(s_life);
    }
    if (s_up.load()) s_srv.handleClient();   // darf bis zu ~5 s blockieren, stört nur diesen Task
    vTaskDelay(pdMS_TO_TICKS(s_up.load() ? 5 : 100));
  }
}

void claude_api_stop() { s_want = false; }

// Port 80 frei? Erst wahr, wenn der Server-Task den Server wirklich gestoppt hat (und keinen Start mehr vorhat).
bool claude_api_released() {
  if (s_want.load()) return false;
  if (!s_life) return true;
  if (xSemaphoreTake(s_life, 0) != pdTRUE) return false;   // Task steckt gerade in Start/Stop
  const bool r = !s_up.load();
  xSemaphoreGive(s_life);
  return r;
}

void claude_api_begin(ClaudeLogic *logic) {
  s_logic = logic;
  if (!store_load_claude_key(s_key)) {
    make_key();
    store_save_claude_key(s_key);
  }
  // Schlüssel steht in den Einstellungen (Seite 1, Info), nicht im seriellen Log.
  s_queue = xQueueCreate(kQueueLen, sizeof(ApiEvent));
  s_life = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(server_task, "claude_srv", kTaskStack, nullptr, 1, nullptr, 0);   // Kern 0 wie net_task
}

void claude_api_loop() {
  if (!s_logic) return;
  s_want = net_wifi_ok() && !net_portal_busy();   // das WLAN-Portal braucht Port 80
  ApiEvent ev;
  while (s_queue && xQueueReceive(s_queue, &ev, 0) == pdTRUE)
    claude_event(*s_logic, ev.e, ev.t, ev.n, ev.sid, ev.interrupt, ev.ms);
  s_state_cache = (uint8_t)claude_state(*s_logic, millis());
}

const char *claude_api_key() { return s_key; }

ClaudeState claude_api_state(uint32_t now_ms) { return s_logic ? claude_state(*s_logic, now_ms) : ClaudeState::OFF; }
