#pragma once
#include <stdint.h>
#include <esp_camera.h>
bool camera_start();    // true = Sensor läuft (QVGA, RGB565)
void camera_stop();     // XCLK aus, Puffer frei (PWDN ist beim XIAO nicht verdrahtet)
bool camera_running();

// Liefert das Bild in aufrechter Lage: bei CAM_ROTATE_CW != 0 in einen PSRAM-Puffer gedreht (*w/*h getauscht),
// sonst (oder bei fehlendem/zu kleinem Puffer) das Original unverändert. Puffer gehört der Kamera-Einheit, nicht freigeben.
const uint8_t *camera_rotated(const camera_fb_t *fb, int *w, int *h);
