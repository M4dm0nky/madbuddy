#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../hw/display.h"
#include "../hw/store.h"
#include "../logic/cam_link.h"

// Einstellungen, Seite 1: Helligkeit (3 Stufen), WLAN/Ort (Portal), Firmware, Info; Seite 2: Kamera (an/aus) und Kamera-Info.
// Der Pfeil in Zeile 3 (rechts auf Seite 1, links auf Seite 2) wechselt die Seite; Schließen oben rechts.
enum class SettingsAction : uint8_t { NONE, CLOSE, BRIGHTNESS, PORTAL, OTA_CHECK, CAM_TOGGLE, PAGE };
struct SettingsCam { bool on; CamConn conn; const char *version; };   // Kamera-Zustand für Seite 2
void settings_open(LGFX &lcd, const Persist &p, uint32_t now_ms, const SettingsCam &cam);   // beginnt immer auf Seite 1
SettingsAction settings_touch(int16_t x, int16_t y, uint32_t now_ms);   // PAGE: Seite wurde gewechselt, danach settings_redraw
void settings_redraw(LGFX &lcd, const Persist &p, const SettingsCam &cam);   // nach Änderung
void settings_tick(LGFX &lcd, const Persist &p, uint32_t now_ms, const SettingsCam &cam);   // Zeilen jede Sekunde auffrischen
bool settings_timed_out(uint32_t now_ms);            // 30 s ohne Eingabe
// Text der Firmware-Zeile; Rückgabe 0 neutral, 1 gut, 2 Problem (wie ota_status_line). Ohne Quelle: „–“.
void settings_firmware_source(int (*fn)(char *out, size_t n, uint32_t now_ms));
