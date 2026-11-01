#include <Arduino.h>
#include <esp_camera.h>
#include "cam_config.h"
#include "camera.h"
#include "detect.h"
#include "link.h"
#include "updater.h"
#include "logic/cam_proto.h"
#include "logic/cam_xiao.h"

// XIAO ESP32S3 Sense: Gesichter erkennen und dem CYD melden (Spec docs/superpowers/specs/2026-10-26-kamera-firmware-design.md).
// Bilder verlassen den XIAO nie; WLAN nur während eines Updates.

// Nach einem Update bestätigt erst der Probelauf das Image (updater_health_tick), nicht der Core beim Start.
extern "C" bool verifyRollbackLater() { return true; }

static bool g_want_on = true;     // Vorgabe an; der CYD schickt nach jedem Neustart C 1/C 0
static CamState g_state = CAM_ST_OFF;
static bool g_ever_ok = false, g_had_err = false;
static uint32_t g_last_heart = 0, g_last_retry = 0, g_beats = 0, g_fb_fail = 0;
static uint32_t g_stat_t0 = 0, g_frames = 0, g_ms_sum = 0;
static CamFace g_last = {0, 0, 0};
static bool g_has = false;

static void wipe_msg(CamMsg &m) {
  volatile char *p = m.pass;
  for (size_t i = 0; i < sizeof m.pass; i++) p[i] = 0;
}

static void send_heart() {
  char b[48];
  const size_t n = cam_build_heart(CAM_FW_VERSION, g_state, b, sizeof b);
  link_send(b, n);
  g_beats++;
}

static void set_err() {
  camera_stop();
  g_state = CAM_ST_ERR;
  g_had_err = true;
  g_last_retry = millis();
}

static void apply_camera() {
  if (!g_want_on) {
    camera_stop();
    g_state = CAM_ST_OFF;
    return;
  }
  if (camera_running()) { g_state = CAM_ST_ON; return; }
  if (camera_start()) { g_state = CAM_ST_ON; g_ever_ok = true; g_fb_fail = 0; }
  else set_err();
}

void setup() {
  Serial.begin(115200);
  link_begin();
  updater_boot();
  Serial.printf("# cam %s psram=%u detect=%d\n", CAM_FW_VERSION, (unsigned)ESP.getPsramSize(), detect_init());
  apply_camera();
  send_heart();
  g_last_heart = g_stat_t0 = millis();
}

void loop() {
  const uint32_t now = millis();
  CamMsg m;
  while (link_poll(&m)) {
    if (m.kind == CAM_CTRL) {
      g_want_on = m.on;
      apply_camera();
      send_heart();
    } else if (m.kind == CAM_UPD_CMD) {
      camera_stop();               // Speicher und Rechenzeit fürs Update
      g_state = CAM_ST_OFF;
      updater_run(m, send_heart);  // kehrt nur bei Fehler zurück
      wipe_msg(m);                 // Passwort aus dem Speicher
      link_wipe();
      apply_camera();
    }
  }
  if (g_want_on && g_state == CAM_ST_ERR && now - g_last_retry >= CAM_RETRY_MS) apply_camera();
  if (now - g_last_heart >= CAM_HEART_MS) { g_last_heart = now; send_heart(); }
  updater_health_tick(now, g_ever_ok && !g_had_err, g_beats);

  if (camera_running()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      if (++g_fb_fail >= 10) set_err();   // Sensor liefert nicht mehr
      delay(10);
      return;
    }
    g_fb_fail = 0;
    CamFace f;
    uint32_t ms = 0;
    g_has = detect_face(fb, &f, &ms);
    esp_camera_fb_return(fb);
    if (g_has) g_last = f;
    char b[32];
    const size_t n = g_has ? cam_build_face(f.x, f.y, f.w, b, sizeof b) : cam_build_noface(b, sizeof b);
    link_send(b, n, false);   // F/N nicht auf USB, dafür die Messzeile unten
    g_frames++;
    g_ms_sum += ms;
  } else {
    delay(20);
  }
  if (now - g_stat_t0 >= 1000) {
    Serial.printf("# fps=%u ms=%u face=%d %d,%d,%d\n", (unsigned)g_frames, (unsigned)(g_frames ? g_ms_sum / g_frames : 0),
                  g_has, g_last.x, g_last.y, g_last.w);
    g_stat_t0 = now;
    g_frames = 0;
    g_ms_sum = 0;
  }
}
