#pragma once
#include <stddef.h>

// Sammelt am CYD die Zeilen eines Schnappschusses, bis "J E"/"J X" kommt oder der Puffer voll ist.
enum class SnapStep { MORE, END, FULL };

struct SnapBuf {
  char *buf;
  size_t cap;
  size_t len = 0, line = 0;   // Füllstand, Beginn der aktuellen Zeile
};

// Ein Zeichen anhängen. END: Zeile "J E"/"J X" ist komplett. FULL: Zeichen passt nicht mehr,
// len ist auf das Ende der letzten vollständigen Zeile gekürzt (keine halbe Zeile ausgeben).
static inline SnapStep snap_feed(SnapBuf &s, char ch) {
  if (s.len >= s.cap) { s.len = s.line; return SnapStep::FULL; }
  s.buf[s.len++] = ch;
  if (ch != '\n') return SnapStep::MORE;
  const char *l = s.buf + s.line;
  const bool end = s.len - s.line >= 3 && l[0] == 'J' && l[1] == ' ' && (l[2] == 'E' || l[2] == 'X');
  s.line = s.len;
  return end ? SnapStep::END : SnapStep::MORE;
}
