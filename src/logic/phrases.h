#pragma once
#include <stdint.h>

// ERZEUGT von tools/gen-phrases.mjs aus docs/superpowers/specs/2026-10-25-saetze.json – nicht von Hand ändern.
// Sätze in UTF-8; vor dem Zeichnen über logic/ascii.h wandeln. Host-Test test/test_phrases.
enum class Say : uint8_t { MORGEN, NACHT, FROH, VERLIEBT, GENERVT, REGEN, SONNE, HITZE, KALT, GEWITTER, POMODORO_START, POMODORO_PAUSE, POMODORO_FERTIG, LECKER, LIEBLINGSESSEN, HIGHFIVE, GEDANKEN, FRECH, VERSCHMUST, CLAUDE_OKAY, CLAUDE_HMM, CLAUDE_FEHLER, CLAUDE_HEY, CLAUDE_FERTIG, WIEDER, WINKEN, COUNT };

namespace phrases_detail {
inline const char *const MORGEN[] = {"Moin! Schon wach?", "Guten Morgen!", "Sonne oder Kaffee?", "Na, gut geschlafen?", "Auf in den Tag!", "Hallo du! Endlich wach."};
inline const char *const NACHT[] = {"Gute Nacht!", "Schlaf gut!", "Bis morgen, träum schön.", "Ich bin sooo müde...", "Nachtruhe. Psst!", "Mondschein-Zeit."};
inline const char *const FROH[] = {"Hurra!", "Super Tag heute!", "Ich freu mich so!", "Alles bestens!", "Du bist toll!", "Juhu!"};
inline const char *const VERLIEBT[] = {"Mir wird ganz warm.", "Du bist mein Liebling.", "Das ist schön...", "Herzchen klopf' schnell!", "Hach, wie schön.", "Ich mag dich!"};
inline const char *const GENERVT[] = {"Hey! Nicht stupsen!", "Lass das!", "Grrr...", "Nicht so doll!", "Ich bin genervt!", "Lass mich in Ruhe!"};
inline const char *const REGEN[] = {"Plitschplatsch!", "Regentropfen-Tag.", "Schirm nicht vergessen!", "Nasse Füße, igitt!", "Draußen regnet's.", "Regen... gemütlich."};
inline const char *const SONNE[] = {"Die Sonne scheint!", "Sonnenbrille an!", "So schön hell heute!", "Ab in die Sonne!", "Sonnenschein-Laune!", "Herrliches Wetter!"};
inline const char *const HITZE[] = {"Puh, ist das heiß!", "Ich schmelze!", "Eis bitte!", "Ventilator an!", "Viel trinken, ja?", "Hitzefrei?"};
inline const char *const KALT[] = {"Brrr, ist das kalt!", "Zieh dich warm an!", "Ich friere...", "Eiskalt draußen!", "Decke bitte!", "Frostig heute."};
inline const char *const GEWITTER[] = {"Donner! Huch!", "Ich hab Angst...", "Blitz! Versteck mich!", "Bleib lieber drinnen.", "Rumms! Gewitter!", "Ist es bald vorbei?"};
inline const char *const POMODORO_START[] = {"Los geht's!", "Konzentration!", "Du schaffst das!", "Fokus! Ich bin still.", "Arbeiten wir zusammen.", "Auf geht's!"};
inline const char *const POMODORO_PAUSE[] = {"Pause! Streck dich!", "Steh mal kurz auf!", "Trink was Wasser!", "Tief durchatmen.", "Gut gemacht! Pause.", "Augen zu, entspann!"};
inline const char *const POMODORO_FERTIG[] = {"Pause vorbei!", "Weiter geht's!", "Wieder dran!", "Frisch ans Werk!", "Noch eine Runde?", "Auf ein Neues!"};
inline const char *const LECKER[] = {"Mmh, lecker!", "Danke!", "Mampf mampf.", "Das war gut!", "Lecker-schmecker!", "Mehr davon!"};
inline const char *const LIEBLINGSESSEN[] = {"Mein Lieblingsessen!", "JAAA! Das mag ich!", "Du kennst mich gut!", "Das Allerbeste!", "Mein Favorit!", "Juhu, das Beste!"};
inline const char *const HIGHFIVE[] = {"High five!", "Klatsch!", "Gib mir fünf!", "Abgeklatscht!", "Team!", "Yeah!"};
inline const char *const GEDANKEN[] = {"Was wohl die Wolken tun?", "Ich denk nach...", "Hmm...", "Wie schmeckt der Mond?", "Ob Fische träumen?", "Tagträume..."};
inline const char *const FRECH[] = {"Na du!", "Fang mich doch!", "Hihi, erwischt!", "Ich bin frech heute.", "Ätsch!", "Wetten, du verlierst?"};
inline const char *const VERSCHMUST[] = {"Ich bin gern bei dir.", "Kuscheln?", "Bleib noch ein bisschen.", "Ich mag dich sehr.", "Du bist so lieb.", "Warm und kuschelig."};
inline const char *const CLAUDE_OKAY[] = {"Okay!", "Bin dran!", "Mach ich!", "Verstanden!", "Alles klar!", "Los geht's!"};
inline const char *const CLAUDE_HMM[] = {"Hmm...", "Mal überlegen...", "Moment...", "Aha...", "Interessant...", "Lass mich denken..."};
inline const char *const CLAUDE_FEHLER[] = {"Mist!", "Huch, Fehler!", "Oje...", "Nochmal!", "Autsch!", "Das klappte nicht."};
inline const char *const CLAUDE_HEY[] = {"Hey, du!", "Ich brauch dich!", "Schau mal her!", "Frage an dich!", "Kurz mal bitte!", "Hallo? Deine Hilfe!"};
inline const char *const CLAUDE_FERTIG[] = {"Fertig!", "Geschafft!", "Erledigt!", "Tadaa!", "Das war's!", "Alles fertig!"};
inline const char *const WIEDER[] = {"Da bist du ja!", "Hallo du!", "Wieder da!", "Schön, dich zu sehen!", "Hab dich vermisst!", "Na, wieder zurück?"};
inline const char *const WINKEN[] = {"Hallo!", "Hi!", "Huhu!", "Winke winke!", "Hallo du!", "Hey, hi!"};
}  // namespace phrases_detail

struct SayList { const char *const *items; uint8_t n; };
inline SayList say_list(Say s) {
  using namespace phrases_detail;
  switch (s) {
    case Say::MORGEN: return SayList{MORGEN, 6};
    case Say::NACHT: return SayList{NACHT, 6};
    case Say::FROH: return SayList{FROH, 6};
    case Say::VERLIEBT: return SayList{VERLIEBT, 6};
    case Say::GENERVT: return SayList{GENERVT, 6};
    case Say::REGEN: return SayList{REGEN, 6};
    case Say::SONNE: return SayList{SONNE, 6};
    case Say::HITZE: return SayList{HITZE, 6};
    case Say::KALT: return SayList{KALT, 6};
    case Say::GEWITTER: return SayList{GEWITTER, 6};
    case Say::POMODORO_START: return SayList{POMODORO_START, 6};
    case Say::POMODORO_PAUSE: return SayList{POMODORO_PAUSE, 6};
    case Say::POMODORO_FERTIG: return SayList{POMODORO_FERTIG, 6};
    case Say::LECKER: return SayList{LECKER, 6};
    case Say::LIEBLINGSESSEN: return SayList{LIEBLINGSESSEN, 6};
    case Say::HIGHFIVE: return SayList{HIGHFIVE, 6};
    case Say::GEDANKEN: return SayList{GEDANKEN, 6};
    case Say::FRECH: return SayList{FRECH, 6};
    case Say::VERSCHMUST: return SayList{VERSCHMUST, 6};
    case Say::CLAUDE_OKAY: return SayList{CLAUDE_OKAY, 6};
    case Say::CLAUDE_HMM: return SayList{CLAUDE_HMM, 6};
    case Say::CLAUDE_FEHLER: return SayList{CLAUDE_FEHLER, 6};
    case Say::CLAUDE_HEY: return SayList{CLAUDE_HEY, 6};
    case Say::CLAUDE_FERTIG: return SayList{CLAUDE_FERTIG, 6};
    case Say::WIEDER: return SayList{WIEDER, 6};
    case Say::WINKEN: return SayList{WINKEN, 6};
    default: return SayList{nullptr, 0};
  }
}
