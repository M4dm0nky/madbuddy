#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Zeilenprotokoll XIAO <-> CYD (Spec docs/superpowers/specs/2026-10-26-kamera-firmware-design.md). Reine Logik ohne
// Arduino-/IDF-Includes, von beiden Projekten benutzt (cam/ per -I../src). Host-Test test/test_cam_proto.
// Zeile: <inhalt>*<xx>\n, xx = XOR über <inhalt> als zwei Hexziffern (gesendet groß, gelesen beides).
constexpr size_t CAM_LINE_MAX = 400;   // ohne '\n'; die U-Zeile (URL, SHA, SSID, Passwort) braucht mehr als 200

enum CamKind : uint8_t { CAM_NONE = 0, CAM_FACE, CAM_NOFACE, CAM_HEART, CAM_CTRL, CAM_UPD_CMD, CAM_UPD_PCT, CAM_UPD_OK, CAM_UPD_ERR };
enum CamState : uint8_t { CAM_ST_ON = 0, CAM_ST_OFF, CAM_ST_ERR };

struct CamMsg {
  CamKind kind = CAM_NONE;
  int16_t x = 0, y = 0, w = 0;     // FACE, je 0..1000
  char version[16] = "";           // HEART
  CamState state = CAM_ST_OFF;     // HEART
  bool on = false;                 // CTRL
  uint8_t pct = 0;                 // UPD_PCT
  char text[40] = "";              // UPD_ERR: Grund
  char url[200] = "", sha[65] = "", ssid[33] = "", pass[64] = "";   // UPD_CMD
};

static inline uint8_t cam_xor(const char *s, size_t n) {
  uint8_t x = 0;
  for (size_t i = 0; i < n; i++) x ^= (uint8_t)s[i];
  return x;
}

// "content*XX\n" nach out. Rückgabe: Länge mit '\n' (ohne '\0'), 0 bei Überlänge, zu kleinem Puffer oder Zeilenumbruch im Inhalt.
static inline size_t cam_seal(const char *content, char *out, size_t cap) {
  const size_t n = strlen(content);
  if (n + 3 > CAM_LINE_MAX || n + 5 > cap) return 0;
  for (size_t i = 0; i < n; i++) if (content[i] == '\n' || content[i] == '\r') return 0;
  static const char kHex[] = "0123456789ABCDEF";
  const uint8_t x = cam_xor(content, n);
  memcpy(out, content, n);
  out[n] = '*';
  out[n + 1] = kHex[x >> 4];
  out[n + 2] = kHex[x & 15];
  out[n + 3] = '\n';
  out[n + 4] = '\0';
  return n + 4;
}

static inline size_t cam_build_face(int x, int y, int w, char *out, size_t cap) {
  char c[24]; snprintf(c, sizeof c, "F %d %d %d", x, y, w); return cam_seal(c, out, cap);
}
static inline size_t cam_build_noface(char *out, size_t cap) { return cam_seal("N", out, cap); }
static inline const char *cam_state_name(CamState s) { return s == CAM_ST_ON ? "on" : s == CAM_ST_ERR ? "err" : "off"; }
static inline size_t cam_build_heart(const char *version, CamState st, char *out, size_t cap) {
  char c[40]; snprintf(c, sizeof c, "H %.15s %s", version, cam_state_name(st)); return cam_seal(c, out, cap);
}
static inline size_t cam_build_ctrl(bool on, char *out, size_t cap) { return cam_seal(on ? "C 1" : "C 0", out, cap); }
static inline size_t cam_build_upd_pct(int pct, char *out, size_t cap) {
  char c[12]; snprintf(c, sizeof c, "U %d", pct); return cam_seal(c, out, cap);
}
static inline size_t cam_build_upd_ok(char *out, size_t cap) { return cam_seal("U ok", out, cap); }
static inline size_t cam_build_upd_err(const char *why, char *out, size_t cap) {
  char c[48]; snprintf(c, sizeof c, "U err %.39s", why); return cam_seal(c, out, cap);
}
static inline size_t cam_build_upd_cmd(const char *url, const char *sha, const char *ssid, const char *pass, char *out, size_t cap) {
  char c[CAM_LINE_MAX + 1];
  const int n = snprintf(c, sizeof c, "U\t%s\t%s\t%s\t%s", url, sha, ssid, pass);
  size_t r = 0;
  if (n >= 0 && (size_t)n < sizeof c) r = cam_seal(c, out, cap);
  volatile char *w = c;   // enthält das Passwort: nicht auf dem Stapel liegen lassen
  for (size_t i = 0; i < sizeof c; i++) w[i] = 0;
  return r;
}

// Ganzzahl aus genau den Zeichen [s, e), nur Ziffern, 0..maxv
static inline bool cam_num(const char *s, const char *e, int maxv, int *v) {
  if (s >= e || e - s > 5) return false;
  int r = 0;
  for (const char *p = s; p < e; p++) {
    if (*p < '0' || *p > '9') return false;
    r = r * 10 + (*p - '0');
  }
  if (r > maxv) return false;
  *v = r;
  return true;
}
static inline int cam_hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
// Feld [s, e) nach out kopieren, wenn min..max Zeichen lang
static inline bool cam_field(const char *s, const char *e, size_t min, size_t max, char *out) {
  const size_t n = (size_t)(e - s);
  if (n < min || n > max) return false;
  memcpy(out, s, n);
  out[n] = '\0';
  return true;
}

static inline bool cam_parse_upd_cmd(const char *c, size_t n, CamMsg *m) {
  const char *f[4];
  const char *fe[4];
  const char *p = c + 2, *end = c + n;   // nach "U\t": url, sha, ssid, pass
  for (int i = 0; i < 4; i++) {
    f[i] = p;
    while (p < end && *p != '\t') p++;
    fe[i] = p;
    if (i < 3) { if (p >= end) return false; p++; }
  }
  if (p != end) return false;   // mehr als 4 Felder
  if (!cam_field(f[0], fe[0], 9, sizeof m->url - 1, m->url)) return false;
  if (!cam_field(f[1], fe[1], 64, 64, m->sha)) return false;
  for (int i = 0; i < 64; i++) {   // Hex, groß oder klein; gespeichert wird klein
    if (cam_hexval(m->sha[i]) < 0) return false;
    if (m->sha[i] >= 'A' && m->sha[i] <= 'F') m->sha[i] = (char)(m->sha[i] - 'A' + 'a');
  }
  if (!cam_field(f[2], fe[2], 1, 32, m->ssid)) return false;
  if (!cam_field(f[3], fe[3], 0, 63, m->pass)) return false;
  m->kind = CAM_UPD_CMD;
  return true;
}

// line ohne '\n' (ein '\r' am Ende wird ignoriert). true = gültige, bekannte Zeile; m ist dann gefüllt.
static inline bool cam_parse(const char *line, size_t n, CamMsg *m) {
  if (n > 0 && line[n - 1] == '\r') n--;
  if (n < 4 || n > CAM_LINE_MAX || line[n - 3] != '*') return false;
  const int h = cam_hexval(line[n - 2]), l = cam_hexval(line[n - 1]);
  if (h < 0 || l < 0) return false;
  const size_t cn = n - 3;
  if (cam_xor(line, cn) != (uint8_t)(h * 16 + l)) return false;
  const char *c = line, *end = line + cn;
  *m = CamMsg();
  if (cn == 1 && c[0] == 'N') { m->kind = CAM_NOFACE; return true; }
  if (cn >= 2 && c[0] == 'U' && c[1] == '\t') return cam_parse_upd_cmd(c, cn, m);
  if (cn < 3 || c[1] != ' ') return false;
  const char *a = c + 2;
  switch (c[0]) {
    case 'F': {
      const char *s1 = (const char *)memchr(a, ' ', end - a);
      if (!s1) return false;
      const char *s2 = (const char *)memchr(s1 + 1, ' ', end - s1 - 1);
      if (!s2 || memchr(s2 + 1, ' ', end - s2 - 1)) return false;
      int x, y, w;
      if (!cam_num(a, s1, 1000, &x) || !cam_num(s1 + 1, s2, 1000, &y) || !cam_num(s2 + 1, end, 1000, &w)) return false;
      m->kind = CAM_FACE; m->x = (int16_t)x; m->y = (int16_t)y; m->w = (int16_t)w;
      return true;
    }
    case 'H': {
      const char *s1 = (const char *)memchr(a, ' ', end - a);
      if (!s1 || !cam_field(a, s1, 1, sizeof m->version - 1, m->version)) return false;
      for (const char *p = m->version; *p; p++) {
        const char ch = *p;
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.' || ch == '-' || ch == '_')) return false;
      }
      const size_t sn = (size_t)(end - s1 - 1);
      const char *st = s1 + 1;
      if (sn == 2 && memcmp(st, "on", 2) == 0) m->state = CAM_ST_ON;
      else if (sn == 3 && memcmp(st, "off", 3) == 0) m->state = CAM_ST_OFF;
      else if (sn == 3 && memcmp(st, "err", 3) == 0) m->state = CAM_ST_ERR;
      else return false;
      m->kind = CAM_HEART;
      return true;
    }
    case 'C':
      if (cn != 3 || (a[0] != '0' && a[0] != '1')) return false;
      m->kind = CAM_CTRL; m->on = a[0] == '1';
      return true;
    case 'U': {
      const size_t an = (size_t)(end - a);
      if (an == 2 && memcmp(a, "ok", 2) == 0) { m->kind = CAM_UPD_OK; return true; }
      if (an > 4 && memcmp(a, "err ", 4) == 0) {
        if (!cam_field(a + 4, end, 1, sizeof m->text - 1, m->text)) return false;
        m->kind = CAM_UPD_ERR;
        return true;
      }
      int pct;
      if (!cam_num(a, end, 100, &pct)) return false;
      m->kind = CAM_UPD_PCT; m->pct = (uint8_t)pct;
      return true;
    }
    default:
      return false;
  }
}

// Sammelt Bytes zu Zeilen. feed() == true: line()/len() enthalten eine vollständige Zeile (ohne '\n' und '\r'),
// gültig bis zum nächsten feed(). Zu lange Zeilen werden bis zum nächsten '\n' verworfen.
struct CamLineBuf {
  char buf[CAM_LINE_MAX + 1] = {};
  size_t n = 0, done = 0;
  bool over = false;
  bool feed(char c) {
    if (c == '\r') return false;
    if (c == '\n') {
      const bool ok = !over && n > 0;
      buf[n] = '\0';
      done = n;
      n = 0;
      over = false;
      return ok;
    }
    if (over) return false;
    if (n >= CAM_LINE_MAX) { over = true; return false; }
    buf[n++] = c;
    return false;
  }
  const char *line() const { return buf; }
  size_t len() const { return done; }
};
