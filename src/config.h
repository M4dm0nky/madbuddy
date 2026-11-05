#pragma once
#include <stdint.h>

// ESP32-2432S028 (CYD2USB) Pins
// Display (HSPI)
constexpr int TFT_SCLK = 14;
constexpr int TFT_MOSI = 13;
constexpr int TFT_MISO = 12;
constexpr int TFT_CS   = 15;
constexpr int TFT_DC   = 2;
constexpr int TFT_RST  = -1;
constexpr int TFT_BL   = 21;  // Backlight (PWM)

// Touch XPT2046 (VSPI)
constexpr int TOUCH_CLK  = 25;
constexpr int TOUCH_MOSI = 32;
constexpr int TOUCH_MISO = 39;
constexpr int TOUCH_CS   = 33;
constexpr int TOUCH_IRQ  = 36;

// Touch-Kalibrierung. Gemessen 2026-10-25 (Querformat, setRotation(1)): Bildschirm-x kommt aus Roh-y und war gespiegelt
// (oben links meldete x=316, Roh-y 3668; oben rechts x=25, Roh-y 482) -> Roh-y-Bereich umgedreht.
// Bildschirm-y kommt aus Roh-x (oben ~3640, Mitte ~1870); unten lag Roh-x unter 200 und wurde verworfen -> X_MIN 0.
constexpr int TOUCH_RAW_X_MIN = 0;
constexpr int TOUCH_RAW_X_MAX = 3700;
constexpr int TOUCH_RAW_Y_MIN = 3700;
constexpr int TOUCH_RAW_Y_MAX = 200;

// Lichtsensor
constexpr int LDR_PIN = 34;
// Gemessen 2026-10-25: bei Raumlicht 0, nur in Dämmerung kurz ~200. Die Beschaltung des CYD ist bekannt schlecht
// (Teiler für 1 V ausgelegt, das Display leuchtet den Sensor an). Darum vorsichtig: "dunkel" erst über 400, "hell" unter 250.
// Geschlafen wird vor allem nach Uhrzeit + Ruhe (mood_night_check).
#define LDR_DARK_IS_HIGH 1                // höherer Rohwert = dunkler
constexpr int LDR_DARK_THRESHOLD = 325;
constexpr int LDR_HYSTERESIS = 75;

// RGB-LED (aktiv low)
constexpr int LED_R = 4;
constexpr int LED_G = 16;
constexpr int LED_B = 17;

// Kamera (XIAO ESP32-S3 Sense an CN1, docs/verkabelung.md)
constexpr int CAM_RX = 27;          // vom XIAO D6 (TX)
constexpr int CAM_TX = 22;          // zum XIAO D7 (RX)
// Roboter, gemessen 2026-11-03: Kamerabild ist seitenverkehrt (Schrift im Schnappschuss gespiegelt), daher kein
// zusätzliches Spiegeln. Sitzende Person bei y ≈ 300 (Kamera 15° nach oben), Gesichtsbreite normal ≈ 210, 20 cm ≈ 400.
constexpr bool CAM_MIRROR_X = false;  // Augen sollen zur Person schauen
constexpr int16_t CAM_GAZE_Y0 = 300;  // Bildhöhe 0..1000, die "geradeaus" bedeutet
constexpr int16_t CAM_NEAR_W = 350;   // Gesichtsbreite 0..1000, ab der er überrascht schielt (ca. 25 cm)
