#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "updater.h"
#include "link.h"
#include "cam_config.h"
#include "logic/ota_policy.h"
#include "logic/cam_xiao.h"

// WLAN-Update des XIAO (Spec: Updates). Ablauf wie src/ota/ota_pull.cpp am CYD, aber Auftrag (URL, SHA-256, WLAN) vom CYD
// über die U-Zeile. Die SHA-256 kommt aus dem Manifest, das der CYD per geprüftem TLS geladen hat. Sie legt den Inhalt fest.

namespace {
constexpr size_t kChunk = 4096;
constexpr uint32_t kStallMs = 15000;
constexpr uint32_t kWifiMs = 20000;
constexpr int kMaxRedirects = 3;
bool g_pending = false;   // läuft auf Probe (PENDING_VERIFY)

#if ESP_ARDUINO_VERSION_MAJOR >= 3
const char *project_name() { return esp_app_get_description()->project_name; }
#define SHA_START(c) mbedtls_sha256_starts(c, 0)
#define SHA_UPDATE(c, b, n) mbedtls_sha256_update(c, b, n)
#define SHA_FINISH(c, d) mbedtls_sha256_finish(c, d)
#else
const char *project_name() { return esp_ota_get_app_description()->project_name; }
#define SHA_START(c) mbedtls_sha256_starts_ret(c, 0)
#define SHA_UPDATE(c, b, n) mbedtls_sha256_update_ret(c, b, n)
#define SHA_FINISH(c, d) mbedtls_sha256_finish_ret(c, d)
#endif

void send_err(const char *why) {
  char b[64];
  const size_t n = cam_build_upd_err(why, b, sizeof b);
  link_send(b, n);
}
void send_pct(int pct) {
  char b[16];
  const size_t n = cam_build_upd_pct(pct, b, sizeof b);
  link_send(b, n);
}

void setup_tls(WiFiClientSecure &tls) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  tls.useBuiltinCACertBundle();
  tls.setHandshakeTimeout(6);   // Sekunden; Vorgabe 120 s würde den Herzschlag blockieren
#else
  tls.setInsecure();   // Kern 2.x ohne eingebautes Zertifikatsbündel: Der Inhalt ist über die SHA-256 festgelegt
#endif
}

// GET mit selbst verfolgten Umleitungen; jede Adresse muss ota_redirect_ok() bestehen.
bool open_follow(HTTPClient &http, WiFiClientSecure &tls, const char *url, int *code, char *why, size_t wn, void (*beat)()) {
  String cur = url;
  static const char *keys[] = {"Location"};
  for (int hop = 0; hop <= kMaxRedirects; hop++) {
    beat();   // ein Schritt (Verbinden + Antwort) dauert höchstens ~8 s, siehe Timeouts in download()
    if (!ota_redirect_ok(cur.c_str())) { strlcpy(why, "Umleitung abgelehnt", wn); return false; }
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (!http.begin(tls, cur)) { strlcpy(why, "URL", wn); return false; }
    http.collectHeaders(keys, 1);
    *code = http.GET();
    if (*code == 301 || *code == 302 || *code == 303 || *code == 307 || *code == 308) {
      cur = http.header("Location");
      http.end();
      continue;
    }
    if (*code <= 0) { strlcpy(why, "Verbindung", wn); http.end(); return false; }
    return true;
  }
  strlcpy(why, "zu viele Umleitungen", wn);
  return false;
}

void hex_of(const uint8_t *b, size_t n, char *out) {
  static const char k[] = "0123456789abcdef";
  for (size_t i = 0; i < n; i++) { out[2 * i] = k[b[i] >> 4]; out[2 * i + 1] = k[b[i] & 15]; }
  out[2 * n] = '\0';
}

void wifi_off() {
  WiFi.disconnect(true, true);   // WLAN aus und gespeicherte Station-Daten löschen
  WiFi.mode(WIFI_OFF);
}

// true = Image geschrieben und gesetzt; sonst why gefüllt
bool download(const CamMsg &cmd, void (*beat)(), char *why, size_t wn) {
  WiFiClientSecure tls;
  setup_tls(tls);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(4000);        // Antwort-Wartezeit je Schritt
  http.setConnectTimeout(4000);  // + 4 s = höchstens ~8 s ohne Herzschlag je Schritt
  int code = 0;
  if (!open_follow(http, tls, cmd.url, &code, why, wn, beat)) return false;
  if (code != 200) { snprintf(why, wn, "HTTP %d", code); http.end(); return false; }
  const int total = http.getSize();
  if (total < (int)OTA_IMG_CHECK_BYTES) { strlcpy(why, "Groesse unbekannt", wn); http.end(); return false; }

  uint8_t *buf = (uint8_t *)malloc(kChunk);
  if (!buf) { strlcpy(why, "kein Speicher", wn); http.end(); return false; }
  auto *s = http.getStreamPtr();
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  SHA_START(&sha);

  uint32_t done = 0, last_data = millis(), last_beat = millis();
  int last_pct = -1;
  bool started = false, fail = false;
  size_t have = 0;
  while (done < (uint32_t)total && !fail) {
    if (millis() - last_beat >= CAM_HEART_MS) { last_beat = millis(); beat(); }
    const int avail = s->available();
    if (avail <= 0) {
      if (!http.connected() && !s->available()) { strlcpy(why, "Verbindung weg", wn); fail = true; }
      else if (millis() - last_data > kStallMs) { strlcpy(why, "Zeitueberschreitung", wn); fail = true; }
      else delay(5);
      continue;
    }
    size_t want = kChunk - have;
    if (want > (size_t)avail) want = avail;
    if (want > (uint32_t)total - done - have) want = (uint32_t)total - done - have;
    const int r = s->read(buf + have, want);
    if (r <= 0) { delay(2); continue; }
    last_data = millis();
    have += r;
    if (!started) {
      if (have < OTA_IMG_CHECK_BYTES) continue;
      char elf[65];
      const OtaImg chk = ota_image_check(buf, have, project_name(), elf, OTA_CHIP_ESP32S3);
      if (chk != OTA_IMG_OK) { strlcpy(why, ota_image_error(chk), wn); fail = true; break; }
      if (!Update.begin(total, U_FLASH)) { strlcpy(why, "Update.begin", wn); fail = true; break; }
      started = true;
    }
    SHA_UPDATE(&sha, buf, have);
    if (Update.write(buf, have) != have) { strlcpy(why, "Schreiben", wn); fail = true; break; }
    done += have;
    have = 0;
    const int pct = ota_progress_pct(done, total);
    if (pct / 5 != last_pct / 5) { last_pct = pct; send_pct(pct); }
    delay(1);
  }
  http.end();
  free(buf);
  uint8_t digest[32];
  SHA_FINISH(&sha, digest);
  mbedtls_sha256_free(&sha);
  if (!fail && done != (uint32_t)total) { strlcpy(why, "unvollstaendig", wn); fail = true; }
  if (!fail) {
    char got[65];
    hex_of(digest, 32, got);
    if (strcmp(got, cmd.sha) != 0) { strlcpy(why, "SHA-256 falsch", wn); fail = true; }
  }
  if (fail) { if (started) Update.abort(); return false; }
  if (!Update.end(true)) { strlcpy(why, "Update.end", wn); return false; }
  return true;
}
}  // namespace

void updater_boot() {
  const esp_partition_t *run = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_UNDEFINED;
  if (run) esp_ota_get_state_partition(run, &st);
  g_pending = (st == ESP_OTA_IMG_PENDING_VERIFY);
  Serial.printf("# ota: Partition %s, Zustand %d\n", run ? run->label : "?", (int)st);
}

void updater_run(const CamMsg &cmd, void (*beat)()) {
  if (!ota_redirect_ok(cmd.url)) { send_err("URL abgelehnt"); return; }
  send_pct(0);
  WiFi.persistent(false);   // Zugangsdaten nie in den Flash (NVS) schreiben
  WiFi.mode(WIFI_STA);
  WiFi.begin(cmd.ssid, cmd.pass);
  const uint32_t t0 = millis();
  uint32_t last_beat = t0;
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < kWifiMs) {
    if (millis() - last_beat >= CAM_HEART_MS) { last_beat = millis(); beat(); }
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) { wifi_off(); send_err("kein WLAN"); return; }
  char why[40] = "";
  const bool ok = download(cmd, beat, why, sizeof why);
  wifi_off();
  if (!ok) { send_err(why[0] ? why : "Fehler"); return; }
  send_pct(100);
  Serial.println("# ota: ok, Neustart");
  delay(500);
  ESP.restart();
}

void updater_health_tick(uint32_t now, bool cam_ok, uint32_t beats) {
  if (!g_pending) return;
  switch (cam_health(now, cam_ok, beats)) {
    case CAM_HEALTH_VALID: {
      g_pending = false;
      esp_ota_mark_app_valid_cancel_rollback();
      char b[16];
      const size_t n = cam_build_upd_ok(b, sizeof b);
      link_send(b, n);
      Serial.println("# ota: neue Firmware bestaetigt");
      break;
    }
    case CAM_HEALTH_ROLLBACK:
      Serial.println("# ota: Probelauf gescheitert, Rollback");
      delay(200);
      esp_ota_mark_app_invalid_rollback_and_reboot();
      break;
    default:
      break;
  }
}
