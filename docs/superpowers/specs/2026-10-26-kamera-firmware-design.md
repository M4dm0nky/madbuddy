# MadBuddy sieht dich: Kamera-Firmware (Teil 3)

Stand 2026-10-26. Branch `feature/kamera-gehaeuse`. Hardware und Gehäuse: `2026-10-26-kamera-gehaeuse-design.md`.

## Ziel
Der XIAO ESP32S3 Sense erkennt Gesichter und meldet sie dem CYD per UART. Der CYD macht daraus vier Funktionen:
1. **Blick:** Die Augen folgen dem Gesicht.
2. **Begrüßung:** Wer nach einer Weile wiederkommt, wird begrüßt.
3. **Einschlafen:** Ist lange niemand da, schläft der Buddy ein. Das ersetzt den unbrauchbaren Lichtsensor.
4. **Nähe:** Kommt ein Gesicht sehr nah, reagiert er überrascht.

Ein Kamerabild zeigt der CYD nicht. Bilder verlassen den XIAO nie.

## Erkennung auf dem XIAO (`cam/`)
- Eigenes PlatformIO-Projekt `cam/` mit ESP-IDF über pioarduino, Board XIAO ESP32S3 (8 MB PSRAM).
- Komponenten: `esp32-camera` (OV2640, QVGA 320 × 240, Graustufe oder RGB565) und `human_face_detect` aus ESP-DL.
- Ziel sind 5–10 Bilder/s. Gemeldet wird das größte Gesicht im Bild.
- **Erste Umsetzungsaufgabe ist ein Spike:** prüfen, ob `cam/` mit ESP-DL baut, und die Rechenzeit pro Bild abschätzen. Scheitert ESP-IDF mit ESP-DL, ist der Fallback der Arduino-Kern 2.x mit der dort eingebauten Gesichtserkennung.
- Bei abgeschalteter Kamera (`C 0`) ist der Sensor im Standby. Der XIAO sendet dann nur noch Lebenszeichen.
- WLAN bleibt aus. Es wird nur für ein Update eingeschaltet (siehe unten).

## UART-Protokoll
- 115200 Baud, 8N1. CYD: `Serial2`, RX = GPIO 27, TX = GPIO 22. XIAO: D6 = TX, D7 = RX.
- Eine Nachricht ist eine Textzeile: `<inhalt>*<xx>\n`. `xx` ist die XOR-Prüfsumme über `<inhalt>` als zwei Hex-Ziffern. Zeilen mit falscher Prüfsumme, unbekanntem Befehl oder mehr als 400 Zeichen (die `U`-Zeile braucht mehr als 200) werden verworfen.

| Richtung | Zeile | Bedeutung |
|---|---|---|
| XIAO → CYD | `F <x> <y> <w>` | Gesicht: Mitte x/y und Breite, je 0–1000, bezogen auf das Bild |
| XIAO → CYD | `N` | Im aktuellen Bild ist kein Gesicht |
| XIAO → CYD | `H <version> <zustand>` | Lebenszeichen alle 2 s. `zustand`: `on`, `off` oder `err` (Kamera-Init gescheitert) |
| XIAO → CYD | `U <prozent>` / `U ok` / `U err <grund>` | Update-Fortschritt bzw. Ergebnis |
| CYD → XIAO | `C 1` / `C 0` | Kamera an / aus |
| CYD → XIAO | `U⇥<url>⇥<sha256>⇥<ssid>⇥<passwort>` | Update starten. Nur hier sind die Felder durch Tab (⇥) getrennt, weil SSID und Passwort Leerzeichen enthalten dürfen |

- `F`/`N` kommen höchstens einmal pro Bild. Der CYD wertet nur Zustandswechsel und Zeitabstände aus.
- Der gemeinsame Header `src/logic/cam_proto.h` (Prüfsumme, Zeilen bauen und zerlegen) wird von beiden Projekten benutzt. `cam/` bindet ihn per `-I../src` ein. Er hat keine Abhängigkeit von Arduino oder ESP-IDF und ist nativ getestet.

## Verhalten am CYD
Neue reine Logik, ohne Hardware nativ testbar. Alle Zeitpunkte laufen über `logic/deadline.h`.
- `src/logic/cam_link.*`: sammelt Bytes zu Zeilen, prüft und zerlegt sie und führt den Verbindungszustand (verbunden, wenn das letzte `H` weniger als 5 s alt ist).
- `src/logic/presence.*`: macht aus den Meldungen Anwesenheit, Begrüßung, Schlaf und Nähe.
- `src/hw/cam_uart.*`: dünner Treiber für `Serial2`, liest pro Bild alle verfügbaren Bytes und reicht sie an `cam_link` weiter.

**Blick**
- Ein Gesicht wird über `look_at()` (`src/logic/look.h`) zum Blickziel, umgerechnet von 0–1000 auf die Bildschirmkoordinaten. Damit die Augen nicht zittern, wird gleitend gemittelt.
- Der Finger hat Vorrang. Solange er gedrückt ist oder die 3-s-Haltezeit läuft, zählt die Kamera nicht.
- `CAM_MIRROR_X` in `config.h` legt fest, ob x gespiegelt wird. Der Wert wird am Gerät bestimmt. Ziel: Die Augen schauen zur Person.
- Im Schlaf und im Claude-Modus bleibt der Blick wie bisher gerade.

**Anwesenheit**
- „Da“: 2 Gesichtsmeldungen innerhalb von 1 s.
- „Weg“: 30 s ohne Gesichtsmeldung.
- Solange jemand da ist, zählt das wie eine Berührung für die Leerlauf-Zeiten (`mood_touch`). Die Bedürfnisse (Streichel-Zähler und Ähnliches) ändern sich dadurch nicht.

**Begrüßung**
- Beim Wechsel auf „da“ nach mindestens 2 min „weg“ zeigt er `Emotion::GREETING` und eine Sprechblase aus der neuen Satzgruppe `Say::WIEDER` („Da bist du ja!“, „Hallo du!“, „Wieder da!“).
- Höchstens alle 10 min. Die Morgen-Begrüßung (`ritual_morning`) hat Vorrang, wenn beide zugleich fällig sind.

**Einschlafen**
- Nach 10 min „weg“ gilt der Buddy als unbeaufsichtigt und schläft ein. Das wirkt wie die bisherige Dunkel-Logik (`mood_set_dark`), die Kamera ersetzt hier den Lichtsensor.
- Wird es tagsüber (07:00–22:00) wieder „da“, wacht er auf und begrüßt dich.
- Nachts weckt ein Gesicht ihn nicht, damit `mood_night_check` und der Nachtschlaf bleiben. Eine Berührung weckt wie bisher.

**Nähe**
- Ist `w` größer als `CAM_NEAR_W` (Vorgabe 450, wird am Gerät gemessen), zeigt er für 2 s einen überraschten Ausdruck mit schielenden Augen.
- Höchstens alle 30 s.

**Ausfall**
- Ohne Verbindung (5 s kein `H`), bei `zustand = err` oder bei abgeschalteter Kamera fällt `presence` ganz weg, und der Buddy verhält sich wie vor der Kamera.
- Lichtsensor und Nacht-Logik laufen wie bisher weiter.

## Bedienung
- **Zweite Einstellungsseite:** Ein Pfeil unten rechts blättert auf Seite 2, ein Pfeil links zurück. Seite 2 enthält:
  - **Kamera an/aus:** Beim Umschalten und nach jedem Neustart von CYD oder XIAO wird `C 1`/`C 0` gesendet.
  - **Kamera-Info:** Version des XIAO und Verbindung (verbunden, keine Verbindung, Fehler).
- **Speichern:** neues Feld `cam_on` in `Persist` (`src/hw/store.*`), Vorgabe an. `kVersion` 3 → 4, alte Stände laden weiter.
- **Augen-Symbol im Statusstreifen:**
  - grün = sieht ein Gesicht
  - grau = Kamera an, niemand da
  - rot = keine Verbindung oder Fehler
  - kein Symbol = Kamera aus

## Updates
- `tools/ota-publish.mjs` baut und prüft zusätzlich `cam/.pio/build/<env>/firmware.bin`. Die Image-Prüfung erwartet dort die Chip-ID 9 (ESP32-S3) und gilt nur für das cam-Image. Beide Dateien kommen in **ein** Release.
- `manifest.json` bekommt den Eintrag `cam: { "version": "...", "url": "...", "sha256": "..." }`.
- Der CYD vergleicht die `cam.version` aus dem Manifest mit `H <version>`. Ist sie neuer, bietet die bestehende Update-Karte das Update an (eigene Zeile „Kamera“). Nach dem Tipp sendet der CYD `U …`.
- Der XIAO verbindet sich mit dem WLAN, lädt die Datei, prüft die SHA-256 und schreibt sie in die freie OTA-Partition. Danach startet er neu und läuft auf Probe. Steht die Kamera 20 s ohne Fehler und kommen die Lebenszeichen, wird das Image bestätigt, sonst rollt er zurück. Anschließend schaltet er das WLAN wieder ab.
- Die WLAN-Zugangsdaten gehen nur beim Update über den Draht und werden auf dem XIAO nicht gespeichert.
- Der erste Flash des XIAO erfolgt per USB-C vor dem Einbau.

## Tests
- Nativ (`pio test -e native`):
  - `test_cam_proto`: Prüfsumme, Bauen und Zerlegen, kaputte Zeilen
  - `test_cam_link`: Zeilen über Byte-Grenzen hinweg, Verbindungs-Timeout
  - `test_presence`: „da“ und „weg“, Begrüßung mit Sperrzeit, Schlaf nach 10 min, nachts kein Wecken, Nähe mit Sperrzeit, Ausfall
- Jeder Test wird einmal mit einem absichtlich eingebauten Fehler rot gesehen.
- `node --test tools/*.test.mjs`: Manifest mit `cam`, Chip-ID-Prüfung für S3.
- Builds: `cyd2usb` und `cam` über PowerShell.

## Hängt an der Hardware (nach Lieferung)
- Bildrate und Reichweite der Erkennung
- `CAM_MIRROR_X`
- `CAM_NEAR_W`
- Pinreihenfolge P1/CN1 am Foto
- erster Kabel-Flash des XIAO
- Wärme im Gehäuse
