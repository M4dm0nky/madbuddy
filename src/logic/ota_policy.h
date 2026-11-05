#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "deadline.h"

// Entscheidungen des Firmware-Updates per Abruf (src/ota/ota_pull.cpp). Reine Logik ohne Arduino-/LVGL-Includes (Host-Test test/test_ota_policy).
// Das Gerät holt die Datei selbst, aber nur nach Zustimmung am Display; was es bootet, prüft es am Image selbst, nicht an den Angaben im Manifest.

// App-Image einer Arduino-/IDF-Firmware: Kopf 24 Byte (Magic 0xE9, Chip-ID bei Byte 12), dann Segmentkopf (8 Byte), dann esp_app_desc_t ab Byte 32.
// esp_app_desc_t: magic 0xABCD5432 (0), project_name (48..79), app_elf_sha256 (144..175).
constexpr size_t OTA_IMG_CHECK_BYTES = 32 + 176; // so viel vom Anfang der Datei reicht für die Prüfung (bis Ende app_elf_sha256; die Vorlage hatte 176 und las darüber hinaus)
constexpr uint16_t OTA_CHIP_ESP32 = 0;          // ESP32 (CYD)
constexpr uint16_t OTA_CHIP_ESP32S3 = 9;        // ESP32-S3 (XIAO-Kamera, cam/)

enum OtaImg : uint8_t { OTA_IMG_OK = 0, OTA_IMG_SHORT, OTA_IMG_MAGIC, OTA_IMG_CHIP, OTA_IMG_DESC, OTA_IMG_PROJECT };

static inline bool ota_hex_ok(const char *s, size_t n) {
  if (!s || strlen(s) != n) return false;
  for (size_t i = 0; i < n; i++) if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
  return true;
}

// Prüft die ersten Bytes eines Images. elf_hex (65 Byte) bekommt bei OTA_IMG_OK die ELF-SHA-256 des Images (64 Hexziffern).
// chip: erwartete Chip-ID aus dem Image-Kopf (CYD: ESP32, cam/: ESP32-S3).
static inline OtaImg ota_image_check(const uint8_t *b, size_t n, const char *project, char *elf_hex, uint16_t chip = OTA_CHIP_ESP32) {
  if (!b || n < OTA_IMG_CHECK_BYTES) return OTA_IMG_SHORT;
  if (b[0] != 0xE9) return OTA_IMG_MAGIC;
  if ((uint16_t)(b[12] | (b[13] << 8)) != chip) return OTA_IMG_CHIP;
  const uint32_t magic = (uint32_t)b[32] | ((uint32_t)b[33] << 8) | ((uint32_t)b[34] << 16) | ((uint32_t)b[35] << 24);
  if (magic != 0xABCD5432u) return OTA_IMG_DESC;
  char name[33];
  memcpy(name, b + 32 + 48, 32);
  name[32] = '\0';
  if (!project || strcmp(name, project) != 0) return OTA_IMG_PROJECT;
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    elf_hex[2 * i] = kHex[b[32 + 144 + i] >> 4];
    elf_hex[2 * i + 1] = kHex[b[32 + 144 + i] & 15];
  }
  elf_hex[64] = '\0';
  return OTA_IMG_OK;
}

static inline const char *ota_image_error(OtaImg r) {
  switch (r) {
    case OTA_IMG_OK: return "ok";
    case OTA_IMG_SHORT: return "Datei zu kurz";
    case OTA_IMG_MAGIC: return "kein Firmware-Image";
    case OTA_IMG_CHIP: return "falscher Chip";
    case OTA_IMG_DESC: return "keine App-Kennung";
    default: return "falsches Projekt";
  }
}

static inline char ota_lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }

// Nur https:// und Host github.com oder Endung .githubusercontent.com (Umleitungen der Release-Downloads).
// Authority = Teil nach "https://" bis zum nächsten '/', '?', '#' oder Ende; enthält sie '@', wird abgelehnt.
// Host = Authority bis ':' (danach nur ein Port aus Ziffern); ohne Groß/Klein-Unterschied.
// Hosts mit anderen Zeichen als Buchstaben, Ziffern, '-' und '.' werden abgelehnt.
static inline bool ota_redirect_ok(const char *url) {
  if (!url) return false;
  for (const char *c = url; *c; c++) if ((unsigned char)*c <= 0x20 || (unsigned char)*c == 0x7F) return false;   // Steuerzeichen/Leerzeichen (würden die U-Zeile zerlegen)
  static const char kScheme[] = "https://";
  for (size_t i = 0; i < sizeof kScheme - 1; i++) if (ota_lower(url[i]) != kScheme[i]) return false;
  const char *h = url + sizeof kScheme - 1;
  // Authority = bis zum ersten '/', '?', '#' oder Ende; '@' darin (Benutzerangabe) wird abgelehnt.
  // Vorsichtshalber wird '@' bis zum ersten '/' abgelehnt, auch hinter '?' oder '#' ("github.com?@evil.com/").
  for (size_t i = 0; h[i] && h[i] != '/'; i++) if (h[i] == '@') return false;
  size_t alen = 0;
  while (h[alen] && h[alen] != '/' && h[alen] != '?' && h[alen] != '#') alen++;
  char host[128];
  size_t n = 0;
  for (; n < alen && h[n] != ':'; n++) {
    const char c = ota_lower(h[n]);
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.')) return false;
    if (n + 1 >= sizeof host) return false;
    host[n] = c;
  }
  host[n] = '\0';
  if (n < alen) {   // ":Port" nur mit Ziffern
    for (size_t i = n + 1; i < alen; i++) if (h[i] < '0' || h[i] > '9') return false;
  }
  if (n == 0) return false;
  if (strcmp(host, "github.com") == 0) return true;
  static const char kSuffix[] = ".githubusercontent.com";
  const size_t ns = sizeof kSuffix - 1;
  return n > ns && strcmp(host + n - ns, kSuffix) == 0;
}

// Kamera-Image aus dem Manifest: nur aus den Releases unseres Repos (nicht jede github.com-Adresse). Umleitungen prüfen weiter nur ota_redirect_ok.
static const char OTA_RELEASE_PREFIX[] = "https://github.com/M4dm0nky/madbuddy/releases/download/";
static inline bool ota_release_url_ok(const char *url) {
  if (!ota_redirect_ok(url)) return false;
  const size_t np = sizeof OTA_RELEASE_PREFIX - 1;
  if (strncmp(url, OTA_RELEASE_PREFIX, np) != 0 || !url[np]) return false;
  for (const char *p = url + np; *p; p++) {
    if (*p == '?' || *p == '#' || *p == '%') return false;   // keine Abfrage/Kodierung (versteckte "..")
    if (p[0] == '.' && p[1] == '.') return false;             // kein Pfad nach oben
  }
  return true;
}

// Gibt es etwas Neues? Beide Kennungen müssen gültige ELF-SHA-256 sein und sich unterscheiden (ein kaputtes Manifest meldet nie "Update").
static inline bool ota_needs_update(const char *running_elf, const char *manifest_elf) {
  return ota_hex_ok(running_elf, 64) && ota_hex_ok(manifest_elf, 64) && strcmp(running_elf, manifest_elf) != 0;
}

// Version "a.b.c" (nur Ziffern) nach 3 Zahlen; false bei allem anderen (Zusatz wie "-broken", zu wenige/viele Teile, leer).
static inline bool ota_ver3(const char *s, unsigned long v[3]) {
  if (!s) return false;
  for (int k = 0; k < 3; k++) {
    if (*s < '0' || *s > '9') return false;
    unsigned long n = 0;
    for (; *s >= '0' && *s <= '9'; s++) { n = n * 10 + (unsigned long)(*s - '0'); if (n > 1000000UL) return false; }
    v[k] = n;
    if (k < 2) { if (*s != '.') return false; s++; }
  }
  return *s == '\0';
}

// Gibt es eine neuere Kamera-Firmware? have = Version aus H des XIAO (leer = keine Verbindung), man = cam.version im Manifest.
// Nur wenn beide gültig sind und man echt größer ist; unbekannt/kaputt = kein Angebot.
static inline bool ota_cam_newer(const char *have, const char *man) {
  unsigned long a[3], b[3];
  if (!ota_ver3(have, a) || !ota_ver3(man, b)) return false;
  for (int k = 0; k < 3; k++) if (a[k] != b[k]) return b[k] > a[k];
  return false;
}

// Zeilenansicht (Kamera-Angebot, ggf. mit CYD-Zeile) statt der CYD-Karte? Nur im ruhigen CYD-Zustand: IDLE oder AVAILABLE ohne Zustimmung.
// Laden, Prüfen, Neustart, Zustimmung gegeben und FAILED behalten die CYD-Karte unverändert. state wie ota_status_line (0 IDLE, 1 AVAILABLE ...).
static inline bool ota_card_list(int cyd_state, bool consented, bool cam_offer) {
  return cam_offer && (cyd_state == 0 || (cyd_state == 1 && !consented));
}

// Hinweis anzeigen? Nur wenn das Display wach ist (nicht dunkel/gedimmt: ein Tipp in ein dunkles Display darf nie zustimmen)
// und die Schlummerzeit (nach "Später" eine Stunde) um ist.
constexpr uint32_t OTA_NAG_MS = 3600000UL;
// snooze_set_ms = millis() beim Schlummern (0 = nicht geschlummert); Abstand statt Zielzeitpunkt (24,8-Tage-sicher).
static inline bool ota_notice_due(uint32_t now_ms, uint32_t snooze_set_ms, bool awake) {
  if (!awake) return false;
  return snooze_set_ms == 0 || due_since(now_ms, snooze_set_ms, OTA_NAG_MS);
}

static inline int ota_progress_pct(uint32_t done, uint32_t total) {
  if (!total) return 0;
  uint64_t p = (uint64_t)done * 100u / total;
  return p > 100 ? 100 : (int)p;
}

// Zeile "Firmware" auf der Status-Seite (Tippen sucht sofort nach einem Update).
// state: 0 IDLE, 1 AVAILABLE, 2 DOWNLOADING, 3 VERIFYING, 4 RESTARTING, 5 FAILED (Reihenfolge von ota_pull::State).
// busy = eine Suche läuft oder ist angefordert. Rückgabe: 0 neutral (grau), 1 gut (grün), 2 Problem (rot).
static inline int ota_status_line(int state, int pct, bool busy, uint32_t last_check_ms, bool last_failed, uint32_t now_ms, char *out, size_t n) {
  if (state == 2) { snprintf(out, n, "Lädt %d %%", pct); return 0; }
  // Nur Latin-1-Zeichen (Schriften U8g2 _tf): "..." statt "…", "-" statt "–"
  if (state == 3) { snprintf(out, n, "Prüft die Datei ..."); return 0; }
  if (state == 4) { snprintf(out, n, "Neustart ..."); return 0; }
  if (busy) { snprintf(out, n, "Prüfe ..."); return 0; }
  if (state == 5) { snprintf(out, n, "Update fehlgeschlagen"); return 2; }
  if (state == 1) { snprintf(out, n, "Update bereit - tippen"); return 0; }
  if (last_check_ms == 0) { snprintf(out, n, "Tippen: Update suchen"); return 0; }
  if (last_failed) { snprintf(out, n, "Abruf fehlgeschlagen"); return 2; }
  const uint32_t min = (now_ms - last_check_ms) / 60000UL;
  if (min < 1) snprintf(out, n, "Aktuell (gerade geprüft)");
  else if (min < 60) snprintf(out, n, "Aktuell (vor %u min)", (unsigned)min);
  else snprintf(out, n, "Aktuell (vor %u h)", (unsigned)(min / 60));
  return 1;
}
