#pragma once

#include <Arduino.h>

// Saklar log serial. Buat build produksi, matiin lewat platformio.ini:
//   build_flags = -DDEBUG_SYS_OFF
// Semua DBG_* jadi no-op dan string literalnya nggak ikut ke flash.
#ifndef DEBUG_SYS_OFF
#define DEBUG_SYS
#endif

#ifdef DEBUG_SYS

// delay-nya biar serial monitor sempat nyambung; tanpa itu baris log pertama
// sering kepotong.
#define DBG_BEGIN(baud)   \
  do {                    \
    Serial.begin(baud);   \
    delay(1000);          \
  } while (0)

#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)

#else

// Argumen sengaja nggak dievaluasi: semua pemanggil DBG_* cuma baca nilai,
// nggak ada efek samping.
#define DBG_BEGIN(baud) ((void)0)
#define DBG_PRINT(...) ((void)0)
#define DBG_PRINTLN(...) ((void)0)
#define DBG_PRINTF(...) ((void)0)

#endif
