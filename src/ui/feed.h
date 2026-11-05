#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Fütter-Bildschirm (F3): 4 Kacheln mit gezeichneten Speisen, oben Sättigungsbalken, krank: Thermometer.
constexpr int FEED_NONE = -1, FEED_CLOSE = -2;
void feed_open(LGFX &lcd, float satiety, uint32_t now_ms);
int feed_touch(int16_t x, int16_t y, uint32_t now_ms);   // 0..3 = Speise (logic/food.h), FEED_CLOSE, FEED_NONE
bool feed_timed_out(uint32_t now_ms);                     // 10 s ohne Eingabe
