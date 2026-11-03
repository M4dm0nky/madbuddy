#pragma once
// XIAO ESP32S3 Sense: Kamera-Pins (Seeed-Wiki, OV2640/OV3660-Modul), UART zum CYD, Zeiten.
#define CAM_PIN_PWDN  -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  10
#define CAM_PIN_SIOD  40
#define CAM_PIN_SIOC  39
#define CAM_PIN_D7    48
#define CAM_PIN_D6    11
#define CAM_PIN_D5    12
#define CAM_PIN_D4    14
#define CAM_PIN_D3    16
#define CAM_PIN_D2    18
#define CAM_PIN_D1    17
#define CAM_PIN_D0    15
#define CAM_PIN_VSYNC 38
#define CAM_PIN_HREF  47
#define CAM_PIN_PCLK  13

#define CAM_UART_BAUD 115200
#define CAM_UART_TX   43   // D6
#define CAM_UART_RX   44   // D7

#define CAM_HEART_MS  2000
#define CAM_RETRY_MS  10000   // Kamera-Init nach Fehler erneut versuchen
#define CAM_FLIP_180  0       // 1 = Bild um 180 Grad drehen (Einbaulage), wird beim Einbau festgelegt

#ifndef CAM_FW_VERSION
#define CAM_FW_VERSION "0.1.1"   // Phase C setzt die Version beim Release
#endif
