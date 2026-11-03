#include <unity.h>
#include <string.h>
#include "logic/cam_proto.h"
#include "logic/cam_xiao.h"
#include "logic/cam_snapbuf.h"
void setUp() {}
void tearDown() {}

static char g_buf[CAM_LINE_MAX + 8];
// Inhalt mit gültiger Prüfsumme, ohne '\n' (wie cam_parse sie bekommt)
static const char *seal(const char *content) {
  size_t n = cam_seal(content, g_buf, sizeof g_buf);
  TEST_ASSERT_TRUE(n > 0);
  g_buf[n - 1] = '\0';
  return g_buf;
}
static bool parse_str(const char *s, CamMsg *m) { return cam_parse(s, strlen(s), m); }

void test_xor_known() {
  TEST_ASSERT_EQUAL_HEX8(0x4E, cam_xor("N", 1));
  TEST_ASSERT_EQUAL_HEX8(0x53, cam_xor("C 0", 3));
  TEST_ASSERT_EQUAL_HEX8(0x52, cam_xor("C 1", 3));
}
void test_build_fixed_lines() {
  char b[32];
  TEST_ASSERT_EQUAL(5, cam_build_noface(b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("N*4E\n", b);
  TEST_ASSERT_EQUAL(7, cam_build_ctrl(true, b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("C 1*52\n", b);
  cam_build_ctrl(false, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("C 0*53\n", b);
  TEST_ASSERT_EQUAL(0, cam_build_noface(b, 4));   // Puffer zu klein
}
void test_roundtrip_face_and_heart() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_face(512, 430, 210, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_FACE, m.kind);
  TEST_ASSERT_EQUAL(512, m.x); TEST_ASSERT_EQUAL(430, m.y); TEST_ASSERT_EQUAL(210, m.w);
  n = cam_build_heart("0.1.0", CAM_ST_ERR, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_HEART, m.kind);
  TEST_ASSERT_EQUAL_STRING("0.1.0", m.version);
  TEST_ASSERT_EQUAL(CAM_ST_ERR, m.state);
  TEST_ASSERT_TRUE(parse_str("N*4E", &m));
  TEST_ASSERT_EQUAL(CAM_NOFACE, m.kind);
  TEST_ASSERT_TRUE(parse_str("C 0*53", &m));
  TEST_ASSERT_EQUAL(CAM_CTRL, m.kind);
  TEST_ASSERT_FALSE(m.on);
  TEST_ASSERT_TRUE(parse_str("C 1*52\r", &m));   // '\r' am Ende wird ignoriert
  TEST_ASSERT_TRUE(m.on);
}
void test_checksum_rules() {
  CamMsg m;
  TEST_ASSERT_FALSE(parse_str("N*4F", &m));
  TEST_ASSERT_TRUE(parse_str("C 0*53", &m));
  char b[64];
  size_t n = cam_build_face(1, 2, 3, b, sizeof b);
  for (size_t i = n - 3; i < n - 1; i++) if (b[i] >= 'A' && b[i] <= 'F') b[i] = (char)(b[i] - 'A' + 'a');
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));      // Kleinbuchstaben gelten auch
}
void test_motion_roundtrip_and_strict() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_motion(512, 430, 37, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("M 512 430 37", (snprintf(g_buf, sizeof g_buf, "%.*s", (int)(n - 4), b), g_buf));
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_MOTION, m.kind);
  TEST_ASSERT_EQUAL(512, m.x); TEST_ASSERT_EQUAL(430, m.y); TEST_ASSERT_EQUAL(37, m.n);
  TEST_ASSERT_TRUE(parse_str(seal("M 0 1000 9999"), &m));
  TEST_ASSERT_EQUAL(9999, m.n);
  const char *bad[] = {"M 1 2", "M 1 2 3 4", "M 1001 0 5", "M 0 1001 5", "M 0 0 10000", "M -1 0 5", "M 1a 2 3", "M  1 2 3", "M"};
  for (const char *s : bad) TEST_ASSERT_FALSE_MESSAGE(parse_str(seal(s), &m), s);
}
void test_rejects_garbage() {
  CamMsg m;
  const char *raw[] = {"", "*", "N", "N*4", "N*4EE", "N*G1"};
  for (const char *s : raw) TEST_ASSERT_FALSE_MESSAGE(parse_str(s, &m), s);
  const char *bad[] = {"X", "F 1 2", "F 1 2 3 4", "F 1001 0 0", "F -1 0 0", "F 1a 2 3", "F  1 2 3",
                       "H 0.1.0 maybe", "H bad/ver on", "H 0.1.0", "H 12345678901234567 on", "C 2", "C",
                       "U 101", "U -1", "U", "N x", "U err "};
  for (const char *s : bad) TEST_ASSERT_FALSE_MESSAGE(parse_str(seal(s), &m), s);
}
void test_length_limit() {
  char content[CAM_LINE_MAX + 2];
  memset(content, 'a', sizeof content);
  content[CAM_LINE_MAX - 3] = '\0';               // passt genau: Inhalt + "*XX" = CAM_LINE_MAX
  char out[CAM_LINE_MAX + 8];
  TEST_ASSERT_EQUAL(CAM_LINE_MAX + 1, cam_seal(content, out, sizeof out));
  content[CAM_LINE_MAX - 3] = 'a'; content[CAM_LINE_MAX - 2] = '\0';
  TEST_ASSERT_EQUAL(0, cam_seal(content, out, sizeof out));
  CamMsg m;
  TEST_ASSERT_FALSE(cam_parse(out, CAM_LINE_MAX + 1, &m));
  TEST_ASSERT_EQUAL(0, cam_seal("a\nb", out, sizeof out));   // Zeilenumbruch im Inhalt verboten
}
void test_update_cmd_roundtrip() {
  char sha[65]; memset(sha, 'a', 64); sha[64] = '\0';
  const char *url = "https://github.com/M4dm0nky/cyd-buddy/releases/download/v20261029-1200/cam.bin";
  char b[CAM_LINE_MAX + 8];
  size_t n = cam_build_upd_cmd(url, sha, "Mein WLAN", "p*ss w0rd", b, sizeof b);
  TEST_ASSERT_TRUE(n > 0);
  CamMsg m;
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_CMD, m.kind);
  TEST_ASSERT_EQUAL_STRING(url, m.url);
  TEST_ASSERT_EQUAL_STRING(sha, m.sha);
  TEST_ASSERT_EQUAL_STRING("Mein WLAN", m.ssid);
  TEST_ASSERT_EQUAL_STRING("p*ss w0rd", m.pass);
  n = cam_build_upd_cmd(url, sha, "offen", "", b, sizeof b);    // offenes WLAN: leeres Passwort erlaubt
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL_STRING("", m.pass);
}
void test_update_cmd_rejects() {
  char sha[65]; memset(sha, 'a', 64); sha[64] = '\0';
  char ssid33[34]; memset(ssid33, 's', 33); ssid33[33] = '\0';
  CamMsg m;
  char line[CAM_LINE_MAX];
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%.63s\tW\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SHA zu kurz
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // nur 3 Felder
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\t\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SSID leer
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\t%s\tp", sha, ssid33);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SSID zu lang
  sha[3] = 'G';
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SHA kein Hex
  sha[3] = 'a';
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW\tp\tzuviel", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // 5 Felder
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW\tp", sha);
  TEST_ASSERT_TRUE(parse_str(seal(line), &m));    // Kontrolle: 4 Felder gehen
}
void test_update_cmd_sha_case() {
  char sha[65]; memset(sha, 'A', 64); sha[64] = '\0';
  sha[0] = 'F'; sha[1] = '9'; sha[2] = 'c';
  char line[CAM_LINE_MAX];
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW\tp", sha);
  CamMsg m;
  TEST_ASSERT_TRUE(parse_str(seal(line), &m));    // Großbuchstaben erlaubt
  TEST_ASSERT_EQUAL_CHAR('f', m.sha[0]);          // gespeichert klein
  TEST_ASSERT_EQUAL_CHAR('9', m.sha[1]);
  TEST_ASSERT_EQUAL_CHAR('c', m.sha[2]);
  TEST_ASSERT_EQUAL_CHAR('a', m.sha[63]);
}
void test_update_cmd_worst_case() {
  char url[200], sha[65], ssid[33], pass[64];
  memset(url, 'u', 199); url[199] = '\0';
  memset(sha, '7', 64); sha[64] = '\0';
  memset(ssid, 's', 32); ssid[32] = '\0';
  memset(pass, 'p', 63); pass[63] = '\0';
  char b[CAM_LINE_MAX + 8];
  size_t n = cam_build_upd_cmd(url, sha, ssid, pass, b, sizeof b);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_TRUE(n - 1 <= CAM_LINE_MAX);
  CamMsg m;
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL_STRING(url, m.url); TEST_ASSERT_EQUAL_STRING(sha, m.sha);
  TEST_ASSERT_EQUAL_STRING(ssid, m.ssid); TEST_ASSERT_EQUAL_STRING(pass, m.pass);
  memset(url, 'u', 200); url[200] = '\0';          // 200 Zeichen URL: zu lang
  n = cam_build_upd_cmd(url, sha, ssid, pass, b, sizeof b);
  if (n > 0) TEST_ASSERT_FALSE(cam_parse(b, n - 1, &m));
}
void test_length_boundary() {
  char content[CAM_LINE_MAX + 8];
  char out[CAM_LINE_MAX + 8];
  CamMsg m;
  // genau CAM_LINE_MAX: cam_seal nimmt an, bei +1 nicht
  memset(content, 'a', CAM_LINE_MAX - 3); content[CAM_LINE_MAX - 3] = '\0';
  size_t n = cam_seal(content, out, sizeof out);
  TEST_ASSERT_EQUAL(CAM_LINE_MAX + 1, n);
  TEST_ASSERT_EQUAL(CAM_LINE_MAX, strlen(out) - 1);
  memset(content, 'a', CAM_LINE_MAX - 2); content[CAM_LINE_MAX - 2] = '\0';
  TEST_ASSERT_EQUAL(0, cam_seal(content, out, sizeof out));
  // 401 Zeichen mit richtiger Prüfsumme von Hand: abgelehnt (sonst unbekannter Befehl, daher nur Längenregel getestet)
  memset(content, 'a', CAM_LINE_MAX - 2); content[CAM_LINE_MAX - 2] = '\0';
  const uint8_t x = cam_xor(content, CAM_LINE_MAX - 2);
  static const char kHex[] = "0123456789ABCDEF";
  memcpy(out, content, CAM_LINE_MAX - 2);
  out[CAM_LINE_MAX - 2] = '*'; out[CAM_LINE_MAX - 1] = kHex[x >> 4]; out[CAM_LINE_MAX] = kHex[x & 15];
  TEST_ASSERT_FALSE(cam_parse(out, CAM_LINE_MAX + 1, &m));
}
void test_linebuf_boundary() {
  CamLineBuf lb;
  for (size_t i = 0; i < CAM_LINE_MAX; i++) TEST_ASSERT_FALSE(lb.feed('a'));
  TEST_ASSERT_TRUE(lb.feed('\n'));                // genau 400 gehen durch
  TEST_ASSERT_EQUAL(CAM_LINE_MAX, lb.len());
  for (size_t i = 0; i < CAM_LINE_MAX + 1; i++) TEST_ASSERT_FALSE(lb.feed('a'));
  TEST_ASSERT_FALSE(lb.feed('\n'));               // 401 werden verworfen
  bool ok = false;
  for (const char *p = "N*4E\n"; *p; p++) ok = lb.feed(*p);
  TEST_ASSERT_TRUE(ok);                           // danach läuft der Puffer wieder
  TEST_ASSERT_EQUAL_STRING("N*4E", lb.line());
}
void test_update_progress() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_upd_pct(42, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_PCT, m.kind); TEST_ASSERT_EQUAL(42, m.pct);
  n = cam_build_upd_ok(b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_OK, m.kind);
  n = cam_build_upd_err("SHA-256 falsch", b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_ERR, m.kind); TEST_ASSERT_EQUAL_STRING("SHA-256 falsch", m.text);
}
void test_linebuf_split_and_cr() {
  CamLineBuf lb;
  const char *in = "N*4";
  for (const char *p = in; *p; p++) TEST_ASSERT_FALSE(lb.feed(*p));
  TEST_ASSERT_TRUE(lb.feed('E') == false);
  TEST_ASSERT_TRUE(lb.feed('\n'));
  TEST_ASSERT_EQUAL_STRING("N*4E", lb.line());
  TEST_ASSERT_EQUAL(4, lb.len());
  const char *in2 = "C 1*52\r\n";
  int done = 0;
  for (const char *p = in2; *p; p++) if (lb.feed(*p)) done++;
  TEST_ASSERT_EQUAL(1, done);
  TEST_ASSERT_EQUAL_STRING("C 1*52", lb.line());
  TEST_ASSERT_FALSE(lb.feed('\n'));               // leere Zeile zählt nicht
}
void test_linebuf_overflow_recovers() {
  CamLineBuf lb;
  for (int i = 0; i < 500; i++) TEST_ASSERT_FALSE(lb.feed('a'));
  TEST_ASSERT_FALSE(lb.feed('\n'));               // übergroße Zeile wird verworfen
  const char *in = "N*4E\n";
  bool ok = false;
  for (const char *p = in; *p; p++) ok = lb.feed(*p);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("N*4E", lb.line());
}

void test_box_center() {
  CamFace f;
  TEST_ASSERT_TRUE(cam_box_to_face(100, 80, 220, 200, 320, 240, &f));
  TEST_ASSERT_EQUAL(500, f.x);   // (100+220)/2 = 160 von 320
  TEST_ASSERT_EQUAL(583, f.y);   // 140 von 240
  TEST_ASSERT_EQUAL(375, f.w);   // 120 von 320
}
void test_box_clamped_and_degenerate() {
  CamFace f;
  TEST_ASSERT_TRUE(cam_box_to_face(-10, -10, 400, 300, 320, 240, &f));
  TEST_ASSERT_EQUAL(500, f.x); TEST_ASSERT_EQUAL(500, f.y); TEST_ASSERT_EQUAL(1000, f.w);
  TEST_ASSERT_FALSE(cam_box_to_face(50, 10, 50, 20, 320, 240, &f));   // Breite 0
  TEST_ASSERT_FALSE(cam_box_to_face(60, 10, 50, 20, 320, 240, &f));   // vertauscht
  TEST_ASSERT_FALSE(cam_box_to_face(0, 0, 10, 10, 0, 240, &f));       // Bildgröße 0
}
void test_health_decision() {
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(19999, true, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_VALID, cam_health(20000, true, 5));
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(20000, true, 4));
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(30000, false, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_ROLLBACK, cam_health(60000, false, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_ROLLBACK, cam_health(60000, true, 2));
  TEST_ASSERT_EQUAL(CAM_HEALTH_VALID, cam_health(60000, true, 5));   // gesund geht vor Zeitablauf
}

void test_snap_cmd() {
  char b[16];
  TEST_ASSERT_EQUAL(5, cam_build_snap_cmd(b, sizeof b));
  CamMsg m;
  TEST_ASSERT_TRUE(parse_str(seal("P"), &m));
  TEST_ASSERT_EQUAL(CAM_SNAP_CMD, m.kind);
  TEST_ASSERT_FALSE(parse_str(seal("P x"), &m));
  TEST_ASSERT_FALSE(parse_str(seal("P 1"), &m));
  // Bildzeilen (J) nimmt cam_parse nicht an, auch nicht mit gültiger Prüfsumme
  TEST_ASSERT_FALSE(parse_str(seal("J D 0 QUJD"), &m));
  TEST_ASSERT_FALSE(parse_str(seal("J E"), &m));
}

// 3x2-Bild, 2 Byte je Pixel: Pixel k hat die Bytes {k, 0x80+k}
//   0 1 2
//   3 4 5
static void rot_src(uint8_t *s) { for (int k = 0; k < 6; k++) { s[2 * k] = (uint8_t)k; s[2 * k + 1] = (uint8_t)(0x80 + k); } }
static void rot_expect(const uint8_t *d, const int *px, int n) {
  for (int i = 0; i < n; i++) {
    TEST_ASSERT_EQUAL_UINT8(px[i], d[2 * i]);
    TEST_ASSERT_EQUAL_UINT8(0x80 + px[i], d[2 * i + 1]);
  }
}
void test_rotate_px2() {
  uint8_t s[12], d[12];
  rot_src(s);
  int ow, oh;
  cam_rot_dims(3, 2, 0, &ow, &oh);   TEST_ASSERT_EQUAL(3, ow); TEST_ASSERT_EQUAL(2, oh);
  cam_rot_dims(3, 2, 90, &ow, &oh);  TEST_ASSERT_EQUAL(2, ow); TEST_ASSERT_EQUAL(3, oh);
  cam_rot_dims(3, 2, 180, &ow, &oh); TEST_ASSERT_EQUAL(3, ow); TEST_ASSERT_EQUAL(2, oh);
  cam_rot_dims(3, 2, 270, &ow, &oh); TEST_ASSERT_EQUAL(2, ow); TEST_ASSERT_EQUAL(3, oh);

  const int e0[]   = {0, 1, 2, 3, 4, 5};
  const int e90[]  = {3, 0, 4, 1, 5, 2};   // linke Quellspalte (0,3) wird oberste Zeile: 3 0
  const int e180[] = {5, 4, 3, 2, 1, 0};
  const int e270[] = {2, 5, 1, 4, 0, 3};
  cam_rotate_px2(s, 3, 2, d, 0);   rot_expect(d, e0, 6);
  cam_rotate_px2(s, 3, 2, d, 90);  rot_expect(d, e90, 6);
  cam_rotate_px2(s, 3, 2, d, 180); rot_expect(d, e180, 6);
  cam_rotate_px2(s, 3, 2, d, 270); rot_expect(d, e270, 6);
}

// Referenz: die bisherige Drehung (Pixel für Pixel mit switch)
static void ref_rotate_px2(const uint8_t *src, int w, int h, uint8_t *dst, int rot_cw) {
  int ow, oh;
  cam_rot_dims(w, h, rot_cw, &ow, &oh);
  for (int y = 0; y < oh; y++) {
    for (int x = 0; x < ow; x++) {
      int sx, sy;
      switch (rot_cw) {
        case 90:  sx = y;         sy = h - 1 - x; break;
        case 180: sx = w - 1 - x; sy = h - 1 - y; break;
        case 270: sx = w - 1 - y; sy = x;         break;
        default:  sx = x;         sy = y;         break;
      }
      const uint8_t *p = src + 2 * ((long)sy * w + sx);
      uint8_t *q = dst + 2 * ((long)y * ow + x);
      q[0] = p[0]; q[1] = p[1];
    }
  }
}
void test_rotate_px2_matches_reference() {
  static uint16_t sbuf[320 * 240], rbuf[320 * 240], dbuf[320 * 240];   // uint16_t: 2-Byte-ausgerichtet wie die Kamerapuffer
  const int sizes[][2] = {{320, 240}, {240, 176}, {37, 23}, {1, 1}, {16, 16}, {17, 33}};
  const int rots[] = {0, 90, 180, 270, 45};   // 45 = ungültig = Kopie
  uint32_t s = 987654321u;
  for (const auto &sz : sizes) {
    const int w = sz[0], h = sz[1];
    for (int i = 0; i < w * h; i++) { s = s * 1664525u + 1013904223u; sbuf[i] = (uint16_t)(s >> 16); }
    for (int rot : rots) {
      memset(rbuf, 0xA5, sizeof rbuf); memset(dbuf, 0xA5, sizeof dbuf);
      ref_rotate_px2((const uint8_t *)sbuf, w, h, (uint8_t *)rbuf, rot);
      cam_rotate_px2((const uint8_t *)sbuf, w, h, (uint8_t *)dbuf, rot);
      char msg[40]; snprintf(msg, sizeof msg, "%dx%d rot %d", w, h, rot);
      TEST_ASSERT_EQUAL_MEMORY_MESSAGE(rbuf, dbuf, sizeof rbuf, msg);   // auch hinter dem Bildende unverändert
    }
  }
}
void test_stats_roundtrip_and_strict() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_stats(25, 12, 3, 80, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("S 25 12 3 80", (snprintf(g_buf, sizeof g_buf, "%.*s", (int)(n - 4), b), g_buf));
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_STATS, m.kind);
  TEST_ASSERT_EQUAL(25, m.fps); TEST_ASSERT_EQUAL(12, m.dps); TEST_ASSERT_EQUAL(3, m.rot_ms); TEST_ASSERT_EQUAL(80, m.det_ms);
  TEST_ASSERT_TRUE(parse_str(seal("S 0 9999 0 9999"), &m));
  TEST_ASSERT_EQUAL(9999, m.dps);
  const char *bad[] = {"S 1 2 3", "S 1 2 3 4 5", "S -1 0 0 0", "S 10000 0 0 0", "S 0 0 0 10000", "S  1 2 3 4", "S 1a 2 3 4", "S"};
  for (const char *s : bad) TEST_ASSERT_FALSE_MESSAGE(parse_str(seal(s), &m), s);
  // falsche Prüfsumme
  n = cam_build_stats(1, 2, 3, 4, b, sizeof b);
  b[n - 3] = (b[n - 3] == '0') ? '1' : '0';
  TEST_ASSERT_FALSE(cam_parse(b, n - 1, &m));
}

static SnapStep feed_str(SnapBuf &s, const char *t) {
  SnapStep r = SnapStep::MORE;
  for (; *t; t++) if ((r = snap_feed(s, *t)) != SnapStep::MORE) return r;
  return r;
}

static void test_snapbuf_end_lines() {
  char mem[64];
  SnapBuf s{mem, sizeof mem};
  TEST_ASSERT_EQUAL(SnapStep::MORE, (int)feed_str(s, "H 0.1.5 run*00\nJ B 5*00\n"));
  TEST_ASSERT_EQUAL(SnapStep::END, (int)feed_str(s, "J E*00\n"));
  SnapBuf x{mem, sizeof mem};
  TEST_ASSERT_EQUAL(SnapStep::END, (int)feed_str(x, "J X cam_off*00\n"));
  SnapBuf d{mem, sizeof mem};   // "J D" ist kein Ende
  TEST_ASSERT_EQUAL(SnapStep::MORE, (int)feed_str(d, "J D 0 AAAA*00\nJE\n"));
}

static void test_snapbuf_full_keeps_whole_lines() {
  char mem[16];
  SnapBuf s{mem, sizeof mem};
  TEST_ASSERT_EQUAL(SnapStep::MORE, (int)feed_str(s, "J B 99*00\n"));   // 10 Zeichen
  TEST_ASSERT_EQUAL(SnapStep::FULL, (int)feed_str(s, "J D 0 AAAAAA"));   // passt nicht mehr
  TEST_ASSERT_EQUAL(10, (int)s.len);   // halbe Zeile verworfen
  TEST_ASSERT_EQUAL_MEMORY("J B 99*00\n", mem, 10);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_snapbuf_end_lines);
  RUN_TEST(test_snapbuf_full_keeps_whole_lines);
  RUN_TEST(test_rotate_px2_matches_reference);
  RUN_TEST(test_stats_roundtrip_and_strict);
  RUN_TEST(test_rotate_px2);
  RUN_TEST(test_snap_cmd);
  RUN_TEST(test_xor_known);
  RUN_TEST(test_build_fixed_lines);
  RUN_TEST(test_roundtrip_face_and_heart);
  RUN_TEST(test_checksum_rules);
  RUN_TEST(test_rejects_garbage);
  RUN_TEST(test_length_limit);
  RUN_TEST(test_update_cmd_roundtrip);
  RUN_TEST(test_update_cmd_rejects);
  RUN_TEST(test_update_cmd_sha_case);
  RUN_TEST(test_update_cmd_worst_case);
  RUN_TEST(test_length_boundary);
  RUN_TEST(test_linebuf_boundary);
  RUN_TEST(test_update_progress);
  RUN_TEST(test_linebuf_split_and_cr);
  RUN_TEST(test_linebuf_overflow_recovers);
  RUN_TEST(test_box_center);
  RUN_TEST(test_box_clamped_and_degenerate);
  RUN_TEST(test_health_decision);
  RUN_TEST(test_motion_roundtrip_and_strict);
  return UNITY_END();
}
