#include "bubble.h"
#include "fonts.h"
#include "icons.h"

static constexpr int kTop = 2, kH = 30, kPad = 10, kTail = 8, kIconW = 24;

void bubble_draw(LovyanGFX &d, const char *utf8, int icon) {
  if (!utf8 || !utf8[0]) return;
  d.setFont(&FONT_M);
  const int extra = icon >= 0 ? kIconW : 0;
  int w = d.textWidth(utf8) + 2 * kPad + extra;
  if (w > 316) w = 316;
  const int x = 160 - w / 2;
  const uint32_t fill = lgfx::color888(235, 235, 240), edge = lgfx::color888(160, 160, 175);
  d.fillRoundRect(x, kTop, w, kH, 10, fill);
  d.drawRoundRect(x, kTop, w, kH, 10, edge);
  d.fillTriangle(152, kTop + kH - 1, 168, kTop + kH - 1, 160, kTop + kH + kTail, fill);   // Zipfel nach unten zum Gesicht
  d.setTextDatum(middle_center);
  d.setTextColor(TFT_BLACK);
  if (icon >= 0) {
    icon_need(d, icon, x + kPad + 10, kTop + kH / 2, 20);
    d.drawString(utf8, 160 + extra / 2, kTop + kH / 2 + 1);
  } else {
    d.drawString(utf8, 160, kTop + kH / 2 + 1);
  }
}
