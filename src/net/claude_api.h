#pragma once
#include <stdint.h>
#include "../logic/claude_state.h"

// Claude-Modus: kleiner HTTP-Empfaenger (Arduino WebServer, Port 80, mDNS "madbuddy").
// POST /claude (Header X-Buddy-Key, JSON <= 256 B) -> claude_event; GET /claude/ping -> {"ok":true,"state":"..."}.
// Der WebServer laeuft in einem eigenen Task (claude_srv, Kern 0); die Handler legen nur Ereignisse in eine Queue,
// claude_api_loop() (UI-Task) wendet sie auf die Logik an.
void claude_api_begin(ClaudeLogic *logic);   // Schluessel laden/erzeugen (NVS "ckey"), Task starten; Server startet spaeter
void claude_api_loop();                      // aus app_loop: Server je nach WLAN/Portal wollen, Queue leeren
void claude_api_stop();                      // Server nicht mehr wollen (asynchron); Port 80 frei erst bei claude_api_released()
bool claude_api_released();                  // true, sobald Port 80 wirklich frei ist (vor dem WLAN-Portal pruefen)
const char *claude_api_key();                // 12 Hex-Zeichen, "" vor claude_api_begin
ClaudeState claude_api_state(uint32_t now_ms);   // aktueller Claude-Zustand (OFF ohne Logik), nur aus dem UI-Task
