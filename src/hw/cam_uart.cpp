#include "cam_uart.h"
#include <Arduino.h>
#include "../config.h"

static CamLink s_link;
static void (*s_cb)(const CamMsg &, uint32_t) = nullptr;

void cam_uart_begin() {
  Serial2.setRxBufferSize(1024);   // vor begin()
  Serial2.begin(CAM_BAUD, SERIAL_8N1, CAM_RX, CAM_TX);
}

void cam_uart_poll(uint32_t now) {
  for (int i = 0; i < 1024 && Serial2.available(); i++) {
    CamMsg m;
    if (cam_link_feed(s_link, (char)Serial2.read(), now, &m) && s_cb) s_cb(m, now);
  }
}

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
