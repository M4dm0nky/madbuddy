# Lautsprecher im Bauch (Gehäuse, Teil A1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Der Oberkörper des Roboters wird so viel höher, dass ein 20 × 30-mm-Lautsprecher vorn unter der Kamera hinter einem eingerasteten Gitter sitzt; alle Prüfungen bleiben grün.

**Architecture:** Neuer Parameter `t_rise` hebt alles im Oberkörper oberhalb der unteren Dome (Kamera, Halsblock, obere Dome, obere Zierschrauben, Hals, Kopf) an; Unterkante, Sitz, Arme und Hände bleiben. Der Lautsprecher liegt in einer Tasche der Brustwand, von vorn eingesetzt, gehalten von einem Gitter (`spk_grille`), das wie die Kopfblende mit einem Steg in eine Nut rastet. Neue Prüfung `3k` in `check_roboter.ps1`.

**Tech Stack:** OpenSCAD 2021.01 (`C:\Program Files\OpenSCAD\openscad.com`), PowerShell-Prüfskript `case\check_roboter.ps1`, Spec `docs/superpowers/specs/2026-11-04-lautsprecher-geraeusche-design.md` (Teil A1).

## Global Constraints

- Nur `case/madbuddy_roboter.scad` und `case/check_roboter.ps1` ändern (plus Ausgaben `case/stl`, `case/3mf`, Doku). `cyd_common.scad`, `xiao_cam_block.scad`, `madbuddy_case.scad` nicht anfassen.
- Arme, Hände, Sitz, Boden, Füße dürfen sich geometrisch NICHT ändern (Nutzer hat sie gedruckt). Nachweis per STL-Vergleich (sortierte Facetten-Zeilen, siehe Task 1).
- Lautsprecher (VORLÄUFIG, Typ „2030“, nicht vermessen): 30,0 × 20,0 mm, 5,0 mm dick inkl. Membranrand, Kabel an einer Schmalseite. Passungsspiel `fit` (0,3) je Seite.
- Alles ohne Stützen druckbar in der vorhandenen Druckausrichtung (`torso_front`: Brust nach unten). Einzige bekannte Ausnahme bleibt die Linsenfase.
- Wände ≥ 2 mm, Kernlöcher ≥ 7,5 mm wie bisher (Asserts nicht lockern).
- Jede neue „muss leer sein“-Prüfung bekommt eine Positiv-Kontrolle („muss treffen“).
- Prüfen nur gezielt mit `-Only` (Abschnitte, die die Änderung betreffen); der Volllauf (~35 min) einmal am Ende in Task 3. Lange Läufe im Hintergrund; keine hängenden `openscad`-Prozesse zurücklassen (nach Abbruch `Get-CimInstance Win32_Process` prüfen).
- Werte mit Herkunft kommentieren wie im Bestand (`QUELLE:`, `VORLÄUFIG (…)`, `GEMESSEN`).
- Commits ohne Claude-Trailer (Projektregel MadBuddy). Branch `feature/sound`.

---

### Task 1: Parameter `t_rise` (Oberkörper höher), zuerst geometrieneutral

**Files:**
- Modify: `case/madbuddy_roboter.scad` (Zeilen ~91 `neck_pt`, 347–366 Oberkörper-Parameter, 362 `t_dome_pts`, 363 `t_deco_pts`, 366 `cam_z`, 383–384 `neck_rec_floor`/`neck_blk_z`, 472 `t_half`, 585 `vent_slots`)

**Interfaces:**
- Produces: `t_rise` (mm, Vorgabe nach Task 2), `t_h = 48 + t_rise`; alle oberen Z-Werte als `alt + t_rise`.

- [ ] **Step 1: Referenz-STLs sichern**

```powershell
$ref = "$env:TEMP\madbuddy_ref_stl"; New-Item -ItemType Directory -Force $ref | Out-Null
Copy-Item case\stl\roboter_*.stl $ref
```

- [ ] **Step 2: `t_rise` einführen**

Direkt vor `t_h`:

```openscad
t_rise     = 0;      // Oberkoerper-Erhoehung fuer den Lautsprecher (Spec 2026-11-04); hebt alles oberhalb der unteren Dome an, Unterkante/Sitz/Arme bleiben
t_h        = 48 + t_rise;   // QUELLE: Konzept (t_h) 48 + Erhoehung
```

Ersetzen (Wert + t_rise, Kommentar ergänzen „+ t_rise“):
- `neck_pt = [0, -8, leg_h + 50]` → `[0, -8, leg_h - 4 + t_h + 6]` (= gleiche Zahl bei t_rise 0)
- `t_dome_pts`: obere Z 41 → `41 + t_rise` (untere 8 bleiben)
- `t_deco_pts`: z 40.8 → `40.8 + t_rise` (12 bleibt)
- `cam_z = 27` → `27 + t_rise`
- `neck_rec_floor = 45` → `45 + t_rise`; `neck_blk_z = 35` → `35 + t_rise`
- `t_half`: `cube([80, t_d / 2 + 1, 70])` → `cube([80, t_d / 2 + 1, 70 + t_rise])`
- `vent_slots`: Z 17 → `17 + t_rise`

Weitere fest eingetragene Z-Werte im Oberkörper/Hals suchen (`grep -n "= *[0-9][0-9]\.\?[0-9]*;" ` im Abschnitt Zeile 337–660 und `module neck`, `cam_test`), die oberhalb Z 12 liegen und an Kamera/Hals hängen; gleich behandeln. Schulter (`arm_S`, `sh_z_l`, Schulterbosse) NICHT ändern.

- [ ] **Step 3: Neutralität prüfen (t_rise = 0)**

```powershell
.\case\check_roboter.ps1 -NoPng -Only '1','3','3b' 2>&1 | Select-String 'FAIL|GRUEN|FEHLGESCHLAGEN'
```
Erwartet: `== ALLES GRUEN ==`. Dann jede `case\stl\roboter_*.stl` gegen `$ref` vergleichen (sortierte Zeilen, CR entfernt):

```bash
for f in case/stl/roboter_*.stl; do b=$(basename $f); a=$(tr -d '\r' < "$TEMP/madbuddy_ref_stl/$b" | sort | md5sum); c=$(tr -d '\r' < $f | sort | md5sum); [ "$a" = "$c" ] || echo "ANDERS $b"; done
```
Erwartet: keine Ausgabe.

- [ ] **Step 4: Gegenprobe**

`-Define t_rise=10` exportieren (`openscad -D t_rise=10 -D 'part="torso_front"'`) und Bounding-Box-Höhe ansehen: muss um 10 größer sein. Arme/Hand/Sitz mit `t_rise=10` exportieren: identisch zur Referenz.

- [ ] **Step 5: Commit**

```bash
git add case/madbuddy_roboter.scad
git commit -m "refactor(case): t_rise hebt Oberkoerper oberhalb der unteren Dome an (neutral bei 0)"
```

### Task 2: Lautsprecher-Tasche, Gitter, Prüfung 3k, `t_rise` festlegen

**Files:**
- Modify: `case/madbuddy_roboter.scad` (neuer Block nach den Kamera-Parametern; `torso_front`; Teile-Schalter ab Zeile ~1081; `dummies()`)
- Modify: `case/check_roboter.ps1` (`$parts`, `$knownSections`, neuer Abschnitt `3k` nach `3j`)

**Interfaces:**
- Consumes: `t_rise`, `t_d`, `wall`, `fit`, `fit_peg`, `ring2d`-Muster/`nut_t`/`bezel_t` (Kopfblende), `cam_frame`, `xiao_dummy_l`, `camera_dummy_l`, `cam_free_volumes`, `t_dome_pts`.
- Produces: Parameter `spk_w, spk_h, spk_t, spk_zc`; Module `spk_dummy()`, `spk_pocket_cut()`, `spk_grille()`, `spk_grille_ring2d()`; Teile `"spk_grille"` (Druck) und `"spk_grille_inlay"` (3MF/AMS, Einbaulage wie `torso_front`); Prüfteile `"spk_fit"`, `"spk_fit_selftest"`, `"spk_cam"`, `"spk_cam_selftest"`, `"spk_open"`.

- [ ] **Step 1: Parameter und Prüfkörper**

```openscad
// --- Lautsprecher (Spec 2026-11-04 Teil A1): Typ 2030, quer, Membran nach vorn, von vorn eingesetzt, Gitter rastet in eine Nut
spk_w      = 30.0;   // VORLÄUFIG (Typ 2030, nicht vermessen): Breite (X)
spk_h      = 20.0;   // VORLÄUFIG (Typ 2030, nicht vermessen): Hoehe (Z)
spk_t      = 5.0;    // VORLÄUFIG (Typ 2030, nicht vermessen): Dicke inkl. Membranrand (Y)
spk_r      = 2.0;    // VORLÄUFIG: Eckradius
spk_zc     = 13.7 + spk_h / 2;   // Mitte Z: Unterkante 2 mm ueber den unteren Domen (Oberkante 11,7)
spk_grille_t = 1.2;  // wie Kopfblende (bezel_t)
spk_rim    = 3.0;    // Gitterrand um die Tasche (Steg + Nut liegen darin)
spk_open_min = 0.40; // Gitter: offene Flaeche ueber der Membran >= 40 %
module spk_dummy(grow = 0) translate([0, -t_d / 2, spk_zc]) rotate([-90, 0, 0])
  linear_extrude(spk_t + grow) offset(r = grow) rrect(spk_w / 2, spk_h / 2, spk_r);
```

Lage in Y: Membranseite bündig auf der Außenfläche der Brust (`Y = -t_d/2`), Körper reicht `spk_t` nach innen. Das Gitter liegt AUF der Brust (wie die Kopfblende auf dem Gesicht).

- [ ] **Step 2: Tasche und Halt in `torso_front`**

- Tasche: `offset(delta = fit) rrect(...)`, von `Y = -t_d/2 - 0.01` bis `-t_d/2 + spk_t + fit`, zieht Wand und Fase weg.
- Hinter der Tasche bleibt ein Anschlag-Rahmen (Breite 1,5 mm) als Auflage; innen frei (Rückraum offen zum Bauch). Der Rahmen kommt in `t_adds()`-Art als Zusatz innerhalb `t_half(true)` und darf nicht in den Kamera-Stapel ragen.
- Nut für den Gittersteg um die Tasche (wie Kopf: 2 mm breit, `nut_t` tief, `fit_peg` Spiel).
- Kabel: Schlitz 3 × 2 mm im Anschlag-Rahmen an der Schmalseite, nach innen.
- Die unteren Zierschrauben (`t_deco_pts`, Z 12, X ±23) dürfen nicht in Tasche/Gitter liegen; falls doch, X nach außen oder Z anpassen und begründen.

- [ ] **Step 3: Gitter `spk_grille()`**

Platte `spk_grille_t` über `rrect(spk_w/2 + spk_rim, spk_h/2 + spk_rim, spk_r + spk_rim)`, Steg nach hinten in die Nut (0,1 kürzer als die Nut tief, wie `bezel()`), Öffnungen: waagerechte Schlitze 1,6 mm breit mit 1,2 mm Steg über der Membranfläche (druckt flach ohne Brücken). Druckteil `"spk_grille"`: Sichtseite nach unten auf dem Bett.

- [ ] **Step 4: Prüfteile**

```openscad
else if (part == "spk_fit") hits() { spk_dummy(); torso_front(); }                       // muss LEER sein
else if (part == "spk_fit_selftest") hits() { spk_dummy(0.6); torso_front(); }           // muss TREFFEN
else if (part == "spk_cam") hits() { spk_dummy(1.0); union() { xiao_dummy_l(); camera_dummy_l(); cam_free_volumes(); } }  // muss LEER sein (1 mm Abstand)
else if (part == "spk_cam_selftest") hits() { translate([0, 0, 15]) spk_dummy(1.0); union() { xiao_dummy_l(); camera_dummy_l(); } }  // muss TREFFEN
else if (part == "spk_open") { echo(SPK_OPEN = spk_open_frac()); }
```

`spk_open_frac()` als Funktion aus den Schlitzmaßen berechnen (Summe Schlitzflächen innerhalb der Membranfläche / `spk_w*spk_h`). `spk_dummy()` zusätzlich in `dummies()` aufnehmen (gehört dann in die Kollision `3`). `"spk_grille"` in `$parts`, `"spk_grille_inlay"` in 1b/1c (AMS-3MF) wie `bezel_inlay`.

- [ ] **Step 5: Abschnitt 3k im Prüfskript**

Muster wie `3`/`3b`: `spk_fit` leer, `spk_fit_selftest` > 0 Facetten, `spk_cam` leer, `spk_cam_selftest` > 0 Facetten, `spk_open` ECHO ≥ 0.40. `'3k'` in `$knownSections`.

- [ ] **Step 6: `t_rise` festlegen**

Kleinsten ganzzahligen `t_rise` (mm) suchen, bei dem `-Only '3','3k','3i'` grün sind (Start 10, dann ±1). Mit `-Define t_rise=N` suchen, Ergebnis als Vorgabe eintragen mit Kommentar `GERECHNET 2026-11-04: kleinster Wert mit 3/3i/3k gruen`.

- [ ] **Step 7: Gezielte Prüfung**

```powershell
.\case\check_roboter.ps1 -NoPng -Only '1','2','2b','3','3b','3e','3e2','3g','3i','3j','3k' 2>&1 | Select-String 'FAIL|GRUEN|FEHLGESCHLAGEN'
```
Erwartet `== ALLES GRUEN ==`. Scheitert 3g (Schraubendreher 60 mm erreicht die Kopfschrauben nicht mehr): melden, NICHT die Prüfung ändern.

- [ ] **Step 8: Überhang Gitter/Tasche**

Wie 3j: neue Flächen von `torso_front` in Druckausrichtung ohne Überhang > 45° und ohne Brücken > 15 mm. Wenn 3j nur die Kamera prüft: Schnittbilder (z 0,5 … 8) der Tasche ansehen und im Bericht nennen.

- [ ] **Step 9: Commit**

```bash
git add case/madbuddy_roboter.scad case/check_roboter.ps1 case/stl case/3mf
git commit -m "feat(case): Lautsprecher 20x30 vorn im Bauch, Gitter, Pruefung 3k, Oberkoerper +N mm"
```

### Task 3: Volllauf, Render, Unverändert-Nachweis, Doku

**Files:**
- Modify: `case/README.md` (Druckteile, Montage Lautsprecher), `docs/FORTSCHRITT.md`, `case/stl/*`, `case/3mf/*`, `case/render/*`

- [ ] **Step 1: Volllauf im Hintergrund**: `.\case\check_roboter.ps1` (mit PNG), Ergebnis nur gefiltert lesen. Erwartet `== ALLES GRUEN ==`.
- [ ] **Step 2: Unverändert-Nachweis**: STL-Vergleich (Task 1 Step 3) — anders sein dürfen nur `roboter_torso_front`, `roboter_torso_back`, `roboter_neck` (nur falls Hals geändert), neue `roboter_spk_grille*`; Kopfteile nur, falls Lage-unabhängig unverändert (Kopf wandert nur in der Baugruppe). Unveränderte STLs per `git checkout --` zurücksetzen.
- [ ] **Step 3: Render** `case\render\roboter_*.png` (Vorderansicht + Seite) erzeugen, für die Abnahme der Proportionen durch den Nutzer.
- [ ] **Step 4: Doku**: README: Druckteile neu (`torso_front`, `torso_back`, `spk_grille`), Montage (Lautsprecher von vorn einlegen, Kabel nach innen, Gitter aufdrücken; Kabel durch den Hals zu SPEAK am CYD, Länge ≥ 15 cm nötig). FORTSCHRITT-Eintrag mit Prüfergebnis.
- [ ] **Step 5: Commit**

```bash
git add case docs/FORTSCHRITT.md
git commit -m "docs(case): Lautsprecher im Bauch, Druckteile und Montage"
```
