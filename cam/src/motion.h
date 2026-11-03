#pragma once
#include <stdint.h>
#include <esp_camera.h>
#include "logic/motion.h"

// Bewegung im aufrechten Bild (camera_rotated): true = Bewegung gefunden, out = Schwerpunkt und Zellenzahl.
// Vergleicht mit dem Bild des vorigen Aufrufs; nach motion_reset() (Kamera aus/an) beginnt der Vergleich neu.
bool motion_detect(camera_fb_t *fb, MotionResult *out);
void motion_reset();
