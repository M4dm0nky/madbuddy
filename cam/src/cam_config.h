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

#define CAM_UART_TX   43   // D6
#define CAM_UART_RX   44   // D7

#define CAM_HEART_MS  2000
#define CAM_RETRY_MS  10000   // Kamera-Init nach Fehler erneut versuchen
#define CAM_FLIP_180  0       // 1 = Bild um 180 Grad drehen (Einbaulage), wird beim Einbau festgelegt
#define CAM_ROTATE_CW 90   // Einbaulage im Roboter: Bild im Uhrzeigersinn drehen (0/90/180/270)

#ifndef CAM_FW_VERSION
#define CAM_FW_VERSION "0.1.7"   // Phase C setzt die Version beim Release
#endif

// Gesichtsmodell: 0 = MSR + MNP (bisher), 1 = espdet_pico_224_224_face (muss im Flash liegen, siehe sdkconfig.defaults)
#ifndef CAM_FACE_MODEL
#define CAM_FACE_MODEL 1
#endif

// Aufloesung der Kamera (HQVGA 240x176 ist schneller als QVGA 320x240) und Erkennung nur jedes n-te Bild (Bewegung jedes Bild)
#ifndef CAM_FRAME_SIZE
#define CAM_FRAME_SIZE FRAMESIZE_HQVGA
#endif
#ifndef CAM_DETECT_EVERY
#define CAM_DETECT_EVERY 1   // 0.1.4 gemessen: mit 2 nur 5 statt ~7 Erkennungen/s, Bewegung braucht keine Extrabilder
#endif
