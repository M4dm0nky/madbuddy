#include <Arduino.h>
#include "link.h"
#include "cam_config.h"

static HardwareSerial g_uart(1);
static CamLineBuf g_ub, g_sb;

void link_begin() {
  g_uart.setRxBufferSize(1024);   // eine U-Zeile (bis 400 Zeichen) muss eine Erkennung (bis ~250 ms) überstehen
  g_uart.begin(CAM_UART_BAUD, SERIAL_8N1, CAM_UART_RX, CAM_UART_TX);
}

void link_send(const char *s, size_t n, bool mirror) {
  g_uart.write((const uint8_t *)s, n);
  if (mirror) Serial.write((const uint8_t *)s, n);
}

// Befehle kommen vom CYD (UART) oder zum Testen vom PC (USB, tools/cam-serial.py).
static bool poll_one(Stream &st, CamLineBuf &lb, CamMsg *m) {
  while (st.available()) {
    if (!lb.feed((char)st.read())) continue;
    if (cam_parse(lb.line(), lb.len(), m) && (m->kind == CAM_CTRL || m->kind == CAM_UPD_CMD)) return true;
  }
  return false;
}

bool link_poll(CamMsg *m) { return poll_one(g_uart, g_ub, m) || poll_one(Serial, g_sb, m); }

// Zeilenpuffer löschen (enthalten nach einer U-Zeile das WLAN-Passwort)
void link_wipe() {
  volatile char *a = g_ub.buf, *b = g_sb.buf;
  for (size_t i = 0; i < sizeof g_ub.buf; i++) { a[i] = 0; b[i] = 0; }
  g_ub.n = g_ub.done = g_sb.n = g_sb.done = 0;
}
