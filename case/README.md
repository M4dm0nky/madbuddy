# MadBuddy Gehäuse "Kopf mit Stirn"

Parametrisches OpenSCAD-Gehäuse (OpenSCAD 2021.01) für das ESP32-2432S028 (CYD 2,8", Micro-USB + USB-C) und ein Seeed XIAO ESP32S3 Sense mit Kameraplatine (Linse in der Stirn, schaut nach vorn).

Dateien: `madbuddy_case.scad` und `xiao_cam_block.scad` (Kamera-Halter v2: Tasche, Endwände des Schachts, Prüfkörper; gemeinsam mit dem Roboter nutzbar). Teil wählen mit `-D 'part="front"'` (`front`, `back`, `cam_test`, `frame_test`, `assembly`, `section`, `cam_section`, `collision`, `info`).
Alle Maße stehen oben in der Datei mit Kommentar: `QUELLE: Originalzeichnung`, `GEMESSEN` oder `VORLÄUFIG`.

## Teile

| Teil | Datei | Inhalt |
|---|---|---|
| front | `stl/madbuddy_front.stl` | Kopfschale mit Display-Fenster (nach vorn angefast), Stirn, Kameratasche (Linsenloch Ø 7,4 mit 45°-Fase), 3 Endwände des Schachts (`xiao_tray()`-Stücke [3,4,5]), 4 Schraubdome (M3, von hinten) |
| cam_test | `stl/madbuddy_cam_test.stl` | Probestück: Stirn um Tasche und Endwände (X −14 … 24,5, Y ±14 um die Linse), flach druckbar. **Zuerst drucken**, Block muss stramm ohne Gewalt hinein (1× drucken) |
| back | `stl/madbuddy_back.stl` | Rückwand mit Falz, USB-C-Öffnung, SD-Aussparung, BOOT/RESET-Fenster, Lüftungsschlitze, Platinen-Andrückfüße mit Zentrierstift, Anschlag und Kabelschienen für den Block, Fuß mit 15° Neigung |
| frame_test | `stl/madbuddy_frame_test.stl` | flacher Streifen (2 mm Front + Display-Auflage) zum Prüfen von Fenstermaß, Glas-Sitz und Lochbild in ca. 20 min |
| assembly / section | nur Ansicht (`render/*.png`) | alles mit farbigen Prüfkörpern |

## Druck (PETG)

- 0,2 mm Schichthöhe, 3 Wände, ohne Stützen. Die STL-Dateien liegen schon in Druckausrichtung.
- front: Gesicht nach unten auf dem Bett (Fenster und Linsenloch sind Durchbrüche, Fenster-Fase ist ein Überhang von 35°).
- back: Rückseite (Außenfläche) liegt flach unten, der Fuß hat nur 15° Überhang.
- frame_test: Gesicht nach unten.
- cam_test: Außenseite nach unten, wie `front`.
- Schrauben: 4× M3×8 (Senkung hinten, Kernloch 2,6 mm in den Domen; bei PETG selbstschneidend).
- Mindestwand 2,4 mm, Passungsspiel `fit = 0,3 mm` (Variable).
- **Wer `front` oder `back` schon gedruckt hat, muss sie neu drucken** (Tasche, Endwände und Anschlag sind neu; Kamera-Halter v2, 2026-11-06). Zuerst `cam_test`.

## Kamera und XIAO einbauen (Stufe 2)

Kamera und XIAO bleiben wie geliefert: Die Kameraplatine steckt auf dem XIAO über dem SD-Steckplatz, nichts kleben. Beides geht als ein Block in den Kopf. **Zuerst `cam_test` drucken** und den Block dort probeweise einschieben (stramm, ohne Gewalt).

1. Leere Tasche: `front` liegt mit dem Gesicht nach unten. Tasche in der Stirn und die drei Endwände des Schachts sind leer.
2. Block von hinten einschieben: Linse (Tubus) durch das Loch Ø 7,4, Kamera 8 × 8 in die Tasche, der XIAO zwischen die Endwände. Der Klappstecker liegt seitlich. Die angelöteten Drähte bleiben frei.
3. Block sitzt: Kamera liegt vorn an der Stirn, ca. 0,3 mm Spiel in X/Y, Lötstellen frei. Vor dem Schließen einschalten und die Kamera testen.
4. Rückwand aufsetzen: Der Anschlag in `back` steht 0,15 mm hinter der XIAO-Unterseite und hält den Block in Z. Mit 4 × M3×8 verschrauben. Die Lage des Blocks relativ zur Linse ist `xiao_pos`.

## Prüfung

`& case\check.ps1` (aus PowerShell im Repo-Ordner) führt alles aus (STL ohne Warnungen, Standsicherheit, Standebene, Kollision gegen alle Prüfkörper inkl. Linsen-Freiraum und aktiver Fläche mit Sichtkegel, Selbsttest der Kollisionsprüfung, PNG-Renders). Exitcode ungleich 0 bei jedem Fehler.
Gegenprobe: `check.ps1 -Define window_shrink=2 -NoPng` muss fehlschlagen (Fenster 2 mm schmaler).
Kamera-Prüfungen (ohne `-Define`): Selbsttest (Kamerakörper +0,6 mm muss die Tasche treffen), Einbauweg `asm_block` (Block lässt sich in Zwischenstellungen von hinten einschieben, mit Gegenprobe), Linsenspitze `lens_tip` (ragt höchstens bis zur Außenhülle, Gegenprobe `lens_tip_ctrl`), `cradle_fit` (Endwände sitzen am Block, je Seite mit Gegenprobe), Tasche und Endwände durchdringen das Rückteil nicht und bleiben 0,8 mm innerhalb der Kopfhaut, Kabelweg frei. Bild `render/madbuddy_kamera_schnitt.png` zeigt den Schnitt bei X = 0 (Block, Endwände und Anschlag).

## Maße

- Aus Originalzeichnung (`QUELLE`): Platine 86 × 50, Eckradius 4, Löcher Ø3,2 mit 4,0 mm Randabstand, Displaymodul 69,2 × 50 × 2,3, LCD-Glas 64,75, Fenster = AA + 0,3 mm je Seite. Die Zeichnungs-AA (57,6 × 43,2, mittig im Glas) passte nicht.
- Gemessen (2026-10-26, nach Testdruck): bespielte Fläche 58,8 mm breit (Fenster rechnet mit 59), beginnt 10 mm nach der linken Platinenkante (Blick von vorn). Höhe 44,3 mm ist daraus hochgerechnet (VORLÄUFIG).
- Gemessen: USB-Buchsen-Lage entlang der Schmalseite (Micro ab 9,67 mm, USB-C 17,88 bis 27,32 mm, von der Kante mit dem SD-Slot).
- VORLÄUFIG (Datenblatt/Schätzung): Platinendicke, Displayhöhe über Platine (4,5), Micro-USB-Breite, alle Höhen (z) der Buchsen/Stecker, SD-Slot (Länge, Lage, Höhe), BOOT/RESET-Lage, Rückseitenbauteile (8 mm, Randstreifen), XIAO-Stapel (Höhe 15, Platinendicke 1,2, Lage und Höhe des Kamerasteckers), Tubuslänge der Kamera (2,7), Sichtwinkel, Wandöffnungen, Schwerpunkt-Massen. Kameramaße (OV3660: Körper 8,0, Tiefe 5,3, Tubus Ø 6,2, FPC 21,2 × 6,5) stammen aus der Seeed-Zeichnung; den Aufdruck am eigenen Modul prüfen (`cam_type`).

## Offene Punkte

- Micro-USB hat standardmäßig keine Wandöffnung (`open_micro_usb = false`), Innenfreiraum bleibt.
- SD-Slot liegt in der Konstruktion an der unteren Längskante; dort steht der Kopf auf dem Fuß, die Karte lässt sich dann nur bei angehobenem Gerät stecken. Die Firmware nutzt die SD-Karte nicht (geprüft 2026-10-26), daher bleibt das so.
- Standsicherheit ist eine Schätzung (Massenliste in der SCAD). Ohne die Zusatztiefe `rear_extra = 10` liegt das Lot bei 15° praktisch auf der Kippkante.
- XIAO-USB-C ist im Gehäuse nicht zugänglich (nur Programmierung vor dem Einbau).
- Fenster, Lochbild und Glas-Sitz zuerst mit `frame_test` prüfen. **Stand 2026-10-26: 1. Testdruck: Lochbild passt, Fenster lag falsch, korrigiert, 2. Testdruck steht aus.**
- Strom über den USB-C des CYD nur mit Kabel USB-A auf USB-C (keine CC-Widerstände). Das Netzteil braucht ca. 1 A, weil die Kamera mitversorgt wird.

## Variante 2 Roboter

Sitzender Roboter mit Akku: Kopf mit dem CYD, Oberkörper mit der Kamera in der Brust, Po und Beine mit dem Akkuhalter DFRobot DFR0969 (2 × 18650), Laden per USB-C im Boden. Alle Teile werden gesteckt und teilweise verschraubt. Datei: `madbuddy_roboter.scad` (Teil wählen mit `-D 'part="seat"'`). Spec: `docs/superpowers/specs/2026-10-26-roboter-gehaeuse-design.md` (Form) und `2026-10-27-roboter-druckteile-design.md` (Druckteile).

Stand: druckbar, die Maße des Akkumoduls sind **vorläufig** (Datenblatt, noch nicht nachgemessen).

### Teile

STL liegen in `stl/roboter_<teil>.stl`, schon in Druckausrichtung (ohne Stützen). Die Farben sind der Vorschlag aus dem Entwurf.

| Teil | Farbe | Menge | Hinweis |
|---|---|---|---|
| `head_front` | blau | 1 | Gesicht nach unten; mit eingesetzter Blende und Zierschrauben auch als `head_front_ams` |
| `bezel` | schwarz | 1 | Blende, Steg nach oben |
| `head_back` | blau | 1 | Rückseite auf dem Bett |
| `ear` | gelb | 2 | Außenseite auf dem Bett, Zapfen nach oben |
| `deco` | silber | 1 Platte | 10 flache Zierschrauben-Einlagen (4 Kopf, 4 Brust, 2 Ohr) |
| `torso_front` | orange | 1 | Brust auf dem Bett |
| `torso_back` | orange | 1 | Rückseite auf dem Bett |
| `neck` | schwarz | 1 | stehend auf dem unteren Flansch |
| `arm` | orange | 2 | links = gespiegelte Datei `roboter_arm_l.stl` |
| `hand` | schwarz | 2 | flache Unterseite auf dem Bett |
| `seat` | blau | 1 | auf dem Kopf gedruckt, Fach nach oben offen |
| `bottom` | blau | 1 | Standfläche auf dem Bett |
| `foot` | schwarz | 2 | Rückseite (Zapfenloch) auf dem Bett |

3MF zum Öffnen in Bambu Studio:
- `3mf/roboter_teile.3mf`: alle 18 Druckkörper nebeneinander, jeder bekommt im Slicer seine Farbe.
- `3mf/roboter_ams.3mf`: Gruppe „kopf“ (Kopfschale, Blende, 4 Zierschrauben als drei Bauteile eines Objekts, in Einbaulage) plus die übrigen Teile einzeln. Im Slicer weist man den drei Bauteilen je eine Farbe zu.
- Neu erzeugen: `check_roboter.ps1` (Schritt 1c) oder `tools/make_3mf.ps1`; Werkzeug `tools/stl2_3mf.py` (wird von `make_3mf.ps1` aufgerufen) (`--group NAME`, `--end`, `--arrange`).

### Druck (PETG)

- 0,2 mm Schichthöhe, 3 Wände, ohne Stützen, Überhänge höchstens 45°.
- `torso_front`: Stützen nur am Linsenloch (Fase 45° bei geneigter Achse, bis ca. 60° Überhang), z. B. mit Support-Painting/Support-Blocker in Bambu Studio. Überall sonst ohne Stützen.
- Steckzapfen sind als Presspassung gedacht (`fit_peg = 0,15 mm`). Zuerst ein Zapfen-Probestück oder den Fuß drucken und die Passung prüfen; bei Bedarf `fit_peg` ändern.
- Kernlöcher für M3 sind Ø 2,6 mm (selbstschneidend in PETG, mindestens 7,5 mm tief).
- **Gewindeeinsätze (Option):** `use_inserts = true` in `madbuddy_roboter.scad` macht alle M3-Kernlöcher zu Einsatzlöchern Ø 4,0 mm (M3-Einsatz zum Einschmelzen, Ø 4,0 bis 4,2 x 5,7 mm lang, **VORLÄUFIG**, am Probestück prüfen). Die Löcher sind weiterhin mindestens 7,5 mm tief, also tiefer als der Einsatz. **Wand um das Loch** bleibt an den Domen nur 1,2 bis 1,7 mm (Standard 2 mm, die Prüfung ist dafür auf 1,2 mm gesenkt): Dome bei Bedarf vergrößern. Standard ist `false`, dann ist die Geometrie unverändert. Alle Teile mit Kernlöchern (`head_front`, `head_back`, `torso_front`, `neck`, `seat`) müssen dann neu gedruckt werden.
- **Lüftung:** `vent_row()` in `madbuddy_roboter.scad` erzeugt alle Schlitzreihen (`vent_n`, `vent_pitch`, `vent_len`, `vent_w`). Reihen: Brust-Rückwand (5 Schlitze, wie bisher), Kopf-Rückwand hinter dem CYD (`head_vent_*`, 9 Schlitze 1,6 x 24 mm, senkrecht durch die Wand, ohne Stützen) und Boden unter dem Akkumodul (`bot_vent_*`, 7 Schlitze 1,6 x 16 mm). Die Schlitze liegen frei von USB-Öffnung, BOOT-Fenster, Schrauben, Füßen, LED-, DIP- und Schalteröffnungen (Prüfung 3k). Stufe 2 hat eigene Schlitze (`madbuddy_case.scad`).
- Die Zierschrauben-Einlagen nur als Einlage einkleben oder pressen (Taschen sind 0,15 mm größer).

### Schrauben (M3)

Insgesamt 20 Stück. Jede Zahl ist aus `madbuddy_roboter.scad` belegt; der Beleg nennt Variablen- und Modulnamen (in der Datei suchen), keine Zeilen, weil die Datei wächst.

| Anzahl | Länge | Wo | Beleg |
|---|---|---|---|
| 4 | M3×8 | Kopf: `head_back` an `head_front` | 4 Dome `dome_pts` (`dome_x`, `dome_y`); Länge: `back_t` 2,6 abzüglich `cbore_t` 1,8 plus `pilot_len` 7,5 = 8,3 |
| 2 | M3×10 | Hals oben an `head_back` | Modul `neck_pilots`, `neck_fl_t` 2,8 plus `neck_pilot_len` 7,6; Kommentarblock „MONTAGEFOLGE“, Schritt 2 |
| 2 | M3×10 | Hals unten an `torso_front` | Modul `neck_pilots_t`, Kernloch `neck_pilot_t` 7,6; Kommentarblock „MONTAGEFOLGE“, Schritt 1 |
| 4 | M3×28 | Oberkörper: `torso_back` an `torso_front` | 4 Dome `t_dome_pts`; Länge: `t_d` / 2 = 22 abzüglich `cbore_t` 1,8 plus `t_dome_pilot` 9 = 29,2; Kommentarblock „MONTAGEFOLGE“, Schritt 3 |
| 4 | M3×10 Flachkopf | Oberkörper von unten auf `seat` | `t_seat_pts`, `seat_screw_l = 10`; Einschraubtiefe geprüft mit dem Prüfteil `screw_seat` (`check_roboter.ps1`, Abschnitt 3h) |
| 4 | M3×10 Flachkopf | `bottom` an `seat` | `dome_pts_s`; Länge: `bot_t` 2,4 − `cbore_t` 1,8 + `boss_h` 2,0 + `pilot_s_len` 7,6 = 10,2; Kommentarblock „MONTAGEFOLGE“, Schritt 5 |

Summe: 4 × M3×8, 12 × M3×10, 4 × M3×28. Das passt zur Massenliste „20 × M3“ (Eintrag „Schrauben (20 x M3)“ in der Massenliste). Die Zierschrauben sind gedruckte Einlagen, keine echten Schrauben.

### Montagereihenfolge

1. Hals von oben mit 2 × M3×10 auf `torso_front` schrauben (`torso_back` ist dabei verschraubt, der Kopf fehlt).
2. Rückwand des Oberkörpers abnehmen, Kopf auf den Hals setzen, 2 × M3×10 von unten durch die Halsschlitze eindrehen.
3. Kamera und XIAO einbauen, solange `torso_back` noch abgenommen ist (bei geschlossenem Oberkörper kommt man nicht an die Tasche). **Bebilderte Anleitung: [PDF](../docs/einbau-kamera-roboter.pdf)** (Text: [`docs/einbau-kamera-roboter.md`](../docs/einbau-kamera-roboter.md)). Kurzfassung:
   1. `cam_test` vorher drucken und den Block dort probeweise einschieben.
   2. XIAO mit aufgesteckter Kamera (wie geliefert, nichts kleben) als ein Block von hinten in Tasche und Schacht schieben, Linse durch das Loch.
   3. Block sitzt (ca. 0,3 mm Spiel, Lötstellen frei). Vor dem Schließen Kamera am CYD testen.
   4. `torso_back` aufschieben und mit 4 × M3×28 schließen: Der Anschlag in `torso_back` liegt hinter dem Block. Der Weg ist per Zwischenstellungen geprüft (`asm_block`, `asm_close`).
4. Arme in den Oberkörper stecken, Hände auf die Arme stecken, Ohren in den Kopf stecken, Blende in die Nut drücken, Zierschrauben einsetzen.
5. Oberkörper mit Hals und Kopf auf `seat` stecken und von unten mit 4 × M3×10 verschrauben (Schraubendreher durch den offenen Boden).
6. Akku einlegen, 5 V und GND verdrahten (Schalter, Kabel nach oben), `bottom` mit 4 × M3×10 schließen. Füße vorn auf die Beine stecken.

Kamera im Roboter: Der Sichtkegel (102° diagonal, 85° horizontal) wird nur 26 mm lang geprüft, weil die Füße ab ca. 27 mm unten im Bild liegen (Entscheidung 2026-10-28).

Jede Schraube ist im Modell mit einem Schraubendreher-Prüfkörper Ø 6 × 60 auf Erreichbarkeit geprüft (`check_roboter.ps1`, Schritte 3g/3h).

### Prüfung

`& case\check_roboter.ps1` (aus PowerShell im Repo-Ordner): STL aller Teile ohne Warnung, Standsicherheit, Kollisionen, Lüftungsschlitze (3k, mit Positiv-Kontrolle), Montage-Erreichbarkeit mit Selbsttests, 3MF (lesbar, größer als 10 kB, richtige Anzahl Objekte und Bauteile), PNG-Renders. Exitcode ungleich 0 bei jedem Fehler.

### Offene Punkte

- Maße des Akkumoduls DFR0969 (Platine, Lage der Buchse, des Schalters, der LED) sind **VORLÄUFIG** nach Datenblatt. Vor dem ersten Druck von `seat` und `bottom` das echte Modul nachmessen.
- Presspassung der Zapfen (`fit_peg`) und die Länge der Schrauben sind noch nicht am gedruckten Teil getestet.
- Die Lage von Schalter und LED im Boden hängt am Modul (siehe oben).
- Kamera-Halter im Oberkörper: eigener Plan `docs/superpowers/plans/2026-10-27-kamera-halter.md`.
