# Kamera-Halter v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** XIAO-Stapel und Kamera in Lieferlage als ein Block hinter die Front setzen. Die Kamera klemmt zwischen einer Tasche in der Front und dem microSD-Steckplatz. Riegel, Nuten und FPC-Bogen entfallen. Gilt für Roboter (`madbuddy_roboter.scad`) und Kopf (`madbuddy_case.scad`).

**Architecture:** Die gemeinsame Datei `case/xiao_cam_common.scad` bekommt neue Maße und Module (Tasche, Wiege, Prüfkörper „Block“). Die Modulnamen bleiben, wo es geht, gleich: `cam_pocket_add/cut`, `xiao_cradle_add/cut`, `cam_dummy`, `xiao_stack_dummy`, `cam_fov_cone`. Damit ändern sich die einbindenden Dateien nur an den Stellen, die mit Riegel, Nuten, FPC und Rückteil-Anschlag zu tun haben. Geprüft wird über die vorhandenen Prüfskripte (OpenSCAD-Schnittmengen, je Prüfung eine Gegenprobe).

**Tech Stack:** OpenSCAD 2021.01 (`C:\Program Files\OpenSCAD\openscad.com`), PowerShell 5.1 (Prüfskripte), Python 3.14 (`C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe`) nur für 3MF.

**Spec:** `docs/superpowers/specs/2026-11-02-kamera-halter-v2-design.md`

## Global Constraints
- Lokales Kamerasystem wie bisher (`cam_frame()`): Z 0 = Innenebene der Front, Z `wall_t` = Außenfläche, −Z = ins Gehäuse. X entlang der langen XIAO-Seite, Y entlang der kurzen. Linsenachse = Z-Achse durch den Ursprung.
- Kamera (OV3660 schmale Linse, gemessen): Körper 8,0 × 8,0 × 2,0; Linse unten Ø 7,0, oben Ø 5,7, Höhe 3,6; Gesamttiefe 5,6.
- Linse endet 0,3 hinter der Außenfläche: `cam_zbf = wall_t − 3,6 − 0,3` (Vorderseite Körper = Taschenboden), `cam_zbb = cam_zbf − 2,0` (Rückseite Körper).
- Linsenloch Ø 7,4, durchgehend, außen 45° gefast. Tasche 8,4 × 8,4. Der Rahmen um die Tasche reicht nur bis `cam_zbb + 0,5` nach hinten (darunter liegt der SD-Steckplatz), seitlich offen zur Klappstecker-Seite (+X), Öffnung 7,2 breit.
- Block-Maße (Seeed-STEP, umgerechnet; u = Abstand ab USB-Kante der XIAO-Platine nach +X, h = Höhe über der XIAO-Unterseite, v = quer, Linsenmitte bei v 0):

  | Teil | u | v | h |
  |---|---|---|---|
  | XIAO-Platine | 0 … 20,96 | −8,25 … 9,53 | 0 … 1,25 |
  | USB-C | −1,53 … 6,37 | −3,86 … 5,14 (9,0 breit, mittig auf der Platine) | 0,26 … 4,46 |
  | Bauteile zwischen den Platinen (Shield) | 6,71 … 17,31 | −5,66 … 6,94 | 0,55 … 3,25 |
  | Sense-Platine | 6,58 … 21,25 | −7,75 … 9,03 | 4,18 … 5,43 |
  | microSD-Steckplatz | −3,11 … 11,95 | −5,77 … 5,41 | 6,85 … 8,50 |
  | Klappstecker (weiß) | 15,12 … 21,12 | −7,43 … 8,77 | 5,42 … 7,43 |
  | Kamerakörper | −0,47 … 7,53 | −4,0 … 4,0 | 8,2 … 10,2 |
  | Linse | Mitte u 3,53, Ø 7,0 (h 10,2 … 11,0), Ø 5,7 (h 11,0 … 13,8) | | |
  | FPC (Hüllraum) | 7,49 … 15,12 | −3,25 … 3,25 | 5,85 … 8,75 |

  Hinweis: Im STEP überlappen Kamera-Rückseite (8,2) und SD-Oberseite (8,5) um 0,3; das Schaumpad wird gedrückt. Für Kollisionsprüfungen den SD-Steckplatz bis h 8,2 kürzen.
- Lage des Blocks im lokalen System: USB-Kante bei X = −3,53 (USB zeigt nach −X), XIAO-Unterseite bei Z = `cam_zbb − 8,2` = `cam_zbf − 10,2`, XIAO-Mittellinie bei Y = +0,64 (Lage der Antenne: v > 0 ist die Seite **ohne** U.FL; die Kamera liegt 0,64 zur U.FL-Seite). Parameter `xiao_flip = false` spiegelt bei Bedarf X (USB nach +X); Standard bleibt wie oben, das deckt sich mit dem bisherigen Stapelplatz (alter `xiao_offset` 7,0 → X −3,5 … 17,5).
- Wiege: Pfosten an beiden Längsseiten (Y außen, 0,3 Spiel zur XIAO-Kante), nur im Bereich u 7,0 … 12,5 (frei bleiben die Lötstellen 5V/GND am USB-Ende und D6/D7 am anderen Ende), Rastlippe 1,0 mit 45°-Einführschräge über der XIAO-Unterseite, 0,1 Luft. Mindestwand 2 mm außer Lippen.
- Rückteil: Anschlag hinter der XIAO-Unterseite mit 0,15 Luft.
- Jede neue „muss leer sein“-Prüfung bekommt eine Gegenprobe, die anschlagen muss.
- Nur Betroffenes bauen und prüfen (`check_roboter.ps1 -Only …`); der Volllauf beider Skripte kommt einmal am Ende (Task 4).
- Keine hängenden Prozesse: nach jedem Abbruch `Get-CimInstance Win32_Process -Filter "Name like 'openscad%'"` prüfen und eigene beenden. Kein `python - <<EOF`, kein `find` über große Bäume.
- Commits ohne Claude-Attribution (Projektregel MadBuddy). Kommentare im SCAD deutsch, ASCII-Umlaute (ae/oe/ue), Herkunft je Maß: `QUELLE:`, `GEMESSEN` oder `VORLÄUFIG`.

---

### Task 1: Gemeinsame Datei `xiao_cam_common.scad` auf den Block umstellen

**Files:**
- Modify: `case/xiao_cam_common.scad` (ganze Datei, ca. 200 Zeilen)

**Interfaces:**
- Produces (alle mit Parameter `wall_t = cam_wall_t`):
  - Funktionen `cam_zbf(w)`, `cam_ztip(w)` (= `w − 0,3`), `cam_zbb(w)`, `xiao_zbot(w)` (= `cam_zbb(w) − 8,2`), `cam_zrear(w)` (= `cam_zbb(w) + 0,5`, Rahmen-Rückseite).
  - `cam_pocket_add(wall_t)`: Rahmen um die Tasche von `cam_zrear` bis 0,5 in die Wand, plus Wandverstärkung um das Loch von Z 0 bis `wall_t`, wenn `cam_zbf < 0` (Innenseite füllt bis zum Taschenboden).
  - `cam_pocket_cut(wall_t)`: Linsenloch Ø 7,4 mit Fase, Tasche 8,4 × 8,4 von `cam_zbf` nach hinten offen, Kabelöffnung nach +X (7,2 breit, von `cam_zbb − 0,5` bis `cam_zbf`).
  - `xiao_cradle_add(wall_t)`, `xiao_cradle_cut(wall_t)` (Hüllraum des Blocks + 0,3 Spiel, ohne Kamera).
  - `cam_dummy(wall_t)`: Kamerakörper + zweistufige Linse + FPC-Hüllraum.
  - `xiao_stack_dummy(wall_t)`: XIAO, USB-C, Shield, Sense, SD (bis h 8,2), Klappstecker.
  - `cam_block_dummy(wall_t)` = `cam_dummy` + `xiao_stack_dummy`.
  - `cam_fov_cone(L, wall_t)` unverändert in der Bedeutung (Spitze bei `cam_ztip`, Startdurchmesser 5,7).
  - Prüfhilfen: `cam_dbg_nocradle` bleibt; `cam_dbg_nogroove`, `cam_clip`, `cam_clip_place`, `cam_seg`, FPC-Funktionen entfallen.
  - Parameter: `xiao_flip`, `xiao_pos` (berechnet: `[ (xiao_flip ? -1 : 1) * (20,96/2 − 3,53), 0,64 ]` = Mitte der XIAO-Platine im lokalen System).

- [ ] **Step 1:** Alle Verwendungen der wegfallenden Namen auflisten: `Grep "cam_clip|cam_groove|cam_zg1|cam_fpc|xiao_offset|xiao_conn|cam_seg|cam_dbg_nogroove|xiao_stack\b|xiao_z0|stack_probe|cam_body_probe" case/` (Ergebnis in den Bericht, wird in Task 2/3 gebraucht).
- [ ] **Step 2:** Parameterblock neu schreiben (Maße aus „Global Constraints“, mit Herkunft). `cam_type` bleibt, OV5640-Zweig darf entfallen (nur `OV3660` belegt).
- [ ] **Step 3:** Module wie unter „Produces“ schreiben. Wiege: Pfosten an den Längsseiten im Bereich u 7,0 … 12,5, von der Rahmen-Rückseite bzw. Wand bis `xiao_zbot − 0,1 − 1,0` (Lippe), Lippe mit 45°-Schräge nach innen, 0,7 Schnappmaß je Seite (Lippe 1,0 minus Spiel 0,3).
- [ ] **Step 4:** Syntax prüfen ohne Einbindung: kleine Datei im Scratchpad, die `include <…/xiao_cam_common.scad>` lädt und `cam_block_dummy(3.9); cam_pocket_add(3.9);` rendert: `openscad.com -o <scratch>/t.stl <scratch>/t.scad` → keine WARNING/ERROR.
- [ ] **Step 5:** Noch nicht committen (die einbindenden Dateien sind erst nach Task 2 wieder lauffähig). Bericht: neue Modul-/Funktionsliste, Liste aus Step 1.

### Task 2: Roboter umstellen (`madbuddy_roboter.scad`, `check_roboter.ps1`)

**Files:**
- Modify: `case/madbuddy_roboter.scad` (Kameraabschnitt ab Zeile ~368 `cam_cone_l`, `cam_wt`; Module 480–530 `cam_frame` … `cam_cut`; `torso_back` ab ~580; Montage-/Prüfteile 1131–1206; `guide_*` 1208–1236; Teil `cam_clip` 1157)
- Modify: `case/check_roboter.ps1` (Kamera-Prüfungen um Zeile 344–372 und der Abschnitt mit `cam_*`/`clip_*`)

**Interfaces:**
- Consumes: alles aus Task 1.
- Produces: Teile `torso_front`, `torso_back` (neu), `cam_test` (Testausschnitt: Brustwand ±14 mm um die Linse mit Tasche, Rahmen, Pfostenwurzeln; flach druckbar, ca. 15 min), `guide_1_pocket`, `guide_2_block`, `guide_3_seated`, `guide_4_close`; `cam_clip` entfällt.

- [ ] **Step 1:** `cam_wt = 3.9` (Linse 3,6 + 0,3) mit Herkunftskommentar; prüfen, dass `cam_fill()` den Bereich Z 0 … Außenfläche weiter füllt und das Loch frei bleibt. Neigung 15° bleibt.
- [ ] **Step 2:** `xiao_dummy_l`, `camera_dummy_l` auf die neuen Prüfkörper umstellen; `cam_clip_l` und alle `asm_clip*`, `clip_test`, `clip_band`, `cam_clip` entfernen; `asm_cam`/`asm_stack` durch `asm_block` ersetzen (ganzer Block wird in −Z-Richtung von hinten eingeschoben, Zwischenstellungen alle 1,5 mm bis 30 mm; der Sondierkörper ist um die Lippen-Überdeckung 2 × 0,7 schmaler, wie bisher `asm_stack`) mit `asm_block_ctrl` (Körper +0,6 muss treffen) und `asm_block_lip` (voller Block trifft die Lippen).
- [ ] **Step 3:** `torso_back`: Klemmsteg/Stützwand an die neue XIAO-Unterseite (0,15 Luft) setzen; `asm_close` und `asm_close_ctrl` auf den Block umstellen.
- [ ] **Step 4:** Lage-Prüfungen anpassen: `cam_inside`, `stack_inside`, `cam_present`, `cam_channels`, `cradle_fit`, `torso_selftest`, `cam_selftest`, `cam_collision_ctrl`, `cone_ctrl` mit den neuen Prüfkörpern; neue Prüfung `lens_tip` (Linse 0,5 länger muss die Front treffen, Normallage leer).
- [ ] **Step 5:** `check_roboter.ps1`: entfernte Teile aus den Listen nehmen, neue (`asm_block`, `asm_block_ctrl`, `asm_block_lip`, `lens_tip` + Gegenprobe) eintragen; `cam_test` in die STL-Liste; `-Only`-Abschnittsnamen beibehalten.
- [ ] **Step 6:** Gegenprobe zeigen: einmal `cam_wt` absichtlich auf 3.5 setzen (Linse ragt 0,1 heraus) → `lens_tip`-Prüfung muss rot werden; zurücksetzen.
- [ ] **Step 7:** Nur betroffene Abschnitte laufen lassen: `& case\check_roboter.ps1 -NoPng -Only <Kamera-/Montage-Abschnitte, STL von torso_front/torso_back/cam_test>` → alles grün. Ausgabe nur gefiltert lesen (`Select-String 'FAIL|ERROR|WARN|OK'`).
- [ ] **Step 8:** `guide_*`-Zweige auf 4 Schritte umstellen (leere Tasche; Block 15 mm hinter der Tasche; Block sitzt; Rückwand 45 mm abgesetzt, Alpha 0.2), `case/tools/render_guide.ps1` auf 4 Bilder (`einbau_kamera_1..4.png`, altes `_5.png` löschen). Bilder rendern.
- [ ] **Step 9:** Commit (Task 1 + 2 zusammen, weil Task 1 allein nicht lauffähig ist): `git add case/xiao_cam_common.scad case/madbuddy_roboter.scad case/check_roboter.ps1 case/tools/render_guide.ps1 case/render/einbau_kamera_*.png` → `feat(case): Kamera-Halter v2 im Roboter (Block in Lieferlage, ohne Riegel)`.

### Task 3: Kopf Stufe 2 umstellen (`madbuddy_case.scad`, `check.ps1`)

**Files:**
- Modify: `case/madbuddy_case.scad` (alle 30 Stellen mit `cam_*`/`xiao_*`, Teil `cam_clip`, `cam_section`, Rückwand-Anschlagpfosten)
- Modify: `case/check.ps1` (Kamera-Prüfungen, `clip_test`, FPC-Luftlinie)

**Interfaces:**
- Consumes: Task 1.
- Produces: Teile `front`, `back` (neu), `cam_test`; `cam_clip` entfällt.

- [ ] **Step 1:** Wandstärke im lokalen System so wählen, dass die Linse 0,3 hinter der Stirn-Außenfläche endet (wie Roboter: Wandverstärkung innen auf 3,9 um das Loch). Prüfen, dass Block und Wiege innerhalb der Kopfschale liegen (Prüfung „0,8 mm innerhalb der Kopfhaut“ bleibt).
- [ ] **Step 2:** Rückwand-Anschlagpfosten auf die neue XIAO-Unterseite (0,15 Luft).
- [ ] **Step 3:** `check.ps1`: Riegel- und FPC-Luftlinien-Prüfung entfernen; Kollision Block, Linsenspitze (mit Gegenprobe), Sichtkegel, Wiege innen, Selbsttest Körper +0,6 eintragen.
- [ ] **Step 4:** `& case\check.ps1 -NoPng` → grün; Gegenprobe `-Define window_shrink=2` bleibt rot.
- [ ] **Step 5:** Commit: `feat(case): Kamera-Halter v2 im Kopf Stufe 2`.

### Task 4: Ausgaben, Anleitung, Schlussprüfung

**Files:**
- Modify: `case/tools/make_3mf.ps1` (Zeilen 23 und 45: `cam_clip` raus, `cam_test` rein), `case/3mf/roboter_teile.3mf` (neu erzeugt; `roboter_ams.3mf` hat lokale Nutzeränderungen → **nicht** überschreiben, nur melden)
- Modify: `case/stl/*.stl` der geänderten Teile; `case/stl/roboter_cam_clip.stl` und `madbuddy_cam_clip.stl` löschen
- Modify: `docs/einbau-kamera-roboter.md`, `docs/einbau-kamera-roboter.html` (4 Schritte, kein Riegel, Hinweis „Kamera bleibt wie geliefert auf dem SD-Steckplatz liegen, nichts kleben“, Testdruck `cam_test` zuerst), PDF neu per Edge headless
- Modify: `case/README.md` (Teiletabelle, Einbau Stufe 2 und Roboter, Schrauben unverändert prüfen), `README.md` (Einkaufsliste/Schritte: Riegel raus), `docs/FORTSCHRITT.md` (Eintrag)

- [ ] **Step 1:** STL der geänderten Teile über die Prüfskripte erzeugen (Abschnitt 1 der Skripte), alte `cam_clip`-STL löschen.
- [ ] **Step 2:** `make_3mf.ps1` anpassen und nur `roboter_teile.3mf` neu erzeugen.
- [ ] **Step 3:** Anleitung MD/HTML anpassen, PDF erzeugen (`msedge --headless=new --no-pdf-header-footer --print-to-pdf=…`), Seite 1–2 als Bild prüfen.
- [ ] **Step 4:** Volllauf `& case\check_roboter.ps1` und `& case\check.ps1` im Hintergrund, nur Fehler und Summe lesen → beide grün.
- [ ] **Step 5:** Prozess-Kassensturz (openscad, msedge headless).
- [ ] **Step 6:** Commit `docs(case): Kamera-Halter v2 – STL, 3MF, Anleitung`, push des Branches.
