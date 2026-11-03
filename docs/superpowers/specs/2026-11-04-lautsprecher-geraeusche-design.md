# Lautsprecher und Geräusche – Design

Stand 2026-11-04. Nutzerentscheidungen aus dem Brainstorming: Lautsprecher + Mikrofon; zuerst „auf Geräusche reagieren“, Weckwort/Befehle später; Klang „beides“ (Roboter-Laute + gesprochene Sätze); Lautsprecher im Bauch, Schall nach vorn; Bauch dafür höher bauen.

## Teilprojekte

| Teil | Inhalt | Wo | Dieses Design |
|---|---|---|---|
| A | Lautsprecher: Roboter-Laute (erzeugt) + Sätze von SD, Lautstärke | CYD + Oberkörper | ja |
| B | Geräusche: Pegel, Klatschen, Krach, Stille | XIAO-Mikrofon → UART → CYD | ja |
| C | Weckwort „Hey Buddy“ + 5–10 deutsche Befehle | XIAO | nein, eigenes Design |

Zu C festgehalten: ESP-SR kann Befehle nur Englisch/Chinesisch, Weckwörter nur aus fester Liste. Für Deutsch und „Hey Buddy“ braucht es ein eigenes Wortmodell (z. B. Edge Impulse, ca. 50 Aufnahmen je Wort vom Nutzer). Die SD-Karte ist dafür nicht nötig (Modell klein).

## Teil A1 – Gehäuse: Bauch höher, Lautsprecher vorn

- Lautsprecher: 8 Ω, 1–2 W, rechteckig **20 × 30 mm** (Typ „2030“, ca. 4–5 mm dick), quer eingebaut (30 breit, 20 hoch), Membran nach vorn, hinter einem Lochgitter im `torso_front` **unter** der Kamera. Stecker JST 1,25 mm 2-polig (MX1.25) für den Anschluss SPEAK am CYD; Kabel durch das vorhandene Kabelloch im Hals.
- Der Oberkörper (`t_h`, heute 48) wird so viel höher, dass Lautsprecher-Halter und Kamera-Stapel ohne Engstelle übereinander passen (Schätzung +12 … 15 mm, genaue Zahl im Plan aus dem Modell). Die Kamera behält ihren Abstand nach oben zum Halsblock; der Lautsprecher kommt in den neuen Raum darunter. Untere Dome werden verlegt, wenn sie den Lautsprecher kreuzen.
- Halter: Rahmen mit Anschlag vorn, Lautsprecher von innen eingelegt und mit Klemmsteg/Riegel gehalten (ohne Kleber, druckbar ohne Stützen, Gesicht/Außenseite nach unten). Gitter: Schlitze oder Löcher ≥ 40 % offene Fläche über der Membran. Rückraum hinter dem Lautsprecher offen zum Bauch (Resonanzraum).
- Der Oberbau (Hals, Kopf, Arme, Hände) wandert mit `t_h` nach oben, wie schon bei `leg_h`. Pflicht-Prüfungen: Standsicherheit (Abschnitt 2/2b), Kollisionen (3, 3e), Montage (3g), Kamera (3i, 3j), neue Prüfung Lautsprecher (Sitz, Gitter frei vor der Membran, Positiv-Kontrolle).
- Neu zu drucken: `torso_front`, `torso_back`; andere Teile nur, wenn das Modell es verlangt (wird im Plan festgestellt). Vor dem Druck einen Render zur Abnahme der Proportionen.
- Kaufempfehlung an den Nutzer erst, wenn das Modell den Sitz bestätigt.

## Teil A2 – Ton am CYD

- Ausgang: eingebauter DAC an GPIO 26 → Verstärker des CYD → SPEAK. Ausgabe per DMA (`dac_continuous` aus ESP-IDF 5), 16 kHz, 8 Bit, mono. Ein Ton-Task füllt den Puffer; das Gesicht wird nicht gebremst.
- Zwei Quellen, eine Ausgabe (eine Quelle zur Zeit, Satz hat Vorrang vor Laut):
  1. **Roboter-Laute**, im Programm erzeugt: Folgen aus Tonabschnitten (Frequenz von/bis, Dauer, Hüllkurve, Wellenform Sinus/Rechteck/Rauschen). Je Stimmung leicht höher/tiefer. Keine Dateien.
  2. **Sätze** als WAV (8 Bit unsigned, 16 kHz, mono) von der SD-Karte des CYD (`/sound/*.wav`). Fehlt die Karte oder Datei: nur der Roboter-Laut, keine Fehlermeldung am Gesicht.
- Sätze erzeugt ein PC-Werkzeug `tools/gen-sounds.*` mit einer freien deutschen Offline-Sprachausgabe (Piper, Stimme „thorsten“), wandelt sie ins Zielformat und legt ein Paket an, das der Nutzer auf die SD-Karte kopiert (Dateiname, Pfad und Größe werden angesagt).
- Reine Logik in `src/logic/sound.h` (mit Tests in `test/test_sound`): Ereignis → Laut/Satz, Ruhezeit nach jedem Laut (≥ 3 s, Begrüßung/Pomodoro/Claude ausgenommen), still im Schlaf, Lautstärkestufe, Sätze nur zu besonderen Momenten und selten (höchstens einer je Minute). Die Ton-Erzeugung (Abtastwerte aus den Tonabschnitten) ist ebenfalls reine Logik und am PC testbar.
- Einstellungen: Zeile „Ton“ mit Stufen aus / leise / mittel / laut (Symbol Lautsprecher, farbig je Stufe), gespeichert wie die übrigen Einstellungen. Vorgabe: mittel.

Ereignisse und Laute:

| Ereignis | Roboter-Laut | Satz (Beispiel) |
|---|---|---|
| Begrüßung (Kamera sieht den Nutzer) | Zwitschern steigend | „Hallo!“ / „Da bist du ja!“ |
| Streicheln | Schnurren (tiefes Brummen) | – |
| High-Five | kurzer Triller | – |
| Füttern | Schmatzen, Lieblingsessen fröhlicher | „Lecker!“ |
| Hunger / betrübt | fallender Ton, leise | selten „Ich hab Hunger“ |
| Niesen | „Hatschi“ (Rauschen + Ton) | – |
| Pomodoro fertig | 3-Ton-Melodie | „Pause!“ |
| Claude fertig / Frage | zwei Töne / Fragelaut steigend | „Fertig!“ / „Ich hab eine Frage“ |
| Klatschen (Teil B) | Aufwach-Zwitschern | – |
| Krach (Teil B) | Erschrecken, kurzer Quiek | – |
| Einschlafen | leises Gähnen | – |

## Teil B – Geräusche über das XIAO-Mikrofon

- XIAO ESP32S3 Sense, PDM-Mikrofon (GPIO 42 Takt, 41 Daten), 16 kHz. Eigener kleiner Task neben der Kamera; Ziel: Kamera bleibt bei ≥ 6 Bildern/s (heute 7).
- Auswertung alle 50 ms: Pegel (RMS, in dB-ähnlicher Skala 0–100), gleitender Grundpegel. Ereignisse:
  - **Klatschen:** kurzer, steiler Anstieg weit über den Grundpegel, nach ≤ 150 ms wieder unten.
  - **Krach:** Pegel ≥ Schwelle für ≥ 1 s.
  - **Stille:** Pegel lange unter Grundpegel + kleiner Abstand (Dauer im CYD ausgewertet).
- Erkennung als reine Logik (`src/logic/sound_level.h`, am PC mit künstlichen Pegelfolgen getestet), vom XIAO eingebunden wie `cam_proto.h`.
- Protokoll: neue Zeile XIAO → CYD `A <pegel> <ereignis>` mit Prüfsumme wie alle Zeilen (ereignis: `-`, `K` Klatschen, `L` Laut). Pegel höchstens 2× je Sekunde, Ereignisse sofort. Alte CYD-Firmware ignoriert unbekannte Zeilen (vorher prüfen).
- CYD: Klatschen weckt aus dem Schlaf (Aufwach-Laut), Krach → Erschrecken (SURPRISED + Quiek), lange Stille beschleunigt das Müdewerden. Die `cam:`-Zeile am USB zeigt den Pegel zum Einmessen.
- Das Mikrofon läuft nur, wenn die Kamera eingeschaltet ist (gleicher Schalter, keine neue Einstellung). Audio verlässt den XIAO nie, nur Pegel und Ereignisse.

## Auslieferung und Reihenfolge

1. Gehäuse (A1) – Druck braucht Zeit, deshalb zuerst; Abnahme per Render.
2. Ton am CYD (A2) mit Tests, Release. Testbar sobald Lautsprecher da ist; vorher nur am PC.
3. Geräusche (B) mit Tests, Release (CYD + Kamera-Version hoch, Update am Netzteil).
4. Sätze-Paket für die SD-Karte.

## Grenzen

- 8-Bit-DAC: Klang wie ein altes Telefon; laut genug für den Schreibtisch, kein HiFi.
- Ein Mikrofon: keine Richtung, nur Pegel/Ereignisse.
- Klatschen-Erkennung muss am Gerät eingemessen werden (Schwellen in einer Konfigurationsdatei, wie `CAM_GAZE_Y0`).
