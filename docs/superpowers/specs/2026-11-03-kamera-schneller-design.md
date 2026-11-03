# Kamera schneller (XIAO 0.1.4) – Design

Stand 2026-11-03. Ausgangslage: Kamera 0.1.3 im Roboter, ~3 Bilder/s mit espdet_pico_224, Bild per Software um 90° gedreht.

## Ursachen (aus dem Code)
- `cam/sdkconfig.cam`: CPU 160 MHz, Compiler `-Og` (Debug), PSRAM 40 MHz, Datencache 32 KB/32 B-Zeilen.
- `camera_rotated()` wird pro Bild zweimal aufgerufen (`detect_face`, `motion_detect`) und dreht jedes Mal 76 800 Pixel byteweise mit `switch` je Pixel.
- Das Pico-Modell rechnet ohnehin auf 224×224 herunter; ein 240×320-Bild (gedreht) wird auf 168×224 verkleinert.

## Ziel
Deutlich mehr Bilder pro Sekunde bei gleicher Erkennungsqualität, messbar am CYD-USB ohne Ausbau des XIAO.

## Maßnahmen
1. **Build-Einstellungen** (`cam/sdkconfig.defaults`): `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y`, `CONFIG_COMPILER_OPTIMIZATION_PERF=y`, `CONFIG_SPIRAM_SPEED_80M=y`, `CONFIG_ESP32S3_DATA_CACHE_64KB=y`, `CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y`. Flash-Modus bleibt unverändert.
2. **Drehung einmal und schnell:** `cam_rotate_px2` kopiert 16-Bit-Pixel in Kacheln (z. B. 16×16), Ergebnis bitgleich zur bisherigen Drehung (Host-Test gegen eine Referenz). Die Bewegung braucht kein gedrehtes Bild mehr: `motion_grid_rot()` liest die Rasterpunkte direkt aus dem ungedrehten Bild (umgerechnete Koordinaten), Ergebnis gleich `motion_grid(gedrehtes Bild)`.
3. **Kleineres Kamerabild:** `CAM_FRAME_SIZE` = `FRAMESIZE_HQVGA` (240×176, gedreht 176×240). Wirksame Auflösung fürs Modell fast gleich (164×224 statt 168×224), halbe Pixelzahl für Drehung, DMA und Schnappschuss. Koordinaten sind auf 0..1000 bezogen, die Eichwerte bleiben; Gegenprobe am Gerät.
4. **Erkennung nur jedes N-te Bild:** `CAM_DETECT_EVERY` = 2. Bewegung bei jedem Bild, Drehung + Gesichtserkennung nur bei jedem 2. Bild; `F`/`N` nur nach einer Erkennung. `M` bleibt auf höchstens alle 250 ms begrenzt. Mit 1 verhält es sich wie bisher.
5. **Messzeile `S fps dps rot det`** (XIAO → CYD, einmal pro Sekunde, mit Prüfsumme): Bilder/s, Erkennungen/s, mittlere ms für Drehung, mittlere ms für die Erkennung (ohne Drehung). Werte je 0..9999. Der CYD merkt sich die letzte Zeile und hängt sie an seine `cam:`-Zeile an (`fps=… dps=… rot=… det=…`). Ältere CYD-Firmware verwirft unbekannte Zeilen, ältere XIAO-Firmware sendet keine `S`-Zeile (CYD zeigt dann 0).

## Nicht im Umfang
Flash-Modus, anderes Gesichtsmodell, Änderungen an Anwesenheit/Blick im CYD.

## Risiken
- `-O2` kann versteckte Fehler zeigen → Host-Tests, Probelauf mit Rollback (20 s) schützt das Gerät.
- 240 MHz braucht mehr Strom → Update wie bisher nur am Netzteil ≥ 2 A.
- HQVGA ist beim Sensor ein anderes Seitenverhältnis (15:11 statt 4:3) → Blick-Nullpunkt/Nähe-Schwelle am Gerät gegenprüfen.

## Prüfung
Host-Tests (Drehung bitgleich für 0/90/180/270 und 320×240 sowie 240×176, `motion_grid_rot` gleich `motion_grid` gedreht, `S` bauen/parsen mit Negativfällen), `pio test -e native`, Builds `cyd2usb` und `tools/cam-build.ps1`, Release, am Gerät `cam:`-Zeile mit `fps` vorher/nachher.
