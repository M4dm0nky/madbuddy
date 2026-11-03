#include "cam_uart.h"
#include <Arduino.h>
#include "../config.h"
#include "../logic/deadline.h"
#include "../logic/cam_snapbuf.h"

static CamLink s_link;
static void (*s_cb)(const CamMsg &, uint32_t) = nullptr;

void cam_uart_begin() {
  Serial2.setRxBufferSize(1024);   // vor begin()
  Serial2.begin(CAM_BAUD, SERIAL_8N1, CAM_RX, CAM_TX);
}

static Deadline s_snap;   // Durchleitung der J-Zeilen an USB
// USB läuft mit derselben Baudrate wie die Kamera: Byte für Byte durchgereicht staut jede Zeichenpause auf und
// Zeilen gehen verloren. Darum das ganze Bild sammeln und nach "J E"/"J X" (oder Zeitablauf) in einem Stück ausgeben.
static constexpr size_t kSnapCap = 24576;
static SnapBuf s_sb{nullptr, kSnapCap};

// Fehlerzeile mit Prüfsumme, sonst verwirft tools/cam-snap.py sie als fehlerhaft
static void snap_error(const char *content) {
  char b[48];
  const size_t n = cam_seal(content, b, sizeof b);
  if (n) Serial.write((const uint8_t *)b, n);
}

static void snap_flush() {
  if (!s_sb.buf) return;
  Serial.write((const uint8_t *)s_sb.buf, s_sb.len);
  free(s_sb.buf);
  s_sb = SnapBuf{nullptr, kSnapCap};
  s_snap.clear();
}

void cam_uart_poll(uint32_t now) {
  const bool pass = s_snap.active(now);
  if (!pass && s_sb.buf) snap_flush();   // Zeit abgelaufen: ausgeben, was da ist
  for (int i = 0; i < 1024 && Serial2.available(); i++) {
    const char ch = (char)Serial2.read();
    if (pass && s_sb.buf) {
      const SnapStep st = snap_feed(s_sb, ch);
      if (st == SnapStep::END) snap_flush();
      else if (st == SnapStep::FULL) { snap_flush(); snap_error("J X cyd_puffer_voll"); }
    }
    CamMsg m;
    if (cam_link_feed(s_link, ch, now, &m) && s_cb) s_cb(m, now);
  }
}

void cam_uart_snap(uint32_t now) {
  if (s_sb.buf) return;   // läuft schon
  s_sb = SnapBuf{(char *)malloc(kSnapCap), kSnapCap};
  if (!s_sb.buf) { snap_error("J X cyd_kein_speicher"); return; }
  s_snap.arm(now, 8000);
  char line[16];
  const size_t n = cam_build_snap_cmd(line, sizeof line);
  if (n) Serial2.write((const uint8_t *)line, n);
}
bool cam_uart_snap_active(uint32_t now) { return s_snap.active(now); }

void cam_uart_send(const char *content) {
  char line[CAM_LINE_MAX + 4];
  const size_t n = cam_seal(content, line, sizeof line);
  if (n) Serial2.write((const uint8_t *)line, n);
  volatile char *p = line;   // die U-Zeile enthält das Passwort: Puffer nicht liegen lassen
  for (size_t i = 0; i < sizeof line; i++) p[i] = 0;
}

void cam_uart_write_sealed(const char *line, size_t n) { if (n) Serial2.write((const uint8_t *)line, n); }

void cam_uart_send_ctrl(bool on) { cam_uart_send(on ? "C 1" : "C 0"); }
bool cam_uart_take_restart() { return cam_link_take_restart(s_link); }
const CamLink &cam_uart_link() { return s_link; }
void cam_uart_upd_start(uint32_t now) { cam_link_upd_start(s_link, now); }
bool cam_uart_upd_timed_out(uint32_t now) { return cam_link_upd_timed_out(s_link, now); }
void cam_uart_upd_end() { cam_link_upd_end(s_link); }
void cam_uart_on_msg(void (*fn)(const CamMsg &, uint32_t)) { s_cb = fn; }
