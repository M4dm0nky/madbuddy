#include "settings.h"
#include <Arduino.h>
#include <stdio.h>
#include "icons.h"
#include "ui_common.h"
#include "../hw/light.h"
#include "../net/net.h"
#include "../net/claude_api.h"

static constexpr uint32_t kTimeoutMs = 30000;
static constexpr int kRowH = 52, kRowY0 = 32;
static constexpr UiRect kClose = {268, 0, 52, 44};
static UiRect row(int i) { return UiRect{0, (int16_t)(kRowY0 + i * kRowH), 320, kRowH}; }
// Seitenpfeil in der (nicht berührbaren) Zeile 3: Seite 1 rechts (x 268..320), Seite 2 links (x 0..52); Trefferfläche 52 x 52
static UiRect arrow_rect(int page) { return UiRect{(int16_t)(page == 0 ? 268 : 0), (int16_t)row(3).y, 52, kRowH}; }

static uint32_t s_last_input = 0, s_last_tick = 0;
static int s_page = 0;   // 0 = Seite 1 (Helligkeit/WLAN/Firmware/Info), 1 = Seite 2 (Kamera)
static int (*s_fw)(char *, size_t, uint32_t) = nullptr;

void settings_firmware_source(int (*fn)(char *out, size_t n, uint32_t now_ms)) { s_fw = fn; }

static void row_bg(LGFX &d, int i) {
  const UiRect r = row(i);
  d.fillRect(r.x, r.y, r.w, r.h, TFT_BLACK);
  d.drawFastHLine(10, r.y + r.h - 1, 300, d.color888(40, 50, 70));
}

static void draw_brightness(LGFX &d, const Persist &p) {
  row_bg(d, 0);
  const int cy = row(0).y + kRowH / 2;
  icon_sun(d, 30, cy, 34, d.color888(255, 210, 0));
  icon_brightness(d, 100, cy, 36, p.brightness, d.color888(255, 210, 0));
}

// Jede Zeile merkt sich, was sie zuletzt gezeichnet hat (Schlüssel aus Text und Farbe), und zeichnet nur bei Änderung neu.
// force = Zeile wurde von außen gelöscht bzw. der Bildschirm neu aufgebaut.
static bool unchanged(char *cache, size_t n, const char *key, bool force) {
  if (!force && strcmp(cache, key) == 0) return true;
  strlcpy(cache, key, n);
  return false;
}

static void draw_wifi(LGFX &d, const Persist &p, bool force) {
  static char s_cache[80] = "";
  const bool ok = net_wifi_ok();
  char ptxt[40];
  const bool portal = net_portal_active();
  if (portal) snprintf(ptxt, sizeof ptxt, "MadBuddy  PW %s", net_portal_pass());   // Font S: passt in die Zeile
  const char *txt = portal ? ptxt : (p.place[0] ? p.place : "WLAN / Ort einrichten");
  char key[80];
  snprintf(key, sizeof key, "%d|%s", ok ? 1 : 0, txt);
  if (unchanged(s_cache, sizeof s_cache, key, force)) return;
  row_bg(d, 1);
  const int cy = row(1).y + kRowH / 2;
  const uint32_t col = ok ? d.color888(60, 220, 120) : d.color888(255, 60, 60);
  icon_wifi(d, 30, cy, 30, col);
  if (portal) ui_text(d, txt, 62, cy, &FONT_S, d.color888(255, 200, 0), middle_left);
  else ui_text(d, txt, 62, cy, &FONT_M, TFT_WHITE, middle_left);
}

static void draw_firmware(LGFX &d, uint32_t now_ms, bool force) {
  static char s_cache[64] = "";
  char buf[48] = "-";   // Strich bis OTA da ist
  int tone = 0;
  if (s_fw) tone = s_fw(buf, sizeof buf, now_ms);
  char key[64];
  snprintf(key, sizeof key, "%d|%s", tone, buf);
  if (unchanged(s_cache, sizeof s_cache, key, force)) return;
  row_bg(d, 2);
  const int cy = row(2).y + kRowH / 2;
  const uint32_t col = tone == 1 ? d.color888(60, 220, 120) : (tone == 2 ? d.color888(255, 80, 80) : d.color888(200, 200, 210));
  icon_update(d, 30, cy, 30, d.color888(80, 200, 255));
  ui_text(d, buf, 62, cy, &FONT_M, col, middle_left);
}

// Charakter-Balken: Mitte = 0, Ausschlag nach links (Symbol links) bzw. rechts (Symbol rechts) bis ±100.
// Die Balken sitzen links vom Seitenpfeil (x 268..320), darum schmal: Balken x 192..240, Symbole bei 180 und 252.
static void char_bar(LGFX &d, int y, float v, void (*left)(LGFX &, int, int), void (*right)(LGFX &, int, int), uint32_t col) {
  const int x0 = 192, x1 = 240, mid = (x0 + x1) / 2;
  left(d, x0 - 12, y);
  right(d, x1 + 12, y);
  d.fillRoundRect(x0, y - 3, x1 - x0, 7, 3, d.color888(50, 50, 65));
  const int px = mid + (int)((x1 - x0) / 2 * v / 100.0f);
  if (px != mid) d.fillRect(px < mid ? px : mid, y - 3, px < mid ? mid - px : px - mid, 7, col);
  d.drawFastVLine(mid, y - 5, 11, TFT_WHITE);
}

static void draw_arrow(LGFX &d, int page);

static void draw_info(LGFX &d, const Persist &p, bool force) {
  static char s_cache[128] = "";
  char buf[64];
  char key[128];
  snprintf(key, sizeof key, "%u|%d|%s|%d|%d", (unsigned)(ESP.getFreeHeap() / 1024), light_raw(), claude_api_key(),
           (int)p.cuddly, (int)p.playful);
  if (unchanged(s_cache, sizeof s_cache, key, force)) return;
  row_bg(d, 3);
  const int cy = row(3).y + kRowH / 2;
  icon_info(d, 30, cy, 30, d.color888(180, 180, 255));
  ui_text(d, __DATE__, 62, cy - 17, &fonts::Font2, d.color888(200, 200, 210), middle_left);
  // Texte in Font0 (6 px je Zeichen, höchstens 16 Zeichen = x 62..158), damit sie mit Abstand vor den Charakter-Balken
  // (Symbole ab x 173) enden; Seitenpfeil-Zone x 268..320 bleibt frei
  snprintf(buf, sizeof buf, "Heap %uk L %d", (unsigned)(ESP.getFreeHeap() / 1024), light_raw());
  ui_text(d, buf, 62, cy, &fonts::Font0, d.color888(200, 200, 210), middle_left);
  // Claude-Modus-Schlüssel für die PC-Hooks (12 Zeichen)
  snprintf(buf, sizeof buf, "Key %s", claude_api_key());
  ui_text(d, buf, 62, cy + 17, &fonts::Font0, d.color888(255, 160, 110), middle_left);
  // Charakter: verschmust (Herz) gegen frech (Blitz), verspielt (Ball) gegen faul (Kissen); Wert -100..100
  char_bar(d, cy - 10, -p.cuddly, [](LGFX &g, int x, int y) { icon_heart(g, x, y, 14, g.color888(255, 80, 170)); },
           [](LGFX &g, int x, int y) { icon_bolt(g, x, y, 16, g.color888(255, 220, 0)); }, d.color888(255, 80, 170));
  char_bar(d, cy + 12, -p.playful, [](LGFX &g, int x, int y) { icon_ball(g, x, y, 15, g.color888(80, 200, 255)); },
           [](LGFX &g, int x, int y) { icon_pillow(g, x, y, 16, g.color888(200, 190, 230)); }, d.color888(80, 200, 255));
  draw_arrow(d, 0);   // row_bg hat die Zeile gelöscht
}

// Seitenpfeil in Zeile 3 (Seite 1 → 2 rechts, Seite 2 → 1 links), violett wie das Zahnrad
static void draw_arrow(LGFX &d, int page) {
  const UiRect r = arrow_rect(page);
  icon_arrow(d, r.x + r.w / 2, r.y + r.h / 2, 26, d.color888(170, 110, 255), page == 0);
}

// Seite 2, Zeile 0: Kamera an/aus (Augen-Symbol, Schalter grün = an, grau = aus)
static void draw_cam(LGFX &d, const SettingsCam &c, bool force) {
  static char s_cache[8] = "";
  char key[8];
  snprintf(key, sizeof key, "%d", c.on ? 1 : 0);
  if (unchanged(s_cache, sizeof s_cache, key, force)) return;
  row_bg(d, 0);
  const int cy = row(0).y + kRowH / 2;
  icon_eye(d, 30, cy, 14, c.on ? d.color888(60, 220, 120) : d.color888(130, 130, 140));
  ui_text(d, "Kamera", 62, cy, &FONT_M, TFT_WHITE, middle_left);
  const int sx = 244, sw = 56, sh = 28;   // Schalter rechts: Pille mit Knopf
  d.fillRoundRect(sx, cy - sh / 2, sw, sh, sh / 2, c.on ? d.color888(40, 170, 90) : d.color888(70, 70, 80));
  d.fillCircle(c.on ? sx + sw - sh / 2 : sx + sh / 2, cy, sh / 2 - 3, TFT_WHITE);
}

// Seite 2, Zeile 1: Kamera-Info nach Verbindung (grün ok, gelb keine Verbindung, rot Fehler, grau aus)
static void draw_cam_info(LGFX &d, const SettingsCam &c, bool force) {
  static char s_cache[48] = "";
  char buf[40];
  uint32_t col;
  switch (c.conn) {
    case CamConn::ON:
      if (c.version && c.version[0]) snprintf(buf, sizeof buf, "XIAO %s verbunden", c.version);
      else strlcpy(buf, "XIAO verbunden", sizeof buf);
      col = d.color888(60, 220, 120);
      break;
    case CamConn::NONE: strlcpy(buf, "keine Verbindung", sizeof buf); col = d.color888(255, 200, 0); break;
    case CamConn::ERR: strlcpy(buf, "Kamera-Fehler", sizeof buf); col = d.color888(255, 60, 60); break;
    case CamConn::OFF: strlcpy(buf, "Kamera aus (XIAO)", sizeof buf); col = d.color888(160, 160, 170); break;
    case CamConn::OFF_USER:
    default: strlcpy(buf, "aus", sizeof buf); col = d.color888(160, 160, 170); break;
  }
  char key[48];
  snprintf(key, sizeof key, "%d|%s", (int)c.conn, buf);
  if (unchanged(s_cache, sizeof s_cache, key, force)) return;
  row_bg(d, 1);
  const int cy = row(1).y + kRowH / 2;
  icon_info(d, 30, cy, 30, col);
  ui_text(d, buf, 62, cy, &FONT_M, col, middle_left);
}

static void draw_page2(LGFX &d, const SettingsCam &c, bool force) {
  draw_cam(d, c, force);
  draw_cam_info(d, c, force);
  if (force) {
    row_bg(d, 2);
    row_bg(d, 3);
    draw_arrow(d, 1);
  }
}

void settings_redraw(LGFX &lcd, const Persist &p, const SettingsCam &cam) {
  lcd.startWrite();
  if (s_page == 0) {
    draw_brightness(lcd, p);
    draw_wifi(lcd, p, true);
    draw_firmware(lcd, millis(), true);
    draw_info(lcd, p, true);
  } else {
    draw_page2(lcd, cam, true);
  }
  lcd.endWrite();
}

void settings_open(LGFX &lcd, const Persist &p, uint32_t now_ms, const SettingsCam &cam) {
  s_last_input = s_last_tick = now_ms;
  s_page = 0;
  lcd.startWrite();
  lcd.fillScreen(TFT_BLACK);
  icon_gear(lcd, 20, 16, 24, lcd.color888(170, 110, 255));
  icon_close(lcd, kClose.x + kClose.w / 2, kClose.y + kClose.h / 2 - 4, 28, lcd.color888(255, 70, 70));
  lcd.endWrite();
  settings_redraw(lcd, p, cam);
}

SettingsAction settings_touch(int16_t x, int16_t y, uint32_t now_ms) {
  s_last_input = now_ms;
  if (ui_hit(kClose, x, y)) return SettingsAction::CLOSE;
  if (ui_hit(arrow_rect(s_page), x, y)) {
    s_page = 1 - s_page;
    return SettingsAction::PAGE;
  }
  if (s_page == 1) return ui_hit(row(0), x, y) ? SettingsAction::CAM_TOGGLE : SettingsAction::NONE;
  if (ui_hit(row(0), x, y)) return SettingsAction::BRIGHTNESS;
  if (ui_hit(row(1), x, y)) return SettingsAction::PORTAL;
  if (ui_hit(row(2), x, y)) return SettingsAction::OTA_CHECK;
  return SettingsAction::NONE;
}

void settings_tick(LGFX &lcd, const Persist &p, uint32_t now_ms, const SettingsCam &cam) {
  if (now_ms - s_last_tick < 1000) return;
  s_last_tick = now_ms;
  lcd.startWrite();
  if (s_page == 0) {
    draw_wifi(lcd, p, false);
    draw_firmware(lcd, now_ms, false);
    draw_info(lcd, p, false);
  } else {
    draw_page2(lcd, cam, false);
  }
  lcd.endWrite();
}

bool settings_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
