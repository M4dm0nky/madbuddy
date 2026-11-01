#pragma once
#include <stdint.h>
#include "logic/cam_proto.h"
void updater_boot();                                               // Zustand der laufenden Partition lesen
void updater_run(const CamMsg &cmd, void (*beat)());              // blockiert; bei Erfolg Neustart, sonst "U err" und zurück
void updater_health_tick(uint32_t now, bool cam_ok, uint32_t beats);   // Probelauf: bestätigen oder zurückrollen
