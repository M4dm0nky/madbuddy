#pragma once
#include <stdint.h>
#include "../hw/display.h"
#include "../logic/cam_proto.h"

// Update-Karte („Update bereit“: Ja / Später) bzw. Fortschritt mit konzentrierten Augen, Fehlerkarte.
bool ota_card_wanted(uint32_t now_ms, bool awake);   // AVAILABLE && ota_notice_due(...) oder DOWNLOADING/VERIFYING/RESTARTING/FAILED
void ota_card_open(uint32_t now_ms);                 // beim Wechsel auf den OTA-Bildschirm (zeichnet beim nächsten draw neu)
void ota_card_draw(LGFX &lcd, uint32_t now_ms);      // Karte bzw. Fortschritt; zeichnet nur bei Änderung
void ota_card_touch(int16_t x, int16_t y, uint32_t now_ms);   // Ja -> consent(), Später/daneben -> snooze(), FAILED -> dismiss_failure()
// Kamera (XIAO): eigene Zeile mit eigenem Ja, wenn das Manifest eine neuere cam-Firmware hat und der XIAO läuft.
void ota_card_cam_msg(const CamMsg &m, uint32_t now_ms);     // U-Meldungen (Fortschritt, ok, Fehler) während des Kamera-Updates
bool ota_card_cam_offer(uint32_t now_ms);                    // neuere Kamera-Firmware im Manifest, XIAO verbunden und läuft
void ota_card_unsnooze();                                    // manuelle Suche: Kamera-Angebot nicht mehr schlummern
