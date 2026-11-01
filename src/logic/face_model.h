#pragma once
#include <stdint.h>
#include "eye_model.h"

// Gesicht = Augen + Pupillen + Brauen + Mund je Gefühl, weiche Übergänge. Reine Logik (Host-Test test/test_face_model).
// Soll-Werte: Liste E im Mockup docs/superpowers/mockups/2026-10-25-gesicht-ausdruecke.html.
enum class Mouth : uint8_t { CURVE, O, ZIG, WAVE, KISS, TONGUE };
struct FaceShape {
  EyeShape eye;                                    // w, h, radius, open, lid_top_tilt, lid_top_cut, lid_bottom_cut, gaze_x, gaze_y
  float pupil_r, pupil_dx, pupil_dy;
  float brow_y, brow_tilt, brow_curve, brow_asym;  // y relativ zur Augenoberkante (negativ = höher)
  float mouth_curve, mouth_open, mouth_w;
  Mouth mouth;
  bool wink;                                       // rechtes Auge zu
  float cross = 0;                                 // Schielen: beide Pupillen um so viele Pixel zur Nase hin
  float switch_acc = 0;                            // intern: aufgelaufener Tween-Anteil bis mouth/wink springen
};
struct FaceExpr { FaceShape s; Rgb color; };

namespace face_detail {
// Reglerwerte wie im Mockup (P()); Reihenfolge der Felder = Reihenfolge dort.
struct P {
  float open = 1, tilt = 0, topCut = 0, botCut = 0;
  float pdx = 0, pdy = 0, pr = 15, cross = 0;
  bool wink = false;
  float by = -12, bt = 0, bc = 0.6f, bAsym = 0;
  float mc = 0.6f, mo = 0, mw = 60;
  Mouth ms = Mouth::CURVE;
};
inline FaceExpr mk(Rgb col, const P &p) {
  FaceShape s{};
  s.eye = EyeShape{64, 72, 20, p.open, p.tilt, p.topCut, p.botCut, 0, 0};
  s.pupil_r = p.pr; s.pupil_dx = p.pdx; s.pupil_dy = p.pdy; s.cross = p.cross;
  s.brow_y = p.by; s.brow_tilt = p.bt; s.brow_curve = p.bc; s.brow_asym = p.bAsym;
  s.mouth_curve = p.mc; s.mouth_open = p.mo; s.mouth_w = p.mw;
  s.mouth = p.ms; s.wink = p.wink;
  return FaceExpr{s, col};
}
constexpr Rgb CYAN{0, 220, 255}, RED{255, 50, 40}, PINK{255, 80, 170}, BLUE{60, 120, 255}, YEL{255, 220, 0};
constexpr Rgb GREY{128, 110, 180}, DIM{0, 140, 170}, MINT{0, 255, 180}, SLEEPC{0, 90, 110};
constexpr Rgb SURPC{120, 240, 255}, SCAREC{180, 200, 255}, WHITE{255, 255, 255};
constexpr Rgb ORANGE{255, 150, 40}, FULLC{140, 200, 80}, SICKC{120, 170, 120}, TIMERC{255, 90, 140};   // Spec Interaktion
constexpr Rgb CLAUDEC{217, 119, 87};   // Claude-Orange (Spec Claude-Modus)
}  // namespace face_detail

inline FaceExpr face_expression(Emotion e) {
  using namespace face_detail;
  P p;
  switch (e) {
    case Emotion::HAPPY:     p.botCut = .45f; p.mc = 1; p.mo = .5f; p.mw = 70; p.by = -16; return mk(CYAN, p);
    case Emotion::BORED:     p.open = .55f; p.topCut = .35f; p.pdy = 6; p.mc = 0; p.mw = 44; p.by = -6; p.bc = 0; return mk(GREY, p);
    case Emotion::SAD:       p.open = .8f; p.tilt = .5f; p.topCut = .2f; p.pdy = 8; p.pr = 13; p.mc = -.8f; p.mw = 44; p.bt = 1; p.bc = .2f; return mk(BLUE, p);
    case Emotion::SLEEPY:    p.open = .35f; p.topCut = .5f; p.pdy = 8; p.mc = .1f; p.mw = 36; p.by = -4; p.bc = .2f; return mk(DIM, p);
    case Emotion::SLEEPING:  p.open = .06f; p.mc = .2f; p.mw = 28; p.by = -6; return mk(SLEEPC, p);
    case Emotion::SURPRISED: p.pr = 10; p.by = -26; p.bc = 1; p.ms = Mouth::O; p.mo = .7f; return mk(SURPC, p);
    case Emotion::LOVE:      p.botCut = .3f; p.pr = 18; p.mc = .9f; p.mw = 50; p.by = -16; return mk(PINK, p);
    case Emotion::ANNOYED:   p.open = .85f; p.tilt = -.6f; p.topCut = .2f; p.pr = 12; p.bt = -1.2f; p.bc = -.2f; p.by = -4; p.ms = Mouth::ZIG; p.mw = 50; return mk(RED, p);
    case Emotion::EXCITED:   p.pr = 17; p.by = -20; p.bc = .9f; p.mc = 1; p.mo = .8f; p.mw = 74; return mk(YEL, p);
    case Emotion::GREETING:  p.wink = true; p.botCut = .4f; p.mc = 1; p.mo = .4f; p.mw = 64; p.by = -16; return mk(MINT, p);
    case Emotion::CONFUSED:  p.pdx = -8; p.pdy = -6; p.bAsym = 12; p.bt = .3f; p.mw = 44; p.ms = Mouth::WAVE; return mk(CYAN, p);
    case Emotion::MISCHIEVOUS: p.wink = true; p.pdx = 6; p.bAsym = -8; p.mc = .8f; p.mw = 56; return mk(MINT, p);
    case Emotion::SCARED:    p.pr = 7; p.bt = 1.2f; p.by = -22; p.bc = .3f; p.ms = Mouth::WAVE; p.mw = 50; return mk(SCAREC, p);
    case Emotion::SKEPTICAL: p.open = .7f; p.topCut = .25f; p.pdx = 12; p.bAsym = 14; p.bt = -.4f; p.mc = -.2f; p.mw = 40; return mk(CYAN, p);
    case Emotion::PROUD:     p.open = .6f; p.topCut = .35f; p.botCut = .2f; p.pdy = -4; p.by = -14; p.mc = .9f; p.mw = 64; return mk(CYAN, p);
    case Emotion::GIGGLE:    p.open = .2f; p.botCut = .6f; p.mc = 1; p.mo = .6f; p.mw = 58; p.by = -14; return mk(YEL, p);
    case Emotion::YAWN:      p.open = .15f; p.topCut = .3f; p.by = -8; p.ms = Mouth::O; p.mo = 1.4f; return mk(DIM, p);
    case Emotion::WHISTLE:   p.pdx = -14; p.pdy = -10; p.by = -14; p.ms = Mouth::O; p.mo = 0; return mk(GREY, p);
    case Emotion::KISS:      p.open = .25f; p.botCut = .5f; p.by = -14; p.ms = Mouth::KISS; return mk(PINK, p);
    case Emotion::CHEEKY:    p.wink = true; p.mc = .6f; p.mw = 50; p.ms = Mouth::TONGUE; return mk(CYAN, p);
    case Emotion::STARTLED:  p.pr = 6; p.by = -28; p.bt = .6f; p.bc = 1; p.ms = Mouth::O; p.mo = 1.2f; return mk(WHITE, p);
    case Emotion::HUNGRY:    p.pdy = 8; p.ms = Mouth::WAVE; p.mw = 40; p.bt = .6f; return mk(ORANGE, p);
    case Emotion::FULL:      p.open = .6f; p.topCut = .3f; p.ms = Mouth::WAVE; return mk(FULLC, p);
    case Emotion::SICK:      p.open = .55f; p.tilt = .4f; p.mc = -.4f; return mk(SICKC, p);
    case Emotion::FOCUSED:   p.open = .7f; p.topCut = .25f; p.bt = -.3f; p.mc = 0; p.mw = 30; return mk(TIMERC, p);
    case Emotion::THINKING:  p.pdx = -10; p.pdy = -10; p.bAsym = 10; p.mc = .1f; p.mw = 30; return mk(CLAUDEC, p);   // Claude-Modus
    case Emotion::NEAR:      p.pr = 10; p.by = -26; p.bc = 1; p.ms = Mouth::O; p.mo = .7f; p.cross = 6; return mk(SURPC, p);   // Gesicht ganz nah: wie SURPRISED, aber schielend
    case Emotion::CONTENT:
    default:                return mk(CYAN, p);
  }
}

// Floats wie eye_tween. mouth/wink springen, sobald der aufgelaufene Tween-Anteil 0,5 erreicht (Hälfte des Übergangs).
inline FaceShape face_tween(const FaceShape &cur, const FaceShape &target, uint32_t dt_ms) {
  const float k = eye_tween_k(dt_ms);
  FaceShape r;
  r.eye = eye_tween(cur.eye, target.eye, dt_ms);
  r.pupil_r = eye_lerp(cur.pupil_r, target.pupil_r, k);
  r.pupil_dx = eye_lerp(cur.pupil_dx, target.pupil_dx, k);
  r.pupil_dy = eye_lerp(cur.pupil_dy, target.pupil_dy, k);
  r.cross = eye_lerp(cur.cross, target.cross, k);
  r.brow_y = eye_lerp(cur.brow_y, target.brow_y, k);
  r.brow_tilt = eye_lerp(cur.brow_tilt, target.brow_tilt, k);
  r.brow_curve = eye_lerp(cur.brow_curve, target.brow_curve, k);
  r.brow_asym = eye_lerp(cur.brow_asym, target.brow_asym, k);
  r.mouth_curve = eye_lerp(cur.mouth_curve, target.mouth_curve, k);
  r.mouth_open = eye_lerp(cur.mouth_open, target.mouth_open, k);
  r.mouth_w = eye_lerp(cur.mouth_w, target.mouth_w, k);
  r.mouth = cur.mouth;
  r.wink = cur.wink;
  r.switch_acc = 0;
  if (cur.mouth != target.mouth || cur.wink != target.wink) {
    const float acc = cur.switch_acc + k;
    if (acc >= 0.5f) { r.mouth = target.mouth; r.wink = target.wink; }
    else r.switch_acc = acc;
  }
  return r;
}

// eye_expression (eye_model.h) wird hier aus der Gesichtstabelle abgeleitet.
inline Expression eye_expression(Emotion e) {
  const FaceExpr f = face_expression(e);
  return Expression{f.s.eye, f.color};
}
