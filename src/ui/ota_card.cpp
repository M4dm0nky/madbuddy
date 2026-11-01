#include "ota_card.h"
#include <stdio.h>
#include <string.h>
#include "icons.h"
#include "eyes.h"
#include "ui_common.h"
#include "../ota/ota_pull.h"
#include "../hw/cam_uart.h"
#include "../net/net.h"
#include "../logic/ota_policy.h"
#include "../logic/cam_link.h"

static constexpr uint32_t kIgnoreTapsMs = 1000;   // Tipps in der ersten Sekunde zählen nicht
static constexpr UiRect kYes = {30, 150, 120, 60};
static constexpr UiRect kLater = {170, 150, 120, 60};
// Zeilenansicht (Kamera-Angebot, ggf. zusammen mit dem CYD-Update): je Zeile ein Ja, unten Später
static constexpr UiRect kRowYes[2] = {{214, 52, 86, 48}, {214, 108, 86, 48}};
static constexpr UiRect kLaterRows = {95, 172, 130, 44};
static constexpr uint32_t kCamOkMs = 3000;        // „Kamera aktualisiert“ so lange zeigen
static constexpr uint32_t kBgCard = 0x141A28;     // wie color888(20, 26, 40)

enum CamPhase : uint8_t { CP_NONE, CP_RUN, CP_OK, CP_ERR };
static CamPhase s_cam_phase = CP_NONE;
static int s_cam_pct = 0;
static uint32_t s_cam_ok_ms = 0, s_cam_snooze = 0;
static char s_cam_err[40] = "";

static uint32_t s_shown_ms = 0;
static int s_drawn_state = -1;
static int s_drawn_pct = -1;
static bool s_consented = false;

static void cam_snooze(uint32_t now_ms) { s_cam_snooze = now_ms ? now_ms : 1; }

bool ota_card_cam_offer(uint32_t now_ms) {
  const ota_pull::CamInfo c = ota_pull::cam_info();
  return c.avail && ota_cam_newer(cam_link_version(cam_uart_link()), c.version) &&
         cam_link_conn(cam_uart_link(), now_ms, true) == CamConn::ON;
}

void ota_card_unsnooze() { s_cam_snooze = 0; }

static void cam_fail(const char *why, uint32_t now_ms) {
  strlcpy(s_cam_err, why, sizeof s_cam_err);
  s_cam_phase = CP_ERR;
  cam_snooze(now_ms);
}

void ota_card_cam_msg(const CamMsg &m, uint32_t now_ms) {
  if (s_cam_phase != CP_RUN) return;
  if (m.kind == CAM_UPD_PCT) {
    s_cam_pct = m.pct;
  } else if (m.kind == CAM_UPD_OK) {
    s_cam_phase = CP_OK;
    s_cam_ok_ms = now_ms;
    cam_snooze(now_ms);
  } else if (m.kind == CAM_UPD_ERR) {
    cam_uart_upd_end();
    cam_fail(m.text[0] ? m.text : "unbekannter Fehler", now_ms);
  }
}

// Tipp auf das Ja der Kamera-Zeile: Zeile mit dem WLAN-Zugang bauen, senden, Puffer löschen, Überwachung starten.
static void cam_start(uint32_t now_ms) {
  const ota_pull::CamInfo c = ota_pull::cam_info();
  char line[CAM_LINE_MAX + 4];
  const size_t n = c.avail ? net_build_cam_upd(c.url, c.sha, line, sizeof line) : 0;
  if (n) {
    cam_uart_write_sealed(line, n);
    cam_uart_upd_start(now_ms);
    s_cam_phase = CP_RUN;
    s_cam_pct = 0;
  } else {
    cam_fail("kein WLAN", now_ms);
  }
  volatile char *w = line;   // enthält das WLAN-Passwort
  for (size_t k = 0; k < sizeof line; k++) w[k] = 0;
}

bool ota_card_wanted(uint32_t now_ms, bool awake) {
  if (s_cam_phase == CP_RUN || s_cam_phase == CP_ERR) return true;
  if (s_cam_phase == CP_OK) {
    if (now_ms - s_cam_ok_ms < kCamOkMs) return true;
    s_cam_phase = CP_NONE;
  }
  if (ota_card_cam_offer(now_ms) && ota_notice_due(now_ms, s_cam_snooze, awake)) return true;
  const ota_pull::Info i = ota_pull::info();
  switch (i.state) {
    case ota_pull::AVAILABLE: return ota_notice_due(now_ms, i.snooze_set_ms, awake);
    case ota_pull::DOWNLOADING:
    case ota_pull::VERIFYING:
    case ota_pull::RESTARTING:
    case ota_pull::FAILED: return true;
    default: return false;
  }
}

void ota_card_open(uint32_t now_ms) {
  s_shown_ms = now_ms;
  s_drawn_state = -1;
  s_drawn_pct = -1;
  s_consented = false;
}

static void card_frame(LGFX &d, uint32_t border) {
  d.fillScreen(TFT_BLACK);
  d.fillRoundRect(10, 10, 300, 220, 18, d.color888(20, 26, 40));
  d.drawRoundRect(10, 10, 300, 220, 18, border);
}

static void draw_available(LGFX &d, const ota_pull::Info &i) {
  card_frame(d, d.color888(80, 200, 255));
  icon_update(d, 160, 50, 46, d.color888(80, 200, 255));
  ui_text(d, "Update bereit", 160, 94, &FONT_L, TFT_WHITE, middle_center);
  char buf[48];
  snprintf(buf, sizeof buf, "%s  (%u kB)", i.built, (unsigned)(i.size / 1024));
  ui_text(d, buf, 160, 122, &fonts::Font2, d.color888(190, 200, 220), middle_center);
  ui_tile(d, kYes, d.color888(30, 160, 70));
  ui_text(d, "Ja", kYes.x + kYes.w / 2, kYes.y + kYes.h / 2, &FONT_L, TFT_WHITE, middle_center);
  ui_tile(d, kLater, d.color888(80, 80, 100));
  ui_text(d, "Später", kLater.x + kLater.w / 2, kLater.y + kLater.h / 2, &FONT_L, TFT_WHITE, middle_center);
}

// Zeilenansicht: Zeile 0 = CYD (falls offen), dann Kamera; unten Später
static void draw_row(LGFX &d, int row, bool cam, const char *l1, const char *l2) {
  const int y = kRowYes[row].y;
  if (cam) icon_eye(d, 36, y + 24, 14, d.color888(80, 200, 255));
  else icon_update(d, 36, y + 24, 30, d.color888(80, 200, 255));
  ui_text(d, l1, 70, y + 12, &FONT_M, TFT_WHITE, middle_left);
  ui_text(d, l2, 70, y + 36, &fonts::Font2, d.color888(190, 200, 220), middle_left);
  ui_tile(d, kRowYes[row], d.color888(30, 160, 70));
  ui_text(d, "Ja", kRowYes[row].x + kRowYes[row].w / 2, y + 24, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_list(LGFX &d, bool cyd, const ota_pull::Info &i) {
  card_frame(d, d.color888(80, 200, 255));
  int row = 0;
  char buf[48];
  if (cyd) {
    snprintf(buf, sizeof buf, "%.23s", i.built);
    draw_row(d, row++, false, "MadBuddy", buf);
  }
  const ota_pull::CamInfo c = ota_pull::cam_info();
  snprintf(buf, sizeof buf, "%.15s -> %.15s", cam_link_version(cam_uart_link()), c.version);
  draw_row(d, row, true, "Kamera", buf);
  ui_tile(d, kLaterRows, d.color888(80, 80, 100));
  ui_text(d, "Später", kLaterRows.x + kLaterRows.w / 2, kLaterRows.y + kLaterRows.h / 2, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_cam_state(LGFX &d) {
  const bool ok = s_cam_phase == CP_OK, err = s_cam_phase == CP_ERR;
  card_frame(d, err ? d.color888(255, 70, 70) : ok ? d.color888(60, 220, 110) : d.color888(80, 200, 255));
  if (err) icon_close(d, 160, 54, 46, d.color888(255, 70, 70));
  else icon_eye(d, 160, 54, 24, ok ? d.color888(60, 220, 110) : d.color888(80, 200, 255));
  if (ok) {
    ui_text(d, "Kamera aktualisiert", 160, 120, &FONT_M, d.color888(120, 240, 150), middle_center);
  } else if (err) {
    ui_text(d, "Kamera-Update", 160, 104, &FONT_M, TFT_WHITE, middle_center);
    ui_text(d, s_cam_err, 160, 140, &FONT_S, d.color888(255, 120, 120), middle_center);
    ui_text(d, "Antippen schließt", 160, 196, &FONT_S, d.color888(170, 170, 190), middle_center);
  } else {
    ui_text(d, "Kamera-Update", 160, 104, &FONT_M, TFT_WHITE, middle_center);
  }
}

static void draw_cam_progress(LGFX &d, int pct) {
  d.drawRoundRect(30, 180, 260, 26, 8, TFT_WHITE);
  d.fillRect(33, 183, 254, 20, kBgCard);
  d.fillRect(33, 183, 254 * pct / 100, 20, d.color888(0, 200, 255));
  char buf[16];
  snprintf(buf, sizeof buf, "%d %%", pct);
  d.fillRect(100, 132, 120, 36, kBgCard);
  ui_text(d, buf, 160, 150, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_waiting(LGFX &d, const char *txt) {
  d.fillScreen(TFT_BLACK);
  EyeShape e = eye_expression(Emotion::CONTENT).shape;
  e.h = 34;              // schmal = konzentriert
  e.radius = 12;
  e.lid_top_cut = 0.25f;
  eyes_draw_pair(d, 160, 80, e, Rgb{0, 220, 255}, 1.0f);
  ui_text(d, txt, 160, 150, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_progress(LGFX &d, int pct) {
  d.drawRoundRect(30, 180, 260, 26, 8, TFT_WHITE);
  d.fillRect(33, 183, 254, 20, TFT_BLACK);
  d.fillRect(33, 183, 254 * pct / 100, 20, d.color888(0, 200, 255));
  char buf[16];
  snprintf(buf, sizeof buf, "%d %%", pct);
  d.fillRect(100, 130, 120, 40, TFT_BLACK);
  ui_text(d, buf, 160, 150, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_failed(LGFX &d, const ota_pull::Info &i) {
  card_frame(d, d.color888(255, 70, 70));
  icon_close(d, 160, 54, 46, d.color888(255, 70, 70));
  ui_text(d, "Update fehlgeschlagen", 160, 104, &FONT_M, TFT_WHITE, middle_center);
  ui_text(d, i.error, 160, 140, &FONT_S, d.color888(255, 180, 180), middle_center);
  ui_text(d, "Antippen schließt", 160, 196, &FONT_S, d.color888(170, 170, 190), middle_center);
}

void ota_card_draw(LGFX &lcd, uint32_t now_ms) {
  if (s_cam_phase == CP_RUN && cam_uart_upd_timed_out(now_ms)) {   // Watchdog: der XIAO schweigt
    cam_uart_upd_end();
    cam_fail("Kamera antwortet nicht", now_ms);
  }
  const ota_pull::Info i = ota_pull::info();
  const bool cyd_row = i.state == ota_pull::AVAILABLE && !s_consented;
  const bool list = s_cam_phase == CP_NONE && ota_card_list((int)i.state, s_consented, ota_card_cam_offer(now_ms));
  int key, pct = -1;
  if (s_cam_phase != CP_NONE) {
    key = 1000 + s_cam_phase;
    if (s_cam_phase == CP_RUN) pct = s_cam_pct;
  } else {
    key = (int)i.state + (list ? 100 : 0);
    if (i.state == ota_pull::DOWNLOADING) pct = i.pct;
  }
  if (key == s_drawn_state && pct == s_drawn_pct) return;
  lcd.startWrite();
  if (key != s_drawn_state) {
    s_drawn_state = key;
    s_drawn_pct = -1;
    if (s_cam_phase != CP_NONE) {
      draw_cam_state(lcd);
    } else if (list) {
      draw_list(lcd, cyd_row, i);
    } else {
      switch (i.state) {
        case ota_pull::AVAILABLE:
          if (s_consented) draw_waiting(lcd, "Starte ...");
          else draw_available(lcd, i);
          break;
        case ota_pull::DOWNLOADING: draw_waiting(lcd, ""); break;
        case ota_pull::VERIFYING: draw_waiting(lcd, "Prüfe die Datei ..."); break;
        case ota_pull::RESTARTING: draw_waiting(lcd, "Neustart ..."); break;
        case ota_pull::FAILED: draw_failed(lcd, i); break;
        default: break;
      }
    }
  }
  if (pct >= 0 && pct != s_drawn_pct) {
    s_drawn_pct = pct;
    if (s_cam_phase == CP_RUN) draw_cam_progress(lcd, pct);
    else draw_progress(lcd, pct);
  }
  lcd.endWrite();
}

void ota_card_touch(int16_t x, int16_t y, uint32_t now_ms) {
  if (now_ms - s_shown_ms < kIgnoreTapsMs) return;
  if (s_cam_phase == CP_ERR) { s_cam_phase = CP_NONE; return; }   // Fehlerkarte schließen
  if (s_cam_phase != CP_NONE) return;
  const ota_pull::Info i = ota_pull::info();
  const bool cyd = i.state == ota_pull::AVAILABLE && !s_consented;
  if (ota_card_list((int)i.state, s_consented, ota_card_cam_offer(now_ms))) {   // Zeilenansicht (nur bei ruhigem CYD-Zustand)
    if (cyd && ui_hit(kRowYes[0], x, y)) {
      s_consented = true;
      s_drawn_state = -1;   // „Starte …“ zeichnen
      ota_pull::consent();
    } else if (ui_hit(kRowYes[cyd ? 1 : 0], x, y)) {
      cam_start(now_ms);
      s_drawn_state = -1;
    } else {
      ota_pull::snooze();
      cam_snooze(now_ms);
    }
    return;
  }
  if (i.state == ota_pull::AVAILABLE && !s_consented) {
    if (ui_hit(kYes, x, y)) {
      s_consented = true;
      s_drawn_state = -1;   // „Starte …“ zeichnen
      ota_pull::consent();
    } else {
      ota_pull::snooze();
    }
  } else if (i.state == ota_pull::FAILED) {
    ota_pull::dismiss_failure();
  }
}
