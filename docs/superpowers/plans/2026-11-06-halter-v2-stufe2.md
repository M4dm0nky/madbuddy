# Kamera-Halter v2 abschließen: Kopf Stufe 2 + Ausgaben/Anleitung

## Context
Der Roboter nutzt schon den neuen Kamera-Halter v2: `case/xiao_cam_block.scad`, XIAO und Kamera als Block im Schacht, ohne Riegel. Task 1+2 und die Schacht-Nachbesserung sind auf `main`.
Der Kopf Stufe 2 (`case/madbuddy_case.scad`, USB-Variante) bindet noch die alte `xiao_cam_common.scad` ein, mit Riegel, Nuten und FPC-Bogen.
Anleitung und READMEs erwähnen den Riegel noch (26 Stellen), und `madbuddy_cam_clip.stl` liegt noch im Repo.
Offen sind Task 3 und Task 4 aus `docs/superpowers/plans/2026-11-02-kamera-halter-v2.md`.

Der Nutzer hat entschieden: Im Kopf passt der volle Schacht nicht. Frei sind ~19 mm zwischen Display-Kante und Stirn-Innenhaut, der Schacht braucht 24,4 mm. Deshalb bekommt der Kopf **nur die Endwände** (USB-Ende und Klappstecker-Ende). Die Kameratasche legt den Block in X/Y fest, der Anschlag im Rückteil hält ihn in Z. Das Aussehen des Kopfs bleibt gleich.

Druckrückmeldungen zum Kopf-Nachdruck und zum Sitz im Schacht stehen noch aus. Sie ändern hieran nichts.

## Ablauf
Branch `feature/halter-v2-stufe2` von `main`. Umsetzung per Subagent, Sonnet für SCAD/Prüfskript und Doku. Opus prüft, macht den Volllauf, committet und merged.
Allen Subagenten ausdrücklich mitgeben:
- Kein `python - <<EOF`, kein `find` über große Bäume.
- Nach Abbruch oder Timeout eigene `openscad`/`msedge`-Prozesse suchen und beenden.
- Ausgaben nur gefiltert lesen.
- Keine Claude-Attribution in Commits und Doku.
- SCAD-Kommentare deutsch mit ae/oe/ue, Herkunft je Maß.

### Task 1: Kopf Stufe 2 auf den Block umstellen (Sonnet)
1. `case/xiao_cam_block.scad`:
   - Optionaler Parameter, den die einbindende Datei vor dem include setzt (Muster wie `cam_chamfer_ang`, Zeile 37): `cam_tray_idx`. Vorgabe sind alle 6 Schachtstücke, damit bleibt der Roboter unverändert.
   - `xiao_cradle_add` baut nur die gewählten Stücke aus `xiao_tray()` (Zeile 103–115).
   - Kopfkommentar Zeile 1–2 korrigieren: Die Datei gilt jetzt für Roboter **und** Kopf.
2. `case/madbuddy_case.scad`:
   - Vor dem include `cam_tray_idx = [2,3,4,5];` setzen (nur Endwände), dann `include <xiao_cam_block.scad>` statt `xiao_cam_common.scad`.
   - `xiao_cy` neu festlegen: Unterkante XIAO samt Spiel (Linse − 8,25 − 0,3) liegt ≥ `pcb_h/2 + 0,4` (gleiche Regel wie bisher, ergibt ≈ 33,95).
   - In `front_shell` die Endwände mit dem Innenraum des Kopfs schneiden (`offset(r=-wall+0.01) outer2d()`, Y ≥ `pcb_h/2 + fit`). Sie wachsen dann innen an die Kopfwand an und ragen weder nach außen noch ins Display.
   - `back_part`: Anschlagpfosten Zeile 149–153 nach dem Muster `cam_stop()` aus `madbuddy_roboter.scad:516` bauen, Anschlagfläche `xiao_zbot(front_t) − xiao_stop_gap`. Lötstellen am USB-Ende (u 2,9/5,4) und u 18,1 bleiben frei.
   - Kabelschienen und Lüftungsschlitze (Zeile 172) über `xiao_pos` statt `xiao_offset` führen, ebenso `cable_dummy` (Zeile 211–213).
   - Entfernen: Teil `cam_clip`, `clip_test`, `clip_band`, FPC-`echo` in `info` (Zeile 268f.) und Riegel in `assembly` und `cam_section`.
   - `cradle_fit` umstellen: Block um Spiel + 0,05 aufgeweitet muss die Endwände treffen, mit Gegenprobe −0,05 leer. Neue Teile `lens_tip`/`lens_tip_ctrl` und `asm_block`/`asm_block_ctrl` nach dem Roboter-Muster (`madbuddy_roboter.scad:1177–1206`).
   - Neues Teil `cam_test`: Stirnausschnitt um Tasche und Endwände, flach druckbar.
3. `case/check.ps1`:
   - `cam_clip` aus der STL-Liste nehmen (Zeile 24/32), `cam_test` aufnehmen.
   - Riegel- und FPC-Prüfungen entfernen (Zeile 123f., 134–143).
   - `lens_tip` + Kontrolle, `asm_block` + Kontrolle und `cradle_fit` + Gegenprobe eintragen.
   - Kamera im Bild `kamera_schnitt` (Zeile 153) auf das neue `xiao_cy` setzen.
4. Gegenproben zeigen:
   - `front_t` testweise so, dass die Linse vorsteht → `lens_tip` rot.
   - `-Define window_shrink=2` bleibt rot.
5. `& case\check.ps1 -NoPng` → alles grün. Roboter gezielt nachprüfen, weil `xiao_cam_block.scad` geändert ist: `check_roboter.ps1 -NoPng -Only 1,3i,3j` (Schacht und `cam_test` unverändert).
6. `case/xiao_cam_common.scad` löschen, sobald nichts sie mehr einbindet (Grep).
7. Commit `feat(case): Kamera-Halter v2 im Kopf Stufe 2 (Endwaende, ohne Riegel)`.

### Task 2: Ausgaben, Anleitung, Schlussprüfung (Sonnet für Doku, Opus für Prüfung)
1. STL über `check.ps1` erzeugen (front, back, cam_test), `case/stl/madbuddy_cam_clip.stl` löschen.
2. `case/tools/make_3mf.ps1`: Prüfen, ob `roboter_teile.3mf` schon `cam_test` ohne Clip enthält. Nur bei Bedarf neu erzeugen. `roboter_ams.3mf` **nicht** anfassen, sie hat Nutzeränderungen.
3. Riegel-Stellen ersetzen: Einbau in 4 Schritten (leere Tasche, Block von hinten einschieben, Block sitzt, Rückwand drauf). Dazu die Hinweise „Kamera bleibt wie geliefert auf dem SD-Steckplatz, nichts kleben“ und „zuerst `cam_test` drucken“.
   Betroffene Dateien:
   - `docs/einbau-kamera-roboter.md` und `.html`, PDF neu per `msedge --headless=new --no-pdf-header-footer --print-to-pdf`, Seite 1 als Bild prüfen
   - `case/README.md` mit Teiletabelle und Einbau Stufe 2 und Roboter
   - `README.md`
   - `docs/verkabelung.md`
4. Volllauf im Hintergrund: `& case\check_roboter.ps1` (~35 min) und `& case\check.ps1`. Nur Fehler und Summe lesen → beide grün.
5. `docs/FORTSCHRITT.md`: neuer Eintrag oben. „Nächste Aufgaben“ auf Stand bringen:
   - „betrübt“ ist durch „Pflege ohne Zwang“ erledigt.
   - SPI 80 MHz ist erledigt.
   - Neu drucken: `madbuddy_front/back` (falls Stufe 2 gebaut wird).
   - Offen bleiben Kopf-Nachdruck und Sitz des Schachts.
   - Memory-Eintrag `cyd-buddy-projekt.md` kurz nachziehen.
6. Prozess-Kassensturz (openscad, msedge).
7. Commit `docs(case): Kamera-Halter v2 – STL, Anleitung, Stufe 2`, Branch pushen, nach `main` mergen und pushen (wie bisher). Kein Release, weil keine Firmware geändert ist.

## Prüfung
- `check.ps1` und `check_roboter.ps1` im Volllauf ALLES GRÜN. Die neuen Prüfungen haben je eine Gegenprobe, die nachweislich rot wird.
- Grep `cam_clip|Riegel|xiao_cam_common` im Repo: nur noch historische Stellen in FORTSCHRITT, Specs und Plänen.
- Ein Blick auf `case/render/kamera_schnitt.png` zur Lage von Block, Endwänden und Anschlag.

## Was der Nutzer danach tun muss
- Nichts Pflicht. Wenn Stufe 2 gebaut wird: `case/stl/madbuddy_front.stl`, `madbuddy_back.stl` und zuerst `madbuddy_cam_test.stl` drucken. Die Größe in Byte nenne ich am Ende.
