// Diagnose über USB (COM6), Kamera-Zähler, Touch-/Lichtsensor-Protokoll. Kein Einfluss auf das Verhalten.
#include "app_internal.h"
#include <Arduino.h>
#include "hw/cam_uart.h"
#include "hw/light.h"
#include "net/net.h"
#include "ota/ota_pull.h"
#include "logic/secure_zero.h"

// Einmal pro Sekunde Zahl der F/N/H/M-Zeilen, letztes Gesicht, letzte Bewegung (x,y,n)
// und die Kamera-Messwerte aus der STATS-Zeile (fps, dps, rot, det).
static uint16_t s_cam_nf = 0, s_cam_nn = 0, s_cam_nh = 0, s_cam_nm = 0;
static CamMsg s_cam_last, s_cam_motion, s_cam_stats;
static uint32_t s_cam_log_t = 0;

void diag_cam_count(const CamMsg &m) {
  if (m.kind == CAM_FACE) { s_cam_nf++; s_cam_last = m; }
  else if (m.kind == CAM_NOFACE) s_cam_nn++;
  else if (m.kind == CAM_HEART) s_cam_nh++;
  else if (m.kind == CAM_MOTION) { s_cam_nm++; s_cam_motion = m; }
  else if (m.kind == CAM_STATS) { s_cam_stats = m; }
}

void cam_log_tick(uint32_t now) {
  if (!due_since(now, s_cam_log_t, 1000)) return;
  s_cam_log_t = now;
  if (cam_uart_snap_active(now)) return;   // während des Schnappschusses keine Textzeilen zwischen den J-Zeilen
  const CamLink &l = cam_uart_link();
  Serial.printf("cam: user=%d conn=%d xiao=%s %s F=%u N=%u H=%u M=%u last=%d,%d,%d mot=%d,%d,%d fps=%d dps=%d rot=%d det=%d\n",
                g_app.persist.cam_on ? 1 : 0, (int)cam_link_conn(l, now, true), l.version, cam_state_name(l.state), s_cam_nf, s_cam_nn,
                s_cam_nh, s_cam_nm, s_cam_last.x, s_cam_last.y, s_cam_last.w, s_cam_motion.x, s_cam_motion.y, s_cam_motion.n,
                (int)s_cam_stats.fps, (int)s_cam_stats.dps, (int)s_cam_stats.rot_ms, (int)s_cam_stats.det_ms);
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
    secure_zero(m.pass, sizeof m.pass);
  }
  Serial.println();
  secure_zero(line, sizeof line);   // enthält das WLAN-Passwort
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
void usb_cmd_poll(uint32_t now) {
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

#if CYD_DIAG_TOUCH
// Nach dem Lesen des Touchs: Beginn einer Berührung seriell ausgeben (Kalibrierung prüfen), dazu Berührungen,
// die der Chip meldet, die Bibliothek aber verwirft (außerhalb des Bereichs): einmal je Berührung.
void diag_touch(bool pressed, int32_t x, int32_t y) {
  static bool s_was_pressed = false;
  static bool s_raw_only = false;
  if (pressed && !s_was_pressed) {
    lgfx::touch_point_t raw;
    g_app.lcd.getTouchRaw(&raw, 1);
    Serial.printf("touch x=%d y=%d raw=%d,%d\n", (int)x, (int)y, (int)raw.x, (int)raw.y);
  }
  s_was_pressed = pressed;
  if (!pressed) {
    lgfx::touch_point_t raw;
    const bool raw_down = g_app.lcd.getTouchRaw(&raw, 1) > 0;
    if (raw_down && !s_raw_only) Serial.printf("touch verworfen raw=%d,%d\n", (int)raw.x, (int)raw.y);
    s_raw_only = raw_down;
  }
}

// Welche Geste mit welcher Position auf welchem Bildschirm.
void diag_gesture(Gesture g, int16_t tx, int16_t ty) {
  if (g != Gesture::NONE) Serial.printf("geste %d bei x=%d y=%d screen=%d\n", (int)g, (int)tx, (int)ty, (int)g_app.screen);
}
#endif

#if CYD_LOG_FPS   // Bildrate messen (Build-Flag -DCYD_LOG_FPS=1): Schleife und gesendete Gesichtsbilder je Sekunde, alle 10 s
static uint32_t s_fps_t = 0, s_fps_frames = 0, s_fps_pushes = 0;
void diag_fps_push() { s_fps_pushes++; }
void diag_fps_frame(uint32_t now) {
  s_fps_frames++;
  if (s_fps_t == 0) { s_fps_t = now; return; }
  const uint32_t dt = now - s_fps_t;
  if (dt < 10000) return;
  Serial.printf("fps: %.1f push: %.1f\n", s_fps_frames * 1000.0f / dt, s_fps_pushes * 1000.0f / dt);
  s_fps_t = now;
  s_fps_frames = s_fps_pushes = 0;
}
#endif

#if CYD_LOG_LDR   // Einmessen des Lichtsensors: Rohwert jede Sekunde (Build-Flag, danach wieder 0)
void diag_ldr_tick(uint32_t now) {
  static uint32_t s_last_ldr = 0;
  if (now - s_last_ldr >= 1000) { s_last_ldr = now; Serial.printf("ldr=%d\n", light_raw()); }
}
#endif
