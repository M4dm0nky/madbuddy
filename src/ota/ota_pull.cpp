#include "ota_pull.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_app_desc.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "../net/net.h"
#include "../logic/ota_policy.h"

// Ablage: neuestes GitHub-Release des öffentlichen Repos. Die .bin enthält keine Geheimnisse (WLAN-Zugang kommt aus dem
// Portal, nicht aus dem Code), darum ohne Token. Geladen wird nur über HTTPS mit dem Zertifikatspaket des Cores; Umleitungen
// verfolgt open_follow() selbst und lässt nur github.com und *.githubusercontent.com zu (ota_redirect_ok).

namespace ota_pull {
namespace {
// Projektname im Image: kommt vom Boardpaket, nicht vom Sketchnamen -- darum der Name der laufenden Firmware als Soll.
const char *project_name() { return esp_app_get_description()->project_name; }
constexpr uint32_t kFirstPollMs = 90UL * 1000UL;
constexpr uint32_t kPollMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kRetryMs = 3UL * 60UL * 1000UL;
constexpr const char *kBase = "https://github.com/M4dm0nky/madbuddy/releases/latest/download";
constexpr const char *kManifest = "manifest.json";
constexpr uint32_t kMinSize = 200UL * 1024UL;
constexpr uint32_t kSlotSize = 1966080UL;    // App-Slot der Partition min_spiffs
constexpr int kMaxRedirects = 3;
constexpr size_t kChunk = 4096;
constexpr uint32_t kStallMs = 15000;
constexpr uint32_t kHealthAfterMs = 20000;   // so lange muss die neue Firmware laufen, bevor sie als gut gilt

portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
Info g_info = {};
volatile bool g_consent = false;
volatile bool g_check_now = false, g_fetching = false;   // Suche angefordert (Status-Seite) / läuft gerade
uint32_t g_check_req_ms = 0;
volatile bool g_ui_alive = false;
volatile bool g_marked = false;   // Gesundheitsnachweis erledigt bzw. nicht nötig (nur health_tick() setzt es nach begin())
uint32_t g_next_poll = 0;
char g_running_elf[65] = "";
char g_slot[17] = "?", g_build[24] = "";
int g_img_state = -1;
char g_man_elf[65] = "", g_man_sha[65] = "", g_man_file[48] = "";
uint32_t g_man_size = 0;
CamInfo g_cam = {};   // geschützt durch g_mux (net_task schreibt, UI liest)

void set_state(State s) { portENTER_CRITICAL(&g_mux); g_info.state = s; portEXIT_CRITICAL(&g_mux); }
void set_error(const char *e) {
  portENTER_CRITICAL(&g_mux);
  strlcpy(g_info.error, e, sizeof g_info.error);
  g_info.state = FAILED;
  portEXIT_CRITICAL(&g_mux);
  Serial.printf("ota: fehlgeschlagen: %s\n", e);
}

void hex_of(const uint8_t *b, size_t n, char *out) {
  static const char k[] = "0123456789abcdef";
  for (size_t i = 0; i < n; i++) { out[2 * i] = k[b[i] >> 4]; out[2 * i + 1] = k[b[i] & 15]; }
  out[2 * n] = '\0';
}

// Dateiname aus dem Manifest: nur Buchstaben, Ziffern, Punkt, Minus, Unterstrich (kein Pfad, keine Umlenkung der URL)
bool file_name_ok(const char *f) {
  size_t n = f ? strlen(f) : 0;
  if (n == 0 || n >= sizeof g_man_file || f[0] == '.') return false;
  for (size_t i = 0; i < n; i++) {
    const char c = f[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return false;
  }
  return true;
}

void make_url(const char *file, char *out, size_t n) { snprintf(out, n, "%s/%s", kBase, file); }

// GET mit selbst verfolgten Umleitungen (höchstens 3 Sprünge). Jede Adresse, auch die erste, muss ota_redirect_ok()
// bestehen. true = Antwort liegt an (*code), http ist offen und muss mit http.end() geschlossen werden.
bool open_follow(HTTPClient &http, NetworkClientSecure &tls, const char *url, int *code, char *reason, size_t rn) {
  String cur = url;
  static const char *keys[] = {"Location"};
  for (int hop = 0; hop <= kMaxRedirects; hop++) {
    if (!ota_redirect_ok(cur.c_str())) { strlcpy(reason, "Umleitung abgelehnt", rn); return false; }
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (!http.begin(tls, cur)) { strlcpy(reason, "URL", rn); return false; }
    http.collectHeaders(keys, 1);
    *code = http.GET();
    if (*code == 301 || *code == 302 || *code == 303 || *code == 307 || *code == 308) {
      cur = http.header("Location");
      http.end();
      continue;
    }
    if (*code <= 0) { snprintf(reason, rn, "Verbindung: %.26s", HTTPClient::errorToString(*code).c_str()); http.end(); return false; }
    return true;
  }
  strlcpy(reason, "zu viele Umleitungen", rn);
  return false;
}

void record_check(bool ok) {
  portENTER_CRITICAL(&g_mux);
  g_info.last_check_ms = millis() ? millis() : 1;
  g_info.last_failed = !ok;
  portEXIT_CRITICAL(&g_mux);
}

// true = Manifest gelesen; *newer sagt, ob es etwas Neues gibt
bool fetch_manifest(bool *newer) {
  *newer = false;
  char url[200];
  make_url(kManifest, url, sizeof url);
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(8000);
  http.setConnectTimeout(5000);
  int code = 0;
  char reason[40] = "";
  if (!open_follow(http, tls, url, &code, reason, sizeof reason)) { Serial.printf("ota: Manifest: %s\n", reason); return false; }
  if (code != 200) { Serial.printf("ota: Manifest HTTP %d\n", code); http.end(); return false; }
  // Standard-Allocator (kein PSRAM), Filter hält das Dokument klein
  JsonDocument filter;
  for (const char *k : {"project", "elf", "sha256", "size", "file", "built"}) filter[k] = true;
  for (const char *k : {"version", "url", "sha256"}) filter["cam"][k] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { Serial.printf("ota: Manifest-JSON %s\n", err.c_str()); return false; }
  const char *proj = doc["project"] | "";
  const char *elf = doc["elf"] | "";
  const char *sha = doc["sha256"] | "";
  const char *file = doc["file"] | "";
  const uint32_t size = doc["size"] | 0u;
  if (strcmp(proj, project_name()) != 0 || !ota_hex_ok(elf, 64) || !ota_hex_ok(sha, 64) || !file_name_ok(file) || size < kMinSize) {
    Serial.println("ota: Manifest unbrauchbar");
    return false;
  }
  if (size > kSlotSize) {
    Serial.println("ota: Manifest: zu gross fuer den Slot");
    return false;
  }
  strlcpy(g_man_elf, elf, sizeof g_man_elf);
  strlcpy(g_man_sha, sha, sizeof g_man_sha);
  strlcpy(g_man_file, file, sizeof g_man_file);
  g_man_size = size;
  {   // Kamera-Eintrag (optional): nur übernehmen, wenn alles stimmt; sonst kein Angebot
    const char *cv = doc["cam"]["version"] | "";
    const char *cu = doc["cam"]["url"] | "";
    const char *cs = doc["cam"]["sha256"] | "";
    CamInfo ci = {};
    if (cv[0] && strlen(cv) <= 15 && ota_redirect_ok(cu) && strlen(cu) < sizeof ci.url && ota_hex_ok(cs, 64)) {
      ci.avail = true;
      strlcpy(ci.version, cv, sizeof ci.version);
      strlcpy(ci.url, cu, sizeof ci.url);
      strlcpy(ci.sha, cs, sizeof ci.sha);
    }
    portENTER_CRITICAL(&g_mux); g_cam = ci; portEXIT_CRITICAL(&g_mux);
  }
  *newer = ota_needs_update(g_running_elf, g_man_elf);
  if (*newer) {
    portENTER_CRITICAL(&g_mux);
    strlcpy(g_info.built, doc["built"] | "", sizeof g_info.built);
    g_info.size = size;
    g_info.pct = 0;
    g_info.error[0] = '\0';
    g_info.state = AVAILABLE;
    portEXIT_CRITICAL(&g_mux);
    Serial.printf("ota: Update bereit %.20s\n", (const char *)(doc["built"] | ""));
  }
  return true;
}

// Blockiert bis zum Ende. Bei Erfolg Neustart, bei Fehler FAILED.
void download() {
  char url[200];
  make_url(g_man_file, url, sizeof url);
  set_state(DOWNLOADING);
  portENTER_CRITICAL(&g_mux); g_info.pct = 0; portEXIT_CRITICAL(&g_mux);

  if (g_man_size > kSlotSize) { set_error("zu gross fuer den Slot"); return; }
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setConnectTimeout(6000);
  int code = 0;
  char why[40] = "";
  if (!open_follow(http, tls, url, &code, why, sizeof why)) { set_error(why); return; }
  if (code != 200) {
    char e[40]; snprintf(e, sizeof e, "HTTP %d", code);
    http.end(); set_error(e); return;
  }
  if ((uint32_t)http.getSize() != g_man_size) { http.end(); set_error("Groesse passt nicht"); return; }

  uint8_t *buf = (uint8_t *)malloc(kChunk);   // kein PSRAM
  if (!buf) { http.end(); set_error("kein Speicher"); return; }
  NetworkClient *s = http.getStreamPtr();

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);

  uint32_t done = 0, last_data = millis();
  bool started = false, fail = false;
  char reason[40] = "";
  size_t have = 0;   // Bytes im Puffer, die noch nicht geschrieben sind (nur vor dem ersten Schreiben nötig)
  while (done < g_man_size && !fail) {
    const int avail = s->available();
    if (avail <= 0) {
      if (!http.connected() && !s->available()) { strlcpy(reason, "Verbindung weg", sizeof reason); fail = true; }
      else if (millis() - last_data > kStallMs) { strlcpy(reason, "Zeitueberschreitung", sizeof reason); fail = true; }
      else vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    size_t want = kChunk - have;
    if (want > (size_t)avail) want = avail;
    if (want > g_man_size - done) want = g_man_size - done;
    const int r = s->read(buf + have, want);
    if (r <= 0) {   // Daten gemeldet, aber nichts gelesen: auch hier die Stall-Zeit überwachen
      if (millis() - last_data > kStallMs) { strlcpy(reason, "Zeitueberschreitung", sizeof reason); fail = true; }
      else vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    last_data = millis();
    have += r;
    if (!started) {
      if (have < OTA_IMG_CHECK_BYTES) continue;   // erst prüfen, wenn der Anfang da ist
      char elf[65];
      const OtaImg chk = ota_image_check(buf, have, project_name(), elf);
      if (chk != OTA_IMG_OK) { strlcpy(reason, ota_image_error(chk), sizeof reason); fail = true; break; }
      if (strcmp(elf, g_man_elf) != 0) { strlcpy(reason, "Image passt nicht zum Manifest", sizeof reason); fail = true; break; }
      if (!Update.begin(g_man_size, U_FLASH)) { snprintf(reason, sizeof reason, "Update.begin: %.24s", Update.errorString()); fail = true; break; }
      started = true;
    }
    mbedtls_sha256_update(&sha, buf, have);
    if (Update.write(buf, have) != have) { snprintf(reason, sizeof reason, "Schreiben: %.24s", Update.errorString()); fail = true; break; }
    done += have;
    have = 0;
    portENTER_CRITICAL(&g_mux); g_info.pct = (uint8_t)ota_progress_pct(done, g_man_size); portEXIT_CRITICAL(&g_mux);
    vTaskDelay(1);   // net_task läuft mit Priorität 1 (knapp über Idle): dem Idle-Task Zeit lassen (Watchdog)
  }
  http.end();
  free(buf);

  uint8_t digest[32];
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);
  if (!fail && done != g_man_size) { strlcpy(reason, "unvollstaendig", sizeof reason); fail = true; }
  if (!fail) {
    set_state(VERIFYING);
    char got[65];
    hex_of(digest, 32, got);
    if (strcmp(got, g_man_sha) != 0) { strlcpy(reason, "SHA-256 falsch", sizeof reason); fail = true; }
  }
  if (fail) {
    if (started) Update.abort();
    set_error(reason[0] ? reason : "Fehler");
    return;
  }
  if (!Update.end(true)) {
    char e[40]; snprintf(e, sizeof e, "Update.end: %.24s", Update.errorString());
    set_error(e);
    return;
  }
  set_state(RESTARTING);
  Serial.println("ota: ok, Neustart in 2 s");
  vTaskDelay(pdMS_TO_TICKS(2000));   // in dieser Zeit sichert app_loop() den Zustand, sobald es RESTARTING sieht
  ESP.restart();
}
}  // namespace

void begin() {
  const esp_app_desc_t *d = esp_app_get_description();
  hex_of(d->app_elf_sha256, 32, g_running_elf);
  g_next_poll = kFirstPollMs;
  const esp_partition_t *run = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_UNDEFINED;
  if (run) esp_ota_get_state_partition(run, &st);
  if (run) snprintf(g_slot, sizeof g_slot, "%s", run->label);
  g_img_state = (int)st;
  snprintf(g_build, sizeof g_build, "%.11s %.11s", d->date, d->time);
  Serial.printf("ota: Partition %s, Zustand %d, ELF %.12s\n", run ? run->label : "?", (int)st, g_running_elf);
  // Bleibt der Zustand PENDING_VERIFY (main.cpp: verifyRollbackLater() = true, der Core markiert also nicht selbst), gilt das
  // Image erst nach dem Gesundheitsnachweis in health_tick(). Nach einem Kabel-Flash ist der Zustand UNDEFINED/VALID: nichts zu tun.
  g_marked = (st != ESP_OTA_IMG_PENDING_VERIFY);
}

// Gesundheitsnachweis nach einem Update: Augen-Schleife läuft (ui_alive) und Mindestlaufzeit. Unabhängig vom WLAN, im Haupt-Task.
// Sonst bootet der Bootloader beim nächsten Reset zur vorigen Firmware zurück.
void health_tick() {
  if (g_marked || !g_ui_alive || millis() <= kHealthAfterMs) return;
  g_marked = true;   // nur einmal versuchen
  if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) Serial.println("ota: neue Firmware als gut bestaetigt");
  else Serial.println("ota: Bestaetigung der neuen Firmware fehlgeschlagen");
}

void loop() {
  const uint32_t now = millis();

  const State st = info().state;
  if (st == AVAILABLE && g_consent) {
    g_consent = false;
    // Zustimmung gilt der Firmware, die auf dem Display stand; seit dem Manifest kann eine neuere veröffentlicht worden sein -> frisch lesen
    bool newer = false;
    if (!fetch_manifest(&newer)) { set_error("Manifest nicht lesbar"); return; }
    if (!newer) { set_error("Update nicht mehr noetig"); return; }
    download();
    g_next_poll = millis() + kPollMs;
    return;
  }
  g_consent = false;
  if (st == DOWNLOADING || st == VERIFYING || st == RESTARTING) return;
  if (g_check_now) {   // Status-Seite: "jetzt suchen"; hebt Schlummern und eine alte Fehlermeldung auf
    g_check_now = false;
    if (!net_wifi_ok()) { record_check(false); return; }
    portENTER_CRITICAL(&g_mux);
    g_info.snooze_set_ms = 0;
    if (g_info.state == FAILED) { g_info.state = IDLE; g_info.error[0] = '\0'; }
    portEXIT_CRITICAL(&g_mux);
    g_fetching = true;
    bool newer = false;
    const bool ok = fetch_manifest(&newer);
    if (ok && !newer && info().state == AVAILABLE) set_state(IDLE);   // inzwischen zurückgenommen/gleich der laufenden Firmware
    g_fetching = false;
    record_check(ok);
    g_next_poll = millis() + (ok ? kPollMs : kRetryMs);
    return;
  }
  if (st == AVAILABLE) return;   // wartet auf Zustimmung; kein erneutes Abfragen nötig
  if ((int32_t)(now - g_next_poll) < 0) return;
  if (!net_wifi_ok()) return;
  g_fetching = true;
  bool newer = false;
  const bool ok = fetch_manifest(&newer);
  g_fetching = false;
  record_check(ok);
  g_next_poll = millis() + (ok ? kPollMs : kRetryMs);
}

Info info() {
  Info i;
  portENTER_CRITICAL(&g_mux);
  i = g_info;
  portEXIT_CRITICAL(&g_mux);
  return i;
}

CamInfo cam_info() {
  CamInfo c;
  portENTER_CRITICAL(&g_mux);
  c = g_cam;
  portEXIT_CRITICAL(&g_mux);
  return c;
}

void consent() {
  if (info().state == AVAILABLE) g_consent = true;
}

void check_now() {
  const State st = info().state;
  if (st == DOWNLOADING || st == VERIFYING || st == RESTARTING) return;
  g_check_req_ms = millis();
  g_check_now = true;
}

bool check_busy() { return g_fetching || (g_check_now && millis() - g_check_req_ms < 60000UL); }

void snooze() {
  portENTER_CRITICAL(&g_mux);
  g_info.snooze_set_ms = millis();
  if (!g_info.snooze_set_ms) g_info.snooze_set_ms = 1;
  portEXIT_CRITICAL(&g_mux);
}

void dismiss_failure() {
  portENTER_CRITICAL(&g_mux);
  if (g_info.state == FAILED) { g_info.state = IDLE; g_info.error[0] = '\0'; }
  portEXIT_CRITICAL(&g_mux);
  g_next_poll = millis() + kRetryMs;
}

void ui_alive() { g_ui_alive = true; }
const char *slot() { return g_slot; }
int image_state() { return g_img_state; }
const char *elf12() { static char e[13]; memcpy(e, g_running_elf, 12); e[12] = '\0'; return e; }
const char *build() { return g_build; }
}  // namespace ota_pull
