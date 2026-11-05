#pragma once
#include <stddef.h>

// Puffer sicher löschen (Passwörter): volatile-Schleife, damit der Compiler sie nicht wegoptimiert.
static inline void secure_zero(void *p, size_t n) {
  volatile unsigned char *v = (volatile unsigned char *)p;
  for (size_t i = 0; i < n; i++) v[i] = 0;
}
