#pragma once
#include <stdint.h>

// Firmware-Update per Abruf: Das Gerät fragt selten ein Manifest ab (manifest.json neben der firmware.bin im neuesten
// GitHub-Release von M4dm0nky/madbuddy, nur über HTTPS mit dem Zertifikatspaket des Cores), erkennt eine neue Version
// an der ELF-SHA-256 im Image und holt die Datei erst, wenn jemand am Display "Jetzt aktualisieren" tippt (Zustimmung vor Ort).
// Geprüft wird am Image selbst (Chip, Projektname, ELF-Kennung) und per SHA-256 über die ganze Datei, erst dann wird die Partition freigegeben.
// Aufbau, Hosting und Sicherheitsmodell: docs/ota.md.
namespace ota_pull {
enum State : uint8_t { IDLE, AVAILABLE, DOWNLOADING, VERIFYING, RESTARTING, FAILED };
struct Info {
  State state;
  char built[24];       // Baudatum der neuen Firmware aus dem Manifest, z. B. "Oct 21 2026 17:05"
  uint32_t size;        // Bytes
  uint8_t pct;          // 0..100 beim Laden
  char error[40];       // bei FAILED: Grund
  uint32_t snooze_set_ms;// millis() beim Schlummern, 0 = nicht geschlummert
  uint32_t last_check_ms;// millis() der letzten beendeten Abfrage des Manifests, 0 = noch keine
  bool last_failed;     // die letzte Abfrage ist fehlgeschlagen (Manifest nicht erreichbar/lesbar)
};

// Kamera-Firmware (XIAO) aus dem Manifest-Eintrag "cam": gilt nur, wenn Version, URL (ota_redirect_ok) und SHA-256 stimmen.
struct CamInfo {
  bool avail;
  char version[16];     // z. B. "0.1.0"
  char url[200];        // https://github.com/.../cam.bin
  char sha[65];         // SHA-256 der cam.bin, 64 Kleinbuchstaben-Hexziffern
};

void begin();
void loop();            // nur aus dem net_task (über net_task_hook; blockiert während des Ladens), intern gedrosselt
Info info();            // threadsicher
CamInfo cam_info();      // threadsichere Kopie; avail = false, wenn das Manifest keinen brauchbaren cam-Eintrag hat
void consent();         // UI: "Jetzt aktualisieren" getippt (nur wirksam im Zustand AVAILABLE)
void check_now();       // UI (Status-Seite): jetzt nach einem Update suchen; hebt das Schlummern auf
bool check_busy();      // eine angeforderte oder laufende Suche (höchstens 60 s nach dem Tipp)
void snooze();          // UI: "Später" oder Tipp daneben: eine Stunde Ruhe
void dismiss_failure(); // UI: Fehlermeldung bestätigt
void ui_alive();        // UI-Takt meldet sich: Teil des Gesundheitsnachweises nach einem Update
void health_tick();     // Gesundheitsnachweis (Rollback abbrechen), aus der Hauptschleife, unabhängig vom WLAN; billig, mehrfach aufrufbar
const char *slot();     // Label der laufenden Partition ("app0"/"app1")
int image_state();      // esp_ota_img_states_t beim Start: -1 undefiniert, 1 PENDING_VERIFY (Rollback aktiv), 2 VALID, 3 INVALID, 4 ABORTED
const char *elf12();    // erste 12 Hex-Zeichen der ELF-SHA-256 der laufenden Firmware
const char *build();    // Baudatum und -zeit der laufenden Firmware, z. B. "Oct 21 2026 17:05:00"
}
