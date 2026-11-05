#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Unveränderte Bilder nicht neu zeichnen/senden (SPI ist der Engpass). Reine Logik, ohne Hardware.

constexpr uint32_t FRAME_MAX_AGE_MS = 1000;   // spätestens jede Sekunde ein volles Bild
constexpr uint32_t FRAME_GAP_MS = 150;        // Pause zwischen zwei Aufrufen = Bildschirm war weg -> sofort neu

// FNV-1a über Rohbytes: Signatur aller Eingaben, die das Bild bestimmen.
struct FrameHash {
  uint32_t h = 2166136261u;
  void add(const void *p, size_t n) {
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 16777619u; }
  }
  template <typename T> void add(const T &v) { add(&v, sizeof v); }   // nur für Werte ohne Padding (float, int, uint8_t)
  void add_str(const char *s) { if (s) add(s, strlen(s)); h ^= 0xA5u; h *= 16777619u; }
};

// true = Bild neu zeichnen und senden.
inline bool frame_should_push(uint32_t sig, uint32_t last_sig, uint32_t now, uint32_t last_push, bool have_last) {
  if (!have_last) return true;
  if (sig != last_sig) return true;
  return now - last_push >= FRAME_MAX_AGE_MS;   // ohne Vorzeichenfehler bei millis()-Überlauf
}

// Bildschirm war zwischendurch weg (Menü, Spiel, OTA ...): letzter Aufruf zu lange her.
inline bool frame_gap_exceeded(uint32_t now, uint32_t last_call) { return now - last_call > FRAME_GAP_MS; }
