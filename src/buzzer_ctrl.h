#pragma once

#include <Arduino.h>

// Pola bunyi buzzer (buzzer aktif, digital on/off). Dijalanin non-blocking dari
// buzzerTick(), jadi countdown dan refresh panel nggak ketahan pas buzzer bunyi.
enum class BuzzPattern : uint8_t {
  START,      // timer mulai: 1 beep pendek
  TICK,       // detik-detik terakhir: beep tipis
  TIME_UP,    // waktu habis: 2 beep pendek + 1 panjang
  GAME_OVER,  // distop paksa: 1 beep pendek + 1 panjang
};

void buzzerInit();

// Pola baru langsung motong pola yang lagi jalan.
void buzzerPlay(BuzzPattern pattern);

void buzzerStop();

// Wajib dipanggil tiap iterasi loop().
void buzzerTick();
