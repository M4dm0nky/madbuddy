#include "status_bar.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "icons.h"
#include "fonts.h"

static StatusInfo s_last;
static bool s_valid = false;

static bool same(const StatusInfo &a, const StatusInfo &b) {
  if (a.time_ok != b.time_ok || a.wifi_ok != b.wifi_ok || a.portal != b.portal || strcmp(a.portal_pass, b.portal_pass) != 0 || a.weather_ok != b.weather_ok ||
      a.update_ready != b.update_ready || a.timer_s != b.timer_s || a.timer_break != b.timer_break ||
      a.claude_on != b.claude_on || a.helpers != b.helpers || a.cam != b.cam)
    return false;
  if (a.time_ok && (a.hour != b.hour || a.minute != b.minute)) return false;
  if (a.weather_ok && (a.sky != b.sky || a.is_day != b.is_day || lroundf(a.temp_c) != lroundf(b.temp_c))) return false;
  return true;
}

void status_invalidate() { s_valid = false; }

void status_draw(LGFX &lcd, const StatusInfo &s) {
  if (s_valid && same(s, s_last)) return;
  s_last = s;
  s_valid = true;

  const int cy = STATUS_Y + STATUS_H / 2;
  lcd.startWrite();
  lcd.fillRect(0, STATUS_Y, 320, STATUS_H, TFT_BLACK);
  lcd.drawFastHLine(0, STATUS_Y, 320, lcd.color888(30, 40, 60));

  // WLAN rechts: grün verbunden, rot ohne, gelb wenn das Portal offen ist
  const uint32_t wcol = s.wifi_ok ? lcd.color888(60, 220, 120) : (s.portal ? lcd.color888(255, 200, 0) : lcd.color888(255, 60, 60));
  icon_wifi(lcd, 300, cy, 26, wcol);
  if (s.update_ready) icon_update(lcd, 266, cy, 24, lcd.color888(80, 200, 255));

  lcd.setTextDatum(middle_left);
  if (s.portal && !s.wifi_ok) {
    lcd.setFont(&FONT_M);
    lcd.setTextColor(lcd.color888(255, 200, 0), TFT_BLACK);
    char pw[40];
    snprintf(pw, sizeof pw, "Passwort: %s", s.portal_pass);
    lcd.drawString("WLAN: MadBuddy", 8, cy - 10);
    lcd.drawString(pw, 8, cy + 10);
    lcd.endWrite();
    return;
  }

  char buf[16];
  lcd.setFont(&FONT_M);
  lcd.setTextColor(lcd.color888(200, 220, 255), TFT_BLACK);
  if (s.time_ok) snprintf(buf, sizeof buf, "%02d:%02d", s.hour, s.minute);
  else snprintf(buf, sizeof buf, "--:--");
  lcd.drawString(buf, 8, cy);

  if (s.claude_on) {   // Claude-Funke (Lücke zwischen Uhrzeit und Wetter), daneben Helfer-Symbol + Zahl
    const uint32_t oc = lcd.color888(217, 119, 87);
    icon_claude(lcd, 82, cy, 20, oc);
    if (s.helpers) {
      icon_helper(lcd, 104, cy, 16, oc);
      snprintf(buf, sizeof buf, "%u", (unsigned)(s.helpers > 9 ? 9 : s.helpers));
      lcd.setFont(&FONT_S);
      lcd.setTextColor(oc, TFT_BLACK);
      lcd.drawString(buf, 113, cy);
      lcd.setFont(&FONT_M);
    }
  }
  if (s.weather_ok) {
    icon_sky(lcd, 138, cy, 30, s.sky, s.is_day);
    snprintf(buf, sizeof buf, "%ld°", lroundf(s.temp_c));
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(buf, 160, cy);
  }
  if (s.cam) {   // Kamera-Auge in der Lücke zwischen Wetter-Temperatur (endet ~185) und Pomodoro-Text (ab 206)
    const uint32_t ec = s.cam == 2 ? lcd.color888(60, 220, 120) : (s.cam == 3 ? lcd.color888(255, 60, 60) : lcd.color888(130, 130, 140));
    icon_eye(lcd, 194, cy, 8, ec);
  }
  if (s.timer_s) {   // Pomodoro-Restzeit mm:ss
    snprintf(buf, sizeof buf, "%u:%02u", (unsigned)(s.timer_s / 60), (unsigned)(s.timer_s % 60));
    lcd.setFont(&FONT_S);
    lcd.setTextColor(s.timer_break ? lcd.color888(60, 220, 120) : lcd.color888(255, 90, 140), TFT_BLACK);
    lcd.drawString(buf, 206, cy);
  }
  lcd.endWrite();
}
