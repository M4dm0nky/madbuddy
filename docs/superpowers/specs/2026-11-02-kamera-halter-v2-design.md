# Kamera-Halter v2: XIAO und Kamera als ein Block (Roboter und Kopf)

Stand 2026-11-02. Ersetzt den Halter aus `2026-10-27-kamera-halter-design.md` (Abschnitt 1, `cam_pocket`/`cam_clip`/`xiao_cradle`). Die Verkabelung bleibt unverändert.

## Anlass
Am echten Teil passt der alte Halter nicht:
- Das Kamerakabel ist **kurz** und steckt im **großen weißen Klappstecker** (JUSHUO AFC01-S24FCA-00) am Ende der Sense-Platine gegenüber USB. Das T-Ende (13,4 mm breit) passt nicht durch FPC-Schlitz und Riegel.
- Der XIAO lässt sich nicht 7 mm seitlich versetzt anordnen, das Kabel reicht nicht.
- Kleben ist keine Lösung, denn unter der Kamera liegt der microSD-Steckplatz.

## Belegte Maße
**Seeed-CAD des XIAO ESP32S3 Sense** (`seeed-studio-xiao-esp32s3-sense-3d_model.zip`, STEP mit cascadio/trimesh vermessen, Achsen: Höhe = Stapelrichtung):
- XIAO-Platine: 20,96 × 17,78, Dicke 1,25, Unterseite = Höhe 0 (im STEP y = −0,25).
- Sense-Platine: 14,68 lang (endet 6,6 vor der USB-Kante), Oberseite auf Höhe 5,43.
- microSD-Steckplatz: Oberseite auf Höhe 8,5. Die Kamera liegt darauf.
- Kamera in Lieferlage: **Rückseite auf Höhe 8,2** über der Unterseite des XIAO. **Linsenmitte 3,53 mm von der USB-Kante** der XIAO-Platine, **0,64 mm neben der Längs-Mittellinie** zur Längskante mit dem U.FL-Antennenstecker. Der Körper ragt 0,47 mm über die USB-Kante.
- USB-C: steht 1,53 über die USB-Kante, Höhe 0 bis 4,2.
- Gegenprobe: Das MakerWorld-Gehäuse „XIAO ESP32S3 Sense Housing Design“ hat 8,2 mm von der XIAO-Auflage bis zur Deckel-Innenseite.

**Eigene Kamera (OV3660, schmale Linse), vom Nutzer gemessen:**
- Körper 8,0 × 8,0 × 2,0 (L3).
- Linse unten Ø 7,0 (L1), oben Ø 5,7 (L2), Höhe 3,6. Gesamttiefe 5,6.
- Auf der Rückseite sitzt ein weißes Schaumpad (Dicke nicht gemessen, federt).

## Entwurf
1. **Block:** XIAO, Sense-Platine und Kamera in Lieferlage, nichts geklebt. Der Block steht senkrecht hinter der Front, Kamera nach vorn.
2. **Front** (`torso_front`, Kopf `front`):
   - **Linsenloch Ø 7,4** (L1 + 0,4), durchgehend, nach außen 45° gefast. Die Linse endet 0,3 hinter der Außenfläche. Dafür ist die Wand um das Loch innen auf **3,9** verstärkt (3,6 + 0,3).
   - **Tasche 8,4 × 8,4**, 2,0 tief (Körper + 0,2 je Seite), auf der Seite zum Klappstecker offen (Breite 7,2) für das Kabel.
3. **Wiege:** Pfosten mit Rastlippen an den Längsseiten (Drähte bleiben frei, kurze Pfosten). Die Unterseite des XIAO liegt **10,2 hinter dem Taschenboden** (8,2 + 2,0). Die Lage in der Ebene folgt aus der Linsenmitte (3,53 / 0,64). USB-Kante oben oder unten nach Platz in der Brust bzw. im Kopf; die Wahl steht als Parameter im Modell.
4. **Rückteil** (`torso_back`, Kopf `back`): Steg bzw. Anschlagpfosten legen sich an die Unterseite des XIAO. Spiel 0,1 bis 0,2; das Schaumpad nimmt Toleranzen auf.
5. **Entfällt:** `cam_clip`, Nuten, FPC-Bogen und Luftlinien-Prüfung, Parameter `xiao_offset` alter Bedeutung.
6. **Firmware:** Bildlage wird nach dem Einbau mit `CAM_FLIP_180` eingestellt (bestehender Schalter, nicht Teil dieser Änderung).

## Prüfungen (`check_roboter.ps1`, `check.ps1`)
Je Prüfung eine Gegenprobe, die anschlagen muss.
- Prüfkörper „Block“ (XIAO, Sense, SD, USB-C, Klappstecker, Kamera mit Linse, Kabel als Band) aus den Maßen oben.
- Kollision Block gegen Front und Rückteil leer (Gegenprobe: Körper +0,6).
- Kamera sitzt: Körper in der Tasche, Linse im Loch, Linsenspitze 0,3 hinter der Außenfläche (Gegenprobe: Linse +0,5 länger trifft).
- Sichtkegel frei (Roboter 26 mm, Kopf 30 mm).
- Einbauweg: Block von hinten einschieben (Zwischenstellungen), Rückteil schließen.
- Rahmen und Wiege bleiben innerhalb der Außenhaut, schneiden keine Schraubkanäle.
- STL ohne Warnungen; nur betroffene Teile neu erzeugen, Volllauf einmal am Ende.

## Danach
- 3MF neu (`make_3mf.ps1`), `cam_clip` raus.
- Einbaubilder (`render_guide.ps1`), Anleitung MD/HTML/PDF und beide READMEs nachziehen.
- Druck testet der Nutzer: zuerst nur den Bereich um die Tasche (Testausschnitt `cam_test`, ca. 15 min).
