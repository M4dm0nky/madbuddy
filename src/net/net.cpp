#include "net.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>
#include <ctype.h>
#include "../logic/cam_proto.h"
#include "../logic/secure_zero.h"
#include "../logic/ota_policy.h"
#include "claude_api.h"

static constexpr const char *kApName = "MadBuddy";
static constexpr const char *kTz = "CET-1CEST,M3.5.0,M10.5.0/3";
static constexpr uint32_t kConnectWaitMs = 30000;          // so lange auf gespeichertes WLAN warten, dann Portal
static constexpr uint32_t kManualPortalMs = 5UL * 60000UL; // von Hand geöffnetes Portal schließt danach wieder
static constexpr uint32_t kWeatherEveryMs = 30UL * 60000UL;
static constexpr uint32_t kRetryMs = 5UL * 60000UL;
static constexpr uint32_t kHttpTimeoutMs = 10000;

static WiFiManager s_wm;
static WiFiManagerParameter s_param_place("place", "Ort", "", 47);
static Persist *s_p = nullptr;
static uint32_t s_offline_since = 0;    // seit wann ohne WLAN
static bool s_portal_auto = false;
static uint32_t s_portal_manual_since = 0;
static bool s_place_changed = false;
static volatile bool s_wifi_ok = false;
static volatile bool s_portal = false;
static int s_portal_pending = 0;        // 1 = automatisches, 2 = manuelles Portal wartet auf freien Port 80 (nur UI-Task)
static char s_portal_pass[9] = "";      // vor startConfigPortal geschrieben, nur UI-Task

// Zwischen Haupt-Task und net_task geteilt, geschützt durch s_mux.
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static char s_geo_req[48];          // Ort, der aufgelöst werden soll ("" = nichts)
static bool s_geo_done = false;     // Ergebnis liegt vor (für s_geo_req)
static char s_geo_name[48];
static float s_geo_lat = 0, s_geo_lon = 0;
static bool s_have_coords = false;
static float s_lat = 0, s_lon = 0;
static NetWeather s_weather{};
static bool s_weather_reset = false; // Ort geändert: Wetter sofort neu holen
static void (*s_hook)() = nullptr;

static void set_task_place(const Persist &p) {
  portENTER_CRITICAL(&s_mux);
  if (p.has_place) {
    s_have_coords = true;
    s_lat = p.lat;
    s_lon = p.lon;
    s_geo_req[0] = '\0';
  } else {
    s_have_coords = false;
    strncpy(s_geo_req, p.place, sizeof s_geo_req - 1);
    s_geo_req[sizeof s_geo_req - 1] = '\0';
  }
  s_geo_done = false;
  s_weather_reset = true;
  portEXIT_CRITICAL(&s_mux);
}

static void url_encode(const char *in, char *out, size_t n) {
  static const char kHex[] = "0123456789ABCDEF";
  size_t o = 0;
  for (const unsigned char *c = (const unsigned char *)in; *c && o + 4 < n; c++) {
    if (isalnum(*c) || *c == '-' || *c == '_' || *c == '.' || *c == '~') out[o++] = (char)*c;
    else { out[o++] = '%'; out[o++] = kHex[*c >> 4]; out[o++] = kHex[*c & 15]; }
  }
  out[o] = '\0';
}

// Ort -> Koordinaten über HTTPS (Zertifikatspaket des Cores).
static bool geocode(const char *place, char *name, float *lat, float *lon) {
  char enc[160], url[256];
  url_encode(place, enc, sizeof enc);
  snprintf(url, sizeof url, "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=de", enc);
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
  HTTPClient http;
  http.useHTTP10(true);   // sonst antwortet Open-Meteo chunked und getStream() liefert die Chunk-Längen mit an ArduinoJson
  http.setTimeout(kHttpTimeoutMs);
  if (!http.begin(tls, url)) return false;
  const int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    JsonDocument filter;
    filter["results"][0]["latitude"] = true;
    filter["results"][0]["longitude"] = true;
    filter["results"][0]["name"] = true;
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    JsonVariant r = doc["results"][0];
    if (!err && r["latitude"].is<float>() && r["longitude"].is<float>()) {
      *lat = r["latitude"].as<float>();
      *lon = r["longitude"].as<float>();
      strncpy(name, r["name"] | place, 47);
      name[47] = '\0';
      ok = true;
    } else {
      Serial.printf("net: Ort \"%s\" nicht gefunden (%s)\n", place, err.c_str());
    }
  } else {
    Serial.printf("net: Geocoding HTTP %d\n", code);
  }
  http.end();
  return ok;
}

// Aktuelles Wetter über HTTP (keine Geheimnisse, spart Speicher).
static bool fetch_weather(float lat, float lon, NetWeather *w) {
  char url[224];
  snprintf(url, sizeof url,
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,weather_code,is_day&timezone=Europe%%2FBerlin",
           lat, lon);
  WiFiClient tcp;
  HTTPClient http;
  http.useHTTP10(true);   // wie beim Geocoding: keine chunked-Antwort
  http.setTimeout(kHttpTimeoutMs);
  if (!http.begin(tcp, url)) return false;
  const int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    JsonDocument filter;
    filter["current"]["temperature_2m"] = true;
    filter["current"]["weather_code"] = true;
    filter["current"]["is_day"] = true;
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    JsonVariant c = doc["current"];
    if (!err && c["temperature_2m"].is<float>() && c["weather_code"].is<int>()) {
      w->valid = true;
      w->temp_c = c["temperature_2m"].as<float>();
      w->code = c["weather_code"].as<int>();
      w->is_day = (c["is_day"] | 1) != 0;
      w->fetched_ms = millis();
      if (w->fetched_ms == 0) w->fetched_ms = 1;   // 0 heißt "nie"
      ok = true;
    } else {
      Serial.printf("net: Wetter-JSON unbrauchbar (%s)\n", err.c_str());
    }
  } else {
    Serial.printf("net: Wetter HTTP %d\n", code);
  }
  http.end();
  return ok;
}

static void net_task(void *) {
  bool ntp_started = false;
  uint32_t last_weather_ok = 0, last_weather_fail = 0, last_geo_fail = 0;
  bool weather_ever = false, weather_failed = false, geo_failed = false;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (!s_wifi_ok) continue;
    const uint32_t now = millis();
    if (!ntp_started) {
      configTzTime(kTz, "pool.ntp.org", "time.google.com");
      ntp_started = true;
    }

    char req[48];
    bool coords, reset;
    float lat, lon;
    portENTER_CRITICAL(&s_mux);
    strncpy(req, s_geo_req, sizeof req);
    req[sizeof req - 1] = '\0';
    const bool geo_pending = req[0] && !s_geo_done;
    coords = s_have_coords;
    lat = s_lat;
    lon = s_lon;
    reset = s_weather_reset;
    s_weather_reset = false;
    portEXIT_CRITICAL(&s_mux);
    if (reset) { weather_ever = false; weather_failed = false; geo_failed = false; }

    if (geo_pending && (!geo_failed || now - last_geo_fail >= kRetryMs)) {
      char name[48];
      float glat, glon;
      if (geocode(req, name, &glat, &glon)) {
        portENTER_CRITICAL(&s_mux);
        if (strcmp(req, s_geo_req) == 0) {   // Ort inzwischen nicht wieder geändert
          strncpy(s_geo_name, name, sizeof s_geo_name);
          s_geo_lat = glat;
          s_geo_lon = glon;
          s_geo_done = true;
          s_have_coords = true;
          s_lat = glat;
          s_lon = glon;
        }
        portEXIT_CRITICAL(&s_mux);
        coords = true;
        lat = glat;
        lon = glon;
        geo_failed = false;
        Serial.printf("net: Ort %s -> %.3f, %.3f\n", name, glat, glon);
      } else {
        geo_failed = true;
        last_geo_fail = now;
      }
    }

    const bool due = !weather_ever || now - last_weather_ok >= kWeatherEveryMs;
    if (coords && due && (!weather_failed || now - last_weather_fail >= kRetryMs)) {
      NetWeather w{};
      if (fetch_weather(lat, lon, &w)) {
        portENTER_CRITICAL(&s_mux);
        s_weather = w;
        portEXIT_CRITICAL(&s_mux);
        weather_ever = true;
        weather_failed = false;
        last_weather_ok = now;
      } else {
        weather_failed = true;
        last_weather_fail = now;
      }
    }

    if (s_hook) s_hook();
  }
}

static void start_portal(bool automatic) {
  if (s_wm.getConfigPortalActive()) { s_portal_pending = 0; return; }
  // Das Portal braucht Port 80: Claude-Empfänger (eigener Task) erst stoppen lassen, sonst später erneut versuchen.
  claude_api_stop();
  if (!claude_api_released()) { s_portal_pending = automatic ? 1 : 2; return; }
  s_portal_pending = 0;
  s_portal_auto = automatic;
  s_portal_manual_since = automatic ? 0 : millis();
  for (int i = 0; i < 8; i++) s_portal_pass[i] = (char)('0' + esp_random() % 10);   // WPA2 braucht >= 8 Zeichen
  s_portal_pass[8] = '\0';
  s_wm.startConfigPortal(kApName, s_portal_pass);
  s_portal = true;   // gleich sichtbar machen, damit der Claude-Empfänger Port 80 nicht wieder belegt
  Serial.println("net: Portal MadBuddy offen (mit Passwort)");
}

void net_begin(Persist *p) {
  s_p = p;
  WiFi.mode(WIFI_STA);
  s_param_place.setValue(p->place, 47);
  s_wm.addParameter(&s_param_place);
  s_wm.setConfigPortalBlocking(false);
  s_wm.setTitle("MadBuddy");
  const char *menu[] = {"wifi", "exit"};
  s_wm.setMenu(menu, 2);
  s_wm.setShowInfoUpdate(false);
  s_wm.setShowInfoErase(false);
  // Das Portal-AP hat ein zufälliges Passwort (am Display); die Firmware-Upload-Seite von WiFiManager ist trotzdem registriert. Ein Upload
  // dort wird abgebrochen (Neustart) – Firmware kommt nur über das geprüfte OTA (ota_pull).
  s_wm.setPreOtaUpdateCallback([]() {
    Serial.println("net: Upload ueber das Portal ist gesperrt");
    ESP.restart();
  });
  s_wm.setSaveParamsCallback([]() { s_place_changed = true; });
  s_wm.setSaveConfigCallback([]() { s_place_changed = true; });

  set_task_place(*p);
  s_offline_since = millis();
  if (s_wm.getWiFiIsSaved()) {
    WiFi.begin();   // gespeicherte Zugangsdaten
  } else {
    start_portal(true);
  }
  xTaskCreatePinnedToCore(net_task, "net_task", 8192, nullptr, 1, nullptr, 0);
}

void net_loop() {
  s_wm.process();
  const uint32_t now = millis();
  const bool ok = WiFi.status() == WL_CONNECTED;
  if (ok || s_wifi_ok != ok) s_offline_since = now;
  s_wifi_ok = ok;
  s_portal = s_wm.getConfigPortalActive();

  if (s_portal_pending == 1 && s_wifi_ok) s_portal_pending = 0;   // WLAN ist doch da: automatisches Portal entfällt
  if (s_portal_pending && !s_portal) start_portal(s_portal_pending == 1);
  if (!s_wifi_ok && !s_portal && now - s_offline_since >= kConnectWaitMs) start_portal(true);
  if (s_portal && s_wifi_ok && s_portal_auto) {   // WLAN doch noch da: automatisches Portal zu
    s_wm.stopConfigPortal();
    s_portal = false;
  }
  if (s_portal && !s_portal_auto && now - s_portal_manual_since >= kManualPortalMs && s_wifi_ok) {
    s_wm.stopConfigPortal();
    s_portal = false;
  }

  if (s_place_changed) {
    s_place_changed = false;
    const char *v = s_param_place.getValue();
    if (v && strcmp(v, s_p->place) != 0) {
      strncpy(s_p->place, v, sizeof s_p->place - 1);
      s_p->place[sizeof s_p->place - 1] = '\0';
      s_p->has_place = false;
      store_save(*s_p);
      set_task_place(*s_p);
      Serial.printf("net: neuer Ort \"%s\"\n", s_p->place);
    }
  }

  bool got = false;
  char name[48];
  float lat = 0, lon = 0;
  portENTER_CRITICAL(&s_mux);
  if (s_geo_done && s_geo_req[0]) {
    got = true;
    strncpy(name, s_geo_name, sizeof name);
    lat = s_geo_lat;
    lon = s_geo_lon;
    s_geo_req[0] = '\0';
  }
  portEXIT_CRITICAL(&s_mux);
  if (got) {
    strncpy(s_p->place, name, sizeof s_p->place - 1);
    s_p->place[sizeof s_p->place - 1] = '\0';
    s_p->lat = lat;
    s_p->lon = lon;
    s_p->has_place = true;
    store_save(*s_p);
  }
}

bool net_wifi_ok() { return s_wifi_ok; }

size_t net_build_cam_upd(const char *url, const char *sha, char *buf, size_t cap) {
  if (!ota_redirect_ok(url) || !ota_hex_ok(sha, 64)) return 0;
  wifi_config_t c;
  memset(&c, 0, sizeof c);
  char ssid[33], pass[65];
  size_t n = 0;
  if (esp_wifi_get_config(WIFI_IF_STA, &c) == ESP_OK && c.sta.ssid[0] != 0) {
    memcpy(ssid, c.sta.ssid, 32); ssid[32] = '\0';          // nicht garantiert terminiert
    memcpy(pass, c.sta.password, 64); pass[64] = '\0';
    bool ok = true;   // Tab/Zeilenumbruch würden die Felder der U-Zeile verschieben
    for (const char *p : {(const char *)ssid, (const char *)pass}) for (; *p; p++) if (*p == '\t' || *p == '\n' || *p == '\r') ok = false;
    if (ok) n = cam_build_upd_cmd(url, sha, ssid, pass, buf, cap);
  }
  secure_zero(&c, sizeof c);   // Passwort nicht liegen lassen
  secure_zero(ssid, sizeof ssid);
  secure_zero(pass, sizeof pass);
  return n;
}
bool net_portal_active() { return s_portal; }
bool net_portal_busy() { return s_portal || s_portal_pending != 0; }
const char *net_portal_pass() { return s_portal ? s_portal_pass : ""; }
void net_open_portal() {
  s_param_place.setValue(s_p ? s_p->place : "", 47);
  start_portal(false);
}

bool net_time_ok() { return time(nullptr) > 1700000000; }

bool net_local_time(struct tm *out) {
  if (!net_time_ok()) return false;
  const time_t t = time(nullptr);
  localtime_r(&t, out);
  return true;
}

int net_hour() {
  struct tm t;
  return net_local_time(&t) ? t.tm_hour : -1;
}

uint32_t net_epoch() { return net_time_ok() ? (uint32_t)time(nullptr) : 0; }

NetWeather net_weather() {
  portENTER_CRITICAL(&s_mux);
  const NetWeather w = s_weather;
  portEXIT_CRITICAL(&s_mux);
  return w;
}

void net_task_hook(void (*fn)()) { s_hook = fn; }
