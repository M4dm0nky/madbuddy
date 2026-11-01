#pragma once
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "../hw/store.h"

// WLAN-Portal (WiFiManager, nicht blockierend), NTP, Ort -> Koordinaten, Wetter. Abrufe laufen im net_task (Kern 0).
struct NetWeather { bool valid; int code; float temp_c; bool is_day; uint32_t fetched_ms; };

void net_begin(Persist *p);          // AP "MadBuddy" mit Zusatzfeld "Ort"; startet net_task
void net_loop();                     // aus app_loop: wm.process(), übernimmt Ort/Geocoding-Ergebnis in *p
bool net_wifi_ok();
bool net_portal_active();
void net_open_portal();              // Einstellungen: Portal (erneut) öffnen
bool net_time_ok();                  // time(nullptr) > 1700000000
int net_hour();                      // -1 wenn Zeit unbekannt
bool net_local_time(struct tm *out);
uint32_t net_epoch();                // 0 wenn unbekannt
NetWeather net_weather();            // threadsichere Kopie
// Zeile "U	url	sha	ssid	pass" (versiegelt) für den XIAO bauen. WLAN-Zugang aus dem gespeicherten STA-Eintrag; nichts wird abgelegt,
// der Aufrufer löscht buf nach dem Senden. 0 = kein WLAN (oder URL/Zeile unbrauchbar).
size_t net_build_cam_upd(const char *url, const char *sha, char *buf, size_t cap);
void net_task_hook(void (*fn)());    // wird im net_task nach dem Wetter aufgerufen (OTA)
