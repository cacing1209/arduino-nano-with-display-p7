#pragma once

#include <Arduino.h>

// Saklar log serial. Komentarin baris di bawah (kasih '//') buat build
// produksi: semua DBG_* jadi no-op, string literalnya nggak ikut kekompilasi
// ke flash, dan UART debug nggak diinisialisasi sama sekali.
//
// Bisa juga dimatiin tanpa nyentuh file ini lewat platformio.ini:
//   build_flags = -DDEBUG_SYS_OFF
#ifndef DEBUG_SYS_OFF
#define DEBUG_SYS
#endif

#ifdef DEBUG_SYS

// delay-nya biar serial monitor sempat nyambung sebelum baris log pertama
// keluar; tanpa itu "SYSTEM BEGIN" sering kepotong.
#define DBG_BEGIN(baud)   \
  do {                    \
    Serial.begin(baud);   \
    delay(1000);          \
  } while (0)

#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)

#else

// Argumennya sengaja nggak dievaluasi: semua pemanggil DBG_* di project ini
// cuma baca nilai, nggak ada yang punya efek samping.
#define DBG_BEGIN(baud) ((void)0)
#define DBG_PRINT(...) ((void)0)
#define DBG_PRINTLN(...) ((void)0)
#define DBG_PRINTF(...) ((void)0)

#endif
