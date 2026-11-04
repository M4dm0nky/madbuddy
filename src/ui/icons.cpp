#include "icons.h"
#include <math.h>

static constexpr float kPi = 3.14159265f;

void icon_play(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s / 2, w = s * 7 / 16;
  d.fillTriangle(cx - w + s / 10, cy - h, cx - w + s / 10, cy + h, cx + w + s / 10, cy, col);
}

void icon_clock(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.drawWideLine(cx, cy, cx, cy - r * 6 / 10, t * 0.6f, col);       // Minutenzeiger
  d.drawWideLine(cx, cy, cx + r * 45 / 100, cy, t * 0.6f, col);     // Stundenzeiger
  d.fillCircle(cx, cy, t, col);
}

void icon_gear(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 34 / 100, hole = s * 14 / 100;
  const float tooth = s * 0.09f;
  for (int i = 0; i < 8; i++) {
    const float a = i * kPi / 4;
    d.drawWideLine(cx + (int)(cosf(a) * r * 0.6f), cy + (int)(sinf(a) * r * 0.6f),
                   cx + (int)(cosf(a) * s * 0.48f), cy + (int)(sinf(a) * s * 0.48f), tooth, col);
  }
  d.fillArc(cx, cy, hole, r, 0, 360, col);   // Ring statt Loch: Hintergrundfarbe egal
}

void icon_close(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s * 36 / 100;
  const float t = s / 14.0f + 1;
  d.drawWideLine(cx - h, cy - h, cx + h, cy + h, t, col);
  d.drawWideLine(cx - h, cy + h, cx + h, cy - h, t, col);
}

void icon_wifi(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int base = cy + s * 4 / 10, step = s * 26 / 100, t = s / 10 + 1;
  for (int i = 1; i <= 3; i++) {
    const int r = step * i;
    d.fillArc(cx, base, r - t, r, 225, 315, col);
  }
  d.fillCircle(cx, base - t / 2, t, col);
}

void icon_sun(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 22 / 100;
  d.fillCircle(cx, cy, r, col);
  for (int i = 0; i < 8; i++) {
    const float a = i * kPi / 4;
    d.drawWideLine(cx + (int)(cosf(a) * r * 1.45f), cy + (int)(sinf(a) * r * 1.45f),
                   cx + (int)(cosf(a) * s * 0.48f), cy + (int)(sinf(a) * s * 0.48f), s / 20.0f + 0.5f, col);
  }
}

// Sichel: gelber Kreis, schwarzer Kreis versetzt darüber (Symbole stehen immer auf schwarzem Grund).
static void moon(LovyanGFX &d, int cx, int cy, int s) {
  const int r = s * 36 / 100;
  d.fillCircle(cx, cy, r, lgfx::color888(240, 230, 150));
  d.fillCircle(cx + r * 55 / 100, cy - r * 25 / 100, r * 85 / 100, TFT_BLACK);
}

static void cloud(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 6;
  d.fillCircle(cx - r, cy, r, col);
  d.fillCircle(cx + r / 2, cy - r / 2, r * 13 / 10, col);
  d.fillCircle(cx + r * 3 / 2, cy + r / 4, r * 3 / 4, col);
  d.fillRoundRect(cx - r * 2, cy, r * 4 + r / 4, r, r / 2, col);
}

void icon_sky(LovyanGFX &d, int cx, int cy, int s, Sky sky, bool is_day) {
  const uint32_t grey = lgfx::color888(170, 180, 195), dark = lgfx::color888(110, 115, 130);
  switch (sky) {
    case Sky::SUNNY:
      if (is_day) icon_sun(d, cx, cy, s, lgfx::color888(255, 210, 0));
      else moon(d, cx, cy, s);
      break;
    case Sky::CLOUDY:
      if (is_day) icon_sun(d, cx + s / 5, cy - s / 5, s * 6 / 10, lgfx::color888(255, 210, 0));
      cloud(d, cx, cy + s / 10, s, grey);
      break;
    case Sky::FOG:
      for (int i = 0; i < 3; i++)
        d.drawWideLine(cx - s * 4 / 10 + (i & 1) * s / 10, cy - s / 4 + i * s / 4, cx + s * 4 / 10, cy - s / 4 + i * s / 4,
                       s / 16.0f + 0.5f, grey);
      break;
    case Sky::RAIN:
      cloud(d, cx, cy - s / 8, s, grey);
      for (int i = -1; i <= 1; i++)
        d.drawWideLine(cx + i * s / 5, cy + s / 6, cx + i * s / 5 - s / 12, cy + s * 4 / 10, s / 24.0f + 0.5f,
                       lgfx::color888(80, 150, 255));
      break;
    case Sky::SNOW:
      cloud(d, cx, cy - s / 8, s, grey);
      for (int i = -1; i <= 1; i++) d.fillCircle(cx + i * s / 5, cy + s / 4 + (i & 1) * s / 8, s / 18 + 1, TFT_WHITE);
      break;
    case Sky::STORM:
      cloud(d, cx, cy - s / 8, s, dark);
      d.fillTriangle(cx + s / 12, cy, cx - s / 8, cy + s / 4, cx + s / 30, cy + s / 4, lgfx::color888(255, 220, 0));
      d.fillTriangle(cx - s / 30, cy + s / 5, cx + s / 8, cy + s / 5, cx - s / 12, cy + s / 2, lgfx::color888(255, 220, 0));
      break;
  }
}

void icon_update(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.drawWideLine(cx, cy - r * 55 / 100, cx, cy + r * 15 / 100, t * 0.7f, col);
  d.fillTriangle(cx - r * 4 / 10, cy + r * 5 / 100, cx + r * 4 / 10, cy + r * 5 / 100, cx, cy + r * 55 / 100, col);
}

void icon_info(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.fillCircle(cx, cy - r * 45 / 100, t, col);
  d.fillRoundRect(cx - t, cy - r * 18 / 100, 2 * t, r * 70 / 100, t / 2, col);
}

void icon_target(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 4 / 10, t = s / 14 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.fillArc(cx, cy, r / 2 - t, r / 2, 0, 360, col);
  d.drawWideLine(cx - s / 2, cy, cx + s / 2, cy, t / 2.0f, col);
  d.drawWideLine(cx, cy - s / 2, cx, cy + s / 2, t / 2.0f, col);
}

void icon_simon(LovyanGFX &d, int cx, int cy, int s) {
  const int r = s / 2;
  d.fillArc(cx, cy, r / 3, r, 180 + 3, 270 - 3, lgfx::color888(255, 60, 60));
  d.fillArc(cx, cy, r / 3, r, 270 + 3, 360 - 3, lgfx::color888(60, 220, 90));
  d.fillArc(cx, cy, r / 3, r, 90 + 3, 180 - 3, lgfx::color888(70, 120, 255));
  d.fillArc(cx, cy, r / 3, r, 0 + 3, 90 - 3, lgfx::color888(255, 220, 0));
}

void icon_brightness(LovyanGFX &d, int cx, int cy, int s, uint8_t level, uint32_t col) {
  const int bw = s / 5, gap = s / 10;
  const uint32_t off = lgfx::color888(60, 60, 70);
  for (int i = 0; i < 3; i++) {
    const int h = s * (i + 1) / 3;
    const int x = cx - s / 2 + i * (bw + gap) + gap / 2;
    d.fillRect(x, cy + s / 2 - h, bw, h, i <= level ? col : off);
  }
}

void icon_back(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s * 4 / 10;
  const float t = s / 10.0f + 1;
  d.drawWideLine(cx - h, cy, cx + h, cy, t, col);
  d.fillTriangle(cx - h - s / 10, cy, cx - h + s / 4, cy - s / 3, cx - h + s / 4, cy + s / 3, col);
}

void icon_heart(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 4;
  d.fillCircle(cx - r, cy - r / 2, r, col);
  d.fillCircle(cx + r, cy - r / 2, r, col);
  d.fillTriangle(cx - 2 * r, cy - r / 4, cx + 2 * r, cy - r / 4, cx, cy + s / 2, col);
}

void icon_bolt(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s / 2;
  d.fillTriangle(cx + s / 8, cy - h, cx - s / 4, cy + s / 10, cx + s / 20, cy + s / 10, col);
  d.fillTriangle(cx - s / 20, cy - s / 10, cx + s / 4, cy - s / 10, cx - s / 8, cy + h, col);
}

void icon_apple(LovyanGFX &d, int cx, int cy, int s) {
  const int r = s * 22 / 100;
  const uint32_t red = lgfx::color888(230, 40, 40);
  d.fillCircle(cx - r * 6 / 10, cy + s / 12, r, red);
  d.fillCircle(cx + r * 6 / 10, cy + s / 12, r, red);
  d.fillCircle(cx, cy + s / 5, r * 9 / 10, red);
  d.drawWideLine(cx, cy - s / 8, cx + s / 16, cy - s * 3 / 8, s / 28.0f + 0.8f, lgfx::color888(120, 70, 30));
  d.fillEllipse(cx + s / 7, cy - s * 3 / 10, s / 8, s / 16, lgfx::color888(60, 190, 70));
}

void icon_hourglass(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int w = s * 36 / 100, h = s * 44 / 100, t = s / 16 + 1;
  d.fillRect(cx - w - t, cy - h - t, 2 * (w + t), t, col);   // Deckel
  d.fillRect(cx - w - t, cy + h, 2 * (w + t), t, col);       // Boden
  d.fillTriangle(cx - w, cy - h, cx + w, cy - h, cx, cy, col);
  d.fillTriangle(cx - w * 6 / 10, cy + h, cx + w * 6 / 10, cy + h, cx, cy + h * 3 / 10, col);
  d.drawLine(cx - w, cy + h, cx, cy, col);
  d.drawLine(cx + w, cy + h, cx, cy, col);
}

static void pizza(LovyanGFX &d, int cx, int cy, int s) {
  const int h = s * 42 / 100;
  d.fillTriangle(cx - h, cy - h * 7 / 10, cx + h, cy - h * 7 / 10, cx, cy + h, lgfx::color888(250, 200, 70));   // Käse
  d.fillRoundRect(cx - h - 2, cy - h * 7 / 10 - s / 10, 2 * h + 4, s / 7, s / 16, lgfx::color888(190, 110, 40));  // Rand
  const uint32_t pep = lgfx::color888(200, 40, 30);
  d.fillCircle(cx - h / 3, cy - h / 5, s / 14, pep);
  d.fillCircle(cx + h / 3, cy - h / 4, s / 14, pep);
  d.fillCircle(cx, cy + h / 3, s / 16, pep);
}

static void cake(LovyanGFX &d, int cx, int cy, int s) {
  const int w = s * 40 / 100, h = s * 30 / 100;
  d.fillRect(cx - w, cy - h / 3, 2 * w, h + h / 3, lgfx::color888(240, 200, 150));            // Teig
  d.fillRect(cx - w, cy + h / 6, 2 * w, h / 5, lgfx::color888(230, 90, 130));                   // Füllung
  d.fillRoundRect(cx - w, cy - h * 2 / 3, 2 * w, h / 2, h / 5, lgfx::color888(255, 245, 250));  // Sahne
  d.fillCircle(cx, cy - h * 3 / 4, s / 11, lgfx::color888(220, 30, 50));                       // Kirsche
}

static void coffee(LovyanGFX &d, int cx, int cy, int s) {
  const int w = s * 30 / 100, h = s * 34 / 100;
  const uint32_t cup = lgfx::color888(240, 240, 245);
  d.fillRoundRect(cx - w, cy - h / 2, 2 * w, h + h / 3, s / 10, cup);
  d.fillArc(cx + w, cy, s / 12, s / 6, 270, 450, cup);                       // Henkel
  d.fillRect(cx - w + 3, cy - h / 2 + 2, 2 * w - 6, s / 12, lgfx::color888(110, 60, 30));   // Kaffee
  for (int i = -1; i <= 1; i += 2)                                           // Dampf
    d.drawWideLine(cx + i * s / 8, cy - h / 2 - s / 14, cx + i * s / 8 + s / 20, cy - h / 2 - s / 4, s / 40.0f + 0.7f,
                   lgfx::color888(200, 200, 210));
}

void icon_food(LovyanGFX &d, int cx, int cy, int s, int food) {
  switch (food) {
    case 0: icon_apple(d, cx, cy, s); break;
    case 1: pizza(d, cx, cy, s); break;
    case 2: cake(d, cx, cy, s); break;
    default: coffee(d, cx, cy, s); break;
  }
}

void icon_thermometer(LovyanGFX &d, int cx, int cy, int s) {
  const int w = s / 8 + 1, h = s * 36 / 100;
  const uint32_t glass = lgfx::color888(230, 230, 240), red = lgfx::color888(230, 40, 40);
  d.fillRoundRect(cx - w, cy - h, 2 * w, 2 * h, w, glass);
  d.fillCircle(cx, cy + h, w * 2, glass);
  d.fillRect(cx - w / 2, cy - h / 3, w, h + h / 3, red);
  d.fillCircle(cx, cy + h, w * 3 / 2, red);
}

void icon_ball(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 4 / 10;
  d.fillCircle(cx, cy, r, col);
  d.fillArc(cx, cy, r - 2, r - 2 + s / 14 + 1, 200, 340, TFT_BLACK);
  d.fillArc(cx, cy, r - 2, r - 2 + s / 14 + 1, 20, 160, TFT_BLACK);
}

void icon_pillow(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int w = s * 44 / 100, h = s * 26 / 100;
  d.fillRoundRect(cx - w, cy - h, 2 * w, 2 * h, h, col);
  d.fillCircle(cx - w, cy - h, s / 12 + 1, col);
  d.fillCircle(cx + w, cy - h, s / 12 + 1, col);
  d.fillCircle(cx - w, cy + h, s / 12 + 1, col);
  d.fillCircle(cx + w, cy + h, s / 12 + 1, col);
}

void icon_stop(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s * 32 / 100;
  d.fillRoundRect(cx - h, cy - h, 2 * h, 2 * h, s / 12, col);
}

// Claude-Funke: vierzackiger Stern mit vier kurzen Zwischenstrahlen (Claude-Orange vom Aufrufer).
void icon_claude(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const float lo = s * 0.5f, sh = s * 0.32f, w = s / 12.0f + 0.9f;
  for (int i = 0; i < 8; i++) {
    const float a = i * kPi / 4.0f, len = (i & 1) ? sh : lo;
    d.drawWideLine(cx, cy, cx + len * cosf(a), cy + len * sinf(a), w, col);
  }
  d.fillCircle(cx, cy, s / 8, col);
}

// Helfer (Subagent): kleines Männchen aus Kopf und Körper.
void icon_helper(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  d.fillCircle(cx, cy - s * 3 / 10, s / 5, col);
  d.fillRoundRect(cx - s * 3 / 10, cy - s / 10, s * 3 / 5, s * 3 / 5, s / 5, col);
}

// Auge: Mandel aus einer Ellipse mit schwarzem Inneren, Iris als Kreis, Pupille dunkel (ca. 2s x 1,2s).
void icon_eye(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int rx = s, ry = s * 6 / 10;
  const int t = s / 6 + 1;   // Randstärke
  d.fillEllipse(cx, cy, rx, ry, col);
  d.fillEllipse(cx, cy, rx - t, ry - t, TFT_BLACK);
  d.fillCircle(cx, cy, s * 4 / 10, col);
  d.fillCircle(cx, cy, s * 17 / 100 + 1, TFT_BLACK);
}

// Pfeil als gefülltes Dreieck nach rechts bzw. links.
void icon_arrow(LovyanGFX &d, int cx, int cy, int s, uint32_t col, bool right) {
  const int h = s / 2, w = s * 4 / 10;
  if (right) d.fillTriangle(cx + w, cy, cx - w, cy - h, cx - w, cy + h, col);
  else d.fillTriangle(cx - w, cy, cx + w, cy - h, cx + w, cy + h, col);
}

// Ein Z: obere Linie, Diagonale, untere Linie (links oben = x, y, Kantenlänge z, Strichdicke t).
static void zz_letter(LovyanGFX &d, float x, float y, float z, float t, uint32_t col) {
  d.drawWideLine(x, y, x + z, y, t, col);
  d.drawWideLine(x + z, y, x, y + z, t, col);
  d.drawWideLine(x, y + z, x + z, y + z, t, col);
}
// Müde: großes Z unten links, kleines z oben rechts.
static void icon_zz(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const float big = s * 0.5f, small = s * 0.32f, t = s / 11.0f + 0.5f;
  zz_letter(d, cx - s * 0.45f, cy - s * 0.05f, big, t, col);
  zz_letter(d, cx + s * 0.1f, cy - s * 0.42f, small, t, col);
}

void icon_need(LovyanGFX &d, int need, int cx, int cy, int s) {
  switch (need) {
    case 0: icon_thermometer(d, cx, cy, s); break;
    case 1: icon_apple(d, cx, cy, s); break;
    case 2: icon_zz(d, cx, cy, s, lgfx::color888(170, 180, 255)); break;
    case 3: icon_heart(d, cx, cy, s, lgfx::color888(255, 80, 170)); break;
    case 4: icon_ball(d, cx, cy, s, lgfx::color888(80, 200, 255)); break;
    default: break;
  }
}
