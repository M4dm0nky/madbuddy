# Plan: Kamera schneller (XIAO 0.1.4)

Spec: `docs/superpowers/specs/2026-11-03-kamera-schneller-design.md`. Branch `feature/kamera-schneller`.
Regeln für alle Tasks: TDD (Test erst rot sehen), nur Betroffenes testen/bauen, keine Prozesse laufen lassen, pio nur über PowerShell:
`$env:PATH = "C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;" + $env:PATH; & C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe test -e native -f test_motion -f test_cam_proto`.
Commits ohne Co-Authored-By-Zeile (Projektregel).

## Task 1 – Logik (Host-testbar), Sonnet
Dateien: `src/logic/cam_xiao.h` (`cam_rotate_px2`, Z. 32–49), `src/logic/motion.h`, `src/logic/cam_proto.h`, Tests `test/test_cam_proto/test_main.cpp`, `test/test_motion/test_main.cpp`.
1. `cam_rotate_px2(src,w,h,dst,rot_cw)` schneller: 16-Bit-Kopie (`memcpy` von 2 Byte oder `uint16_t`, Puffer sind 2-Byte-ausgerichtet), Kacheln 16×16, eigene Schleife je Winkel ohne `switch` im Innern. Signatur bleibt. Test: Referenz (die bisherige Implementierung, in den Test kopiert) vs. neu, bitgleich, für 0/90/180/270 und Größen 320×240, 240×176 und eine ungerade Größe (z. B. 37×23).
2. `motion_grid_rot(const uint8_t *raw, int rw, int rh, int rot_cw, uint8_t *grid)` in `motion.h`: liefert dasselbe Raster wie `motion_grid(gedreht, ow, oh, grid)` mit `cam_rot_dims`, liest aber jeden der 2×2 Punkte direkt aus dem ungedrehten Bild (Koordinaten wie in `cam_rotate_px2`). Test: für 0/90/180/270, 320×240 und 240×176, Zufallsbild → gleiches Raster wie Drehen + `motion_grid`. `motion.h` darf dafür `cam_xiao.h` einbinden oder `cam_rot_dims` lokal nutzen.
3. Kommentar in `motion.h` zu `MOT_W/MOT_H` anpassen (Zellgröße hängt von der Bildgröße ab: 8×8 bei 240×320, 5×6 bei 176×240).
4. Protokoll `S fps dps rot det` (je 0..9999): `cam_build_stats(int fps,int dps,int rot,int det,char*out,size_t cap)`, `CamKind` neu `CAM_STATS` (ans Ende der enum), Felder in `CamMsg` (z. B. `int16_t fps, dps, rot_ms, det_ms`), `case 'S'` in `cam_parse` wie `M` (genau 4 Felder, nur Ziffern, Max 9999). Tests: Rundweg, Negativfälle (3 und 5 Felder, `-1`, 10000, doppeltes Leerzeichen, falsche Prüfsumme).
Akzeptanz: `-f test_motion -f test_cam_proto` grün, Mutationsprobe (z. B. eine Koordinate in `motion_grid_rot` vertauscht) einmal rot gesehen. Commit `feat(cam): schnelle Drehung, Bewegungsraster ohne Drehung, Messzeile S`.

## Task 2 – XIAO-Firmware, Sonnet
Dateien: `cam/sdkconfig.defaults`, `cam/src/cam_config.h`, `cam/src/camera.cpp/.h`, `cam/src/detect.cpp/.h`, `cam/src/motion.cpp/.h`, `cam/src/main.cpp` (loop ab Z. ~115, `send_snapshot` ~Z. 50–75).
1. `sdkconfig.defaults`: die fünf Zeilen aus der Spec (Maßnahme 1) mit Kommentar; `CONFIG_APP_PROJECT_VER="0.1.4"`; `cam_config.h` `CAM_FW_VERSION "0.1.4"`.
2. `cam_config.h`: `CAM_FRAME_SIZE FRAMESIZE_HQVGA` (mit `#ifndef`), `CAM_DETECT_EVERY 2`. `camera.cpp` nutzt `CAM_FRAME_SIZE`. Der Rotationspuffer bleibt 320*240*2 (passt für beide Größen).
3. `detect_face` bekommt das schon gedrehte Bild: neue Signatur `detect_face(const uint8_t *px, int w, int h, CamFace *out, uint32_t *infer_ms)`; `infer_ms` misst nur `run()`.
4. `motion_detect(camera_fb_t *fb, MotionResult *out)` nutzt `motion_grid_rot(fb->buf, fb->width, fb->height, CAM_ROTATE_CW, g_cur)` statt `camera_rotated` (Formatprüfung RGB565 und `fb->len` bleibt).
5. `loop()`: Bewegung bei jedem Bild; nur wenn `g_frame_no % CAM_DETECT_EVERY == 0`: `camera_rotated` (Zeit messen = rot), `detect_face`, dann `F`/`N` senden wie bisher. `send_snapshot` nutzt weiter `camera_rotated`.
6. Statistik: je Sekunde Bilder, Erkennungen, Summe rot-ms, Summe det-ms; einmal pro Sekunde `S` per `link_send(..., false)` an den CYD und wie bisher `# fps=…` auf USB (um dps/rot/det erweitert). Zeitvergleiche wie im Bestand (`now - t >= X`, vorzeichenlos).
Akzeptanz: `& tools\cam-build.ps1` SUCCESS (im Hintergrund, Ausgabe gefiltert), in `%LOCALAPPDATA%\3zoll-cam\cam\sdkconfig.cam` stehen `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240`, `CONFIG_COMPILER_OPTIMIZATION_PERF=y`, `CONFIG_SPIRAM_SPEED=80`; Flash-Belegung melden. Kein Flashen. Commit `perf(cam): 240 MHz, -O2, PSRAM 80 MHz, HQVGA, Erkennung jedes 2. Bild, Messzeile S, Kamera 0.1.4`.

## Task 3 – CYD zeigt die Messzeile, Haiku
Datei `src/app.cpp`: bei `CAM_STATS` (neben `CAM_MOTION`, Z. ~406) letzte Werte in `static CamMsg s_cam_stats` merken; in `cam_log_tick` (Z. ~339) an die `cam:`-Zeile ` fps=%d dps=%d rot=%d det=%d` anhängen. Akzeptanz: `pio run -e cyd2usb` SUCCESS. Commit `feat: cam:-Zeile zeigt Messwerte der Kamera`.

## Task 4 – Schlussprüfung und Release (Opus)
Volle `pio test -e native`, Review über den Diff, Release `node tools/ota-publish.mjs --release`, Nutzer installiert am Netzteil, vorher/nachher `fps` in der `cam:`-Zeile, Eichwerte gegenprüfen, FORTSCHRITT, Merge.
