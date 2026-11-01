#pragma once
#include <stdint.h>
#include "../logic/cam_link.h"

// Kamera-Leitung zum XIAO (Serial2): Bytes lesen, Zeilen prüfen, Meldungen weitergeben.
void cam_uart_begin();
void cam_uart_poll(uint32_t now);                              // liest alle verfügbaren Bytes (höchstens 1024 je Aufruf)
void cam_uart_send(const char *content);                       // versiegelt (cam_seal) und schreibt
void cam_uart_send_ctrl(bool on);                              // C 1 / C 0
bool cam_uart_take_restart();                                  // true einmal nach erstem H bzw. Trennung
const CamLink &cam_uart_link();
void cam_uart_on_msg(void (*fn)(const CamMsg &, uint32_t now));
