# Fortschritt MadBuddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

## Winken erkennen – GEBAUT (Stand 2026-11-05)
- Nur CYD, Kamera-Firmware unverändert. `src/logic/wave.h` (Test `test/test_wave`): zählt Richtungsumkehrungen des M-Schwerpunkts (Hub ≥ 120/1000, ≥ 8 bewegte Zellen, 3 Umkehrungen in 2,5 s, Lücke > 800 ms setzt zurück, 8 s Sperre). Verdrahtet in `on_cam_msg` (nur bei aktiver Kamera), `mood_frame` (weckt tagsüber, nachts im Schlaf nicht) und `speech_frame` (Grußgesicht + Blase aus Kategorie „winken“, nur Augen-Bildschirm). **Schwellen vorläufig, am Gerät einstellen.**

## Gesamt-Review Pakete A–E + Pflege ohne Zwang – GEBAUT, wartet auf Release (Stand 2026-11-05)
- Branch `fix/review-paket-a` (von `main`), Review-Plan: Befunde und Pakete siehe Commit-Nachrichten.
- **A** Kamera-Watchdog 30 s mit Reset (vorher 5 s, nur Meldung), Kamera-Download höchstens 5 min, Kamera-URL nur aus `M4dm0nky/madbuddy/releases/download/`, kein `setInsecure` mehr, Wartezeiten über `Wait` (`deadline.h`), Claude-Empfänger in eigenem Task (Gesicht friert nicht mehr ein), Einrichtungs-WLAN mit 8-stelligem Passwort auf dem Display, Schlüssel nicht mehr im seriellen Log. Kamera **0.1.7**.
- **Pflege ohne Zwang** (Nutzerwunsch): kein Hunger, keine Krankheit/Niesen, nie betrübt oder gelangweilt durch Nichtstun (Untergrenze 50). Füttern, Streicheln, Spielen geben nur kurze Freude. Müdigkeit/Schlaf unverändert. `feature/beduerfnis-hinweis` wird NICHT gemergt (Testbild liegt nur dort).
- **B** CI `.github/workflows/test.yml` (Host-Tests, Node-Tests, CYD-Build).
- **C** `app.cpp` (872 → 180 Zeilen) in `app_diag/speech/mood/eyes/screens.cpp` + `app_internal.h`, `secure_zero`, `CAM_BAUD` in `cam_proto.h`.
- **D** Unveränderte Bilder werden nicht neu gesendet (`logic/frame_skip.h`, spätestens 1 s voll). SPI-Takt `CYD_SPI_WRITE_MHZ` (Standard seit 2026-11-05 80, Rückfall-Env `cyd2usb_spi40`), fps-Log mit `-DCYD_LOG_FPS=1`.
- **E** `verkabelung.md` „Schutz und Strom“ (Elko, Schalter ≥ 1 A, Polyfuse 1,5 A, Antennentest). Lüftung Kopf hinten + Boden, Option `use_inserts` (Gewindeeinsätze), `case/tools/check_lib.ps1`, Prüfung 3k. **Neu drucken: `roboter_head_back.stl`, `roboter_bottom.stl`.**
- Prüfung: `pio test -e native` 251/251, Builds CYD 75,9 %, Kamera SUCCESS; `check_roboter.ps1 -Only 3k,3,3b,3c,1c,3h` und `check.ps1` grün. Volllauf `check_roboter.ps1` steht vor dem Merge noch aus.
- Offen:
  1. Release (Nutzer-Ja), Kamera-Update nur am Netzteil ≥ 2 A.
  2. Am Gerät: Gesicht flüssig, Claude-Hook kommt an, Portal-Passwort lesbar, Pflege neutral.
  3. ~~80 MHz testen~~ erledigt 2026-11-05: Test-Release v20261105-2217 am Gerät sauber, 80 MHz ist Standard.
  4. Altstand: `roboter_torso_back.stl` passt nicht zum SCAD (nicht von diesem Branch).
  5. Volllauf Gehäuseprüfung, dann Merge nach `main`.

## Nächste Aufgaben (Stand 2026-11-03 spät, alles auf `main`)
Für jede Aufgabe einen neuen Branch von `main`. Geräte-Stand: Release `v20261103-2108` (CYD + Kamera 0.1.6) ist installiert. Kamera-Updates nur am Netzteil ≥ 2 A. Releases (`node tools/ota-publish.mjs --release`, vorher Branch pushen) und das Löschen von Branches braucht das ausdrückliche Ja des Nutzers.

1. **Roboterkopf-Nachdruck abnehmen** – Kopf vorn + hinten druckt gerade (Stand 2026-11-03 abends).
   - Fragen an den Nutzer: Bild mittig im Fenster? USB-C steckbar? Sitzt der CYD fest (Andruckfüße greifen)?
   - Klemmt USB weiter oder wackelt der CYD: messen lassen Glasoberfläche → Platinenunterseite (Modell 6,1 = `disp_h` 4,5 + 1,6) und Dicke der USB-C-Steckerhülle (Modell 6,5), dann `disp_h` in `case/cyd_common.scad` an der Ursache korrigieren statt `usb_face_extra` weiter zu erhöhen.
   - Details: Abschnitt „Roboterkopf Passung nach Druck“ unten.
2. **Kamera-Halter v2 abschließen** – Roboter ist komplett gedruckt; Sitz des Blocks im Schacht beim Nutzer erfragen (Spiel 0,3 mm, Lötstellen frei).
   - Danach Task 3 (Kopf Stufe 2: `case/madbuddy_case.scad`, `case/check.ps1`) und Task 4 (Ausgaben, Anleitung, Schlussprüfung) aus `docs/superpowers/plans/2026-11-02-kamera-halter-v2.md`. Prüfen gezielt mit `-Only`, Volllauf `check_roboter.ps1` (~35 min) einmal am Ende.
3. **Stimmung „betrübt“ klären** – zuerst mit dem Nutzer, was genau stört.
   - Bekannt: `Emotion::SAD` kommt, sobald `affection < 20` (`src/logic/mood.h:160`); die Zuneigung sinkt ohne Streicheln/Füttern.
   - Idee: Anwesenheit per Kamera hält die Zuneigung langsam oben oder bremst den Abfall. Erst brainstorming, dann TDD in `test/test_mood`.
4. **Blick am Gerät abnehmen** – mit dem Nutzer, kein Code.
   - Folgt der Blick flüssig? Geradeaus beim normalen Sitzen (`CAM_GAZE_Y0 300`)? Schielen erst bei ~20 cm (`CAM_NEAR_W 350`)? Werte live über `cam:`-Zeile (COM6, 115200, DTR/RTS aus). Bei Gelegenheit ein `python tools/cam-snap.py COM6 --out bild.jpg` (prüft den neuen Snap-Puffer).
5. **Später, nur bei Bedarf**
   - Kamera noch schneller: Erkennung braucht 135 ms je Bild (Pico 224). Nächste Hebel: kleineres Modell-Eingangsbild, eigenes Kopfmodell mit ESP-Detection (~2–3 Arbeitstage, RTX 5080).
   - CYD: Bildrate 24–26 fps (SPI 80 MHz, Ring-Spur, Brillen-Raster), `src/app.cpp` aufteilen.
   - Aufräumen: Branches `feature/kamera-*` (einrichtung, firmware, gehaeuse, halter-v2, kleinkram, schneller), `rename-madbuddy`, `fix/kopf-passung` (lokal + GitHub) sind gemergt und können weg (Nutzer-Ja; Löschen durch Claude wurde von der Rechteprüfung abgelehnt).

## Roboterkopf Passung nach Druck – ERLEDIGT bis Nachdruck (Stand 2026-11-03 spät)
- Anlass: Roboter komplett gedruckt. Bild saß 3 mm zu weit rechts im Fenster; USB-C-Stecker ließ sich nicht stecken, er stieß zum Gesicht hin an (Buchse sitzt näher am Gesicht als im Modell).
- Gemacht: `aa_left` 10 → 13 in `case/cyd_common.scad` (CYD wandert im Kopf 3 mm nach links, Fenster bleibt mittig). Roboter: USB-Öffnung 2 mm Richtung Gesicht höher (`usb_face_extra`, Kerbe ab der Trennfuge, kein Überhang). Stufe 2: Platine bleibt, `disp_x` −1 → 2 (Fenster folgt der echten Bildfläche), `hx_in` fest 47,4 (hätte sich sonst mit `aa_left` um 6 mm verkleinert) + Assert.
- Prüfung: `check_roboter.ps1 -Only 1,1b,1c,3,3b` ALLES GRÜN; `check.ps1` ALLES GRÜN. Geänderte Druckteile: `roboter_head_front(_ams)`, `roboter_head_back`, 3MF; Stufe 2 `madbuddy_front`, `madbuddy_frame_test`.
- Nachtrag: CYD-Löcher gemessen 3,0 (nicht 3,2) → `hole_d` 3,0, Zentrierstift 2,6 (Druck kam 2,9–3,1 bei Modell 2,8). Prüfungen grün.
- Offen: Nachdruck Kopf vorn + hinten. Die 2 mm sind ohne Messung; genauer wird es mit der Messung Glasoberfläche → Platinenunterseite (Modell 6,1 mm, `disp_h` im Datenblatt-Stand) und Dicke der USB-C-Steckerhülle (Modell 6,5). Sitzt der CYD danach lose (Andruckfüße greifen nicht), liegt es an `disp_h`.

## Kamera-Kleinkram (0.1.6) – ERLEDIGT (Stand 2026-11-03 spät)
- Zeit: 2026-11-03 (Opus), Branch `feature/kamera-kleinkram`.
- Gemacht: Schnappschuss-Sammeln am CYD als reine Logik `src/logic/cam_snapbuf.h` (`snap_feed`, Tests in `test_cam_proto`). Läuft der 24-KB-Puffer über, gibt der CYD die ganzen Zeilen aus und meldet `J X cyd_puffer_voll` (vorher still verloren). Fehlerzeilen des CYD jetzt mit Prüfsumme (sonst hat `cam-snap.py` sie verworfen). XIAO schickt beim Schnappschuss alle 2 s ein Lebenszeichen. `cam-snap.py`: `write_timeout = 2`. Kamera-Version 0.1.6.
- Prüfung: `pio test -e native` 242/242, Gegenprobe (halbe Zeile nicht verwerfen) wird rot; `cam_snap_test` OK; Builds CYD SUCCESS, Kamera 86,2 %.
- Release `v20261103-2108` (CYD + Kamera 0.1.6), auf `main` gemergt (047c0e4).
- Am Gerät installiert (CYD + Kamera 0.1.6, Nutzer 2026-11-03). Offen nur noch bei Gelegenheit: einmal `python tools/cam-snap.py COM6 --out bild.jpg`.

## Kamera schneller (0.1.5) – ERLEDIGT (Stand 2026-11-03 abends)
- Zeit: 2026-11-03 (Opus plant und prüft, Sonnet/Haiku setzen um), Branch `feature/kamera-schneller`, Releases `v20261103-2019` (CYD + Kamera 0.1.4) und `v20261103-2036` (Kamera 0.1.5). Spec `docs/superpowers/specs/2026-11-03-kamera-schneller-design.md`, Plan `docs/superpowers/plans/2026-11-03-kamera-schneller.md`.
- Ursache der ~3 Bilder/s: XIAO lief mit 160 MHz, `-Og` (Debug), PSRAM 40 MHz; dazu wurde das Bild zweimal und byteweise gedreht.
- Gemacht: `sdkconfig.defaults` 240 MHz, `-O2` (PERF), PSRAM 80 MHz, Datencache 64 KB/64 B; `cam_rotate_px2` in 16×16-Kacheln (bitgleich zur alten, Host-Test); `motion_grid_rot` liest das Bewegungsraster direkt aus dem ungedrehten Bild; Kamerabild `FRAMESIZE_HQVGA` (240×176); `CAM_DETECT_EVERY` (0.1.4: 2, 0.1.5: 1); Messzeile `S fps dps rot det` XIAO → CYD, in der `cam:`-Zeile am CYD-USB.
- Gemessen am Gerät: vorher ~3 Bilder/s. 0.1.4: 10 Bilder/s, 5 Erkennungen/s. 0.1.5: **7 Bilder/s = 7 Erkennungen/s**, Erkennung 135 ms, Drehung 10 ms. Gesichtsbreite normal ~176–215, Höhe ~245–290 (Eichwerte `CAM_GAZE_Y0 300`, `CAM_NEAR_W 350` unverändert).
- Prüfung: `pio test -e native` 240/240, `node --test` 12/12, Builds CYD 77,4 %, Kamera 86,2 % des Slots.
- Offen: Blick am Gerät bei Gelegenheit ansehen (Nutzer konnte noch nichts sagen); Rest aus dem Review unten (Snap-Puffer, `send_heart` beim Snap, `write_timeout`).

## Kamera im Roboter eingerichtet – ERLEDIGT (Stand 2026-11-03 abends)
- Zeit: 2026-11-03 (Opus mit Nutzer, Sonnet setzt um), Branch `feature/kamera-einrichtung`, letztes Release `v20261103-1747` (CYD) mit Kamera **0.1.3**.
- Ursachen, warum die Kamera nie ein Gesicht meldete (alle am Gerät belegt):
  1. **Bild um 90° gedreht** (Kamera sitzt in Lieferlage quer zum XIAO, Halter v2). Belegt per Schnappschuss. Lösung: Software-Drehung `CAM_ROTATE_CW 90` vor Erkennung und Schnappschuss (`camera_rotated`, `cam_rotate_px2`). `CAM_FLIP_180` bleibt 0.
  2. **Bild seitenverkehrt** (Schrift „LOS“ im Schnappschuss gespiegelt) → `CAM_MIRROR_X = false`.
  3. **XIAO-Update am PC-USB:** Der XIAO startet beim WLAN-Start neu (Spannungseinbruch), meldet nur `U 0`. Mit Netzteil ≥ 2 A läuft es durch. Leitung CYD↔XIAO in Ordnung (lange Zeilen kommen an, `utest`).
- Eingemessen: Sitzhöhe y ≈ 300 → `CAM_GAZE_Y0 = 300`; Gesichtsbreite normal ≈ 210, 20 cm ≈ 400 → `CAM_NEAR_W = 350`; Pupillen-Verstärkung 2,5; Glättung 0,6 (Gesicht) / 0,4 (Bewegung).
- Neu: Anwesenheit auch per **Bewegung** (`M x y n`, Raster 30×40 auf dem XIAO), Gesichtsmodell **espdet_pico_224** statt MSR+MNP (erkennt auch beim Arbeiten am Monitor 2–3 Gesichter/s; nur ~3 Bilder/s). Diagnose über USB am CYD: `snap` (`tools/cam-snap.py`, JPEG), `uinfo`, `utest`, `utestslow`; Zeile `cam:` jede Sekunde.
- Behobene Fehler: Update-Watchdog am CYD (blieb bei 0 % hängen; Fehlalarm nach 100 %), CYD gleicht Kamera-Zustand nach XIAO-Neustart ab, `cam-build.ps1` erzeugt `sdkconfig.cam` neu, wenn `sdkconfig.defaults` neuer ist, Schnappschuss am CYD gepuffert.
- Prüfung: `pio test -e native` 237/237 (Stand 0.1.3), Builds CYD 75,8 %, XIAO 86,9 %.
- Branch-Review 2026-11-03 (Sonnet, nur Quellcode): nichts Kritisches, gemergt nach `main`. Hinweise für später: (a) `camera_rotated()` dreht jedes Bild **zweimal** (in `detect_face` und `motion_detect`) → einmal pro Bild drehen, erster Schritt zum Beschleunigen; (b) Snap-Puffer am CYD 24 KB reicht nur für ~17,7 KB JPEG, Überlauf geht still verloren → Qualität 60–70 oder `J X cyd_puffer_voll` melden; (c) Kleinkram: `send_heart()` zwischen den Snap-Stücken, `write_timeout` in `tools/cam-snap.py`, Rotationspuffer (150 KB PSRAM) wird nie freigegeben.
- Offen: Kamera nur ~3 Bilder/s (Drehung doppelt und byteweise, Pico 224) – bei Bedarf beschleunigen; eigenes Kopfmodell mit ESP-Detection (Bericht: RTX 5080, ~2–3 Arbeitstage, erst wenn Stufe 1 im Alltag nicht reicht); Mood „betrübt“ unabhängig von der Kamera; Branch-Review und Merge nach `main`.

## Kamera-Halter v2 – Schacht statt Pfosten (Roboter) – ERLEDIGT bis Testdruck (Stand 2026-11-03)
- Zeit: 2026-11-02/03 (Opus), Branch `feature/kamera-halter-v2`
- Anlass: Testdruck 2026-11-02, Pfosten mit Rastlippen zu filigran.
- Gemacht: `xiao_cam_block.scad` Schacht `xiao_tray()` (6 Wandstücke, 3,0 mm, Spiel 0,3, Lücken für Lötstellen, USB-C, U.FL, Werkzeugkanal), `xiao_lip = 0`; den Block hält der Anschlag im Rückteil. Neue Prüfung `tray_fit` je Seite (Bauteil um Spiel + 0,05 muss die Wand treffen, Gegenprobe Spiel − 0,05 leer) ersetzt `asm_block_lip`. `cam_test` reicht jetzt bis zur Wand am Klappstecker-Ende (X 21,02), dünne Schalenrippe dort entfernt.
- Prüfung: `check_roboter.ps1 -Only 1,3i,3j` ALLES GRÜN (51); `-Only 1,1b,1c` und `-Only 3,3b,3f` grün; Gegenprobe `asm_block` mit Spiel −0,1 trifft; Überhang `cam_test` 0 steile Flanken, 0 Decken; Querschnitte z 0,5…15 angesehen (nichts frei hängend, obere Längswand verschmilzt mit der Schalenwand); Lötstellen 5V/GND/D6/D7 ≥ 1,0 mm frei.
- Offen: Testdruck `case/stl/roboter_cam_test.stl` (Nutzer); danach Task 3 (Kopf Stufe 2) und Task 4 des Plans `2026-11-02-kamera-halter-v2.md`.

## Kamera-Firmware Phase B – Task 7: Release und erster Gerätetest – IN ARBEIT (Stand 2026-11-01 abends)
- Zeit: 2026-11-01 (Opus mit Nutzer), Branch `feature/kamera-firmware` (d38a35b)
- Volle Prüfung: `pio test -e native` 223/223, `node --test tools/gen-phrases.test.mjs tools/ota-publish.test.mjs` 12/12, `cyd2usb` SUCCESS (Flash 75,5 %), `tools\cam-build.ps1` SUCCESS (72,1 %).
- Release `v20261101-2056` (CYD-Firmware + `cam.bin` 0.1.0, Manifest mit `cam`) per OTA am CYD installiert. Der XIAO hat schon 0.1.0, daher kein Kamera-Update angeboten.
- Am Gerät: Augen-Symbol erscheint, Einstellungen Seite 2 zeigt „verbunden“ → UART-Leitung, Lebenszeichen und `C` funktionieren.
- Problem: Das Augen-Symbol bleibt grau, es kommen also keine `F`-Zeilen. Der XIAO sendet `F` nur, wenn `detect_face` ein Gesicht findet. Verdacht 1: zu dunkel (Abendlicht; auch die Phase-A-Messung im Dunkeln hatte kaum Treffer). Verdacht 2: Die Kamera sitzt kopfüber im Gehäuse; das Gesichtsmodell erkennt nur aufrechte Gesichter (`CAM_FLIP_180` noch 0).
- Offen (nächste Sitzung):
  - Test bei Licht. Wenn nötig, Handy mit Gesichtsfoto **auf dem Kopf** vor die Kamera halten. Wird das Auge dann grün: `CAM_FLIP_180 1` in `cam/src/cam_config.h`, Version 0.1.1 (`CAM_FW_VERSION` **und** `CONFIG_APP_PROJECT_VER` in `cam/sdkconfig.defaults`), Release. Damit wird zugleich das XIAO-Update über den CYD getestet.
  - Dann Blickrichtung (`CAM_MIRROR_X`), Nähe/Schielen (`CAM_NEAR_W`), Schriftbreiten der Einstellungen am Gerät prüfen.
  - Danach das finale Branch-Review und der Merge auf main.

## Kamera-Firmware Phase B – Task 6: XIAO-Update über das Release – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `ota-publish.mjs` nimmt das cam-Image (`%LOCALAPPDATA%\3zoll-cam\cam\.pio\build\cam\firmware.bin`, `CAM_PROJECT = "cam"`, Chip 9, 200 KB bis 0x330000, `--no-cam` lässt es aus) als `ota-out/cam.bin` mit; `inspectImage(buf, project, chip)`, `buildManifest(main, cam, tag)` (Manifest-Eintrag `cam {version,url,sha256}`, URL am Release-Tag); `--release` lädt `cam.bin` hoch. Die Version kommt aus dem App-Deskriptor; damit sie `CAM_FW_VERSION` entspricht, setzt `cam/sdkconfig.defaults` `CONFIG_APP_PROJECT_VER` (bisher stand dort „1“), ota-publish bricht bei Abweichung zu `cam_config.h` ab. `ota_pull::cam_info()` (Manifest-Filter `cam`, geprüft mit `ota_redirect_ok`/`ota_hex_ok`, Version 1..15 Zeichen). `ota_cam_newer()` (a.b.c numerisch, unbekannt = kein Angebot). `net_build_cam_upd()` (SSID/Passwort aus `esp_wifi_get_config`, alle Kopien volatil gelöscht; `cam_build_upd_cmd` löscht jetzt auch seinen Stapelpuffer). Karte: Zeilenansicht mit eigenem Ja für Kamera (und CYD), Fortschrittsbalken, „Kamera aktualisiert“ (3 s), roter Grund bei `U err`, „Kamera antwortet nicht“ (Watchdog), „kein WLAN“; `cam_uart_upd_*`-Wrapper und `cam_link_upd_end`; `on_cam_msg` reicht `U` an die Karte.
- Prüfung: `test_ota_policy`, `test_cam_link`, `test_cam_proto`, `test_presence` 46/46 (Mutationen rot gesehen); `node --test` 12/12; `pio run -e cyd2usb` SUCCESS (Flash 75,5 %, RAM 18,2 %); `ota-publish --check` zeigt beide Images und das Manifest mit `cam`; cam neu gebaut (Version 0.1.0 im Deskriptor).
- Offen: Anzeige der Zeilenansicht und der echte Ablauf am Gerät (nicht geflasht); XIAO-Image in diesem Stand muss einmal per Kabel mit der neuen Version geflasht sein, bevor ein Update über das Release Sinn hat.

## Kamera-Firmware Phase B – Task 5: Einstellungen Seite 2 und Augen-Symbol – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `icon_eye` und `icon_arrow`; Einstellungen mit zweiter Seite (Zeile 0 Kamera-Schalter grün/grau, Zeile 1 Kamera-Info: grün „XIAO <Version> verbunden“, gelb „keine Verbindung“, rot „Kamera-Fehler“, grau „aus“); Seitenpfeil in Zeile 3 (Seite 1 rechts x 268..320, Seite 2 links x 0..52, Trefferfläche 52 x 52); Charakter-Balken auf Seite 1 nach links geschoben (x 192..240) und Info-Texte enden vor x 170; `SettingsAction::CAM_TOGGLE/PAGE`, `SettingsCam` an `settings_open/redraw/tick`; `app.cpp` schaltet `cam_on`, sendet `C`, speichert. Statusstreifen: `StatusInfo::cam` (0/1 grau/2 grün/3 rot), Auge bei x = 194 (s = 8, 186..202) zwischen Wetter-Temperatur und Pomodoro.
- Prüfung: `pio run -e cyd2usb` SUCCESS (Flash 75,3 %, RAM 18,1 %); HTML-Mockup `docs/superpowers/mockups/2026-11-01-einstellungen-kamera.html` per Edge headless geprüft, keine Überlappung.
- Offen: Anzeige am Gerät (Schriftbreiten der Info-Zeile, Auge im Statusstreifen) nicht geflasht geprüft.

## Kamera-Firmware Phase B – Task 4: Treiber, Speichern und Verdrahtung in `app.cpp` – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `config.h` Kamera-Abschnitt (Pins 27/22, 115200, CAM_MIRROR_X, CAM_NEAR_W). Neu `hw/cam_uart.{h,cpp}` (Serial2, Poll mit Rückruf, `send` mit gelöschtem Puffer, `send_ctrl`, `take_restart`, `cam_uart_link`). `Persist::cam_on` (NVS `cam`, Standard an), `kVersion = 4`; `store_load` verwirft nur Stände neuer als kVersion, Stände 1–3 laden weiter. `app.cpp`: `cam_active`, `on_cam_msg` (presence_msg), `mood_frame` (presence_tick, Wachhalten bei Gesicht außer nachts im Schlaf, Kamera ersetzt LDR-Dunkel, `light_update` läuft immer, NEAR nur bei steigender Flanke), `speech_frame` (WIEDER-Begrüßung nach dem Morgen-Ritual), `eyes_frame` (Blick folgt dem Gesicht nach dem Finger), Poll und C-Neusendung im `app_loop`.
- Prüfung: `pio run -e cyd2usb` SUCCESS (Flash 75,3 %, vorher 75,1 %; RAM 18,1 %); `pio test -e native -f test_mood -f test_presence -f test_cam_link` 64/64.
- Offen: Anzeige/Einstellung `cam_on` und Update-Karte (Task 5/6); CAM_MIRROR_X und CAM_NEAR_W am Gerät bestimmen; nicht geflasht.

## Kamera-Firmware Phase B – Task 3: Ausdruck NEAR (schielend) und Sätze WIEDER – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `Emotion::NEAR` (vor `COUNT`), `FaceShape::cross` (Pixel, Pupillen beider Augen zur Nase hin, im Tween gemischt), Ausdruck NEAR = Werte von SURPRISED + `cross = 6`, Mund O; `eyes.cpp` zieht die Pupille um `side * cross` nach innen. Satzgruppe `wieder` (6 Sätze) in `saetze.json`, `phrases.h` neu erzeugt (`Say::WIEDER`). Weitere Tabellen über `Emotion` gibt es nicht (nur `face_expression`, der Rest läuft über `default`).
- Prüfung: neue Tests `test_near_schielend` und `test_wieder_exists`; `pio test -e native` (face_model, phrases, mood, speech, eye_model, weather_mood, claude_face) 93/93 PASSED; `node --test tools/gen-phrases.test.mjs` 4/4; `gen-phrases --check` aktuell; `pio run -e cyd2usb` SUCCESS (Flash 75,1 %). Mutation `cross = 0` im NEAR-Ausdruck → Test rot, zurückgenommen.
- Offen: NEAR und WIEDER sind noch nirgends ausgelöst (kommt in den folgenden Phase-B-Tasks); optische Abnahme am Gerät steht aus.

## Kamera-Firmware Phase B – Task 2: `presence.h` (Anwesenheit, Begrüßung, Schlaf, Nähe, Blick) – ERLEDIGT
- Zeit: 2026-11-01 (Haiku, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `src/logic/presence.h` (`Presence`, `presence_msg`, `presence_tick`, `presence_gaze_raw`, `PRES_*`; nutzt `cam_proto.h` und `deadline.h`, Header-Namen wie im Brief, keine Anpassung nötig). Host-Tests `test/test_presence/test_main.cpp` (13 Tests: 9 aus dem Brief plus 4 zusätzlich, siehe unten).
- Prüfung: `pio test -e native -f test_presence` → vorher ERRORED (Header fehlte), danach 10/10 PASSED; nach Fix-Runde 1: 13/13 PASSED.
- Fix-Runde 1: Begrüßung nach unbeaufsichtigter Zeit (Bezug seit Aktivierung, wenn nie jemand da war); Begrüßungssperre bleibt über Kamera-Aus/An und Verbindungsabbrüche; Nähe nur mit frischem Gesicht (≤ 500 ms). Neue Tests `test_greet_after_unattended_arrival`, `test_greet_gap_survives_camera_toggle`, `test_near_needs_fresh_face`. Mutationen je rot: Begrüßung braucht `was_away` → `test_greet_after_unattended_arrival`; Reset löscht Sperre → `test_greet_gap_survives_camera_toggle`; Nähe ohne Frische-Prüfung → `test_near_needs_fresh_face`. Mutationen, je rot gesehen und zurückgesetzt: Paarzeit 1 s → 2 s → `test_present_needs_two_faces_in_1s`; Weg-Zeit 30 s → 31 s → `test_gone_after_30s`, `test_greet_after_2min_away_with_cooldown`, `test_greet_is_one_shot`; Begrüßungs-Sperrzeit aus → `test_greet_after_2min_away_with_cooldown`; `unattended` ohne `since_ms` → `test_unattended_counts_from_activation`; Nähe `>` → `>=` → `test_near_with_cooldown`; Spiegeln invertiert → `test_gaze_mapping_and_smoothing`; Glättung aus → `test_gaze_mapping_and_smoothing`.
- Abweichungen vom Brief (nur Tests): (1) `presence_tick(p, true, 0, NEAR)` vor dem ersten F, sonst verwirft `presence_msg` das F bei inaktiver Kamera und die Paarzeit wird nicht geprüft; (2) neuer Test `test_unattended_counts_from_activation` (Aktivierung bei 5 s), weil alle Brief-Tests bei 0 aktivieren und die Mutation `since_ms` sonst unentdeckt bliebe.
- Offen: Verdrahtung in der CYD-Hauptschleife (nächste Tasks Phase B), Prüfung mit echter Kamera am Gerät.

## Kamera-Firmware Phase B – Task 1: `cam_link.h` (Zeilen, Verbindung, Update-Watchdog am CYD) – ERLEDIGT
- Zeit: 2026-11-01 (Haiku, Umsetzung nach Plan), Branch `feature/kamera-firmware`
- Gemacht: `src/logic/cam_link.h` (`CamLink`, `cam_link_feed` über `CamLineBuf`/`cam_parse`, Verbindung mit 5 s Zeitlimit, `CamConn`-Zustände, Neustart-Erkennung, Update-Watchdog 20 s). Host-Tests `test/test_cam_link/test_main.cpp` (6 Tests). Feldnamen des Headers passen zum Plan, keine Anpassung nötig.
- Prüfung: `pio test -e native -f test_cam_link` → vorher ERRORED (Header fehlte), danach 6/6 PASSED. Mutationen, je rot gesehen und zurückgesetzt: Timeout `>=` → `>` (in `cam_link_connected` nachgebildet) → `test_connection_timeout`, `test_conn_states`; Neustart-Zuweisung entfernt → `test_restart_detect`; `U ok` beendet Watchdog nicht → `test_update_watchdog`; `state`-Übernahme entfernt → `test_conn_states`.
- Offen: Task 2 ff. des Kamera-Plans (CYD-Seite einbinden), Phase B am Gerät.

## Kamera-Firmware Phase A – Task 5: am Gerät abgenommen (XIAO an COM7, vor dem Löten) – ERLEDIGT bis auf Gesichts-Messwerte
- Zeit: 2026-11-01 (Opus mit Nutzer), Branch `feature/kamera-firmware`
- Volle Prüfung: `pio test -e native` 199/199, `node --test` 7/7, `pio run -e cyd2usb` SUCCESS (Flash 75,0 %), `tools\cam-build.ps1` SUCCESS (Flash 72,1 %, RAM 60 792 B).
- Flash per USB-C: `cam-build.ps1 -Upload -Port COM7`, Hash verified. Partitionstabelle am Gerät ausgelesen (0x8000): nvs 0x9000, otadata 0xe000, app0 0x10000 und app1 0x340000 (je 0x330000), spiffs, coredump.
- Protokoll: `H 0.1.0 on` alle 2 s; `C 0` → `off`, fps 0; `C 1` → Kamera neu gestartet, `on`, 12 fps, 77 ms/Bild.
- WLAN-Update (Pre-Release `cam-test-20261101`, danach gelöscht): gutes Bild → `U 0`…`U 100` in ~50 s, Neustart auf app1 Zustand 1, nach 20 s `U ok`, „neue Firmware bestaetigt“. Falsche SHA → `U err SHA-256 falsch`, kein Neustart, Kamera wieder an. Kaputtes Bild → Neustart, `H 0.1.0-broken err`, nach 60 s „Probelauf gescheitert, Rollback“, Neustart auf app1 Zustand 2, `H 0.1.0 on`. Endstand: gute Firmware 0.1.0 auf app1.
- Erkenntnisse: Ohne die aufgesteckte Stabantenne (U.FL) findet der XIAO kein WLAN (`U err kein WLAN` nach 20 s, Fehlerweg sauber). Ein Download blieb einmal bei 15 % hängen → nach 15 s `U err Zeitueberschreitung`, kein Neustart; zweiter Versuch lief durch.
- Offen: Gesichts-Messwerte (größte Entfernung, `w` normal/nah → `CAM_NEAR_W`, Richtung von `x` → `CAM_MIRROR_X`) und `CAM_FLIP_180` erst nach dem Einbau bei Tageslicht (17 Uhr Winter, zu dunkel; Probemitschnitt nur vereinzelte Treffer). Danach Phase B (CYD-Seite, Watchdog ≥ 20 s).

## Kamera-Firmware Phase A – Task 4: XIAO-Firmware (Link, Updater, Ablauf) – ERLEDIGT, gebaut (nicht geflasht)
- Zeit: 2026-11-01 (Sonnet), Branch `feature/kamera-firmware`
- Gemacht: `cam/src/link.*` (UART zum CYD + USB-Seriell, Zeilenprotokoll), `updater.*` (WLAN nur während des Updates, Umleitungen nur über `ota_redirect_ok`, Chip-/Projektprüfung S3, SHA-256 vor `Update.end`, Probelauf mit `cam_health`, Rollback), `main.cpp` endgültig (Kamera an/aus per `C`, Herzschlag, F/N, Wiederholung von `camera_start` alle `CAM_RETRY_MS`). `CAM_FLIP_180 0` in `cam_config.h` und in `camera_start()` ausgewertet. `cam_parse_upd_cmd` hat 4 Felder (Header gilt), keine Anpassung nötig.
- Abweichung: `cam/sdkconfig.defaults` + `CONFIG_MBEDTLS_PSK_MODES=y` und `CONFIG_MBEDTLS_KEY_EXCHANGE_PSK=y`; ohne sie kompiliert `ssl_client.cpp` des Arduino-Kerns leer, Linker meldet `start_ssl_client` undefiniert. `getStreamPtr()` mit `auto *`.
- Prüfung: `tools\cam-build.ps1` → SUCCESS, Flash 2 410 272 B (72,1 % von 3 342 336), RAM 60 792 B. Rollback-Testbild (`-DCAM_TEST_BROKEN`, Version `0.1.0-broken`) → SUCCESS, 2 313 040 B, abgelegt als `%TEMP%\cam-broken.bin`. Kein Flash, kein Seriell (das ist Task 5).
- Fix-Runde 1: WLAN-Zugangsdaten nie gespeichert (`WiFi.persistent(false)`, `disconnect(true, true)`); Herzschlag vor jedem Umleitungsschritt, Timeouts 4 s + 4 s; Passwort und Zeilenpuffer nach dem Auftrag genullt; Kommentar zu den MBEDTLS-PSK-Zeilen.
- Offen: Task 5 (Flashen, Rollback-Probe, Update-Probe am Gerät mit dem Nutzer), Phase B (CYD-Seite). Längste Stille zwischen zwei Herzschlägen im Update: ein Umleitungsschritt, Verbinden 4 s + TLS-Handshake 6 s + Antwort 4 s ≈ 14 s, dazu `Update.end(true)` rund 1–3 s (ungemessen). `Update.begin` löscht nicht, das Löschen läuft in `Update.write` in kleinen Stücken (unter 1 s, Schleife schlägt weiter). CYD-Watchdog (Phase B) mindestens 20 s.

## Kamera-Firmware Phase A – Task 3: `cam_xiao.h` (Box, Probelauf) und S3 in `ota_policy.h` – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet), Branch `feature/kamera-firmware`
- Gemacht: `cam_health()` (Probelauf: gut ab 20 s ohne Kamerafehler mit mindestens 5 Lebenszeichen, sonst Rollback nach 60 s; `up_ms` ist die kurze Zeit seit Start des Images, daher kein `deadline.h` nötig). `ota_policy.h`: `OTA_CHIP_ESP32S3 = 9` und `ota_image_check(..., chip = OTA_CHIP_ESP32)`; der CYD-Aufruf bleibt unverändert. `cam_box_to_face` stand schon seit Task 1. Tests: 3 neue in `test_cam_proto`, 1 in `test_ota_policy`.
- Prüfung: `pio test -e native -f test_cam_proto -f test_ota_policy` → vorher ERRORED (`OTA_CHIP_ESP32S3` fehlte), danach beide grün. Mutationen, je rot gesehen und zurückgesetzt: Box-Faktor → `test_box_center`; Klemmung → `test_box_clamped_and_degenerate`; leere Box → dasselbe; `cam_ok` → `test_health_decision`; Lebenszeichen → `test_health_decision`; 20-s-Grenze → `test_health_decision`; 60-s-Grenze → `test_health_decision`; Chip-Parameter ignoriert → `test_image_chip_s3` (+ `test_image_ok_esp32`, `test_image_rejects`). Kein `pio run -e cyd2usb` (Vorgabe: kein Build); Aufrufer in `ota_pull.cpp` bleiben durch das Standardargument gültig.
- Offen: Task 4 ff. (cam/-Firmware nutzt das), Phase B (CYD-Seite).

## Kamera-Firmware Phase A – Task 2: Zeilenprotokoll `cam_proto.h` – ERLEDIGT
- Zeit: 2026-11-01 (Sonnet), Branch `feature/kamera-firmware`
- Gemacht: `src/logic/cam_proto.h` (Prüfsumme XOR, Bauen/Parsen der Zeilen F/N/H/C/U, `CamLineBuf`), Host-Tests `test/test_cam_proto/test_main.cpp` (11 Tests). Spec Zeile 24: Zeilenmaximum 200 → 400 Zeichen.
- Abweichung vom Plan-Code: `cam_parse_upd_cmd` zerlegte 5 statt 4 Felder (url, sha, ssid, pass) und wies darum jede gültige U-Zeile ab; auf 4 Felder korrigiert.
- Prüfung: `pio test -e native -f test_cam_proto` → erst ERRORED (Header fehlte), dann 1 FAIL (`test_update_cmd_roundtrip`, der Fehler oben), nach Korrektur 11/11 grün. Gegenprobe (Prüfsummenvergleich auskommentiert) → `test_checksum_rules` FAIL, zurückgesetzt wieder 11/11.
- Fix-Runde 1: Tests für Längengrenze (400 ok / 401 weg, auch `CamLineBuf`), 5 Felder, SHA groß/klein (gespeichert klein), Worst-Case-U-Zeile; `CamLineBuf::buf` nullinitialisiert. 15 Tests grün. Mutationen, je rot gesehen und zurückgesetzt: F-Bereich → `test_rejects_garbage`; H-Zustand → `test_roundtrip_face_and_heart`; U-Prozent → `test_rejects_garbage`; U ok/err → `test_update_progress`; U-Parser Zusatzfeld → `test_update_cmd_rejects`; SHA klein → `test_update_cmd_sha_case`; `CamLineBuf`-Grenze → `test_linebuf_boundary`; `cam_seal`-Länge → `test_length_limit` + `test_length_boundary`. Ausnahme: die Längenprüfung in `cam_parse` ist redundant (kein gültiger Inhalt reicht an 401 Zeichen, max. 363) und nicht einzeln prüfbar.
- Offen: Task 3 (cam/-Firmware nutzt das Protokoll), Phase B (CYD-Seite).

## Kamera-Firmware Phase A – Task 1: Spike `cam/` (XIAO) – ERLEDIGT, am Gerät gemessen
- Zeit: 2026-10-29 (Sonnet), Branch `feature/kamera-firmware`
- Weg: **Weg 1** (pioarduino 55.03.312-1, Arduino als IDF-Komponente, ESP-DL v3 `human_face_detect`). Grund: baut sauber, neuere ESP-DL; Weg 2 (Arduino-Kern 2.0.17) bleibt Rückfall.
- Anpassungen gegenüber dem Plan: Pixeltyp heißt `DL_IMAGE_PIX_TYPE_RGB565BE` (nicht `..._RGB565`); zusätzlich `CONFIG_AUTOSTART_ARDUINO=y` in `cam/sdkconfig.defaults`, sonst fehlt `app_main` beim Linken. Erzeugtes (`managed_components/`, `dependencies.lock`, `sdkconfig.cam`, `CMakeLists.txt`) steht in `cam/.gitignore`.
- Build: SUCCESS, Flash 908 009 B von 3 342 336 B (27,2 %), RAM 36 516 B (11,1 %).
- Neu: `cam/` (Kamera, Erkennung, Mess-`main.cpp`), `src/logic/cam_xiao.h` (`CamFace`, `cam_box_to_face`), `tools/cam-serial.py` (pyserial 3.5 vorhanden).
- Am Gerät (COM7, Kabel-Flash, Opus mit Nutzer): PSRAM 8 MB, Kamera und Erkennung starten. **10–12 Bilder/s, 77–83 ms pro Bild** → Weg 1 bleibt. Gesicht aufrecht im Bild: 17 Treffer mit RGB565**BE**, 0 mit LE (Diagnose lief beide abwechselnd) → BE ist richtig.
- Gefunden und behoben:
  - Konsole und `Serial` lagen auf UART0 = GPIO 43/44 (genau die CYD-Leitung) → `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, `-DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1`.
  - „Do not support Concat“ + Absturz beim Modell-Laden: esp-dl baut nur die Operatoren angemeldeter Modelle; angemeldet hatte nur `esp-sr` (kommt mit dem Arduino-Kern). → `custom_component_remove = espressif/esp-sr`, damit `DL_COMPILE_ALL 1`.
  - Build im Dropbox-Ordner scheitert (Cloud-Platzhalter, WinError 32; `com.dropbox.ignored` greift hier nicht). → `tools/cam-build.ps1` spiegelt `cam/` und `src/logic/` nach `%LOCALAPPDATA%\3zoll-cam` und baut dort (`-Upload -Port COM7` zum Flashen). Bin: `%LOCALAPPDATA%\3zoll-cam\cam\.pio\build\cam\firmware.bin`.
- Lage der Kamera: Beim Halten in der Hand stand das Bild auf dem Kopf; aufrecht wird erkannt. Die Drehung (`CAM_FLIP_180`, per `set_vflip`/`set_hmirror`) wird erst beim Einbau festgelegt.

## Kamera + Gehäuse – Teil 1: Gehäuse-Entwurf – ERLEDIGT, wartet auf Testdruck
- Zeit: 2026-10-26 (Opus plant und prüft, Sonnet baut das SCAD-Modell), Branch `feature/kamera-gehaeuse`
- Entscheidung: Kamera = **Seeed XIAO ESP32S3 Sense** (eigener Rechner mit PSRAM, erkennt Gesichter selbst, schickt dem CYD nur Koordinaten). Direkt am CYD geht keine Kamera: Es sind nur GPIO 22/27/35 frei, und PSRAM fehlt. Bestellt **ohne Header**. Spec: `docs/superpowers/specs/2026-10-26-kamera-gehaeuse-design.md`.
- Gemacht: `case/madbuddy_case.scad` (parametrisch, OpenSCAD 2021.01 per winget), Form „Kopf mit Stirn“ 114 × 82 × 30 mm, Linse mittig über dem Display, Fuß mit 15°, nur USB-C nach außen (`open_micro_usb = false`), STL-Dateien `front`/`back`/`frame_test`, Prüfskript `case/check.ps1`.
- Maße: Platine, Löcher und Display aus der Originaldoku (witnessmenow/ESP32-Cheap-Yellow-Display: `Dimensions.png`, Datenblatt JC2432A028N). USB-Lage vom Nutzer gemessen (ab SD-Kante: Micro ab 9,67 mm, USB-C 17,88–27,32 mm). Die übrigen Werte sind im SCAD als VORLÄUFIG markiert.
- Prüfung: `& case\check.ps1 -NoPng` → alles grün (3 STL ohne Warnung, Stand: Lot 6,9 mm vor der hinteren Kippkante, nichts unter dem Fuß, Kollision leer, Selbsttest erkennt Treffer). Gegenprobe `-Define window_shrink=2` → `[FAIL] Kollision` (288 Facetten), exit 1. Danach läuft kein OpenSCAD-Prozess mehr.
- Hinweis: `powershell -ExecutionPolicy Bypass` wird von der Rechteprüfung abgelehnt, deshalb das Skript direkt mit `& case\check.ps1` starten.
- Nächster Schritt: Nutzer druckt `case/stl/madbuddy_frame_test.stl` und meldet die Passung (Fenster über der Leuchtfläche, Lochbild). Danach Maße anpassen und das ganze Gehäuse drucken.
- Offen: Teil 2 Verkabelung (Pinreihenfolge P1/CN1 am Foto prüfen, bevor gelötet wird). Teil 3 Firmware (`cam/` für den XIAO, `src/logic/cam_link.*` + `Serial2` am CYD), wird geplant, sobald die Hardware da ist. Der USB-C des XIAO ist im eingebauten Zustand nicht erreichbar, deshalb vor dem Einbau flashen und danach per WLAN aktualisieren.

## Umbenennung CYD-Buddy -> MadBuddy – ERLEDIGT (2026-10-25)

Umbenennung CYD-Buddy -> MadBuddy (Portal, mDNS madbuddy.local, Repo M4dm0nky/madbuddy).

## Lokal 4 – Code-Review und Fixes – ERLEDIGT
- Zeit: 2026-10-25 (Review: 3 Sonnet-Prüfer, Funde von Opus am Code/Core-Quellen bestätigt; Fixes: Sonnet in Paketen A/B/C + Nachtrag)
- Behoben: 24,8-Tage-Fehler aller langlebigen Zeitpunkte (`logic/deadline.h`, `due_since`), Nickerchen weckbar, OTA-Umleitung lehnt `@`/fremde Ports ab, Claude-Reihenfolge (späte Ereignisse), Empfänger prüft Schlüssel/Länge vor dem Körper + Handler einmal + Multipart-Grenzen, **Rollback wirksam** (`verifyRollbackLater`, `health_tick` unabhängig vom WLAN), Stall-Lücke im Download, Start-Absicherung (Touch-Warten 10 s, Kalibrier-Plausibilität), 5. Tipp außerhalb der Augen, Simon-Zeitlimit 30 s, Rückblick ignoriert erste Sekunde, Sprite-Neuversuch, OTA-Karte nicht während Arbeit, Speichern vor OTA-Neustart (Haupt-Task), Einstellungen ohne Flackern, Animationen ohne Float-Drift, Aufräumen (testscreen, tote Felder, Kommentare, Doku).
- Prüfung: 180/180 nativ, 18/18 Node, Build Flash 1 475 391 B. Am Gerät (v20261025-2011): `Zustand 1` nach OTA, nach 20 s „neue Firmware als gut bestaetigt“; curl: 100 KB ohne Schlüssel 401, falscher Schlüssel 401, 5 KB 413, Content-Length 1 MB gelogen 413 (alle < 0,35 s, danach Verbindungsabbruch wie vorgesehen), Multipart ohne Schlüssel 401, gültig 204, Ping 200; Heap danach 120 KB, Minimum 60 212 B (vorher 49 776 B); fps 24–26.
- Offen (nicht im Paket): Bildrate (SPI 80 MHz testen, Ring-Spur, Brillen-Raster), Aufteilung von `app.cpp`, Projektkennung im Image generisch, keine Signatur.

## Lokal 3 – Gesicht, F1–F4, Umlaute, Claude-Modus am Gerät – ERLEDIGT
- Zeit: 2026-10-25 (lokal, Opus orchestriert; Sonnet/Haiku umgesetzt; Qwen Sätze-Entwurf)
- Gemacht: Gesicht mit Brauen/Pupillen/Nase/Mund und 22 Ausdrücken; F1–F4 aus der Cloud übernommen und einzeln ausgeliefert (v20261025-1529/-1539/-1549/-1611); echte Umlaute mit U8g2 Latin-1 (v-1637); Claude-Modus (Zustandslogik, HTTP-Empfänger mit Schlüssel, Darstellung, Hook-Skript; v-1648/-1703), globale Hooks in `~/.claude/settings.json` (Sicherung `settings.json.bak-20261025-1703`).
- Prüfung: `pio test -e native` → 153/153; `node --test` (ota-publish, gen-phrases, claude-hooks) → 18/18; `pio run -e cyd2usb` → Flash 1 473 891 B. Am Gerät: Endpunkte 200/401/413/400 gemessen; Hook-Ereignisse kommen an (`running` während Bash); Nutzer bestätigt drehenden Ring, „Mist!“ bei Fehler, Blick + gelbe LED bei Rückfrage; fps 24–26 (Ring kurz 24), Heap-Minimum 49 776 B (vorher 63 KB, Webserver).
- Gefunden & behoben: Open-Meteo chunked → `useHTTP10`; Sonnenbrille zu hoch/blind → mittig + Schachbrett-Raster; erstes Hook-Ereignis ging verloren, weil Node `cyd-buddy.local` beim ersten Mal langsamer als 800 ms auflöst → Konfiguration zuerst per IP (192.168.2.137), Name als Ersatz; Sätze-Check CRLF unter Windows.
- Offen: fps mit Ring knapp unter 25; Heap-Minimum 49,8 KB knapp für TLS (OTA-Abruf im Blick behalten, ggf. Sprite während Manifest-Abruf freigeben); feste IP im Router (Nutzer); Mitte-unten-Touch reagiert kaum (Panel).

## Interaktion Task 10 – Abschluss – ERLEDIGT
- Zeit: 2026-10-25 14:22 UTC
- Gemacht: README-Bedienung ergänzt (High-Five, Blick folgt, Menü 2×3, Füttern, Timer, Sprechblasen, Rituale, Rückblick, Hunger/Krankheit/Charakter); dabei die veralteten Abschnitte „Schlaf“ (jetzt Uhrzeit + Ruhe) und „Testbild“ (jetzt Touch-Einrichtung) an den lokalen Stand angepasst; Sätze-Generator unter Entwicklung. Abschlussprüfung.
- Dateien: README.md
- Prüfung: `pio test -e native` → 124 Tests, 0 Fehler (14 Gruppen; 78 alte + 46 neue: look 4, phrases 3, speech 14, care 17, pomodoro 5, je 1 neu in mood, face_model, weather_mood). `node --test tools/*.test.mjs` → 7 Tests, 0 Fehler. `node tools/gen-phrases.mjs --check` → aktuell. `pio run -e cyd2usb` → SUCCESS, RAM 17,5 % (57 340 B, +376 B gegenüber 56 964 B vor F1), Flash 74,5 % (1 464 019 B); `firmware.bin` 1 493 696 B < 1 966 080 B
- Gegenprobe: je Logik-Gruppe in Task 1, 2, 3, 4, 6, 7, 8 (siehe dort)
- Nächster Schritt: **Lokaler Claude** übernimmt `claude/dreamy-galileo-bvdw39` nach `buddy-umsetzung` (Fast-Forward), prüft am Gerät und veröffentlicht je Funktion ein Release (Spec: F1–F4 einzeln). Hinweis: Alle vier Funktionen liegen in einem Branch; für getrennte Releases die Commits von Task 1–2 (F1), 3–5 (F2), 6–7 (F3), 8–9 (F4) nacheinander bauen.
- Offen/Fragen (Sammlung): (1) Branch `claude/dreamy-galileo-bvdw39` statt `buddy-umsetzung` (Sitzungsvorgabe). (2) frech/verschmust über die Achse `cuddly` (Task 4). (3) SICK als erstes *Grundgefühl* (Task 6). (4) Rückblick-Schwellen, Nacht-Ritual 22:00–22:59 (Task 4). (5) Ring als Ellipse (Task 9). (6) FEED-Bildschirm mit Schließen-Kreuz (Task 7). (7) `store_load` übernimmt alle Versionen 1…3. Keine Sätze ergänzt, `saetze.json` unverändert; `src/ota/*`, `ota_policy.h`, `ota-publish.mjs`, `display.h`, `config.h`, `main.cpp` unverändert. Kein Release, kein Flash, kein PR.
- Am Gerät zu prüfen: **F1** Pupillen folgen dem Finger, High-Five in beiden oberen Ecken (Hand, LED weiß), Mitte oben = normales Tippen. **F2** Blasen lesbar (Breite, Umlaute als ae/oe/ue), höchstens alle 90 s, Morgengruß + „Heute …° und …“, 22-Uhr-Gähnen, Rückblick um 20 Uhr und Nachholen per Berührung bis 23 Uhr, Zähler nach Neustart erhalten (NVS v3). **F3** Menü 2×3 trifft, Fütter-Bildschirm, Flug + Kauen, lecker/Lieblingsessen/satt, Hunger-Gesicht, Krankheit + Hatschi (zum Testen Werte im NVS senken), Heilung, Charakter-Balken. **F4** Timer starten/stoppen, FOCUSED-Gesicht, Ring, Restzeit, LED 5× grün / 2× blau, Pomodoro-Zähler im Rückblick. Überall: fps ≥ 25, Heap-Minimum.

## Interaktion Task 9 – Pomodoro (UI) – ERLEDIGT
- Zeit: 2026-10-25 14:21 UTC
- Gemacht: Step 3: `app.cpp` – Menü → TIMER (15/25/45 starten mit Blase „pomodoro_start“, Stopp, Zurück). Während WORK: Gefühl FOCUSED (außer Schlaf), keine Eigenarten, keine Blasen außer Start, keine Rituale, Tippen ohne Überraschung (nur Blick folgt), kein Nacht-/Nickerchen-/Dunkel-Schlaf. Ring im Sprite (Arbeit pink, Pause grün), Restzeit im Statusstreifen. BREAK_START: EXCITED 3 s, „pomodoro_pause“, LED 5× grün, `pomodoros` +1. BREAK_END: „pomodoro_fertig“, LED 2× blau, danach IDLE. Pomodoro läuft auf allen Bildschirmen weiter.
- Dateien: src/app.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,5 % (57 340 B), Flash 74,5 % (1 464 019 B). `pio test -e native` → 124 Tests, 0 Fehler
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 10
- Offen/Fragen: (1) Streicheln/Stupsen ändern während der Arbeit weiter Werte und Zähler, nur ohne Blase. (2) Läuft beim Pausenbeginn ein anderer Bildschirm, ist die Blase nach 4 s weg; die LED blinkt trotzdem.
- Am Gerät zu prüfen: Timer-Kacheln treffen, Ring flüssig (fps ≥ 25), Restzeit läuft, LED-Signale, FOCUSED-Gesicht.

## Interaktion Task 9 – Pomodoro (UI) – TEILWEISE
- Zeit: 2026-10-25 14:19 UTC
- Gemacht: Step 2: Fortschrittsring als dünne Ellipse am Sprite-Rand (rx ~153, ry ~96, dunkle Spur + Bogen im Uhrzeigersinn ab oben, `eyes_set_ring(progress, Farbe)`), Restzeit mm:ss im Statusstreifen (`StatusInfo.timer_s/timer_break`, Arbeit pink, Pause grün).
- Dateien: src/ui/eyes.h, src/ui/eyes.cpp, src/ui/status_bar.h, src/ui/status_bar.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 9 Step 3 (Verdrahtung in app.cpp: TIMER-Bildschirm, FOCUSED, keine Eigenarten/Blasen außer Start, kein Schlaf während WORK, BREAK_START/END mit Blase und LED, Zähler)
- Offen/Fragen: Ring als Ellipse statt Kreis: ein Kreis um die Mitte schnitte die äußeren Augenränder (x 58 bei Radius 98).

## Interaktion Task 9 – Pomodoro (UI) – TEILWEISE
- Zeit: 2026-10-25 14:18 UTC
- Gemacht: Step 1: Bildschirm TIMER `src/ui/timer_view.*` (Kacheln 15/25/45 mit Sanduhr, Zurück; läuft ein Timer: Stopp-Kachel mit Quadrat + Zurück; 10 s Timeout).
- Dateien: src/ui/timer_view.h, src/ui/timer_view.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 9 Step 2 (Fortschrittsring im Sprite, Restzeit im Statusstreifen), Step 3 (Verdrahtung: FOCUSED, LED-Signale, Zähler)
- Offen/Fragen: –

## Interaktion Task 8 – Pomodoro (Logik) – ERLEDIGT
- Zeit: 2026-10-25 14:16 UTC
- Gemacht: `src/logic/pomodoro.h`: Zustände IDLE/WORK/BREAK, `pomo_start` (nur 15/25/45 min), Pause fest 5 min, `pomo_tick` → NONE/BREAK_START/BREAK_END (nächste Phase beginnt am Ende der vorigen), nach BREAK_END IDLE, `pomo_stop`, `pomo_progress` 0..1, `pomo_remaining_s` aufgerundet.
- Dateien: src/logic/pomodoro.h, test/test_pomodoro/test_main.cpp
- Prüfung: `pio test -e native -f test_pomodoro` vorher → ERRORED (Header fehlt); danach 5 Tests, 0 Fehler
- Gegenprobe: Pause ab spätem Abfragen statt Phasenende → `test_late_tick…: Expected 240 Was 300`; Restzeit abgerundet → `Expected 1500 Was 1499`; jede Minutenzahl erlaubt → `test_start_only_15_25_45` FAIL; alles zurück → 5 PASS
- Nächster Schritt: Interaktion Task 9 Step 1
- Offen/Fragen: –

## Interaktion Task 7 – Füttern und Charakter (UI) – ERLEDIGT
- Zeit: 2026-10-25 14:15 UTC
- Gemacht: Step 5: Info-Zeile der Einstellungen mit zwei Charakter-Balken (Herz ↔ Blitz für `cuddly`, Ball ↔ Kissen für `playful`; Mitte = 0, Ausschlag zur jeweiligen Seite). Info-Text gekürzt auf Build-Datum + „Heap …k Licht …“.
- Dateien: src/ui/settings.cpp, src/app.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,5 % (57 292 B), Flash 73,9 % (1 452 891 B). `pio test -e native` → 119 Tests, 0 Fehler
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 8
- Offen/Fragen: –
- Am Gerät zu prüfen: Menü 2×3 trifft (Touch-Kalibrierung), Fütter-Bildschirm, Flug + Kauen flüssig, Reaktionen (lecker / Lieblingsessen / satt), Hatschi bei Krankheit, Charakter-Balken.

## Interaktion Task 7 – Füttern und Charakter (UI) – TEILWEISE
- Zeit: 2026-10-25 14:14 UTC
- Gemacht: Step 4: `OV_SNEEZE = 2048` + Test; Tropfen an der Nase und Augen 600 ms zu, Blase „krank“. Füttern: Menü → FEED → Speise fliegt 600 ms von unten zum Mund (ins Sprite), Mund kaut 3× (mouth_open 0↔0,6 alle 200 ms), dann `food_feed` → Reaktion + Blase, `meals`-Zähler. Charakter: Streicheln/Stupsen/Spiel/High-Five/Langeweile ändern `cuddly`/`playful`, `mood_set_character` jedes Bild, Gedanken-Blasen nach `cuddly`. Krankheit, Charakter, Lieblingsessen (beim ersten Start ausgelost) werden gespeichert.
- Dateien: src/logic/weather_mood.h, test/test_weather_mood/test_main.cpp, src/ui/eyes.h, src/ui/eyes.cpp, src/app.cpp
- Prüfung: `pio test -e native -f test_weather_mood` vorher → ERRORED (`OV_SNEEZE` fehlt), danach 7 Tests, 0 Fehler. `pio run -e cyd2usb` → SUCCESS, RAM 17,5 % (57 292 B), Flash 73,9 % (1 452 207 B)
- Gegenprobe: `OV_SNEEZE = 4096` → `test_hand_bit: Expected 2048 Was 4096`; zurück → PASS
- Nächster Schritt: Interaktion Task 7 Step 5 (Charakter-Balken im Info-Bereich der Einstellungen)
- Offen/Fragen: –

## Interaktion Task 7 – Füttern und Charakter (UI) – TEILWEISE
- Zeit: 2026-10-25 14:12 UTC
- Gemacht: Step 3: Bildschirm FEED `src/ui/feed.*` (Sättigungsbalken grün ≥ 60 / gelb ≥ 25 / rot, Thermometer bei Krankheit, Schließen oben rechts, 4 Kacheln Apfel/Pizza/Kuchen/Kaffee, 10 s Timeout). Neue Symbole `icon_food`, `icon_thermometer`, `icon_ball`, `icon_pillow`, `icon_stop`.
- Dateien: src/ui/feed.h, src/ui/feed.cpp, src/ui/icons.h, src/ui/icons.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 7 Step 4 (Flug-/Kau-Animation, OV_SNEEZE, Verdrahtung Charakter/Krankheit/Füttern in app.cpp), Step 5 (Charakter-Balken in den Einstellungen)
- Offen/Fragen: Schließen-Kreuz im FEED-Bildschirm ist eine Ergänzung (Spec nennt nur „Tipp → zurück zum Gesicht“ nach der Wahl).

## Interaktion Task 7 – Füttern und Charakter (UI) – TEILWEISE
- Zeit: 2026-10-25 14:10 UTC
- Gemacht: Step 2: Menü 2×3 (Kacheln 96×90, Abstand 8): Spielen grün, Füttern orange (Apfel), Timer rot-violett (Sanduhr), Uhr gelb, Einstellungen violett, Schließen rot; Spielen-Ebene: Fang mich, Simon, Zurück. `MenuChoice::FEED/TIMER` neu (in app.cpp noch ohne Wirkung).
- Dateien: src/ui/menu.h, src/ui/menu.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 7 Step 3 (FEED-Bildschirm `src/ui/feed.*`)
- Offen/Fragen: –

## Interaktion Task 7 – Füttern und Charakter (UI) – TEILWEISE
- Zeit: 2026-10-25 14:09 UTC
- Gemacht: Step 1: `Persist` um `sick`, `cuddly`, `playful`, `favorite_food` erweitert (Sättigung in `needs.satiety`), NVS-Schlüssel `sat sick cud pla fav`, `kVersion` 3 mit Übernahme von 1/2.
- Dateien: src/hw/store.h, src/hw/store.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 7 Step 2 (Menü 2×3 mit Füttern/Timer), Step 3 (FEED-Bildschirm), Step 4 (Flug-/Kau-Animation, OV_SNEEZE, Verdrahtung), Step 5 (Charakter-Balken in den Einstellungen)
- Offen/Fragen: –

## Interaktion Task 6 – Füttern, Krankheit, Charakter (Logik) – ERLEDIGT
- Zeit: 2026-10-25 14:08 UTC
- Gemacht: `mood.h`: `Needs.satiety` (Start 70, wach −0,12/min, schlafend −0,04/min, auch für die Aus-Zeit), Grundgefühl HUNGRY bei < 25 (nach SLEEPY, vor SAD), Krankheit nach 2 h ununterbrochen Sättigung < 15 und Zuneigung < 20 (SICK als erstes Grundgefühl, doppelter Energie-Abbau, `mood_sneeze_due` alle 1–3 min), Heilung bei Sättigung > 60 und danach 3× Streicheln → HAPPY 3 s. Neue Emotionen HUNGRY, FULL, SICK, FOCUSED mit Werten aus der Spec in `face_expression`. `food.h` (`food_effect`, `food_feed` → Say LECKER/LIEBLINGSESSEN/SATT, FULL bei > 95). `character.h` (`cuddly`/`playful` mit den Spec-Schritten, ±100, Wirkung über `mood_set_character`: Eigenarten-Abstand, CHEEKY-Wahrscheinlichkeit, LOVE-Dauer). `speech_for_base` kennt jetzt HUNGRY/SICK.
- Dateien: src/logic/mood.h, src/logic/food.h, src/logic/character.h, src/logic/face_model.h, src/logic/speech.h, test/test_care/test_main.cpp, test/test_face_model/test_main.cpp
- Prüfung: `pio test -e native -f test_care` vorher → ERRORED (`logic/food.h` fehlt); `test_face_model` vorher → `test_new_emotions_f3_f4: Expected 8 Was 0`. Danach `pio test -e native` → 119 Tests, 0 Fehler (alle bestehenden unverändert grün). `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: Hunger-Grenze 25 → 20 → `test_hungry_rank` FAIL; Krankheit nach 1 h → 2 Tests FAIL; Heilung nach 2× Streicheln → `test_heal…` FAIL; satt ab > 94 → `test_feed_full_strictly_above_95` FAIL; Eigenarten-Abstand ohne Charakter → 2 Tests FAIL; FOCUSED open .75 → `test_new_emotions_f3_f4` FAIL; alles zurück → 119 PASS
- Nächster Schritt: Interaktion Task 7 Step 1
- Offen/Fragen: (1) „SICK Rang direkt nach SLEEPING“ als *Grundgefühl* umgesetzt (vor SLEEPY), kurzzeitige Gefühle (z. B. Reaktion aufs Essen) gehen weiter vor. (2) Eigenarten wählen CHEEKY jetzt mit `cheeky_prob` (Start 0,5) statt Münzwurf über ein Bit – bestehender Test „beide kommen vor“ grün. (3) Heil-Streicheln zählen jede STROKE-Geste (auch im 2-s-Cooldown), sobald die Sättigung > 60 ist. (4) Kaffee gibt laut Spec nur Sättigung/Energie; Lieblingsessen kann auch Kaffee sein.

## Interaktion Task 5 – Sprechblasen und Rückblick (UI) – ERLEDIGT
- Zeit: 2026-10-25 14:03 UTC
- Gemacht: Step 3: `app.cpp` – Blase 4 s (ins Sprite), Gedanken/Gefühlswechsel über `speech_due`, Ereignis-Sätze (STROKE → verliebt 25 % mit 90-s-Abstand, POKE → genervt, High-Five → highfive, Gewitter-Schreck → gewitter, neues Wetter mit geändertem Himmel → hitze/kalt/regen/sonne), Rituale (Morgen: GREETING + „morgen“, 4 s später Tageswetter „Heute 14° und Regen“; Nacht 22 Uhr: YAWN + „nacht“; Rückblick 20 Uhr bzw. Nachholen bei Berührung bis 23 Uhr → Bildschirm `REVIEW`, Tippen schließt). Tageszähler (Streicheln, Stupsen, Spiele, Siege) in `Persist.day`, NVS-Schlüssel `d_*`, `kVersion` 2 (Stand 1 wird übernommen, fehlende Schlüssel → 0).
- Dateien: src/app.cpp, src/hw/store.h, src/hw/store.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,5 % (57 220 B, +224 B gegenüber Task 0), Flash 73,6 % (1 447 827 B). `pio test -e native` → 101 Tests, 0 Fehler
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 6
- Offen/Fragen: (1) `store_load` nimmt jetzt jede Version 1…kVersion an (vorher nur genau kVersion), damit Bedürfnisse/Helligkeit/Ort nach dem Update bleiben. (2) Ereignis-Blasen (POKE, High-Five, Gewitter, Wetter) ersetzen eine gerade sichtbare Blase. (3) „gestreichelt“ zählt STROKE-Gesten (alle 1,5 s beim Dauerstreicheln eine).
- Am Gerät zu prüfen: Blase lesbar und Brauen nur kurz überdeckt, Morgengruß + Wettersatz, 22-Uhr-Gähnen, Rückblick um 20 Uhr, Zähler nach Neustart erhalten.

## Interaktion Task 5 – Sprechblasen und Rückblick (UI) – TEILWEISE
- Zeit: 2026-10-25 14:01 UTC
- Gemacht: Step 2: Bildschirm REVIEW `src/ui/review.*` („Dein Tag mit mir“, 5 Zeilen Symbol + Zahl: gestreichelt, gestupst, Spiele gewonnen/gesamt, gegessen, Pomodoros; unten kleines Augenpaar mit `daystats_mood`; 12 s). Neue Symbole `icon_heart`, `icon_bolt`, `icon_apple`, `icon_hourglass`.
- Dateien: src/ui/review.h, src/ui/review.cpp, src/ui/icons.h, src/ui/icons.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS (vor der Verschiebung des Augenpaars nach unten; die Verschiebung ist eine Zahl)
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 5 Step 3 (Verdrahtung in app.cpp: Blasen, Ereignis-Sätze, Rituale, Rückblick-Bildschirm, Tageszähler in Persist/NVS mit kVersion 2)
- Offen/Fragen: –

## Interaktion Task 5 – Sprechblasen und Rückblick (UI) – TEILWEISE
- Zeit: 2026-10-25 13:58 UTC
- Gemacht: Step 1: `src/ui/bubble.*` (abgerundete Blase oben mittig y 2..32, hellgrau, Zipfel nach unten, Text schwarz FreeSansBold9pt7b über ascii.h, Breite nach Text). `eyes_draw(..., const char *bubble = nullptr)` zeichnet sie ins Sprite.
- Dateien: src/ui/bubble.h, src/ui/bubble.cpp, src/ui/eyes.h, src/ui/eyes.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 5 Step 2 (Bildschirm REVIEW `src/ui/review.*`), dann Step 3 (Verdrahtung: Sätze, Rituale, Tageszähler in NVS)
- Offen/Fragen: –

## Interaktion Task 4 – Sprechregeln, Rituale, Tageszähler – ERLEDIGT
- Zeit: 2026-10-25 13:56 UTC
- Gemacht: `speech.h` (`speech_init/pick/may/shown/due`: 90 s Abstand, 40 % beim Wechsel des Grundgefühls zu BORED/SAD/HAPPY, Gedanken alle 5–10 min, bei |cuddly| > 40 verschmust/frech, kein Satz zweimal hintereinander je Kategorie), `rituals.h` (Morgen 5–12 Uhr einmal pro Tag beim ersten Aufwachen, Nacht 22 Uhr einmal, Rückblick 20:00 bzw. Nachholen bei Berührung bis 23 Uhr, `weather_sentence` „Heute 14° und Regen“, `day_key`), `daystats.h` (Zähler, `daystats_roll` setzt bei neuem Tag zurück, `daystats_mood` für den Rückblick).
- Dateien: src/logic/speech.h, src/logic/rituals.h, src/logic/daystats.h, test/test_speech/test_main.cpp
- Prüfung: `pio test -e native -f test_speech` vorher → ERRORED (`logic/speech.h` fehlt); danach 14 Tests, 0 Fehler. `pio test -e native` → 101 Tests, 0 Fehler
- Gegenprobe: Wiederholungs-Sperre aus → `test_pick_never_repeats` FAILED; 90 s → 60 s → `test_cooldown_90s` FAILED; Rückblick bis 23 → bis einschl. 23 → `test_review_20_or_catch_up_until_23` FAILED; `pokes > strokes` → `>=` zunächst **nicht** erkannt → Test ergänzt (gleich viel Streicheln wie Stupsen → CONTENT), dann FAILED; alles zurück → PASS
- Nächster Schritt: Interaktion Task 5 Step 1
- Offen/Fragen: (1) „frech bzw. verschmust“: Spec sagt nur „|Charakter| > 40“. Umgesetzt mit der Achse `cuddly`: > 40 → verschmust, < −40 → frech (passt zu „CHEEKY-Wahrscheinlichkeit 0,5 − cuddly/200“). (2) HUNGRY/SICK als Grundgefühl gibt es erst mit Task 6; `speech_for_base` bekommt sie dort. (3) Nacht-Ritual: „um 22:00“ als erstes wache Bild zwischen 22:00 und 22:59 (sonst ginge es verloren, wenn er genau um 22:00 in einem Menü ist). (4) Schwellen im Rückblick-Gefühl (≥ 10 Streicheln → HAPPY, ≥ 5 Stupsen und mehr als Streicheln → ANNOYED) sind meine Wahl, die Spec nennt keine Zahlen. (5) `speech_init` bekommt zusätzlich die Startzeit.

## Interaktion Task 3 – Sätze als Header – ERLEDIGT
- Zeit: 2026-10-25 13:53 UTC
- Gemacht: `tools/gen-phrases.mjs` erzeugt aus `saetze.json` den Header `src/logic/phrases.h` (`enum class Say` in JSON-Reihenfolge ohne `_quelle`, Tabellen `const char *const` UTF-8, `say_list(Say)` → {items, n}); `--check` meldet einen veralteten Header. Erzeugte Datei mit committet. Sätze unverändert.
- Dateien: tools/gen-phrases.mjs, tools/gen-phrases.test.mjs, src/logic/phrases.h, test/test_phrases/test_main.cpp
- Prüfung: `node --test tools/gen-phrases.test.mjs` vor dem Skript → ERR_MODULE_NOT_FOUND; vor dem Erzeugen 3/4 (Test „eingecheckte phrases.h ist aktuell“ FAIL); danach `node --test tools/*.test.mjs` → 7 Tests, 0 Fehler. `pio test -e native -f test_phrases` → 3 Tests, 0 Fehler (24 Kategorien, je ≥ 6, nach ASCII ≤ 28 Zeichen, kein unbekanntes Zeichen). `pio run -e cyd2usb` → SUCCESS
- Gegenprobe: Grenze 28 → 20 → `test_ascii_max_28: Hallo du! Endlich wach. [FAILED]`; zurück → PASS
- Nächster Schritt: Interaktion Task 4
- Offen/Fragen: –

## Interaktion Task 2 – Blick folgt und High-Five (UI) – ERLEDIGT
- Zeit: 2026-10-25 13:51 UTC
- Gemacht: Step 2: Hand (Handfläche, 4 Finger, Daumen zur Mitte) in Augenfarbe in der getippten oberen Ecke, ploppt in 150 ms auf (`eyes_set_hand(right, since)`). Step 3: `app.cpp` – solange der Finger auf dem Augen-Bildschirm liegt (Touch-Beginn und Streicheln) `look_at`, aktives Blickziel ersetzt den Gazer; TAP in Ecke → `MoodEvent::HIGHFIVE` statt TAP, `OV_HAND` 1,5 s, LED 250 ms weiß.
- Dateien: src/ui/eyes.h, src/ui/eyes.cpp, src/app.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,4 % (56 996 B, +32 B), Flash 73,1 % (1 437 795 B). `pio test -e native` → 84 Tests, 0 Fehler
- Gegenprobe: – (UI)
- Nächster Schritt: Interaktion Task 3
- Offen/Fragen: Blickziel wird in jedem Bild mit Finger neu gesetzt (nicht nur bei Beginn) – deckt „während des Streichelns“ ab, Pupillen folgen also auch beim Halten.
- Am Gerät zu prüfen: Pupillen folgen dem Finger, High-Five in beiden oberen Ecken (Hand, LED weiß, EXCITED), Mitte oben bleibt normales Tippen.

## Interaktion Task 2 – Blick folgt und High-Five (UI) – TEILWEISE
- Zeit: 2026-10-25 13:49 UTC
- Gemacht: Step 1: `OV_HAND = 1024` in weather_mood.h + Test `test_hand_bit` (Bit-Wert, `overlays_for` setzt es nie).
- Dateien: src/logic/weather_mood.h, test/test_weather_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_weather_mood` vorher → ERRORED (`OV_HAND` nicht deklariert); danach 7 Tests, 0 Fehler
- Gegenprobe: `OV_HAND = 2048` → `test_hand_bit` FAILED; zurück → 7 PASS
- Nächster Schritt: Interaktion Task 2 Step 2 (Hand in eyes.cpp zeichnen), dann Step 3 (app.cpp: Blick folgt, High-Five, LED weiß), Build
- Offen/Fragen: –

## Interaktion Task 1 – Blick folgt und High-Five (Logik) – ERLEDIGT
- Zeit: 2026-10-25 13:48 UTC
- Gemacht: `src/logic/look.h` (`LookState`, `look_at` mit Abbildung (x−160)/160·30 und (y−100)/100·15, geklemmt, 3000 ms; `look_active`, `look_target`, `highfive_hit` für x<64 oder x>256 bei y<56). `MoodEvent::HIGHFIVE` in mood.h: Unterhaltung +5, Zuneigung +2, EXCITED 2 s.
- Dateien: src/logic/look.h, test/test_look/test_main.cpp, src/logic/mood.h, test/test_mood/test_main.cpp (neuer Test `test_highfive`, bestehende unverändert)
- Prüfung: `pio test -e native -f test_look` vor Implementierung → ERRORED (`logic/look.h` fehlt); danach 4 Tests, 0 Fehler. `pio test -e native -f test_mood` → 36 Tests, 0 Fehler
- Gegenprobe: Ecken-Grenze y<56 → y<80 → `test_highfive_corners: Expected FALSE Was TRUE`; HIGHFIVE fun +5 → +4 → `test_highfive: Expected 55 Was 54`; beides zurück → PASS
- Nächster Schritt: Interaktion Task 2 Step 1
- Offen/Fragen: –

## Interaktion Task 0 – Werkzeuge und Branch (Cloud) – ERLEDIGT
- Zeit: 2026-10-25 13:47 UTC
- Gemacht: Basis `origin/buddy-umsetzung` (00ad726). Diese Cloud-Sitzung darf nur auf **`claude/dreamy-galileo-bvdw39`** pushen; der Branch wurde per Fast-Forward auf `buddy-umsetzung` gebracht und trägt ab hier die Interaktions-Commits. Werkzeuge wie unter „Task 0 – Werkzeuge (Cloud)“ (gleicher Container, noch eingerichtet).
- Dateien: –
- Prüfung: `pio test -e native` → 78 Tests, 0 Fehler; `pio run -e cyd2usb` → SUCCESS, RAM 17,4 % (56 964 B), Flash 73,1 % (1 436 759 B); `node --test tools/*.test.mjs` → 3/3
- Gegenprobe: –
- Nächster Schritt: Interaktion Task 1
- Offen/Fragen: Lokal `claude/dreamy-galileo-bvdw39` nach `buddy-umsetzung` übernehmen (Fast-Forward möglich, solange `buddy-umsetzung` nicht weiterläuft).

## Lokal 2 – WLAN, Wetter, Touch, OTA am Gerät – ERLEDIGT
- Zeit: 2026-10-25 (lokal, Opus)
- Gemacht: WLAN-Portal eingerichtet (am Handy ging die Seite nicht auf, vom PC über dessen WLAN-Adapter ging sie: Gerät ok, Handy bleibt vermutlich auf mobilen Daten). Wetter repariert (`useHTTP10`, Open-Meteo antwortete chunked). Manuelles Update: Einstellungen → Firmware zeigt die Karte sofort. Tipps nutzen die letzte gültige Position statt (0,0). **Touch-Kalibrierung** mit 4 Eckpunkten beim ersten Start, im NVS (`touchcal`); Finger beim Einschalten = neu kalibrieren. Gemessen: Bildschirm-x aus Roh-x (umgekehrt), y aus Roh-y; Werte `3679,310 3672,3689 516,371 474,3703`. Die frühere Eck-Messung ohne gezeichnete Ziele war falsch, die Konstanten in config.h sind nur noch Startwerte.
- Prüfung am Gerät: Menü → Uhr trifft (Seriell `geste 1 bei x=249 y=56 screen=1` → screen 2), Uhr zeigt Zeit, Datum, Wetter mit Temperatur (Nutzer). **OTA Ende zu Ende gemessen:** Release v20261025-1259 → „Update bereit“ → Ja → `ota: ok, Neustart` → `Partition app1, Zustand 2` (gültig, **kein Rollback**, wie beim HA-Buddy). Heap-Minimum beim HTTPS-Abruf 63 312 B.
- Releases: v20261025-1240, -1259 (veraltet), **-1350 = aktueller Stand** (ELF 971f8741…, auch per Kabel aufgespielt).
- Lehre: Nach jedem Kabel-Flash sofort ein Release mit genau diesem Stand, sonst „aktualisiert“ das Gerät auf ein älteres Release. Kabel-Flash, wenn das Gerät auf app1 läuft: `boot_app0.bin` an 0xe000 mitschreiben, nur die App an 0x10000 (nicht das Factory-Image, das löscht NVS/WLAN).
- Nicht geprüft am Gerät: Streicheln/Stupsen, Nachtschlaf (22–7 Uhr), Spiele, Overlays (Sonnenbrille usw.), unterer Bildrand reagiert kaum auf Touch.
- Nächster Schritt: Diagnose-Ausgaben (`touch`, `geste`, `touch verworfen`) wieder entfernen oder hinter ein Build-Flag, Rest am Gerät prüfen.

## Lokal 1 – Übernahme, erster Flash, Lichtsensor – ERLEDIGT
- Zeit: 2026-10-25 (lokal, Opus)
- Gemacht: Cloud-Branch `claude/dreamy-galileo-bvdw39` als `buddy-umsetzung` übernommen und lokal komplett nachgeprüft. Open-Meteo-URLs per curl geprüft (Geocoding HTTPS ok, Wetter über **http** ok, 200). Per Kabel geflasht. Lichtsensor gemessen und als unbrauchbar erkannt → Schlafen nach Uhrzeit + Ruhe (`mood_touch`, `mood_night_check`, 4 neue Tests). `ota-publish --release` setzt den Tag jetzt mit `--target <Commit>` und verweigert nicht gepushte Commits.
- Dateien: src/logic/mood.h, test/test_mood/test_main.cpp, src/app.cpp, src/config.h, tools/ota-publish.mjs, docs/superpowers/specs/…design.md
- Prüfung: `pio test -e native` → 51 Tests, 0 Fehler; `node --test tools/ota-publish.test.mjs` → 3/3; `pio run -e cyd2usb` → SUCCESS, Flash 1 431 499 B. Am Gerät (Seriell): Panel ST7789, `invert=0` richtig (Nutzer: Farben stimmen, türkise Augen), 29 fps, heap 123 732 / min 122 104 bei offenem Portal, OTA `app0 Zustand -1`. Touch: Rohwert in der Mitte (1880, 2450), Lang drücken öffnet das Menü (Nutzer).
- Gegenprobe: Ruhezeit 10 → 20 min → 3 Tests FAIL; zurück → PASS.
- Messung LDR (GPIO 34, Rohwert, Mittel über 16): Raumlicht/dämmrig 0, einmal 13 s lang ~200, Display nach unten 0. Bekannt: Spannungsteiler für ESP8266 (1 V) ausgelegt, Display leuchtet den Sensor an (Arduino-Forum „Cheap Yellow Display LDR Sensor“). Schwelle jetzt 325 ± 75, `LDR_DARK_IS_HIGH 1`.
- Hardware: **Auto-Reset geht an diesem Board nicht** (DTR erreicht GPIO0 nicht, gemessen: `boot:0x13` auch bei DTR aktiv während EN). Kabel-Flash nur so: esptool mit `--connect-attempts 0` starten, dann BOOT ~15 s halten.
- Nebenbei: `.pio` ist per `com.dropbox.ignored` von Dropbox ausgenommen (Dropbox sperrte Build-Dateien → ERRORED-Tests).
- Nächster Schritt: WLAN-Portal am Handy einrichten, dann OTA Ende zu Ende mit erstem Release (spielt diesen Stand ohne Kabel auf).
- Offen/Fragen: Touch-Kalibrierung der Ecken nur indirekt (Menü-Kacheln treffen) zu prüfen.

## Task 14 – Doku und Abschluss – ERLEDIGT
- Zeit: 2026-10-25 10:48 UTC
- Gemacht: `docs/ota.md` (GitHub-Release, keine Geheimnisse in der .bin, Ablauf, Prüfungen, Alltag, Grenzen: Rollback ungeprüft, erster Flash per Kabel) und `README.md` (Gesten, Menü, Portal „CYD-Buddy“, Ort, Entwicklung, Verweis auf docs/ota.md). Abschlussprüfung.
- Dateien: docs/ota.md, README.md
- Prüfung: `pio test -e native` → 47 Tests, 0 Fehler (8 Gruppen: mood 13, gesture 7, eye_model 6, weather_mood 4, games 7, ota_policy 5, ascii 2, smoke 3). `node --test tools/*.test.mjs` → 3 Tests, 0 Fehler. `pio run -e cyd2usb` → SUCCESS, RAM 17,4 % (56 860 B von 327 680 B, statisch), Flash 72,8 % (1 431 027 B); `firmware.bin` 1 459 792 B < 1 966 080 B
- Gegenprobe: je Testgruppe in Task 1–6, 10, 13 gemacht (siehe dort)
- Nächster Schritt: **Lokaler Claude übernimmt** den Branch `claude/dreamy-galileo-bvdw39` (siehe Task 0: die Cloud-Sitzung durfte nicht auf `buddy-umsetzung` pushen). Am Gerät messen und prüfen (Liste unten), Messwerte in `src/config.h`, dann erstes Release.
- Offen/Fragen (Sammlung): (1) Branch-Name, s. o. (2) `OTA_IMG_CHECK_BYTES`/`CHECK_BYTES` 176 → 208 (Fehler der Vorlage, Task 6). (3) `SimonState::INPUT` → `AWAIT_INPUT` (Arduino-Makro, Task 11). (4) Open-Meteo-URLs in der Cloud nicht prüfbar (Task 9). (5) `node --test tools/` geht mit Node 22 nicht → `node --test tools/*.test.mjs` (Task 13). (6) Lichtsensor meldet nie „dunkel“, solange `LDR_DARK_THRESHOLD` ≤ 0; `LDR_DARK_IS_HIGH` fehlt in config.h (Task 7). (7) WiFiManager-Upload-Seite per Neustart gesperrt (Task 9). (8) Kein Release, kein Flash, kein PR (wie verlangt).
- Am Gerät zu prüfen (aus Spec „Prüfung“): Testbild/Farben (Panel ST7789 vs. ILI9341, `CYD_INVERT`), Touch-Kalibrierung; Augen flüssig (fps ≥ 25 im Seriell-Log); Gesten (Tippen, Streicheln, Stupsen, Lang drücken); Lichtsensor (Richtung, Schwelle, Hysterese → config.h); Portal „CYD-Buddy“ mit Ort; Wetter im Statusstreifen und als Overlays; Uhr (Zeit, Datum, Wetter, Augen von unten); Spiele Fang mich und Simon; Einstellungen (Helligkeit, Portal, Firmware, Info); OTA Ende zu Ende mit zweitem Release (Karte, Später, Ja, Fortschritt, Neustart, Zustand im Seriell-Log `ota: Partition … Zustand …`); Heap-Minimum nach Manifest-Abruf.

## Task 13 – Veröffentlichungs-Skript (tools/ota-publish.mjs) – ERLEDIGT
- Zeit: 2026-10-25 10:46 UTC
- Gemacht: Skript aus der Vorlage portiert: `.pio/build/cyd2usb/firmware.bin`, Chip 0 („falscher Chip (erwartet ESP32)“), Slot-Grenze 1 966 080, `manifest.json`/`firmware.bin`, Git-Prüfpfade `src tools platformio.ini`, `--serve`/`--ha`/`uploadHa`/`secretToken` entfernt, `--release` ruft `gh release create v<YYYYMMDD-HHMM> … --latest --repo M4dm0nky/cyd-buddy` auf. CLI nur bei direktem Aufruf (`import.meta.url === pathToFileURL(...)`). `PROJECT = 'arduino-lib-builder'` (aus `--check`).
- Dateien: tools/ota-publish.mjs, tools/ota-publish.test.mjs
- Prüfung: vor dem Skript `node --test tools/*.test.mjs` → FAIL (ERR_MODULE_NOT_FOUND); danach → 3 Tests, 0 Fehler. `node tools/ota-publish.mjs --check` → `Projektname im Image: "arduino-lib-builder"`, ELF f31cbf477819e8eb…, Größe 1 459 792 B (74,2 % des Slots). Schreibmodus mit `--allow-dirty` → `ota-out/firmware.bin` + `manifest.json` korrekt (danach gelöscht). **`--release` nicht ausgeführt.**
- Gegenprobe: `CHIP_ESP32` 0 → 9 → alle 3 Tests FAIL; zurück → 3 PASS
- Nächster Schritt: Task 14 Step 1
- Offen/Fragen: (1) `node --test tools/` (wie im Plan) geht mit Node 22 nicht: Node sucht ein Modul namens `tools` (MODULE_NOT_FOUND). Richtig ist `node --test tools/*.test.mjs` (auch in der Doku so). (2) `CHECK_BYTES` = 208 statt 176 (siehe Task 6). (3) Datum/Zeit in `esp_app_desc_t` kommen aus den vorgebauten Core-Bibliotheken („Sep 16 2026 21:15:45“), nicht vom eigenen Build; `built` im Manifest ist deshalb die Änderungszeit der .bin. (4) Zusätzlich exportiert: `imageProject(buf)`.

## Task 12 – OTA-Abruf portieren (ota_pull, ota_card) – ERLEDIGT
- Zeit: 2026-10-25 10:44 UTC
- Gemacht: `ota_pull.*` aus der Vorlage mit allen 9 Änderungen des Plans: Includes/diag entfernt (→ Serial), `kBase` GitHub-Release + `manifest.json`, neue `open_follow()` (Umleitungen selbst, ≤ 3 Sprünge, jede URL durch `ota_redirect_ok`, sonst „Umleitung abgelehnt“; TLS mit `useBuiltinCACertBundle()`), JsonDocument Standard-Allocator mit Filter, `malloc` statt PSRAM, Gesundheitsnachweis ohne HA, `project_name()` bleibt, `size > 1966080` → „zu gross fuer den Slot“, `loop()` über `net_task_hook`. `ota_card.*`: Karte „Update bereit“ (Ja/Später, Tipps < 1 s ignoriert), Fortschritt mit schmalen „konzentrierten“ Augen, Fehlerkarte. `app.cpp`: OTA-Bildschirm nur aus EYES bei wachem Display, `eyes_free()` bei DOWNLOADING, `eyes_begin()` danach, `ui_alive()` je Augenbild, Firmware-Zeile + „jetzt suchen“ in den Einstellungen, Update-Symbol im Statusstreifen.
- Dateien: src/ota/ota_pull.h, src/ota/ota_pull.cpp, src/ui/ota_card.h, src/ui/ota_card.cpp, src/app.cpp, docs/FORTSCHRITT.md (Zeitangaben korrigiert)
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,4 % (56 860 B), Flash 72,8 % (1 431 027 B); `ls -l .pio/build/cyd2usb/firmware.bin` → 1 459 792 B < 1 966 080 B. `pio test -e native` → 47 Tests, 0 Fehler
- Gegenprobe: –
- Nächster Schritt: Task 13 Step 1
- Offen/Fragen: (1) Zeitangaben der früheren Einträge waren geschätzt und falsch; jetzt auf die Commit-Zeiten korrigiert. (2) Plan nennt die Datei `src/net/net.cpp` (Hook) und `src/ui/settings.cpp` (Firmware-Zeile) als zu ändern: Der Hook war schon in Task 9 da, die Firmware-Zeile kommt über `settings_firmware_source()` aus `app.cpp` – beide Dateien blieben unverändert. (3) Speicher während des Manifest-Abrufs: TLS (~40–50 KB) läuft bei angelegtem Sprite (64 KB); am Gerät `min` im Heap-Log prüfen.
- Am Gerät zu prüfen: OTA Ende zu Ende mit zweitem Release (Karte, Später = 1 h Ruhe, Ja → Laden → Neustart, Fehlerkarte).

## Task 11 – Spiel-Bildschirme (Fang mich, Simon) – ERLEDIGT
- Zeit: 2026-10-25 10:41 UTC
- Gemacht: „Fang mich“ (kleines cyan Auge, alte Stelle schwarz übermalt, Restzeit-Balken + Treffer-Punkte oben) und „Simon“ (4 Felder rot/grün/blau/gelb, 500 ms hell / 200 ms Pause, Tipp leuchtet 150 ms). Spielfeld 320×200 unter 40-px-Kopf (`GAME_TOP`). Ende: 2 s großes Augenpaar (EXCITED bzw. BORED), dann EYES und `mood_event(GAME_WON/GAME_LOST)`. Seed aus `esp_random()`.
- Dateien: src/ui/game_common.*, src/ui/game_catch.*, src/ui/game_simon.*, src/app.cpp, src/logic/simon.h
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 428 B), Flash 72,2 % (1 420 259 B). `pio test -e native` → 47 Tests, 0 Fehler (8 Gruppen)
- Gegenprobe: –
- Nächster Schritt: Task 12 Step 1
- Offen/Fragen: **Abweichung:** `SimonState::INPUT` heißt jetzt `SimonState::AWAIT_INPUT`, weil Arduino `INPUT` als Makro definiert (Build-Fehler „expected identifier before numeric constant“). Der Test aus dem Plan benutzt `INPUT` nicht und läuft unverändert.
- Am Gerät zu prüfen: Spiele spielbar (Treffer-Toleranz, Tempo), Ergebnis-Augen.

## Task 10 – Menü, Uhr, Einstellungen – ERLEDIGT
- Zeit: 2026-10-25 10:38 UTC
- Gemacht: Menü 2×2 Kacheln (Spielen grün, Uhr gelb, Einstellungen violett, Schließen rot; 2. Ebene Fang-mich/Simon + Zurück), Uhr 15 s (Font7, Datum „So 25.10.“, Wetter, Augen schauen von unten herein; ohne Zeit „--:--“ + rotes WLAN), Einstellungen (Helligkeit 0→1→2→0, WLAN/Ort → Portal, Firmware-Zeile über Quelle ab Task 12, Info: Build-Datum, Heap, LDR). `app.cpp` mit `enum class Screen`; Stimmung/Licht laufen auf allen Bildschirmen weiter. Neu `logic/ascii.h` (UTF-8 → ASCII, die eingebauten Schriften haben keine Umlaute) mit Test.
- Dateien: src/ui/menu.*, src/ui/clock_view.*, src/ui/settings.*, src/ui/ui_common.*, src/logic/ascii.h, test/test_ascii/test_main.cpp, src/app.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 340 B), Flash 72,1 % (1 417 923 B). `pio test -e native -f test_ascii` → 2 Tests, 0 Fehler
- Gegenprobe: `ü → "u"` statt `"ue"` → `test_umlauts` und `test_truncates` FAILED; zurück → 2 PASS
- Nächster Schritt: Task 11 Step 1
- Offen/Fragen: Abweichungen: `menu_touch(x, y, now_ms)` und `settings_touch(x, y, now_ms)` bekommen die Zeit (für den 10-s-Timeout). Zusätzlich `settings_tick`, `settings_timed_out` (30 s, nicht im Plan) und `settings_firmware_source`. Langes Drücken öffnet das Menü nicht, solange der Buddy schläft (dunkles Display, sichere Variante).
- Am Gerät zu prüfen: Kachel-Treffer (Touch-Kalibrierung), Lesbarkeit der Uhr, Helligkeitsstufen.

## Task 9 – Netz (net.*) – ERLEDIGT
- Zeit: 2026-10-25 10:35 UTC
- Gemacht: WiFiManager nicht blockierend (AP „CYD-Buddy“, Feld „Ort“ id `place`, 47 Zeichen), Portal beim ersten Start bzw. nach 30 s ohne WLAN; NTP mit `configTzTime`; Geocoding per HTTPS (`NetworkClientSecure::useBuiltinCACertBundle()` – im Core-Header `NetworkClientSecure.h` nachgesehen; Bundle ist laut sdkconfig `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL`); Wetter alle 30 min per HTTP, Fehler → 5 min; JSON mit Filter. Abrufe im `net_task` (Kern 0, Prio 1, Stack 8192); nur der Haupt-Task schreibt `Persist`/NVS. `app.cpp`: Stunde an mood, Statusstreifen aus net, Wetter-Overlays (nur frisch < 3 h), Aus-Zeit einmalig nachgerechnet, Epoche beim Speichern.
- Dateien: src/net/net.h, src/net/net.cpp, src/app.cpp
- Prüfung: Step 1 `curl` → **nicht geprüft, weil** das Cloud-Netz `geocoding-api.open-meteo.com` sperrt (403 vom Proxy) und `api.open-meteo.com` nicht auflöst. `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 292 B), Flash 71,6 % (1 407 107 B von 1 966 080 B)
- Gegenprobe: –
- Nächster Schritt: Task 10 Step 1
- Offen/Fragen: (1) URLs ungeprüft – lokal mit den beiden `curl`-Befehlen aus Plan Task 9 Step 1 gegenprüfen (Wetter bleibt wie in der Spec auf `http://`). (2) Sicherheit: WiFiManager registriert die Upload-Seite `/update` immer, auch wenn sie im Menü fehlt; das Portal-AP ist offen. Deshalb Menü nur „wifi, exit“, Info-Update aus und `setPreOtaUpdateCallback` → `ESP.restart()`, damit niemand über das Portal Firmware aufspielen kann. (3) Von Hand geöffnetes Portal schließt nach 5 min wieder, wenn WLAN verbunden ist (nicht im Plan, damit das AP nicht ewig offen bleibt). (4) Der Ort wird nach dem Geocoding durch den Namen aus der Antwort ersetzt (z. B. „Berlin“).
- Am Gerät zu prüfen: Portal „CYD-Buddy“ erscheint, Ort-Feld, Verbindung, Uhrzeit, Wetter im Statusstreifen.

## Task 8 – Augen zeichnen und Hauptschleife – ERLEDIGT
- Zeit: 2026-10-25 10:31 UTC
- Gemacht: Augen-Renderer mit 8-Bit-Sprite 320×200 (Fallback direkt), Overlays (Herz, Sonnenbrille, Schweiß, Träne, zZz, Zittern, Regen-Tönung), Symbole aus Primitiven, Statusstreifen (zeichnet nur bei Änderung), `app.cpp` mit Bildtakt 33 ms, Gesten → Stimmung, Licht → Schlaf, Blinzeln/Blick/Tween, LED 15 %, Hintergrundlicht, Speichern alle 10 min, Seriell-Log alle 60 s. Testbild nach `ui/testscreen.cpp` ausgelagert (beim Start gedrückt halten).
- Dateien: src/ui/eyes.h, src/ui/eyes.cpp, src/ui/icons.h, src/ui/icons.cpp, src/ui/status_bar.h, src/ui/status_bar.cpp, src/ui/testscreen.h, src/ui/testscreen.cpp, src/app.h, src/app.cpp, src/main.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,6 % (34 736 B statisch; Sprite 64 000 B kommt zur Laufzeit dazu), Flash 27,3 % (537 303 B). `pio test -e native` → 45 Tests, 0 Fehler (7 Gruppen)
- Gegenprobe: – (Renderer nicht unit-getestet)
- Nächster Schritt: Task 9 Step 1
- Offen/Fragen: (1) Symbol-Funktionen nehmen `LovyanGFX &` statt `LGFX &` (Basisklasse; `LGFX` passt weiter, dazu gehen Sprites). Zusätzliche Symbole: `icon_target`, `icon_simon`, `icon_brightness`, `icon_back`. Zusätzlich `status_invalidate()` und `eyes_draw_pair()` (für Spiele/OTA). (2) Lid-Schnitte beziehen sich auf die *geöffnete* Höhe `h*open`, nicht auf `h` – sonst wäre SLEEPY (open 0,35, top_cut 0,5) ganz zu. (3) Sonnenbrillen-Rahmen grau statt schwarz, weil Schwarz auf schwarzem Grund unsichtbar wäre. (4) Mond-Symbol setzt schwarzen Hintergrund voraus.
- Am Gerät zu prüfen: Augen flüssig (fps im Seriell-Log, Ziel ≥ 25), Gesten (Tippen/Streicheln/Stupsen/Lang drücken), LDR-Richtung/-Schwelle, Testbild beim Start gedrückt halten.

## Task 7 – Hardware-Schicht (LED, Licht, Speicher) – ERLEDIGT
- Zeit: 2026-10-25 10:27 UTC
- Gemacht: `led.*` (ledcAttach/ledcWrite, im Core-Header `esp32-hal-ledc.h` geprüft: `bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution)`, `bool ledcWrite(uint8_t pin, uint32_t duty)`), `light.*` (Ringpuffer 16, alle 60 ms, Hysterese), `store.*` (Preferences, Namensraum `buddy`, Versions-Schlüssel `ver`).
- Dateien: src/hw/led.h, src/hw/led.cpp, src/hw/light.h, src/hw/light.cpp, src/hw/store.h, src/hw/store.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B) – Module noch nicht benutzt
- Gegenprobe: – (keine Unit-Tests für Hardware vorgesehen)
- Nächster Schritt: Task 8 Step 1
- Offen/Fragen: `LDR_DARK_IS_HIGH` fehlt in config.h → in light.cpp Standard 1 mit Kommentar „am Gerät messen“. Sichere Variante: Solange `LDR_DARK_THRESHOLD` ≤ 0 (Platzhalter), meldet `light_update` nie „dunkel“, sonst würde der Buddy mit Schwelle 0 dauernd schlafen.

## Task 6 – OTA-Regeln portieren (ota_policy.h) – ERLEDIGT
- Zeit: 2026-10-25 10:26 UTC
- Gemacht: Vorlage kopiert, `OTA_CHIP_ESP32 = 0`, Kommentar auf `test/test_ota_policy`, neu `ota_redirect_ok(url)` (nur `https://`, Host `github.com` oder Endung `.githubusercontent.com`, Groß/Klein egal, Host nur aus Buchstaben/Ziffern/`-`/`.`).
- Dateien: src/logic/ota_policy.h, test/test_ota_policy/test_main.cpp
- Prüfung: `pio test -e native -f test_ota_policy` vor Implementierung → ERRORED (Header fehlt); mit unveränderter Vorlage → SIGSEGV; nach Korrektur → 5 Tests, 0 Fehler
- Gegenprobe: Chip-Konstante 0 → 9 → `test_image_ok_esp32: Expected 0 Was 3` und `test_image_rejects` FAILED; zurück → 5 PASS
- Nächster Schritt: Task 7 Step 1
- Offen/Fragen: **Abweichung von der Vorlage:** `OTA_IMG_CHECK_BYTES` war 176, `ota_image_check` liest die ELF-SHA aber aus Byte 176…207 (`32 + 144 + i`). Mit genau 176 Byte Puffer (wie im Test) liest es über das Ende hinaus → Absturz. Jetzt `32 + 176 = 208`. Dasselbe steckt in der Vorlage `ota-publish.mjs` (`CHECK_BYTES = 176`, liest bis 208) und betrifft vermutlich auch den HA-Buddy (dort harmlos, weil der Puffer größer ist). In Task 13 im Skript ebenfalls auf 208 gesetzt.

## Task 5 – Spiel-Logik (simon.h, catch_game.h) – ERLEDIGT
- Zeit: 2026-10-25 10:24 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/simon.h` und `src/logic/catch_game.h` nach den Interfaces im Plan (Zufall über `rng_range` aus eye_model.h).
- Dateien: src/logic/simon.h, src/logic/catch_game.h, test/test_games/test_main.cpp
- Prüfung: `pio test -e native -f test_games` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: Gewinnprüfung in `simon_press` auf `len == 5` → `test_simon_win: Expected 0 Was 2 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 6 Step 1
- Offen/Fragen: –

## Task 4 – Wetter-Laune und Overlays (weather_mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:24 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/weather_mood.h` mit WMO-Zuordnung, Flags hot/cold, `weather_fresh` (< 3 h), `overlays_for`.
- Dateien: src/logic/weather_mood.h, test/test_weather_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_weather_mood` vor Implementierung → ERRORED (Header fehlt); danach → 4 Tests, 0 Fehler
- Gegenprobe: `> 28` → `>= 28` → `test_flags: Expected FALSE Was TRUE [FAILED]`; zurück → 4 PASS
- Nächster Schritt: Task 5 Step 1
- Offen/Fragen: –

## Task 3 – Augenmodell (eye_model.h) – ERLEDIGT
- Zeit: 2026-10-25 10:23 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/eye_model.h` mit Tabelle (switch), xorshift32, `eye_tween`/`rgb_tween` (k = 1 − e^(−dt/150)), Blinker und Gazer nach Plan.
- Dateien: src/logic/eye_model.h, test/test_eye_model/test_main.cpp
- Prüfung: `pio test -e native -f test_eye_model` vor Implementierung → ERRORED (Header fehlt); danach → 6 Tests, 0 Fehler
- Gegenprobe: Blinzelabstand 2000–6000 → 7000–9000 → `test_blinker_closes_within_window` und `test_blinker_reopens` FAILED; zurück → 6 PASS
- Nächster Schritt: Task 4 Step 1
- Offen/Fragen: –

## Task 2 – Gesten (gesture.h) – ERLEDIGT
- Zeit: 2026-10-25 10:23 UTC
- Gemacht: Test aus dem Plan wörtlich, `src/logic/gesture.h` nach den Regeln im Plan (Tipp-Ringpuffer der letzten 5, Streichel-Fenster 1,5 s, Bewegungsschwelle 15 px).
- Dateien: src/logic/gesture.h, test/test_gesture/test_main.cpp
- Prüfung: `pio test -e native -f test_gesture` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: `GESTURE_STROKE_PX` 60 → 200 → `test_stroke: Expected 2 Was 0 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 3 Step 1
- Offen/Fragen: –

## Task 1 – Stimmung (mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:22 UTC
- Gemacht: Test aus dem Plan wörtlich übernommen, `src/logic/mood.h` exakt nach Plan.
- Dateien: src/logic/mood.h, test/test_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_mood` vor Implementierung → ERRORED (`logic/mood.h: No such file`); danach → 13 Tests, 0 Fehler
- Gegenprobe: `+= 8` → `+= 7` in STROKE → `test_stroke_cooldown: Expected 58 Was 57 [FAILED]`; zurückgeändert → 13 PASS
- Nächster Schritt: Task 2 Step 1
- Offen/Fragen: –

## Task 0 – Werkzeuge (Cloud) – ERLEDIGT mit Umweg
- Zeit: 2026-10-25 10:22 UTC
- Gemacht: PlatformIO 6.2.0 per pip. Die PlatformIO-Registry (`api.registry.platformio.org`, `collector.platformio.org`) ist im Cloud-Netz gesperrt (403), GitHub geht. Umweg nur außerhalb des Repos: Proxy-CA an `~/.platformio/penv/.../certifi/cacert.pem` angehängt (pioarduino überschreibt `REQUESTS_CA_BUNDLE`), `tool-scons` 4.11.1 aus PyPI nach `~/.platformio/packages/tool-scons` gelegt, `platform-native` aus GitHub nach `~/.platformio/platforms/native`, Bibliotheken per `git clone` mit `.piopm` nach `.pio/libdeps/` (LovyanGFX 1.2.34, WiFiManager 2.0.17, ArduinoJson 7.4.3, Unity 2.6.1; bei Unity Kommentarzeilen aus `library.json` entfernt). `platformio.ini` unverändert.
- Dateien: keine im Repo
- Prüfung: `pio test -e native -f test_smoke` → 3 Tests, 0 Fehler. `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B von 1 966 080 B)
- Gegenprobe: –
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Lokal mit normalem Internet ist kein Umweg nötig. Branch: Die Cloud-Sitzung darf nur auf `claude/dreamy-galileo-bvdw39` pushen, nicht auf `buddy-umsetzung` (Vorgabe der Sitzung). Der lokale Claude übernimmt also `claude/dreamy-galileo-bvdw39` (ggf. lokal in `buddy-umsetzung` umbenennen).

## Task 0 – Gerüst und Testbild – ERLEDIGT (lokal)
- Zeit: 2026-10-25
- Gemacht: PlatformIO-Gerüst (cyd2usb + native), Pins/Display-Klasse, Testbild, Smoke-Test, OTA-Vorlagen aus dem HA-Buddy unter reference/.
- Dateien: platformio.ini, src/config.h, src/hw/display.h, src/main.cpp, src/logic/clamp.h, test/test_smoke/, reference/ha_buddy_ota/
- Prüfung: siehe Commit „chore: Gerüst“ (Build und Smoke-Test lokal)
- Gegenprobe: Smoke-Test mit falschem Erwartungswert schlägt fehl
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Panel-Variante, Touch-Kalibrierung, LDR-Richtung/-Schwelle misst der lokale Claude am Gerät.
