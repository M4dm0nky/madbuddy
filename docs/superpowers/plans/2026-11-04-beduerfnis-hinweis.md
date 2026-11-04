# Plan: Bedürfnis-Hinweis + Testbild für den 3-mm-Rand

## Context
- **Aufgabe 4 (FORTSCHRITT):** Wenn der Buddy betrübt ist, weiß man nicht, was ihm fehlt. Die Antworten des Nutzers vom 2026-11-04: Er will Variante **c**, also erst eine Blase mit Symbol und danach ein kleines Symbol, das stehen bleibt. Das soll **für jedes niedrige Bedürfnis** gelten. **Keinen** Ton.
- **Aufgabe 3:** Im Fenster des Roboterkopfs ist rechts ein ca. 3 mm breiter Streifen ohne Bild zu sehen. Der Streifen ist schwarz, nicht hell. Der Kopf ist nachgedruckt (CYD 3 mm weiter links), USB-C passt, der CYD sitzt fest.
  - Foto IMG_0993: Rechts liegt schwarz zwischen dem Bildinhalt (WLAN-Symbol) und dem Rahmen.
  - Weil der Gesichtshintergrund auch schwarz ist, kann man so nicht unterscheiden, ob das Bild zu schmal gezeichnet wird, der CYD versetzt sitzt oder das Fenster breiter ist als die bespielte Fläche (`act_w` 59 ist gemessen; das Datenblatt des ILI9341 sagt 57,6).
  - Deshalb kommt zuerst ein Testbild, dann erst eine Änderung am Gehäuse.
- Lautsprecher fehlt noch. Teil A2 (Ton) läuft getrennt auf `feature/sound` weiter und ist nicht Teil dieses Plans.

Branch: `feature/beduerfnis-hinweis` von `main`.

## Teil 1: Bedürfnis-Hinweis

### Logik (neu: `src/logic/need_hint.h`, auf dem PC testbar)
- `enum class Need { SICK, HUNGRY, SLEEPY, SAD, BORED }`. Die Reihenfolge ist die Dringlichkeit und entspricht `mood_base_emotion` in [mood.h:156](src/logic/mood.h#L156).
- **Schwellen** wie in `mood_base_emotion`, mit 5 Punkten Hysterese, damit nichts flackert:

  | Bedürfnis | gilt als niedrig, wenn … |
  |---|---|
  | SICK | `s.sick` |
  | HUNGRY | Sättigung < 25 |
  | SLEEPY | Energie < 30 (nur Energie, nicht „Nacht“) |
  | SAD | Zuneigung < 20 |
  | BORED | Spaß < 25 |

  Ein Bedürfnis zählt erst wieder als erfüllt, wenn es über Schwelle + 5 liegt.
- `NeedHintState` hält die Maske der aktiven Bedürfnisse, den Zeitpunkt der letzten Blase je Bedürfnis und das zuletzt angezeigte Bedürfnis.
- `need_hint_update(st, mood, now)` liefert:
  - die **Maske** für die stehenden Symbole;
  - das Bedürfnis, für das **jetzt eine Blase** fällig ist, oder keins.
- Wann eine Blase fällig ist:
  - beim Unterschreiten der Schwelle (Flanke);
  - danach als Erinnerung alle 15 min, solange das Bedürfnis niedrig bleibt;
  - sind mehrere Bedürfnisse fällig, kommt nur das dringendste dran.
- Text und Symbol je Bedürfnis (Texte darf Qwen gegenlesen):

  | Bedürfnis | Symbol | Text der Blase |
  |---|---|---|
  | SICK | `icon_thermometer` | „Mir ist schlecht – füttern + streicheln“ |
  | HUNGRY | `icon_apple` | „Ich hab Hunger!“ |
  | SLEEPY | `icon_pillow` | „Bin so müde …“ |
  | SAD | `icon_heart` | „Streichel mich mal!“ |
  | BORED | `icon_ball` | „Spiel mit mir!“ |

  Alle Texte unter `SPEECH_CHARACTER_LIMIT` = 40.

### Einbau
- **[src/app.cpp](src/app.cpp)**, in `speech_frame` (Zeilen 483–521): `need_hint_update` aufrufen.
  - Die Blase läuft über `show_bubble` (Zeile 198), mit dem Symbol als neuem Begleitwert `s_bubble_icon`.
  - Es gelten dieselben Sperren wie heute: kein Hinweis im Schlaf (Zeile 486), keiner in der Arbeits- bzw. Pomodoro-Phase oder im Claude-Modus (Zeile 490).
  - Ein fälliger Hinweis wartet, bis er gezeigt werden darf.
  - Die vorhandene zufällige Stimmungsblase (`speech_due`, 40 %) bleibt.
- **[src/ui/bubble.cpp](src/ui/bubble.cpp)**: `bubble_draw` bekommt ein optionales Symbol (Funktionszeiger oder Enum), das links vom Text in der Blase sitzt, ca. 20 px groß. Ohne Symbol bleibt die Blase wie bisher.
- **[src/ui/eyes.cpp](src/ui/eyes.cpp)**, Zeilen 415–423, und [eyes.h:16](src/ui/eyes.h#L16): Parameter für das Symbol der Blase und die Maske der stehenden Symbole.
- **Stehende Symbole:**
  - oben links im Gesicht, eine Reihe ab (6, 6), je 22 px, Abstand 4, in der Reihenfolge der Dringlichkeit;
  - farbig in den Farben der vorhandenen Symbole;
  - ausgeblendet, solange eine Blase sichtbar ist oder der Buddy schläft.
  - Die untere Statusleiste ist voll (x 8–326), deshalb kommen die Symbole nicht dorthin.
- Wird das Gesicht nur teilweise neu gezeichnet: Ändert sich die Maske, wird neu gezeichnet. Ob dafür etwas zu tun ist, prüft der Umsetzer am Sprite-Weg in `eyes_draw`.

### Tests (`test/test_need_hint/`, TDD)
- Flanke löst genau eine Blase aus. Unter der Schwelle kommt die nächste erst nach 15 min.
- Hysterese: 24 → 26 → 24 ergibt keine zweite Blase. Erst über 30 (bzw. Schwelle + 5) wird das Bedürfnis wieder als erfüllt gewertet.
- Mehrere Bedürfnisse niedrig: Die Maske enthält alle, die Blase kommt für das dringendste.
- SICK geht vor HUNGRY.
- Gegenprobe: Ein absichtlich falsch gesetzter Schwellwert muss einen Test rot machen.

## Teil 2: Testbild (Diagnose 3 mm)
- Neuer USB-Befehl `testbild` in [app.cpp:393](src/app.cpp#L393), neben `snap` und `uinfo`. Er zeichnet:
  - weißen Grund;
  - einen 1-px-Rahmen in Rot genau auf Pixel 0 und 319 bzw. 0 und 239;
  - Striche alle 10 px an allen Rändern, jeder 50. länger;
  - die Ecken in verschiedenen Farben.
- Das Bild bleibt stehen, bis man auf den Bildschirm tippt oder 60 s vergangen sind.
- Ich sende den Befehl selbst per pyserial an den CYD (Port wie bei `cam-snap.py`).
- **Nutzer:** sagt bzw. fotografiert, ob links und rechts die rote Linie zu sehen ist und wie viel Weiß bzw. Schwarz bis zum Rahmen bleibt. Daraus folgt:
  - Rot links verdeckt, rechts Schwarz → CYD sitzt zu weit links: `aa_left` etwa 11,5 statt 13.
  - Beide roten Linien sichtbar, auf beiden Seiten Rand → Fenster zu breit: `act_w` bzw. `win_w` verkleinern.
  - Rand nur rechts, links bündig → `win_off_x` bzw. `aa_left` anpassen.
- Die Änderung am Gehäuse folgt als eigener kleiner Schritt nach dieser Messung, danach läuft `check_roboter.ps1 -Only 1,1b,1c,3,3b`. Sie gehört nicht zu diesem Plan.

## Ablauf und Modelle
1. Opus legt Spec und Plan als Dateien unter `docs/superpowers/` an und erstellt den Branch.
2. Sonnet-Subagent: Teil 1 per TDD, dann Teil 2. Ihm wird ausdrücklich gesagt: nur `pio test -e native -f test_need_hint` plus die betroffenen Gruppen (`test_speech`, `test_care`) laufen lassen, keine hängenden Prozesse, pio nur über PowerShell.
3. Opus prüft: ein Review über den Diff von `src/` und `test/`, den vollen Lauf `pio test -e native` einmal, Build `cyd2usb`.
4. **Release** `node tools/ota-publish.mjs --release`, nur für den CYD, vorher den Branch pushen. **Die Freigabe dieses Plans gilt als Ja für genau dieses Release.** Danach installiert der Nutzer das Update am Gerät, wie bisher über die Einstellungen.
5. Opus sendet `testbild`, der Nutzer meldet, was er sieht. Opus trägt alles in FORTSCHRITT ein und committet.

## Prüfung
- `pio test -e native`: alles grün, die Gegenprobe wurde einmal rot.
- Build `pio run -e cyd2usb`: SUCCESS, Speicher im Rahmen.
- Am Gerät:
  - Bedürfnis künstlich niedrig setzen (z. B. per vorhandenem USB-Befehl oder abwarten). Blase mit Symbol, danach Symbol oben links.
  - Füttern bzw. Streicheln lässt das Symbol verschwinden.
- Testbild per USB erscheint, ein Tipp beendet es.
- Zum Schluss: keine verwaisten `pio`-, `python`- oder `node`-Prozesse.
