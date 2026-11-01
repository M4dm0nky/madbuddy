# Kamera Phase B (CYD-Seite) – Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Der CYD liest die Zeilen des XIAO über `Serial2` und macht daraus Blick, Begrüßung, Einschlafen und Nähe. Dazu kommen eine zweite Einstellungsseite (Kamera an/aus und Info), ein Augen-Symbol im Statusstreifen und das XIAO-Update über dasselbe Release.

**Architecture:** Die Logik liegt header-only in `src/logic/` und ist nativ getestet: `cam_link.h` (Zeilen und Verbindung) und `presence.h` (Anwesenheit, Begrüßung, Schlaf, Nähe, Blickziel). Dazu kommt ein dünner Treiber `src/hw/cam_uart.*`. `app.cpp` verdrahtet beides an den vorhandenen Stellen (`mood_frame`, `speech_frame`, `eyes_frame`). Für das XIAO-Update baut und prüft `ota-publish.mjs` das zweite Image, und `ota_pull` liest den `cam`-Eintrag aus dem Manifest.

**Tech Stack:** Arduino-ESP32 (pioarduino 55.03.312-1), LovyanGFX, ArduinoJson 7, Unity (native), node:test.

**Spec:** `docs/superpowers/specs/2026-10-26-kamera-firmware-design.md` (Abschnitte „Verhalten am CYD“, „Bedienung“, „Updates“). Schnittstellenkarte: `.superpowers/sdd/phase-b-recon.md`.

## Global Constraints

- UART: `Serial2.begin(115200, SERIAL_8N1, 27, 22)`, also RX = GPIO 27 (vom XIAO D6) und TX = GPIO 22 (zum XIAO D7). Die Pins sind frei, `Serial2` wird sonst nirgends benutzt.
- Protokoll und Parser kommen nur aus `src/logic/cam_proto.h` (Phase A, nicht ändern): `CamLineBuf::feed(c)` gibt true zurück, wenn eine Zeile ohne `\n` fertig ist, `cam_parse(line(), len(), &m)` prüft sie.
- Verbunden heißt: Das letzte gültige `H` ist weniger als **5 s** alt. Ohne Verbindung, bei `err` oder bei abgeschalteter Kamera ist `presence` aus, und der Buddy verhält sich wie vor der Kamera (Lichtsensor und Nacht-Logik).
- Anwesenheit: „da“ = 2 `F` innerhalb von **1 s**; „weg“ = **30 s** ohne `F`. Solange jemand da ist, wird `mood_touch` aufgerufen, außer nachts im Schlaf. Die Bedürfnisse ändern sich dadurch nicht.
- Begrüßung: beim Wechsel auf „da“ nach **≥ 2 min** „weg“, höchstens **alle 10 min**. Sie zeigt `Emotion::GREETING` für 3 s und sagt `Say::WIEDER`. `ritual_morning` hat Vorrang.
- Schlaf: **10 min** „weg“ → unbeaufsichtigt und damit dunkel für `mood_set_dark`. Die Kamera ersetzt den Lichtsensor, solange `presence` aktiv ist. Nachts (`mood_is_night`) weckt ein Gesicht nicht, eine Berührung schon.
- Nähe: `w > CAM_NEAR_W` (Vorgabe **450**) → 2 s Ausdruck `Emotion::NEAR` (überrascht, schielend), höchstens **alle 30 s**.
- Blick: Rangfolge Finger (`look_active`) > Kamera > Gazer. Im Schlaf und im Claude-Modus bleibt der Blick gerade (die vorhandene Zeile in `eyes_frame` gewinnt). Gleitender Mittelwert gegen Zittern. `CAM_MIRROR_X` (Vorgabe `true`) wird am Gerät bestimmt.
- `C 1`/`C 0` geht beim Umschalten raus, nach dem Start des CYD und bei jedem ersten `H` nach einer Trennung (XIAO neu gestartet).
- `Persist.cam_on`, Vorgabe `true`, NVS-Schlüssel `"cam"`, `kVersion` 3 → 4. Alte Stände laden weiter, ein fehlender Schlüssel gibt die Vorgabe.
- XIAO-Update: Manifest-Eintrag `cam: { "version", "url", "sha256" }`. Verglichen wird mit `H <version>`. Die Image-Prüfung des cam-Images erwartet Chip 9 (ESP32-S3) und Projektname `arduino-lib-builder` oder den tatsächlichen Namen aus `cam.bin` (Task 6, Step 1 ermittelt ihn). Beide Dateien kommen in **ein** Release.
- WLAN-Zugangsdaten für die `U`-Zeile kommen zur Laufzeit aus dem WLAN-Stack (`esp_wifi_get_config`). Sie werden nur kurz für die Zeile kopiert, nie gespeichert, und alle Puffer werden nach dem Senden genullt.
- Watchdog für das XIAO-Update: Ohne `U`-Zeile und ohne `H` meldet der CYD nach **20 s** einen Fehler (längste Stille auf dem XIAO ≈ 14 s).
- Zeitpunkte nur über `logic/deadline.h` (`due_since`, `Deadline`), nie `(int32_t)(a-b)`.
- Logik-Header in `src/logic/` binden weder `config.h` noch Arduino ein. Schwellen sind Parameter oder `constexpr` im Header, Pins stehen in `config.h`.
- Tests nur über das PowerShell-Werkzeug: pio `C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe`, vorher `$env:PATH = "C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;" + $env:PATH`. Gezielt mit `pio test -e native -f test_<name>`. Der Build `pio run -e cyd2usb` läuft nur in den Tasks, die `src/` außerhalb von `logic/` ändern. Der volle Lauf kommt einmal in Task 7. Ausgaben auf Fehler und Summe filtern.
- Jeder neue Test wird einmal mit einem absichtlich eingebauten Fehler rot gesehen (eine Mutation je Logikbereich, im Bericht notiert).
- `phrases.h` nie von Hand ändern: Text in `docs/superpowers/specs/2026-10-25-saetze.json`, dann `node tools/gen-phrases.mjs`.
- Commits: Branch `feature/kamera-firmware`, ein Commit pro Task, **keine** `Co-Authored-By`-Zeile, kein Hinweis auf Claude. Nur eigene Dateien stagen, nie `case/3mf/roboter_ams.3mf` oder `brag-output-*/`.
- Jeder Task trägt einen Eintrag oben in `docs/FORTSCHRITT.md` ein (Zeit, Gemacht, Prüfung, Offen).
- Kein Flashen, kein serieller Port in Task 1–6. Kein Prozess bleibt laufen, kein `python - <<EOF`.
- Stil wie `src/logic/*.h`: `static inline`, deutsche Kommentare, kurze Zeilenkommentare.

## Dateien

| Datei | Aufgabe |
|---|---|
| `src/logic/cam_link.h` (neu) | Bytes → Zeilen → `CamMsg`, Verbindungszustand, Neustart-Erkennung, Update-Watchdog |
| `src/logic/presence.h` (neu) | Anwesenheit, Begrüßung, Schlaf, Nähe, geglättetes Blickziel |
| `test/test_cam_link/`, `test/test_presence/` (neu) | native Tests |
| `src/hw/cam_uart.{h,cpp}` (neu) | `Serial2`, liest pro Bild alle Bytes, sendet `C`/`U` |
| `src/config.h` | `CAM_RX`, `CAM_TX`, `CAM_BAUD`, `CAM_MIRROR_X`, `CAM_NEAR_W` |
| `src/hw/store.{h,cpp}` | `cam_on`, `kVersion` 4 |
| `src/logic/mood.h`, `src/logic/face_model.h`, `src/ui/eyes.cpp` | `Emotion::NEAR` mit schielenden Pupillen |
| `docs/superpowers/specs/2026-10-25-saetze.json`, `src/logic/phrases.h` | `Say::WIEDER` |
| `src/app.cpp` | Verdrahtung |
| `src/ui/settings.{h,cpp}`, `src/ui/icons.{h,cpp}`, `src/ui/status_bar.{h,cpp}` | Seite 2, Pfeile, Augen-Symbol |
| `tools/ota-publish.mjs`, `tools/ota-publish.test.mjs` | cam-Image und Manifest |
| `src/ota/ota_pull.{h,cpp}`, `src/ui/ota_card.{h,cpp}` | `cam`-Eintrag lesen, Zeile „Kamera“, `U` senden, Fortschritt |

---

### Task 1: `src/logic/cam_link.h` – Zeilen und Verbindung

**Files:** Create `src/logic/cam_link.h`, `test/test_cam_link/test_main.cpp`

**Interfaces:**
- Consumes: `CamLineBuf`, `CamMsg`, `cam_parse`, `CamKind`, `CamState` aus `cam_proto.h`; `due_since` aus `deadline.h`.
- Produces:
  - `struct CamLink`
  - `bool cam_link_feed(CamLink&, char c, uint32_t now, CamMsg *out)`
  - `bool cam_link_connected(const CamLink&, uint32_t now)`
  - `enum class CamConn { OFF_USER, NONE, ERR, OFF, ON }`
  - `CamConn cam_link_conn(const CamLink&, uint32_t now, bool user_on)`
  - `bool cam_link_take_restart(CamLink&)`
  - `void cam_link_upd_start(CamLink&, uint32_t now)`
  - `bool cam_link_upd_timed_out(const CamLink&, uint32_t now)`
  - `const char *cam_link_version(const CamLink&)`

- [ ] **Step 1: Test schreiben** – `test/test_cam_link/test_main.cpp`:

```cpp
#include <unity.h>
#include <string.h>
#include "logic/cam_link.h"

void setUp() {}
void tearDown() {}

// Schickt einen Inhalt mit Prüfsumme Byte für Byte; liefert die Anzahl fertiger Meldungen.
static int feed_line(CamLink &l, const char *content, uint32_t now, CamMsg *m) {
  char line[CAM_LINE_MAX + 8];
  const size_t n = cam_seal(content, line, sizeof line);
  int got = 0;
  for (size_t i = 0; i < n; i++) if (cam_link_feed(l, line[i], now, m)) got++;
  return got;
}

void test_line_across_chunks() {
  CamLink l;
  CamMsg m;
  char line[64];
  const size_t n = cam_seal("F 500 400 300", line, sizeof line);
  int got = 0;
  for (size_t i = 0; i < n; i++) got += cam_link_feed(l, line[i], 100, &m) ? 1 : 0;   // einzeln wie aus Serial2
  TEST_ASSERT_EQUAL(1, got);
  TEST_ASSERT_EQUAL(CAM_FACE, m.kind);
  TEST_ASSERT_EQUAL(500, m.x);
  TEST_ASSERT_EQUAL(300, m.w);
}

void test_garbage_dropped() {
  CamLink l;
  CamMsg m;
  static const char junk[] = "\x00\xff rst:0xc boot\nF 1 2 3*00\n";   // ROM-Ausgabe mit NUL, falsche Prüfsumme
  int got = 0;
  for (size_t i = 0; i + 1 < sizeof junk; i++) got += cam_link_feed(l, junk[i], 0, &m) ? 1 : 0;
  TEST_ASSERT_EQUAL(0, got);
  TEST_ASSERT_EQUAL(1, feed_line(l, "N", 0, &m));   // danach geht es normal weiter
  TEST_ASSERT_EQUAL(CAM_NOFACE, m.kind);
}

void test_connection_timeout() {
  CamLink l;
  CamMsg m;
  TEST_ASSERT_FALSE(cam_link_connected(l, 0));        // nie ein H
  feed_line(l, "H 0.1.0 on", 1000, &m);
  TEST_ASSERT_TRUE(cam_link_connected(l, 1000));
  TEST_ASSERT_TRUE(cam_link_connected(l, 5999));
  TEST_ASSERT_FALSE(cam_link_connected(l, 6000));     // 5 s ohne H
  TEST_ASSERT_EQUAL_STRING("0.1.0", cam_link_version(l));
}

void test_conn_states() {
  CamLink l;
  CamMsg m;
  TEST_ASSERT_EQUAL((int)CamConn::OFF_USER, (int)cam_link_conn(l, 0, false));
  TEST_ASSERT_EQUAL((int)CamConn::NONE, (int)cam_link_conn(l, 0, true));
  feed_line(l, "H 0.1.0 err", 0, &m);
  TEST_ASSERT_EQUAL((int)CamConn::ERR, (int)cam_link_conn(l, 10, true));
  feed_line(l, "H 0.1.0 off", 20, &m);
  TEST_ASSERT_EQUAL((int)CamConn::OFF, (int)cam_link_conn(l, 30, true));
  feed_line(l, "H 0.1.0 on", 40, &m);
  TEST_ASSERT_EQUAL((int)CamConn::ON, (int)cam_link_conn(l, 50, true));
  TEST_ASSERT_EQUAL((int)CamConn::NONE, (int)cam_link_conn(l, 40 + 5000, true));
}

void test_restart_detect() {
  CamLink l;
  CamMsg m;
  feed_line(l, "H 0.1.0 on", 0, &m);
  TEST_ASSERT_TRUE(cam_link_take_restart(l));          // erstes H überhaupt
  TEST_ASSERT_FALSE(cam_link_take_restart(l));         // nur einmal
  feed_line(l, "H 0.1.0 on", 2000, &m);
  TEST_ASSERT_FALSE(cam_link_take_restart(l));         // laufend verbunden
  feed_line(l, "H 0.1.0 on", 9000, &m);                // nach Trennung wieder da
  TEST_ASSERT_TRUE(cam_link_take_restart(l));
}

void test_update_watchdog() {
  CamLink l;
  CamMsg m;
  cam_link_upd_start(l, 0);
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 19999));
  TEST_ASSERT_TRUE(cam_link_upd_timed_out(l, 20000));
  cam_link_upd_start(l, 0);
  feed_line(l, "U 40", 15000, &m);                     // jede U- oder H-Zeile stößt den Watchdog an
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 34999));
  TEST_ASSERT_TRUE(cam_link_upd_timed_out(l, 35000));
  feed_line(l, "U ok", 35001, &m);                     // Ergebnis beendet die Überwachung
  TEST_ASSERT_FALSE(cam_link_upd_timed_out(l, 90000));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_line_across_chunks);
  RUN_TEST(test_garbage_dropped);
  RUN_TEST(test_connection_timeout);
  RUN_TEST(test_conn_states);
  RUN_TEST(test_restart_detect);
  RUN_TEST(test_update_watchdog);
  return UNITY_END();
}
```

- [ ] **Step 2: rot sehen** – `pio test -e native -f test_cam_link` → ERRORED (Header fehlt).

- [ ] **Step 3: Umsetzung** – `src/logic/cam_link.h`:

```cpp
#pragma once
#include <stdint.h>
#include <string.h>
#include "cam_proto.h"
#include "deadline.h"

// CYD-Seite der Kamera-Leitung (Phase B): Bytes → Zeilen → Meldungen, Verbindung, Update-Watchdog. Reine Logik (test/test_cam_link).
constexpr uint32_t CAM_LINK_TIMEOUT_MS = 5000;    // so lange ohne H → keine Verbindung
constexpr uint32_t CAM_UPD_WATCHDOG_MS = 20000;   // so lange ohne U/H während eines Updates → Fehler (XIAO schweigt höchstens ~14 s)

enum class CamConn : uint8_t { OFF_USER, NONE, ERR, OFF, ON };   // aus (Einstellung), keine Verbindung, Kamerafehler, XIAO meldet aus, läuft

struct CamLink {
  CamLineBuf buf;
  bool ever = false;          // je ein H gesehen
  uint32_t heart_ms = 0;      // Zeit des letzten H
  CamState state = CAM_ST_OFF;
  char version[16] = {};
  bool restart = false;       // erstes H nach Trennung: C erneut senden
  bool upd = false;           // Update läuft
  uint32_t upd_ms = 0;        // letzte U/H-Zeile während des Updates
};

static inline bool cam_link_connected(const CamLink &l, uint32_t now) {
  return l.ever && !due_since(now, l.heart_ms, CAM_LINK_TIMEOUT_MS);
}

// Ein Byte von Serial2; true, wenn *out eine gültige Meldung enthält.
static inline bool cam_link_feed(CamLink &l, char c, uint32_t now, CamMsg *out) {
  if (!l.buf.feed(c)) return false;
  if (!cam_parse(l.buf.line(), l.buf.len(), out)) return false;
  if (out->kind == CAM_HEART) {
    if (!cam_link_connected(l, now)) l.restart = true;
    l.ever = true;
    l.heart_ms = now;
    l.state = out->state;
    strncpy(l.version, out->version, sizeof l.version - 1);
    l.version[sizeof l.version - 1] = '\0';
  }
  if (l.upd) {
    if (out->kind == CAM_UPD_OK || out->kind == CAM_UPD_ERR) l.upd = false;
    else if (out->kind == CAM_UPD_PCT || out->kind == CAM_HEART) l.upd_ms = now;
  }
  return true;
}

static inline CamConn cam_link_conn(const CamLink &l, uint32_t now, bool user_on) {
  if (!user_on) return CamConn::OFF_USER;
  if (!cam_link_connected(l, now)) return CamConn::NONE;
  if (l.state == CAM_ST_ERR) return CamConn::ERR;
  return l.state == CAM_ST_ON ? CamConn::ON : CamConn::OFF;
}

// true genau einmal nach dem ersten H bzw. nach einer Trennung.
static inline bool cam_link_take_restart(CamLink &l) {
  const bool r = l.restart;
  l.restart = false;
  return r;
}

static inline void cam_link_upd_start(CamLink &l, uint32_t now) { l.upd = true; l.upd_ms = now; }
static inline bool cam_link_upd_timed_out(const CamLink &l, uint32_t now) {
  return l.upd && due_since(now, l.upd_ms, CAM_UPD_WATCHDOG_MS);
}
static inline const char *cam_link_version(const CamLink &l) { return l.version; }
```

Hinweis: Wenn ein Feldname in `CamMsg` (`state`, `version`) im echten Header anders heißt, gilt der Header. Der Bericht nennt dann die Anpassung.

- [ ] **Step 4: grün** – `pio test -e native -f test_cam_link` → 6/6 PASS.
- [ ] **Step 5: Mutationen** – je einmal rot sehen und zurücksetzen: Timeout `>=` → `>` (`test_connection_timeout`); `restart`-Zuweisung entfernen (`test_restart_detect`); `U ok` beendet die Überwachung nicht (`test_update_watchdog`); `state`-Übernahme entfernen (`test_conn_states`).
- [ ] **Step 6: FORTSCHRITT und Commit** – `git add src/logic/cam_link.h test/test_cam_link docs/FORTSCHRITT.md`, `git commit -m "feat(cam): cam_link – Zeilen, Verbindung und Update-Watchdog am CYD"`.

---

### Task 2: `src/logic/presence.h` – Anwesenheit, Begrüßung, Schlaf, Nähe, Blick

**Files:** Create `src/logic/presence.h`, `test/test_presence/test_main.cpp`

**Interfaces:**
- Consumes: `CamMsg`/`CamKind` (`cam_proto.h`), `due_since`/`Deadline` (`deadline.h`), `clampf` o. ä. aus `clamp.h` (Name im Header nachsehen).
- Produces:
  - `struct Presence`
  - `void presence_msg(Presence&, const CamMsg&, uint32_t now)`
  - `PresenceOut presence_tick(Presence&, bool active, uint32_t now, int16_t near_w)`
  - `struct PresenceOut { bool present, unattended, greet, near, gaze; float gx, gy; }`
  - `void presence_gaze_raw(int16_t x, int16_t y, bool mirror, float *gx, float *gy)`
  - Konstanten `PRES_*`

- [ ] **Step 1: Test schreiben** – `test/test_presence/test_main.cpp`:

```cpp
#include <unity.h>
#include "logic/presence.h"

void setUp() {}
void tearDown() {}

static CamMsg face(int16_t x, int16_t y, int16_t w) { CamMsg m; m.kind = CAM_FACE; m.x = x; m.y = y; m.w = w; return m; }
static CamMsg noface() { CamMsg m; m.kind = CAM_NOFACE; return m; }
static const int16_t NEAR = 450;

// Meldet ab t alle 100 ms ein Gesicht bis t_end; liefert die letzte Ausgabe.
static PresenceOut faces(Presence &p, uint32_t t, uint32_t t_end, int16_t w = 300) {
  PresenceOut o{};
  for (; t <= t_end; t += 100) { presence_msg(p, face(500, 500, w), t); o = presence_tick(p, true, t, NEAR); }
  return o;
}

void test_present_needs_two_faces_in_1s() {
  Presence p;
  presence_msg(p, face(500, 500, 300), 0);
  TEST_ASSERT_FALSE(presence_tick(p, true, 0, NEAR).present);
  presence_msg(p, face(500, 500, 300), 1500);           // zweites erst nach 1,5 s
  TEST_ASSERT_FALSE(presence_tick(p, true, 1500, NEAR).present);
  presence_msg(p, face(500, 500, 300), 2000);           // 0,5 s nach dem vorigen
  TEST_ASSERT_TRUE(presence_tick(p, true, 2000, NEAR).present);
}

void test_gone_after_30s() {
  Presence p;
  faces(p, 0, 1000);
  presence_msg(p, noface(), 1100);
  TEST_ASSERT_TRUE(presence_tick(p, true, 1000 + 29999, NEAR).present);
  TEST_ASSERT_FALSE(presence_tick(p, true, 1000 + 30000, NEAR).present);
}

void test_greet_after_2min_away_with_cooldown() {
  Presence p;
  PresenceOut o = faces(p, 0, 200);
  TEST_ASSERT_FALSE(o.greet);                           // erster Kontakt nach Start: keine Begrüßung (nie „weg“ gewesen)
  // weg ab 200, gilt nach 30 s als weg; 2 min weg = ab 200 + 120000
  presence_tick(p, true, 60000, NEAR);
  o = faces(p, 200 + 30000 + 60000, 200 + 30000 + 60000 + 200);   // nur 1,5 min weg
  TEST_ASSERT_FALSE(o.greet);
  uint32_t t = 200 + 30000 + 60000 + 200;               // zuletzt gesehen
  presence_tick(p, true, t + 30000, NEAR);              // wieder weg
  bool greeted = false;
  for (uint32_t s = t + 30000 + 120000; s <= t + 30000 + 120000 + 200; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    greeted |= presence_tick(p, true, s, NEAR).greet;
  }
  TEST_ASSERT_TRUE(greeted);                            // nach 2 min weg
  // gleich wieder weg und nach 2 min zurück: Sperrzeit 10 min seit der letzten Begrüßung
  const uint32_t g = t + 30000 + 120000;
  presence_tick(p, true, g + 200 + 30000, NEAR);
  greeted = false;
  for (uint32_t s = g + 200 + 30000 + 120000; s <= g + 200 + 30000 + 120000 + 200; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    greeted |= presence_tick(p, true, s, NEAR).greet;
  }
  TEST_ASSERT_FALSE(greeted);
}

void test_greet_is_one_shot() {
  Presence p;
  faces(p, 0, 200);
  presence_tick(p, true, 200 + 30000, NEAR);
  int n = 0;
  for (uint32_t s = 200 + 30000 + 120000; s <= 200 + 30000 + 120000 + 3000; s += 100) {
    presence_msg(p, face(500, 500, 300), s);
    n += presence_tick(p, true, s, NEAR).greet ? 1 : 0;
  }
  TEST_ASSERT_EQUAL(1, n);
}

void test_unattended_after_10min() {
  Presence p;
  faces(p, 0, 200);
  TEST_ASSERT_FALSE(presence_tick(p, true, 200 + 600000 - 1, NEAR).unattended);
  TEST_ASSERT_TRUE(presence_tick(p, true, 200 + 600000, NEAR).unattended);
  PresenceOut o = faces(p, 200 + 700000, 200 + 700200);
  TEST_ASSERT_FALSE(o.unattended);                      // wieder da
  TEST_ASSERT_TRUE(o.present);
}

void test_unattended_from_start_without_faces() {
  Presence p;
  presence_tick(p, true, 0, NEAR);                      // Kamera ab 0 aktiv
  TEST_ASSERT_FALSE(presence_tick(p, true, 600000 - 1, NEAR).unattended);
  TEST_ASSERT_TRUE(presence_tick(p, true, 600000, NEAR).unattended);   // seit Aktivierung nie jemand
}

void test_near_with_cooldown() {
  Presence p;
  PresenceOut o = faces(p, 0, 200, 500);
  TEST_ASSERT_TRUE(o.near);                             // w 500 > 450, gilt 2 s
  TEST_ASSERT_TRUE(presence_tick(p, true, 1999, NEAR).near);
  TEST_ASSERT_FALSE(presence_tick(p, true, 2300, NEAR).near);
  o = faces(p, 5000, 5200, 500);
  TEST_ASSERT_FALSE(o.near);                            // Sperrzeit 30 s
  o = faces(p, 31000, 31200, 500);
  TEST_ASSERT_TRUE(o.near);
  o = faces(p, 70000, 70200, 450);
  TEST_ASSERT_FALSE(o.near);                            // genau 450 ist nicht „größer“
}

void test_inactive_resets() {
  Presence p;
  faces(p, 0, 200);
  PresenceOut o = presence_tick(p, false, 300, NEAR);
  TEST_ASSERT_FALSE(o.present);
  TEST_ASSERT_FALSE(o.unattended);
  TEST_ASSERT_FALSE(o.gaze);
  TEST_ASSERT_FALSE(presence_tick(p, false, 900000, NEAR).unattended);   // aus = nie unbeaufsichtigt
  presence_msg(p, face(500, 500, 300), 900100);         // Meldungen bei inaktiv zählen nicht
  TEST_ASSERT_FALSE(presence_tick(p, false, 900100, NEAR).present);
}

void test_gaze_mapping_and_smoothing() {
  float gx, gy;
  presence_gaze_raw(1000, 500, false, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, PRES_GAZE_X_MAX, gx);    // ganz rechts im Bild
  presence_gaze_raw(1000, 500, true, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -PRES_GAZE_X_MAX, gx);   // gespiegelt
  presence_gaze_raw(500, 0, false, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -PRES_GAZE_Y_MAX, gy);   // oben
  Presence p;
  presence_tick(p, true, 0, NEAR);                      // aktivieren, sonst zählt die Meldung nicht
  presence_msg(p, face(1000, 500, 300), 0);
  PresenceOut o = presence_tick(p, true, 0, NEAR);
  TEST_ASSERT_TRUE(o.gaze);
  TEST_ASSERT_TRUE(o.gx > 0 && o.gx < PRES_GAZE_X_MAX);    // geglättet, nicht sofort am Ziel
  for (uint32_t t = 100; t < 3000; t += 100) { presence_msg(p, face(1000, 500, 300), t); o = presence_tick(p, true, t, NEAR); }
  TEST_ASSERT_FLOAT_WITHIN(0.5f, PRES_GAZE_X_MAX, o.gx);   // läuft hin
  presence_msg(p, noface(), 3000);                      // letztes F war bei 2900
  TEST_ASSERT_TRUE(presence_tick(p, true, 2900 + PRES_GAZE_HOLD_MS - 1, NEAR).gaze);
  TEST_ASSERT_FALSE(presence_tick(p, true, 2900 + PRES_GAZE_HOLD_MS, NEAR).gaze);   // ohne frisches F kein Kamerablick
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_present_needs_two_faces_in_1s);
  RUN_TEST(test_gone_after_30s);
  RUN_TEST(test_greet_after_2min_away_with_cooldown);
  RUN_TEST(test_greet_is_one_shot);
  RUN_TEST(test_unattended_after_10min);
  RUN_TEST(test_unattended_from_start_without_faces);
  RUN_TEST(test_near_with_cooldown);
  RUN_TEST(test_inactive_resets);
  RUN_TEST(test_gaze_mapping_and_smoothing);
  return UNITY_END();
}
```

Hinweis: Wenn ein Zeitbezug in einem Test nicht zur Regel in den Global Constraints passt, gilt die Regel. Der Test wird dann korrigiert und die Stelle im Bericht genannt. Der Zeitbezug für „gone“ ist das letzte `F`.

- [ ] **Step 2: rot sehen** – `pio test -e native -f test_presence` → ERRORED.

- [ ] **Step 3: Umsetzung** – `src/logic/presence.h`:

```cpp
#pragma once
#include <stdint.h>
#include "cam_proto.h"
#include "deadline.h"

// Kamera → Verhalten (Phase B): Anwesenheit, Begrüßung, Unbeaufsichtigt (Schlaf), Nähe, Blickziel. Reine Logik (test/test_presence).
constexpr uint32_t PRES_PAIR_MS = 1000;        // 2 Gesichter innerhalb dieser Zeit → da
constexpr uint32_t PRES_GONE_MS = 30000;       // so lange kein Gesicht → weg
constexpr uint32_t PRES_GREET_AWAY_MS = 120000;// mindestens so lange weg → Begrüßung
constexpr uint32_t PRES_GREET_GAP_MS = 600000; // höchstens alle 10 min
constexpr uint32_t PRES_ALONE_MS = 600000;     // 10 min weg → unbeaufsichtigt
constexpr uint32_t PRES_NEAR_FOR_MS = 2000;    // Nähe-Ausdruck so lange
constexpr uint32_t PRES_NEAR_GAP_MS = 30000;   // höchstens alle 30 s
constexpr uint32_t PRES_GAZE_HOLD_MS = 500;    // Kamerablick gilt so lange nach dem letzten F
constexpr float PRES_GAZE_X_MAX = 30, PRES_GAZE_Y_MAX = 15;   // wie GAZE_X_MAX/GAZE_Y_MAX
constexpr float PRES_SMOOTH = 0.25f;           // Anteil des neuen Ziels je Meldung

struct PresenceOut { bool present, unattended, greet, near, gaze; float gx, gy; };

struct Presence {
  bool active = false;
  uint32_t since_ms = 0;        // Aktivierung (Bezug für „nie jemand da“)
  bool seen = false;            // je ein F seit Aktivierung
  uint32_t face_ms = 0;         // letztes F
  int16_t last_w = 0;           // Breite des letzten F
  bool present = false;
  bool was_away = false;        // seit Aktivierung schon einmal weg gewesen
  uint32_t away_ms = 0;         // letztes F vor dem Wegsein
  bool greeted = false; uint32_t greet_ms = 0; bool greet_pending = false;
  bool near_ever = false; uint32_t near_ms = 0; Deadline near;
  float gx = 0, gy = 0;         // geglättetes Blickziel
};

static inline float pres_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Bildposition 0..1000 → Blickziel in Gazer-Einheiten (±30/±15), +x rechts, +y unten.
static inline void presence_gaze_raw(int16_t x, int16_t y, bool mirror, float *gx, float *gy) {
  const float fx = mirror ? (float)(1000 - x) : (float)x;
  *gx = pres_clampf((fx - 500) / 500.0f * PRES_GAZE_X_MAX, -PRES_GAZE_X_MAX, PRES_GAZE_X_MAX);
  *gy = pres_clampf((y - 500) / 500.0f * PRES_GAZE_Y_MAX, -PRES_GAZE_Y_MAX, PRES_GAZE_Y_MAX);
}

// Eine Meldung vom XIAO; zählt nur bei aktiver Kamera und nur F.
static inline void presence_msg(Presence &p, const CamMsg &m, uint32_t now, bool mirror = false) {
  if (!p.active || m.kind != CAM_FACE) return;
  const bool pair = p.seen && !due_since(now, p.face_ms, PRES_PAIR_MS);
  float tx, ty;
  presence_gaze_raw(m.x, m.y, mirror, &tx, &ty);
  p.gx += (tx - p.gx) * PRES_SMOOTH;
  p.gy += (ty - p.gy) * PRES_SMOOTH;
  p.seen = true;
  p.face_ms = now;
  p.last_w = m.w;
  if (!p.present && pair) {
    p.present = true;
    const bool long_away = p.was_away && due_since(now, p.away_ms, PRES_GREET_AWAY_MS);
    const bool gap_ok = !p.greeted || due_since(now, p.greet_ms, PRES_GREET_GAP_MS);
    if (long_away && gap_ok) { p.greet_pending = true; p.greeted = true; p.greet_ms = now; }
  }
}

// Einmal pro Bild. active = Kamera eingeschaltet, verbunden und ohne Fehler.
static inline PresenceOut presence_tick(Presence &p, bool active, uint32_t now, int16_t near_w) {
  PresenceOut o{};
  if (!active) { p = Presence(); return o; }   // Ausfall: alles vergessen, Buddy wie vor der Kamera
  if (!p.active) { p = Presence(); p.active = true; p.since_ms = now; }
  if (p.present && due_since(now, p.face_ms, PRES_GONE_MS)) { p.present = false; p.was_away = true; p.away_ms = p.face_ms; }
  if (p.present && p.last_w > near_w && !p.near.active(now) && (!p.near_ever || due_since(now, p.near_ms, PRES_NEAR_GAP_MS))) {
    p.near.arm(now, PRES_NEAR_FOR_MS);
    p.near_ms = now;
    p.near_ever = true;
  }
  o.present = p.present;
  o.unattended = !p.present && due_since(now, p.seen ? p.face_ms : p.since_ms, PRES_ALONE_MS);
  o.greet = p.greet_pending;
  p.greet_pending = false;
  o.near = p.near.active(now);
  o.gaze = p.seen && !due_since(now, p.face_ms, PRES_GAZE_HOLD_MS);
  o.gx = p.gx;
  o.gy = p.gy;
  return o;
}
```

Hinweis: Wenn `Deadline` in `deadline.h` anders heißt oder `active()` nicht `const` ist, gilt der Header. `presence_msg` vor dem ersten `presence_tick(…, true, …)` wird ignoriert, weil die Kamera erst mit dem Tick aktiv wird.

- [ ] **Step 4: grün** – `pio test -e native -f test_presence` → 9/9 PASS.
- [ ] **Step 5: Mutationen**, je rot gesehen: Paarzeit 1 s → 2 s; Weg-Zeit 30 s → 31 s; Sperrzeit Begrüßung aus; `unattended` ohne `since_ms`-Bezug; Nähe `>` → `>=`; Spiegeln invertiert; Glättung aus (direkt Ziel).
- [ ] **Step 6: FORTSCHRITT und Commit** – `feat(cam): presence – Anwesenheit, Begrüßung, Schlaf, Nähe, Blick`.

---

### Task 3: Ausdruck `NEAR` (schielend) und Sätze `WIEDER`

**Files:**
- Modify `src/logic/mood.h` (enum `Emotion`, vor `COUNT`)
- Modify `src/logic/face_model.h` (`FaceShape` + Ausdruck)
- Modify `src/ui/eyes.cpp` (Pupille je Auge)
- Modify `docs/superpowers/specs/2026-10-25-saetze.json`
- Regenerate `src/logic/phrases.h`
- Tests `test/test_face_model`, `test/test_phrases`

**Interfaces:** Produces `Emotion::NEAR`, `FaceShape::cross` (float, Pixel, Pupillen beider Augen zur Nase hin), `Say::WIEDER`.

- [ ] **Step 1: Test** in `test/test_face_model/test_main.cpp`: `face_expression(Emotion::NEAR)` hat `cross > 0`, `eye.open >= face_expression(Emotion::SURPRISED).s.eye.open` und `mouth == Mouth::O`. Alle anderen Ausdrücke haben `cross == 0` (Schleife über `Emotion` bis `COUNT`). Den vorhandenen Test, der über alle Emotionen läuft (falls einer da ist), auf die neue Anzahl prüfen.
- [ ] **Step 2: rot sehen** (`-f test_face_model`).
- [ ] **Step 3: Umsetzung:**
  - `Emotion::NEAR` direkt vor `COUNT` anhängen. Alle `switch`- und Tabellen-Stellen ergänzen, die über `Emotion` gehen: `face_expression`, Namen- und Farbtabellen, `overlays_for` usw. Dafür nach `Emotion::STARTLED` greppen und dieselben Stellen nehmen.
  - Im Ausdruck die Werte von SURPRISED nehmen, dazu `cross = 6` und Mund `O`.
  - In `FaceShape` kommt `float cross = 0;` hinzu, ebenso in den Tween bzw. die Interpolation (dort, wo `pupil_dx` gemischt wird).
  - In `eyes.cpp:191` wird `px = ir(cx + f.pupil_dx - side * f.cross)`. `side` heißt dort so wie bei `wink` (Zeile 183: rechts > 0). Damit wandern beide Pupillen nach innen.
- [ ] **Step 4: Sätze:** In `saetze.json` an der Stelle, die der Enum-Reihenfolge entspricht (am Ende vor einem eventuellen Kommentarblock), den Schlüssel `"wieder"` mit genau 6 Sätzen von höchstens 28 Zeichen anlegen: „Da bist du ja!“, „Hallo du!“, „Wieder da!“, „Schön, dich zu sehen!“, „Hab dich vermisst!“, „Na, wieder zurück?“. Dann `node tools/gen-phrases.mjs` und `node tools/gen-phrases.mjs --check`.
- [ ] **Step 5: grün:**
  - `pio test -e native -f test_face_model -f test_phrases -f test_mood` muss PASS sein.
  - `node --test tools/gen-phrases.test.mjs` muss PASS sein.
  - Build `pio run -e cyd2usb` (wegen `eyes.cpp`) muss SUCCESS sein.
  - Mutation: `cross` im Ausdruck auf 0 → rot.
- [ ] **Step 6: FORTSCHRITT und Commit** – `feat(cam): Ausdruck NEAR (schielend) und Sätze WIEDER`.

---

### Task 4: Treiber, Speichern und Verdrahtung in `app.cpp`

**Files:**
- Create `src/hw/cam_uart.h`, `src/hw/cam_uart.cpp`
- Modify `src/config.h`, `src/hw/store.h`, `src/hw/store.cpp`, `src/app.cpp`

**Interfaces:**
- Consumes: Task 1–3.
- Produces:
  - `void cam_uart_begin()`
  - `void cam_uart_poll(uint32_t now)`: liest alle verfügbaren Bytes, füttert `cam_link`, gibt Meldungen an einen Rückruf
  - `void cam_uart_send(const char *content)`: versiegelt per `cam_seal` und schreibt
  - `void cam_uart_send_ctrl(bool on)`
  - `const CamLink &cam_uart_link()`
  - `void cam_uart_on_msg(void (*fn)(const CamMsg&, uint32_t now))`
  - In `app.cpp`: `static bool cam_active(uint32_t now)` = `s_persist.cam_on && cam_link_conn(...) == CamConn::ON`
  - `Persist::cam_on`

- [ ] **Step 1: `config.h`**, neuer Abschnitt am Ende im vorhandenen Stil:

```cpp
// Kamera (XIAO ESP32-S3 Sense an CN1, docs/verkabelung.md)
constexpr int CAM_RX = 27;          // vom XIAO D6 (TX)
constexpr int CAM_TX = 22;          // zum XIAO D7 (RX)
constexpr uint32_t CAM_BAUD = 115200;
constexpr bool CAM_MIRROR_X = true; // am Gerät bestimmen: Augen sollen zur Person schauen
constexpr int16_t CAM_NEAR_W = 450; // Gesichtsbreite 0..1000, ab der er überrascht schielt (am Gerät messen)
```

- [ ] **Step 2: `cam_uart.{h,cpp}`** nach dem Muster von `src/hw/light.*`:
  - `cam_uart_begin()`: `Serial2.begin(CAM_BAUD, SERIAL_8N1, CAM_RX, CAM_TX); Serial2.setRxBufferSize(1024)`. `setRxBufferSize` gehört **vor** `begin`.
  - `poll`: `while (Serial2.available()) { CamMsg m; if (cam_link_feed(s_link, (char)Serial2.read(), now, &m) && s_cb) s_cb(m, now); }`, höchstens 1024 Bytes pro Aufruf.
  - `send`: `char line[CAM_LINE_MAX + 4]; size_t n = cam_seal(content, line, sizeof line); if (n) Serial2.write(line, n);`, danach `memset(line, 0, sizeof line)` über einen `volatile`-Zeiger, weil die `U`-Zeile das Passwort enthält.
- [ ] **Step 3: Store:**
  - `Persist` bekommt `bool cam_on;`.
  - In `store_defaults` steht `cam_on = true`.
  - Laden mit `prefs.getBool("cam", true)`, Speichern mit `prefs.putBool("cam", p.cam_on)`.
  - `kVersion = 4`, Kommentar dazu: „4: cam_on“.
  - Alte Stände (ver 3) laden weiter. Prüfen, ob `store_load` bei `ver < kVersion` etwas verwirft; falls ja, so ändern, dass 1–3 weiter laden.
- [ ] **Step 4: `app.cpp`:**
  - **`app_setup`:** Nach `light_begin()` `cam_uart_begin(); cam_uart_on_msg(on_cam_msg); cam_uart_send_ctrl(s_persist.cam_on);` einfügen.
  - **`app_loop`:** Hinter `claude_api_loop();` folgt `cam_uart_poll(now); if (cam_link_take_restart(...)) cam_uart_send_ctrl(s_persist.cam_on);`. Damit `take_restart` ein nicht-const `CamLink&` bekommt, liefert `cam_uart` dafür eine Funktion `bool cam_uart_take_restart()`.
  - **`on_cam_msg`:** ruft `presence_msg(s_presence, m, now, CAM_MIRROR_X)`. Die Weitergabe der `U`-Meldungen an die Update-Karte kommt erst in Task 6.
  - **`mood_frame`:**

```cpp
  const bool cam = cam_active(now);
  const PresenceOut po = presence_tick(s_presence, cam, now, CAM_NEAR_W);
  s_pres = po;   // für eyes_frame/speech_frame
  const bool asleep = mood_is_sleeping(s_mood, now);
  if (po.present && !(mood_is_night(hour) && asleep)) mood_touch(s_mood, now);   // da = wach halten; nachts weckt ein Gesicht nicht
  const bool dark = cam ? po.unattended : light_update(now);   // Kamera ersetzt den Lichtsensor
  mood_set_dark(s_mood, calm ? false : dark, hour, now);
  if (po.near) mood_set_transient(s_mood, Emotion::NEAR, now, PRES_NEAR_FOR_MS);
```

  Die vorhandene Zeile `mood_set_dark(...light_update(now)...)` wird dadurch ersetzt. `light_update` muss weiter jedes Bild laufen, damit die Glättung nicht springt: Rufe `light_update(now)` immer auf und verwende das Ergebnis nur ohne Kamera. `mood_set_transient(NEAR)` darf nur beim Start der 2 s gerufen werden. Dafür auf die Flanke `po.near && !s_prev_near` prüfen.
  - **`speech_frame`:** Nach dem `ritual_morning`-Block folgt `else if (s_pres.greet) { mood_set_transient(s_mood, Emotion::GREETING, now, 3000); say(Say::WIEDER, now); }`. Damit hat der Morgen Vorrang. `greet` ist ein Ein-Bild-Impuls aus `mood_frame`. `speech_frame` läuft im selben Bild nach `mood_frame` (in `screen_frame` → `eyes_frame`), deshalb merkt sich `s_pres` den Impuls bis dahin. Danach `s_pres.greet = false`. Wenn `speech_frame` früh zurückkehrt (Schlaf, Arbeit), entfällt die Begrüßung. Das ist gewollt.
  - **`eyes_frame`:** Zwischen der Finger-Zeile und der Schlaf/Claude-Zeile kommt `else if (s_pres.gaze) { gx = s_pres.gx; gy = s_pres.gy; }`. Daraus wird `if (look_active(...)) look_target(...); else if (s_pres.gaze) {...}`.
- [ ] **Step 5: Prüfen:**
  - Build `pio run -e cyd2usb` → SUCCESS.
  - `pio test -e native -f test_mood -f test_presence -f test_cam_link` → PASS.
  - Bericht: Flash- und RAM-Zuwachs.
- [ ] **Step 6: FORTSCHRITT und Commit** – `feat(cam): CYD liest die Kamera – Blick, Begrüßung, Schlaf, Nähe`.

---

### Task 5: Einstellungen Seite 2 und Augen-Symbol

**Files:**
- Modify `src/ui/settings.{h,cpp}`, `src/ui/icons.{h,cpp}`, `src/ui/status_bar.{h,cpp}`, `src/app.cpp`

**Interfaces:**
- Produces:
  - `SettingsAction::CAM_TOGGLE`
  - `settings_open`/`settings_redraw` mit Seite 1/2: neuer Parameter oder Zustand im Modul
  - `struct SettingsCam { bool on; CamConn conn; const char *version; }`, an `settings_redraw` bzw. `settings_tick` übergeben (Signatur im Modul anpassen, Aufrufer in `app.cpp`)
  - `icon_eye(LovyanGFX&, int cx, int cy, int s, uint32_t col)`
  - `icon_arrow(LovyanGFX&, int cx, int cy, int s, uint32_t col, bool right)`
  - `StatusInfo::cam` (`uint8_t`: 0 = kein Symbol, 1 = grau, 2 = grün, 3 = rot)

- [ ] **Step 1: Icons:**
  - `icon_eye`: Mandelform aus zwei Bögen bzw. Ellipse, Iris als gefüllter Kreis, Pupille dunkel, ca. 20 × 12 px bei `s = 10`.
  - `icon_arrow`: gefülltes Dreieck.
  - Stil wie die vorhandenen Vektor-Icons in `icons.cpp`.
- [ ] **Step 2: Seite 2:**
  - Ein Pfeil unten rechts (Seite 1 → 2) und unten links (Seite 2 → 1) als Trefferfläche von mindestens 48 × 40 px.
  - Seite 1 ist voll belegt (4 Zeilen à 52 px bis y = 240). Den Pfeil deshalb in die nicht berührbare Info-Zeile 3 rechts legen, mit einem eigenen `UiRect` am rechten Rand (x 268..320 der Zeile 3). Den Text der Info-Zeile nur so weit kürzen, dass nichts überlappt. Pfeil-Ziel auf Seite 2 sind dieselben Koordinaten links.
  - Seite 2 Zeile 0: **Kamera** mit Augen-Symbol und Schalter (grün = an, grau = aus). Tipp → `CAM_TOGGLE`.
  - Seite 2 Zeile 1: **Kamera-Info**, z. B. „XIAO 0.1.0 · verbunden“, gelb „keine Verbindung“ bzw. rot „Fehler“ und „aus“. Farben nach der UI-Regel (grün ok, gelb mittel, rot Fehler).
  - Seite 2 Zeile 2–3 leer, Zeile 3 enthält den Zurück-Pfeil links.
  - `settings_open` beginnt immer auf Seite 1. Die 30-s-Zeitsperre gilt auf beiden Seiten.
- [ ] **Step 3: `app.cpp`:** `CAM_TOGGLE` → `s_persist.cam_on = !s_persist.cam_on; cam_uart_send_ctrl(s_persist.cam_on); save_now(); settings_redraw(...)`.
- [ ] **Step 4: Statusstreifen:**
  - `StatusInfo::cam` in `status_info(now)` füllen:
    - `!cam_on` → 0
    - `conn` ist NONE oder ERR → 3
    - `conn == ON && s_pres.present` → 2
    - sonst 1
  - In `same()` aufnehmen.
  - Zeichnen bei x ≈ 189 (Lücke 178..200 laut Recon), `icon_eye(..., col)`. Überlappt es mit Wettertext oder Pomodoro, die Position so wählen, dass nichts überlappt. Den Bericht mit der gewählten Position ergänzen.
- [ ] **Step 5: Prüfen:**
  - Build `cyd2usb` muss SUCCESS sein.
  - Bildprüfung: Gibt es ein Mockup- bzw. Screenshot-Werkzeug für die UI (`docs/superpowers/mockups`), die Seite 2 als HTML-Mockup im Stil der vorhandenen Mockups zeichnen und per Edge headless (`msedge --headless=new --screenshot`) prüfen. Sonst entfällt dieser Schritt, und der Bericht sagt das.
- [ ] **Step 6: FORTSCHRITT und Commit** – `feat(cam): Einstellungen Seite 2 (Kamera) und Augen-Symbol`.

---

### Task 6: XIAO-Update über das Release

**Files:**
- Modify `tools/ota-publish.mjs`, `tools/ota-publish.test.mjs`, `src/ota/ota_pull.{h,cpp}`, `src/ui/ota_card.{h,cpp}`, `src/app.cpp`, `src/net/net.{h,cpp}`

**Interfaces:**
- Produces:
  - `inspectImage(buf, project, chip = 0)` in JS
  - `ota_pull::cam_info()` → `{ bool avail; char version[16]; char url[200]; char sha[65]; }`
  - `size_t net_build_cam_upd(const char *url, const char *sha, char *buf, size_t cap)` in `net.cpp`: holt SSID und Passwort per `esp_wifi_get_config(WIFI_IF_STA, …)`, baut die Zeile mit `cam_build_upd_cmd` und nullt die lokale Kopie. 0 = kein WLAN.
  - Karte: Zeile „Kamera“ mit eigenem Ja

- [ ] **Step 1: Projektname des cam-Images ermitteln** – `node -e` mit `imageProject` aus `ota-publish.mjs` auf `%LOCALAPPDATA%\3zoll-cam\cam\.pio\build\cam\firmware.bin` anwenden. Den Namen im Bericht notieren und als Konstante `CAM_PROJECT` in `ota-publish.mjs` eintragen. Gibt es das Bin nicht, vorher `& tools\cam-build.ps1` ausführen.
- [ ] **Step 2: JS-Tests zuerst** (`tools/ota-publish.test.mjs`):
  - `inspectImage(img(9, CAM_PROJECT), CAM_PROJECT, 9)` gilt als ok.
  - Das ESP32-Image mit Chip 9 wird abgelehnt.
  - Ein S3-Image ohne Chip-Parameter wird abgelehnt (wie bisher).
  - Ein Manifest-Bau mit cam (die reine Funktion, falls nötig herauslösen, z. B. `buildManifest(main, cam)`) enthält `cam.version`, `cam.url` (`https://github.com/M4dm0nky/madbuddy/releases/download/<tag>/cam.bin`) und `cam.sha256` (64 Zeichen, klein).
  - Zuerst rot sehen.
- [ ] **Step 3: `ota-publish.mjs`:**
  - Liest zusätzlich `%LOCALAPPDATA%/3zoll-cam/cam/.pio/build/cam/firmware.bin`. Wenn die Datei fehlt: mit Hinweis „erst tools\cam-build.ps1“ abbrechen. Mit `--no-cam` wird sie ausgelassen.
  - Prüfungen für das cam-Image: Chip 9, ≥ 200 KB, ≤ 0x330000.
  - Die Version kommt aus dem App-Deskriptor (`version`-Feld wie beim CYD, also `CAM_FW_VERSION`).
  - Ausgabe `ota-out/cam.bin`.
  - Im Manifest steht `cam` vor dem atomaren Schreiben. `--release` lädt `cam.bin` mit hoch.
- [ ] **Step 4: `ota_pull.cpp`:**
  - Der Filter bekommt `cam.version`, `cam.url` und `cam.sha256`.
  - Geprüft wird: `ota_redirect_ok(url)` gilt, `ota_hex_ok(sha, 64)` gilt, die Version ist nicht leer und höchstens 15 Zeichen lang.
  - Das Ergebnis landet in `g_cam` (geschützt wie die übrigen `g_man_*`, gleiche Sperre bzw. Muster).
  - `cam_info()` gibt eine Kopie zurück.
- [ ] **Step 5: Versionsvergleich als reine Logik** in `src/logic/ota_policy.h`: `bool ota_cam_newer(const char *have, const char *man)` vergleicht `a.b.c` numerisch. Leer bzw. unbekannt bei `have` → false (keine Verbindung = kein Angebot). Test in `test_ota_policy`: 0.1.0 < 0.1.1 < 0.2.0 < 1.0.0, gleich → false, `0.1.0-broken` vs. `0.1.0` → false, Müll → false. Zuerst rot sehen.
- [ ] **Step 6: Karte und Ablauf:**
  - `ota_card` zeigt eine zweite Zeile „Kamera <have> → <man>“ mit eigenem Knopf, wenn `cam_info().avail && ota_cam_newer(cam_link_version(), info.version) && conn == ON`. Das Layout passt sich dem vorhandenen an.
  - Gibt es nur das Kamera-Update, öffnet die Karte wie beim CYD-Update. `ota_card_wanted` berücksichtigt dafür beide.
  - Tipp auf den Knopf:
    - Zeile mit `net_build_cam_upd(url, sha, buf, sizeof buf)` bauen. Bei 0 erscheint „kein WLAN“ in Rot.
    - Sonst `cam_uart_send(buf)` senden.
    - `buf` volatil nullen.
    - `cam_link_upd_start(link, now)` aufrufen.
  - Fortschritt: `U <pct>` zeigt einen Balken wie beim CYD-Update. `U ok` zeigt „Kamera aktualisiert“ (grün, 3 s). `U err <grund>` zeigt den Grund in Rot. `cam_link_upd_timed_out` zeigt „Kamera antwortet nicht“ in Rot.
  - `on_cam_msg` in `app.cpp` reicht die `U`-Meldungen an die Karte weiter.
  - `net_build_cam_upd`: `wifi_config_t c; esp_wifi_get_config(WIFI_IF_STA, &c)`, SSID und Passwort sauber terminiert in lokale Puffer kopieren, `cam_build_upd_cmd(...)` bauen, beide Puffer und `c` volatil nullen. Ist `c.sta.ssid[0] == 0`, liefert es 0.
- [ ] **Step 7: Prüfen:**
  - `node --test tools/gen-phrases.test.mjs tools/ota-publish.test.mjs` → PASS
  - `pio test -e native -f test_ota_policy` → PASS
  - `pio run -e cyd2usb` → SUCCESS
  - `node tools/ota-publish.mjs --check` (ohne Release) → zeigt beide Images, Manifest mit `cam`
- [ ] **Step 8: FORTSCHRITT und Commit** – `feat(cam): XIAO-Update über das Release (Manifest cam, Karte, Watchdog)`.

---

### Task 7: Volle Prüfung, Release, Abnahme am Gerät (Opus mit Nutzer)

- [ ] **Step 1: Volle Prüfung:**
  - `pio test -e native` (Summe)
  - `node --test tools/gen-phrases.test.mjs tools/ota-publish.test.mjs`
  - `pio run -e cyd2usb`
  - `& tools\cam-build.ps1`
  - Ausgaben gefiltert.
- [ ] **Step 2: Release – Freigabe des Nutzers einholen** (wirkt nach außen, der CYD lädt es über WLAN). `node tools/ota-publish.mjs --release`. Beide Images landen in einem Release. Der CYD bietet das Update an, und der Nutzer tippt „Ja“.
- [ ] **Step 3: Abnahme am Gerät:** Der Nutzer sitzt vor dem Buddy. Mitschnitt des CYD-Logs über COM6, mit Zeitlimit (`--seconds`).
  - Augen-Symbol: grau, mit Gesicht grün.
  - Seite 2 zeigt „XIAO 0.1.0 · verbunden“.
  - Kamera aus: Symbol weg, Lichtsensor-Verhalten.
  - Kamera an: Symbol wieder da.
  - Blick folgt dem Gesicht. Stimmt die Richtung nicht, `CAM_MIRROR_X` umdrehen.
  - Ganz nah an die Kamera: schielt.
  - Begrüßung und Schlaf werden mit verkürzten Zeiten nicht am Gerät geprüft, die Tests decken sie ab.
  - Bei Tageslicht nach dem Einbau: Messwerte `CAM_NEAR_W` und `CAM_FLIP_180` (aus Phase A offen).
- [ ] **Step 4: Kassensturz** – keine `python`-, `pio`- oder `esptool`-Prozesse mehr.
- [ ] **Step 5: Abschluss** – FORTSCHRITT, finales Branch-Review (ein Review über den ganzen Branch), dann `superpowers:finishing-a-development-branch`.
