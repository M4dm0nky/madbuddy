#include "eyes.h"
#include <Arduino.h>
#include <math.h>
#include "../logic/weather_mood.h"
#include "bubble.h"
#include "icons.h"

static LGFX_Sprite s_spr;
static bool s_ready = false;
static bool s_hand_right = false;
static uint32_t s_hand_since = 0;

static float s_ring = -1;
static Rgb s_ring_col{255, 90, 140};
void eyes_set_ring(float progress, Rgb color) {
  s_ring = progress;
  s_ring_col = color;
}

static bool s_spin = false;
static Rgb s_spin_col{217, 119, 87};
void eyes_set_spin(bool on, Rgb color) {
  s_spin = on;
  s_spin_col = color;
}

// Ring-Geometrie: Ellipse am Rand (rx 153, ry 96), 64 Stuetzpunkte einmal berechnet, im Uhrzeigersinn ab oben.
// Gezeichnet wird als Kette kurzer Linien: keine Trigonometrie und kein Bogen-Scan pro Bild
// (fillEllipseArc prueft je Bild jedes Pixel der Ellipse und kostete rund 2 fps).
static constexpr int kRingN = 64;
static int16_t s_ring_x[kRingN + 1], s_ring_y[kRingN + 1];
static bool s_ring_ready = false;
static void ring_init() {
  if (s_ring_ready) return;
  for (int i = 0; i <= kRingN; i++) {
    const float a = (270.0f + 360.0f * i / kRingN) * 0.0174533f;
    s_ring_x[i] = (int16_t)lroundf(160 + 153.0f * cosf(a));
    s_ring_y[i] = (int16_t)lroundf(100 + 96.0f * sinf(a));
  }
  s_ring_ready = true;
}
static void ring_pos(float u, float *x, float *y) {   // u in Stuetzpunkt-Einheiten 0..64 (auch darueber: umlaufend)
  while (u >= kRingN) u -= kRingN;
  const int i = (int)u;
  const float f = u - i;
  *x = s_ring_x[i] + (s_ring_x[i + 1] - s_ring_x[i]) * f;
  *y = s_ring_y[i] + (s_ring_y[i + 1] - s_ring_y[i]) * f;
}
// Bogen von u0 bis u1 (u1 > u0, hoechstens ein Umlauf) als Kette von Linien, Dicke 2r.
static void ring_arc(LovyanGFX &d, float u0, float u1, float r, uint32_t col) {
  float px, py;
  ring_pos(u0, &px, &py);
  float u = u0;
  while (u < u1) {
    float nu = floorf(u) + 1.0f;
    if (nu > u1) nu = u1;
    float nx, ny;
    ring_pos(nu, &nx, &ny);
    d.drawWideLine(px, py, nx, ny, r, col);
    px = nx; py = ny; u = nu;
  }
}

static int s_food = -1, s_food_y = 0;
void eyes_set_food(int food, int y) {
  s_food = food;
  s_food_y = y;
}

void eyes_set_hand(bool right, uint32_t since_ms) {
  s_hand_right = right;
  s_hand_since = since_ms;
}

static constexpr int kEyeDx = 70;   // Abstand der Augenmitten von der Bildmitte

bool eyes_begin(LGFX &lcd) {
  if (s_ready) return true;
  s_spr.setPsram(false);
  s_spr.setColorDepth(8);   // RGB332
  s_ready = s_spr.createSprite(EYES_W, EYES_H) != nullptr;
  if (!s_ready) Serial.printf("eyes: kein Speicher fuer Sprite (frei %u), zeichne direkt\n", (unsigned)ESP.getFreeHeap());
  (void)lcd;
  return s_ready;
}

void eyes_free() {
  if (!s_ready) return;
  s_spr.deleteSprite();
  s_ready = false;
}

bool eyes_ready() { return s_ready; }

static uint32_t rgb(Rgb c) { return lgfx::color888(c.r, c.g, c.b); }

static Rgb mix(Rgb a, Rgb b, float k) {
  return Rgb{(uint8_t)(a.r + (b.r - a.r) * k), (uint8_t)(a.g + (b.g - a.g) * k), (uint8_t)(a.b + (b.b - a.b) * k)};
}

// Ein Auge. mirror = linkes Auge (äußere Ecke links).
static void draw_eye(LovyanGFX &d, int cx, int cy, const EyeShape &e, uint32_t col, bool mirror, float sc) {
  const float w = e.w * sc, h = e.h * sc;
  float hh = h * e.open;
  if (hh < 3) hh = 3;
  const int x0 = (int)(cx - w / 2), top = (int)(cy - hh / 2);
  const int iw = (int)w, ih = (int)hh;
  float r = e.radius * sc;
  if (r > hh / 2) r = hh / 2;
  if (r > w / 2) r = w / 2;
  d.fillRoundRect(x0, top, iw, ih, (int)r, col);

  // Oberes Lid: schwarzes Viereck, deckt lid_top_cut*h von oben ab, geneigt um lid_top_tilt*h*0.5.
  const float cut = e.lid_top_cut * hh, tilt = e.lid_top_tilt * h * 0.5f;
  if (cut > 0.5f || fabsf(tilt) > 0.5f) {
    const int xi = mirror ? x0 + iw + 1 : x0 - 1;   // innere Ecke (zur Nase)
    const int xo = mirror ? x0 - 1 : x0 + iw + 1;   // äußere Ecke
    const int yi = (int)(top + cut - tilt / 2);
    const int yo = (int)(top + cut + tilt / 2);
    const int yt = top - 2;
    d.fillTriangle(xi, yt, xo, yt, xo, yo, TFT_BLACK);
    d.fillTriangle(xi, yt, xo, yo, xi, yi, TFT_BLACK);
  }
  // Unteres Lid („^^“-Lächeln): schwarze Ellipse von unten, deckt lid_bottom_cut*h ab.
  if (e.lid_bottom_cut > 0.01f) {
    const int rx = (int)(w * 0.85f), ry = (int)(w * 0.55f);
    const int c = (int)(e.lid_bottom_cut * hh);
    d.fillEllipse(cx, top + ih + ry - c, rx, ry, TFT_BLACK);
  }
}

void eyes_draw_pair(LovyanGFX &d, int cx, int cy, const EyeShape &shape, Rgb color, float scale) {
  const uint32_t col = rgb(color);
  const int dx = (int)(kEyeDx * scale);
  const int gx = (int)(shape.gaze_x * scale), gy = (int)(shape.gaze_y * scale);
  draw_eye(d, cx - dx + gx, cy + gy, shape, col, true, scale);
  draw_eye(d, cx + dx + gx, cy + gy, shape, col, false, scale);
}

static void heart(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 4;
  d.fillCircle(cx - r, cy - r / 2, r, col);
  d.fillCircle(cx + r, cy - r / 2, r, col);
  d.fillTriangle(cx - 2 * r, cy - r / 4, cx + 2 * r, cy - r / 4, cx, cy + s / 2, col);
}

static void drop(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 3;
  d.fillCircle(cx, cy + r, r, col);
  d.fillTriangle(cx - r, cy + r, cx + r, cy + r, cx, cy - s / 2, col);
}

static void letter_z(LovyanGFX &d, int x, int y, int s, uint32_t col) {
  const float t = s / 8.0f + 0.5f;
  d.drawWideLine(x, y, x + s, y, t, col);
  d.drawWideLine(x + s, y, x, y + s, t, col);
  d.drawWideLine(x, y + s, x + s, y + s, t, col);
}

// ---- Gesicht (Zeichenvorschrift = face() im Mockup 2026-10-25-gesicht-ausdruecke.html) ----
static constexpr int kFaceL = 90, kFaceR = 230, kFaceEY = 96, kFaceW = 64, kFaceH = 72;
static constexpr int kNoseY = 146, kMouthY = 172;
static constexpr int kBezN = 12;   // Segmente je Kurve

static inline int ir(float v) { return (int)lroundf(v); }

// Quadratische Bezier-Kurve als Kette dicker Linien (r = Radius, Dicke = 2r).
static void qbez_line(LovyanGFX &d, float x0, float y0, float cx, float cy, float x1, float y1, int n, float r, uint32_t col) {
  float px = x0, py = y0;
  for (int i = 1; i <= n; i++) {
    const float t = (float)i / n, u = 1.0f - t;
    const float x = u * u * x0 + 2 * u * t * cx + t * t * x1;
    const float y = u * u * y0 + 2 * u * t * cy + t * t * y1;
    d.drawWideLine(px, py, x, y, r, col);
    px = x; py = y;
  }
}

static void face_eye(LovyanGFX &d, const FaceShape &f, int side, int ox, int oy, uint32_t col, uint32_t pupil_col) {
  const EyeShape &e = f.eye;
  const int cx = (side < 0 ? kFaceL : kFaceR) + ox, ey = kFaceEY + oy;
  const float W = e.w, H = e.h;
  float h = H * e.open * ((f.wink && side > 0) ? 0.08f : 1.0f);
  if (h < 4) h = 4;
  const float y0 = ey - h / 2, x0 = cx - W / 2;
  float rad = e.radius;
  if (rad > h / 2) rad = h / 2;
  d.fillRoundRect(ir(x0), ir(y0), ir(W), ir(h), ir(rad), col);
  if (h > 20) {
    const float pr = f.pupil_r;
    const int px = ir(cx + f.pupil_dx - side * f.cross), py = ir(ey + f.pupil_dy);
    d.fillCircle(px, py, ir(pr), pupil_col);
    d.fillCircle(ir(px + pr * 0.35f), ir(py - pr * 0.4f), ir(fmaxf(2.0f, pr * 0.3f)), TFT_WHITE);
  }
  // oberes Lid (schwarz); innen = Richtung Gesichtsmitte
  const float inner = side < 0 ? x0 + W : x0, outer = side < 0 ? x0 : x0 + W;
  const float cutIn = y0 + h * e.lid_top_cut - e.lid_top_tilt * H * 0.35f;
  const float cutOut = y0 + h * e.lid_top_cut + e.lid_top_tilt * H * 0.35f;
  if (e.lid_top_cut > 0.001f || fabsf(e.lid_top_tilt) > 0.001f) {
    const int yt = ir(y0) - 2;
    d.fillTriangle(ir(inner), yt, ir(outer), yt, ir(outer), ir(cutOut), TFT_BLACK);
    d.fillTriangle(ir(inner), yt, ir(outer), ir(cutOut), ir(inner), ir(cutIn), TFT_BLACK);
  }
  // unteres Lid
  if (e.lid_bottom_cut > 0.0f)
    d.fillEllipse(cx, ir(y0 + h + H * 0.32f), ir(W * 0.72f), ir(H * e.lid_bottom_cut * 0.9f + 6), TFT_BLACK);
  // Braue
  const float byy = y0 + f.brow_y - (side > 0 ? f.brow_asym : 0.0f);
  const float ti = f.brow_tilt * 10.0f;
  const float yIn = byy - ti, yOut = byy + ti;
  const float ya = side < 0 ? yOut : yIn, yb = side < 0 ? yIn : yOut;
  qbez_line(d, cx - 28.0f, ya, (float)cx, (ya + yb) / 2 - f.brow_curve * 12.0f, cx + 28.0f, yb, kBezN, 3.5f, col);
}

static void face_mouth(LovyanGFX &d, const FaceShape &f, int ox, int oy, uint32_t col) {
  const int cx = 160 + ox, my = kMouthY + oy;
  const float w = f.mouth_w / 2;
  switch (f.mouth) {
    case Mouth::O: {
      const int rx = ir(8 + f.mouth_open * 6), ry = ir(8 + f.mouth_open * 10);
      d.fillEllipse(cx, my, rx + 3, ry + 3, col);
      d.fillEllipse(cx, my, rx - 3, ry - 3, TFT_BLACK);
      break;
    }
    case Mouth::ZIG: {
      float x = cx - w, y = (float)my;
      for (int i = 0; i < 4; i++) {
        const float nx = x + w / 2, ny = (i & 1) ? y + 7 : y - 7;
        d.drawWideLine(x, y, nx, ny, 3.0f, col);
        d.fillCircle(ir(nx), ir(ny), 3, col);
        x = nx; y = ny;
      }
      break;
    }
    case Mouth::WAVE:
      qbez_line(d, cx - w, (float)my, cx - w / 2, my - 10.0f, (float)cx, (float)my, 8, 3.0f, col);
      qbez_line(d, (float)cx, (float)my, cx + w / 2, my + 10.0f, cx + w, (float)my, 8, 3.0f, col);
      break;
    case Mouth::KISS:
      qbez_line(d, cx - 6.0f, my - 8.0f, cx + 4.0f, my - 4.0f, cx - 6.0f, (float)my, 8, 2.5f, col);
      qbez_line(d, cx - 6.0f, (float)my, cx + 4.0f, my + 4.0f, cx - 6.0f, my + 8.0f, 8, 2.5f, col);
      break;
    case Mouth::CURVE:
    case Mouth::TONGUE:
    default: {
      const float c = f.mouth_curve * 22.0f;
      const float xl = cx - w, xr = cx + w;
      if (f.mouth_open > 0.0f && f.mouth != Mouth::TONGUE) {
        // Fläche zwischen oberer und unterer Kurve (gleiche x(t)) als Streifen
        const float c2 = c + f.mouth_open * 26.0f;
        float pxx = xl, pyt = (float)my, pyb = (float)my;
        for (int i = 1; i <= kBezN; i++) {
          const float t = (float)i / kBezN, u = 1.0f - t;
          const float x = u * u * xl + 2 * u * t * cx + t * t * xr;
          const float yt = u * u * my + 2 * u * t * (my + c) + t * t * my;
          const float yb = u * u * my + 2 * u * t * (my + c2) + t * t * my;
          d.fillTriangle(ir(pxx), ir(pyt), ir(x), ir(yt), ir(pxx), ir(pyb), col);
          d.fillTriangle(ir(x), ir(yt), ir(x), ir(yb), ir(pxx), ir(pyb), col);
          pxx = x; pyt = yt; pyb = yb;
        }
      }
      // Oberlinie immer (bei kleinem mouth_open fließt die Form so ohne Sprung aus der Linie)
      qbez_line(d, xl, (float)my, (float)cx, my + c, xr, (float)my, kBezN, 3.0f, col);
      if (f.mouth == Mouth::TONGUE) {
        const uint32_t pink = lgfx::color888(255, 92, 122);
        const float x0 = 168.0f + ox, y0 = 178.0f + oy;   // q8 18 18 0: Kontrollpunkt (+8,+18), Ende (+18,0)
        float px = x0, py = y0;
        for (int i = 1; i <= 8; i++) {
          const float t = i / 8.0f, u = 1.0f - t;
          const float x = u * u * x0 + 2 * u * t * (x0 + 8) + t * t * (x0 + 18);
          const float y = u * u * y0 + 2 * u * t * (y0 + 18) + t * t * y0;
          d.fillTriangle(ir(x0), ir(y0), ir(px), ir(py), ir(x), ir(y), pink);
          px = x; py = y;
        }
      }
      break;
    }
  }
}

// Fragezeichen aus Primitiven; (x, y) = Mitte des Bogens.
static void question_mark(LovyanGFX &d, int x, int y, float u, uint32_t col) {
  const float r = 9.0f * u;
  float px = 0, py = 0;
  bool first = true;
  for (int i = 0; i <= 10; i++) {
    const float a = (170.0f + (440.0f - 170.0f) * i / 10.0f) * 0.0174533f;
    const float qx = x + r * cosf(a), qy = y + r * sinf(a);
    if (!first) d.drawWideLine(px, py, qx, qy, 2.6f * u, col);
    px = qx; py = qy; first = false;
  }
  d.drawWideLine(px, py, (float)x, y + 17.0f * u, 2.6f * u, col);
  d.fillCircle(x, y + 26 * u, ir(3.2f * u), col);
}

// Achtelnote; (x, y) = Mitte des Kopfes.
static void music_note(LovyanGFX &d, int x, int y, uint32_t col) {
  d.fillEllipse(x, y, 7, 5, col);
  d.drawWideLine(x + 6.0f, (float)y, x + 6.0f, y - 26.0f, 1.8f, col);
  d.drawWideLine(x + 6.0f, y - 26.0f, x + 16.0f, y - 17.0f, 2.0f, col);
  d.drawWideLine(x + 16.0f, y - 17.0f, x + 14.0f, y - 9.0f, 2.0f, col);
}

// Offene Hand (Handfläche + 5 Finger) in Augenfarbe; (cx, cy) = Mitte der Handfläche, u = Größe (1 = ~56 px hoch).
static void hand(LovyanGFX &d, int cx, int cy, float u, bool right, uint32_t col) {
  const int pw = ir(30 * u), ph = ir(26 * u);
  d.fillRoundRect(cx - pw / 2, cy - ph / 2, pw, ph, ir(9 * u), col);
  const int fw = ir(7 * u);
  static const int8_t kDx[4] = {-11, -4, 3, 10};   // Zeige- bis kleiner Finger
  static const int8_t kLen[4] = {20, 24, 22, 17};
  for (int i = 0; i < 4; i++) {
    const int fx = cx + ir(kDx[i] * u) - fw / 2, fl = ir(kLen[i] * u);
    d.fillRoundRect(fx, cy - ph / 2 - fl + ir(4 * u), fw, fl, fw / 2, col);
  }
  // Daumen schräg zur Bildmitte hin
  const float s = right ? -1.0f : 1.0f;
  d.drawWideLine(cx + s * 12 * u, cy + 4 * u, cx + s * 24 * u, cy - 8 * u, 3.6f * u, col);
}

static void render(LovyanGFX &d, const FaceShape &f, Rgb color, uint32_t ov, uint32_t now) {
  d.fillRect(0, 0, EYES_W, EYES_H, TFT_BLACK);
  Rgb c = color;
  if (ov & OV_RAIN_TINT) c = mix(c, Rgb{60, 120, 255}, 0.3f);
  const uint32_t col = rgb(c);
  const uint32_t pupil_col = lgfx::color888((uint8_t)(c.r / 10), (uint8_t)(c.g / 10), (uint8_t)(c.b / 10));
  const int shiver = (ov & OV_SHIVER) ? (((now / 40) & 1) ? 2 : -2) : 0;
  const int ox = (int)f.eye.gaze_x + shiver, oy = (int)f.eye.gaze_y;

  face_eye(d, f, -1, ox, oy, col, pupil_col);
  face_eye(d, f, 1, ox, oy, col, pupil_col);
  d.fillCircle(160 + ox, kNoseY + oy, 4, col);
  face_mouth(d, f, ox, oy, col);

  const int lx = kFaceL + ox, rx = kFaceR + ox, ey = kFaceEY + oy;
  const int eh = (int)(f.eye.h * f.eye.open);

  if (ov & OV_SUNGLASSES) {
    const uint32_t lens = lgfx::color888(40, 40, 48), frame = lgfx::color888(120, 120, 130);
    // Gläser mittig auf dem Auge (vorher saßen sie über der Augenoberkante und wirkten zu hoch, Nutzer am Gerät)
    const int lw = (int)f.eye.w + 16, lh = (int)(f.eye.h * 0.7f);
    const int ly = ey - lh / 2 + 2;
    for (int x : {lx, rx}) {
      // Getöntes Glas: Schachbrett-Raster (jedes zweite Pixel), damit Augen und Pupillen durchscheinen.
      // Das 8-Bit-Sprite kann keine echte Transparenz; ohne sichtbare Pupillen ginge viel Ausdruck verloren (Nutzer).
      const int x0 = x - lw / 2;
      for (int yy = ly + 2; yy < ly + lh - 2; yy++)
        for (int xx = x0 + 2 + (yy & 1); xx < x0 + lw - 2; xx += 2) d.drawPixel(xx, yy, lens);
      d.drawRoundRect(x - lw / 2, ly, lw, lh, 10, frame);
      d.drawRoundRect(x - lw / 2 + 1, ly + 1, lw - 2, lh - 2, 9, frame);
    }
    d.drawWideLine(lx + lw / 2, ly + 8, rx - lw / 2, ly + 8, 2.0f, frame);
  }
  if (ov & OV_HEART) {
    const int s = 22 + (int)(4 * sinf((now % 1257) / 200.0f));   // 1257 ms ~ Periode 2*pi*200; float ab 2^24 ms zu grob
    const uint32_t pink = lgfx::color888(255, 80, 170);
    heart(d, 160 + ox, 28 + oy, s, pink);
    if (f.mouth == Mouth::KISS) heart(d, 205 + ox, 158 + oy, 14, pink);
  }
  if (ov & OV_SWEAT) {
    const float ph = (now % 2000) / 2000.0f;
    drop(d, rx + (int)f.eye.w / 2 + 10, 25 + (int)(ph * 40), 14, lgfx::color888(140, 210, 255));
  }
  if (ov & OV_TEAR) {
    const float ph = (now % 3000) / 3000.0f;
    drop(d, lx - (int)f.eye.w / 4, ey + eh / 2 + 6 + (int)(ph * 30), 12, lgfx::color888(100, 170, 255));
  }
  if (ov & OV_QUESTION) {
    const int bob = (int)(3 * sinf((now % 2199) / 350.0f));   // 2199 ms ~ Periode 2*pi*350
    question_mark(d, 270, 34 + bob, 1.0f, col);
  }
  if (ov & OV_NOTE) {
    const int dy = (int)(4 * sinf((now % 1885) / 300.0f));   // 1885 ms ~ Periode 2*pi*300
    music_note(d, 212, 168 + dy, col);
  }
  if (s_ring >= 0 || s_spin) {   // Pomodoro-Bogen bzw. Claude-Segment: um das Gesicht, im Uhrzeigersinn ab oben
    ring_init();
    const Rgb rc = s_ring >= 0 ? s_ring_col : s_spin_col;
    const uint32_t track = lgfx::color888(rc.r / 5, rc.g / 5, rc.b / 5);
    for (int i = 0; i < kRingN; i++) {   // Spur: 2 Pixel dick aus zwei versetzten Linien
      d.drawLine(s_ring_x[i], s_ring_y[i], s_ring_x[i + 1], s_ring_y[i + 1], track);
      d.drawLine(s_ring_x[i] + 1, s_ring_y[i] + 1, s_ring_x[i + 1] + 1, s_ring_y[i + 1] + 1, track);
    }
    if (s_ring >= 0) {
      const float p = s_ring > 1 ? 1 : s_ring;
      if (p > 0.002f) ring_arc(d, 0, kRingN * p, 2.0f, rgb(s_ring_col));
    } else {
      const float u0 = kRingN * (float)(now % 1500) / 1500.0f;
      ring_arc(d, u0, u0 + kRingN / 6.0f, 2.0f, rgb(s_spin_col));
    }
  }
  if (ov & OV_TYPING) {   // Claude schreibt: drei Punkte unter dem Mund, der aktive wandert
    const int active = (int)((now / 200) % 3);
    const uint32_t oc = lgfx::color888(217, 119, 87);
    for (int i = 0; i < 3; i++) d.fillCircle(160 + ox + (i - 1) * 18, 190 + oy, i == active ? 5 : 3, oc);
  }
  if (ov & OV_SNEEZE) {   // Hatschi: Tropfen an der Nase
    drop(d, 160 + ox + 8, kNoseY + oy + 14, 12, lgfx::color888(140, 210, 255));
  }
  if (s_food >= 0) icon_food(d, 160 + ox, s_food_y, 40, s_food);
  if (ov & OV_HAND) {
    const uint32_t t = now - s_hand_since;
    const float u = t < 150 ? 0.6f + 0.4f * t / 150.0f : 1.0f;   // ploppt kurz auf
    hand(d, s_hand_right ? 282 : 38, 38, u, s_hand_right, col);
  }
  if (ov & OV_ZZZ) {
    const uint32_t zc = lgfx::color888(150, 170, 255);
    for (int i = 0; i < 3; i++) {
      const float ph = fmodf((now % 3000) / 3000.0f + i / 3.0f, 1.0f);   // jedes Z steigt in 3 s auf
      const int s = 10 + (int)(ph * 14);
      letter_z(d, 250 + (int)(ph * 30), 80 - (int)(ph * 70), s, zc);
    }
  }
}

void eyes_draw(LGFX &lcd, const FaceShape &face, Rgb color, uint32_t overlays, uint32_t now_ms, const char *bubble) {
  if (s_ready) {
    render(s_spr, face, color, overlays, now_ms);
    if (bubble) bubble_draw(s_spr, bubble);
    s_spr.pushSprite(&lcd, 0, 0);
  } else {
    lcd.startWrite();
    render(lcd, face, color, overlays, now_ms);
    if (bubble) bubble_draw(lcd, bubble);
    lcd.endWrite();
  }
}
