#pragma once
#include <stddef.h>
#include "logic/cam_proto.h"
void link_begin();
void link_send(const char *s, size_t n, bool mirror = true);   // UART zum CYD, mirror = zusätzlich auf USB (Messen)
bool link_poll(CamMsg *m);   // true = Befehl (C oder U-Auftrag) vom CYD oder vom USB-Seriell
void link_wipe();            // Zeilenpuffer löschen (Passwort aus der U-Zeile)
