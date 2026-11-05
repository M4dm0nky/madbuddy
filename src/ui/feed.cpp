#include "feed.h"
#include "icons.h"
#include "ui_common.h"

static constexpr uint32_t kTimeoutMs = 10000;
static constexpr UiRect kTiles[4] = {{10, 46, 145, 92}, {165, 46, 145, 92}, {10, 144, 145, 92}, {165, 144, 145, 92}};
static constexpr UiRect kClose = {270, 0, 50, 42};
static uint32_t s_last_input = 0;

void feed_open(LGFX &d, float satiety, uint32_t now_ms) {
  s_last_input = now_ms;
  d.startWrite();
  d.fillScreen(TFT_BLACK);
  // Sättigungsbalken (rein zur Freude, keine Pflicht)
  const uint32_t col = d.color888(60, 220, 120);
  const int x0 = 20, w = 240 - x0;
  d.drawRoundRect(x0, 12, w, 20, 8, TFT_WHITE);
  d.fillRoundRect(x0 + 3, 15, (int)((w - 6) * satiety / 100.0f), 14, 6, col);
  icon_close(d, kClose.x + kClose.w / 2, kClose.y + kClose.h / 2, 26, d.color888(255, 70, 70));
  for (int i = 0; i < 4; i++) {
    ui_tile(d, kTiles[i], d.color888(50, 40, 30));
    icon_food(d, kTiles[i].x + kTiles[i].w / 2, kTiles[i].y + kTiles[i].h / 2, 64, i);
  }
  d.endWrite();
}

int feed_touch(int16_t x, int16_t y, uint32_t now_ms) {
  s_last_input = now_ms;
  if (ui_hit(kClose, x, y)) return FEED_CLOSE;
  for (int i = 0; i < 4; i++) if (ui_hit(kTiles[i], x, y)) return i;
  return FEED_NONE;
}

bool feed_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
