#pragma once
#include <stdint.h>
#include <string.h>
#include "cam_proto.h"
#include "deadline.h"

// CYD-Seite der Kamera-Leitung (Phase B): Bytes → Zeilen → Meldungen, Verbindung, Update-Watchdog. Reine Logik (test/test_cam_link).
constexpr uint32_t CAM_LINK_TIMEOUT_MS = 5000;    // so lange ohne H → keine Verbindung
constexpr uint32_t CAM_UPD_WATCHDOG_MS = 20000;   // so lange ohne U/H während eines Updates → Fehler (XIAO schweigt höchstens ~14 s)

enum class CamConn : uint8_t { OFF_USER, NONE, ERR, OFF, ON };   // aus (Einstellung), keine Verbindung, Kamerafehler, XIAO meldet aus, läuft

struct CamLink {
  CamLineBuf buf;
  bool ever = false;          // je ein H gesehen
  uint32_t heart_ms = 0;      // Zeit des letzten H
  CamState state = CAM_ST_OFF;
  char version[16] = {};
  bool restart = false;       // erstes H nach Trennung: C erneut senden
  bool upd = false;           // Update läuft
  uint32_t upd_ms = 0;        // letzte U/H-Zeile während des Updates
};

static inline bool cam_link_connected(const CamLink &l, uint32_t now) {
  return l.ever && !due_since(now, l.heart_ms, CAM_LINK_TIMEOUT_MS);
}

// Ein Byte von Serial2; true, wenn *out eine gültige Meldung enthält.
static inline bool cam_link_feed(CamLink &l, char c, uint32_t now, CamMsg *out) {
  if (!l.buf.feed(c)) return false;
  if (!cam_parse(l.buf.line(), l.buf.len(), out)) return false;
  if (out->kind == CAM_HEART) {
    if (!cam_link_connected(l, now)) l.restart = true;
    l.ever = true;
    l.heart_ms = now;
    l.state = out->state;
    strncpy(l.version, out->version, sizeof l.version - 1);
    l.version[sizeof l.version - 1] = '\0';
  }
  if (l.upd) {
    if (out->kind == CAM_UPD_OK || out->kind == CAM_UPD_ERR) l.upd = false;
    // H nur mit Kamera aus: läuft sie weiter, hat der XIAO den Befehl nicht angenommen, und der Watchdog soll greifen
    else if (out->kind == CAM_UPD_PCT || (out->kind == CAM_HEART && out->state != CAM_ST_ON)) l.upd_ms = now;
  }
  return true;
}

static inline CamConn cam_link_conn(const CamLink &l, uint32_t now, bool user_on) {
  if (!user_on) return CamConn::OFF_USER;
  if (!cam_link_connected(l, now)) return CamConn::NONE;
  if (l.state == CAM_ST_ERR) return CamConn::ERR;
  return l.state == CAM_ST_ON ? CamConn::ON : CamConn::OFF;
}

// true genau einmal nach dem ersten H bzw. nach einer Trennung.
static inline bool cam_link_take_restart(CamLink &l) {
  const bool r = l.restart;
  l.restart = false;
  return r;
}

static inline void cam_link_upd_start(CamLink &l, uint32_t now) { l.upd = true; l.upd_ms = now; }
static inline bool cam_link_upd_timed_out(const CamLink &l, uint32_t now) {
  return l.upd && due_since(now, l.upd_ms, CAM_UPD_WATCHDOG_MS);
}
// Nach der Anzeige des Ergebnisses (auch der Zeitüberschreitung): Überwachung beenden, sonst bleibt upd_timed_out wahr.
static inline void cam_link_upd_end(CamLink &l) { l.upd = false; }
static inline const char *cam_link_version(const CamLink &l) { return l.version; }
