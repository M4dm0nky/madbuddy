#pragma once
#include "../hw/display.h"

// Sprechblase oben mittig (y 2..32, Zipfel Richtung Gesicht), Text UTF-8 (Latin-1-Schrift, 1:1).
// Wird ins Augen-Sprite gezeichnet (eyes_draw), darum Ziel LovyanGFX.
// icon: Bedürfnis-Symbol (icon_need, 0..4) links vom Text, -1 = keins.
void bubble_draw(LovyanGFX &d, const char *utf8, int icon = -1);
