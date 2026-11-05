# Firmware-Update per Abruf (MadBuddy)

Das Gerät holt sich neue Firmware selbst aus dem neuesten GitHub-Release von `M4dm0nky/madbuddy`. Ein Mensch am Display stimmt zu, danach läuft alles ohne Kabel.
Portiert aus dem HA-Buddy (7"-Display), Vorlage in `reference/ha_buddy_ota/`.
**Das erste Mal** muss die Firmware mit diesem Feature per Kabel aufgespielt werden (`pio run -e cyd2usb -t upload`); danach geht es ohne Kabel.
Jede Änderung der Abruf-Adresse, der Dateinamen oder des Projektnamens kostet wieder einen Kabel-Flash.

## Ablauf

```
PC (lokaler Claude)                     Ablage                                   Display
pio run -e cyd2usb                      GitHub-Release (öffentlich)
node tools/ota-publish.mjs --release ►  releases/latest/download/        ◄────  fragt nach 90 s, dann alle 10 min manifest.json
  prüft Image, Veralterung, git           firmware.bin                          (nach Fehler 3 min)
  schreibt ota-out/, dann                 manifest.json                         neue ELF-SHA-256? -> Karte "Update bereit"
  gh release create v<JJJJMMTT-HHMM>                                            Tipp "Ja" (Zustimmung) -> Download über HTTPS
                                                                                Image prüfen -> SHA-256 -> inaktiver Slot
                                                                                Neustart, Gesundheitsnachweis
```

Basis-URL im Gerät: `https://github.com/M4dm0nky/madbuddy/releases/latest/download` (Konstante `kBase` in `src/ota/ota_pull.cpp`).
GitHub leitet `…/latest/download/<Datei>` um (auf `…/releases/download/<Tag>/<Datei>` und weiter auf `*.githubusercontent.com`).

## Zustimmung und Sicherheit

1. **Zustimmung vor Ort.** Heruntergeladen wird erst nach einem Tipp auf *Ja*. *Später* und jeder Tipp daneben schlummern eine Stunde.
   Die Karte erscheint nur auf dem Augen-Bildschirm bei wachem Display (nicht schlafend/gedimmt, damit ein Tipp in ein dunkles Display nie zustimmt) und ignoriert Tipps in der ersten Sekunde.
2. **Keine Geheimnisse in der `.bin`.** WLAN-Zugang und Ort kommen aus dem Portal „MadBuddy“ und liegen im NVS, nicht im Code. Darum darf die Datei öffentlich im Release liegen, ohne Token.
3. **Nur HTTPS, nur GitHub.** `NetworkClientSecure` mit dem Zertifikatspaket des Cores (`useBuiltinCACertBundle()`), **kein** `setInsecure()`.
   Umleitungen verfolgt `open_follow()` selbst (`HTTPC_DISABLE_FOLLOW_REDIRECTS`, Header `Location`, höchstens 3 Sprünge).
   Jede Adresse, auch die erste, muss `ota_redirect_ok()` bestehen: nur `https://`, Host `github.com` oder Endung `.githubusercontent.com` (Host-Test `test/test_ota_policy`).
4. **Prüfung am Image selbst** (`src/logic/ota_policy.h`, nicht nach den Angaben im Manifest): Magic `0xE9`, Chip-ID **0 (ESP32)**, App-Kennung `0xABCD5432`,
   Projektname gleich dem der **laufenden** Firmware (`esp_app_get_description()->project_name`, beim pioarduino-Core `arduino-lib-builder`), ELF-SHA-256 gleich der im Manifest.
   Dazu SHA-256 über die ganze Datei; erst danach `Update.end(true)`, bei Abweichung `Update.abort()`. Größe ≥ 200 KB und ≤ 1 966 080 B (App-Slot `min_spiffs`).
   Ein kaputtes oder fremdes Manifest meldet nie „Update bereit“.
5. **A/B-Slot.** Das Image landet im inaktiven Slot (`Update`-Klasse). Bootloader, Partitionstabelle, NVS und der laufende Slot werden nicht beschrieben. Rettung bleibt USB.
6. **Kein PSRAM.** 4-KB-Puffer per `malloc`, ArduinoJson mit Standard-Allocator und Filter. Während des Ladens gibt die UI das 64-KB-Augen-Sprite frei
   (Fortschrittsbalken mit „konzentrierten“ Augen, direkt gezeichnet); nach einem Fehler wird es neu angelegt.
7. **WiFiManager-Upload gesperrt.** Das Portal-AP hat ein zufälliges Passwort (steht auf dem Display), WiFiManager registriert seine Upload-Seite immer. Ein Upload dort startet das Gerät neu (`setPreOtaUpdateCallback`), Firmware kommt nur über diesen geprüften Weg.

## Dateien

| Datei | Aufgabe |
|---|---|
| `src/ota/ota_pull.*` | Manifest holen (im `net_task` über `net_task_hook`), Download, SHA-256, `Update`, Neustart; `health_tick()` (Gesundheitsnachweis, Hauptschleife) |
| `src/ui/ota_card.*` | Karte „Update bereit“ (Ja/Später), Fortschritt, „Prüfe …“, „Neustart …“, Fehlerkarte (Antippen schließt) |
| `src/logic/ota_policy.h` | Image-Prüfung, Vergleich, Umleitungs-Regel, Schlummern, Fortschritt, Zeile „Firmware“ (Host-Test `test/test_ota_policy`) |
| Einstellungen, Zeile „Firmware“ | Text aus `ota_status_line`; Tippen: `ota_pull::check_now()` |
| `tools/ota-publish.mjs` | Prüft die `.bin`, schreibt `ota-out/firmware.bin` + `manifest.json` (Manifest zuletzt, atomar), `--release` legt das GitHub-Release an |

Manifest (Beispiel):

```json
{ "project": "arduino-lib-builder", "elf": "<64 hex>", "sha256": "<64 hex>", "size": 1459792, "file": "firmware.bin", "built": "Oct 25 2026 10:44", "commit": "c9a7270" }
```
`built` ist die Änderungszeit der `.bin`: Datum/Zeit in `esp_app_desc_t` stammen aus den vorgebauten Core-Bibliotheken und sagen nichts über den eigenen Build.

## Alltag

```
pio run -e cyd2usb && node tools/ota-publish.mjs --release
```
- `node tools/ota-publish.mjs --check` – nur prüfen und anzeigen (Projektname, ELF, Größe), nichts schreiben
- `node tools/ota-publish.mjs` – prüfen und nach `ota-out/` schreiben, ohne Release
- `node tools/ota-publish.mjs --release` – zusätzlich `gh release create v<JJJJMMTT-HHMM> ota-out/firmware.bin ota-out/manifest.json --latest --repo M4dm0nky/madbuddy`

Gates des Skripts: Image gültig und vom richtigen Projekt; nicht größer als der Slot; Quellcode nicht neuer als die `.bin` (`--allow-stale`);
keine uncommitteten Änderungen in `src`, `tools`, `platformio.ini` (`--allow-dirty`), damit kein Stand verschickt wird, den es im Repo nicht gibt.
Veröffentlichen macht nur der lokale Claude (braucht `gh` mit Schreibrecht). Am Display innerhalb von ca. 10 min *Update bereit* → *Ja*.

Tests: `node --test tools/*.test.mjs` (Image-Erkennung) und `pio test -e native -f test_ota_policy`.

## Grenzen / offen

- **Rollback.** Die Core-Bibliotheken sind mit `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` / `CONFIG_APP_ROLLBACK_ENABLE=y` gebaut, der Bootloader kann also zurückrollen.
  Der Core markiert ein frisch geflashtes Image aber standardmäßig schon vor `setup()` als gut; die erste Messung am Gerät (`Zustand 2`, „kein Rollback“) war deshalb nur ein Beleg dafür.
  Seit `main.cpp` `verifyRollbackLater()` mit `true` überschreibt, bleibt das Image im Zustand PENDING_VERIFY (seriell: `ota: Partition <app0|app1>, Zustand 1`),
  bis `ota_pull::health_tick()` (Hauptschleife, **unabhängig vom WLAN**) es nach laufender Augen-Schleife (`ui_alive()`) und 20 s Laufzeit mit
  `esp_ota_mark_app_valid_cancel_rollback()` bestätigt (`ota: neue Firmware als gut bestaetigt`). Ein Neustart/Absturz davor lässt den Bootloader auf den vorigen Slot zurückgehen.
  Nach einem Kabel-Flash ist der Zustand UNDEFINED/VALID: nichts zu bestätigen, Verhalten wie zuvor. Am Gerät noch nicht Ende zu Ende geprüft (kaputtes Image absichtlich ausliefern).
  Grenze: Die Bestätigung prüft nur „Augen laufen“, nicht WLAN oder Wetter; eine Firmware, die startet, aber das Netz nicht kann, bleibt.
- **Keine Signatur.** Echtheit ruht allein auf HTTPS zu GitHub und dem Repo-/Release-Zugang des Besitzers; wer das Release ersetzen kann, kann Firmware einspielen.
  Das SHA-256 im Manifest schützt nur vor Übertragungsfehlern, nicht vor einem manipulierten Release (Manifest und Datei liegen am selben Ort).
- **Downgrade möglich.** Verglichen wird nur „andere ELF-SHA-256“, nicht „neuer“. Ein älteres Release als `latest` wird ebenso angeboten und eingespielt (Anti-Rollback ist aus).
- **Projektkennung generisch.** Der Projektname im Image ist beim pioarduino-Core `arduino-lib-builder`, trifft also auch jedes andere Arduino-ESP32-Image desselben Cores.
  Sicherheit bringt das nicht; es verhindert nur offensichtliche Verwechslungen (anderer Chip/Core).
- **Erster Flash per Kabel**, danach ohne.
- Der Download blockiert den `net_task` (Wetter pausiert so lange). Die Augen laufen weiter bzw. zeigen den Fortschritt. Bleibt der Datenstrom 15 s stehen
  (auch wenn der Socket Daten meldet, aber nichts lesbar ist), bricht der Download mit „Zeitueberschreitung“ ab. Vor dem Neustart (2 s Vorlauf im Zustand RESTARTING) speichert die Hauptschleife einmal alle Werte (`app.cpp`, im Haupt-Task – nicht im `net_task`, damit kein Datenrennen entsteht).
- Speicher (gemessen am Gerät): Der Manifest-Abruf (TLS) läuft bei angelegtem Sprite; Heap-Minimum dabei 63 312 B, mit dem Claude-Webserver danach 49 776 B (`min=` im seriellen Heap-Log).
  Firmware ca. 1,47 MB von 1 966 080 B Slot. Der Download selbst braucht nur den 4-KB-Puffer plus TLS.
- Das Claude-API (`POST /claude`) prüft Schlüssel und Content-Length vor dem Lesen des Körpers (401/413, Verbindung wird geschlossen); siehe `src/net/claude_api.cpp`.
- In der Cloud-Sitzung nicht Ende zu Ende geprüft (kein Gerät, kein Release).
