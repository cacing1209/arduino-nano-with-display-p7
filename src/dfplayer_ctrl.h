#pragma once

#include <Arduino.h>

// Layout SD card:
//   /MP3/0001.mp3 -> musik idle (loop)
//   /01/001.mp3   -> voice mulai
//   /01/002.mp3   -> voice waktu habis
//   /01/003.mp3   -> voice game selesai, TIDAK DIPAKAI (lihat project.ino)

constexpr uint8_t DF_VOICE_START = 1;
constexpr uint8_t DF_VOICE_TIME_UP = 2;
// Pemanggilnya di project.ino lagi dikomentarin. Konstantanya sengaja
// dibiarin ada biar ngaktifin lagi cukup hapus '//' di satu baris itu.
constexpr uint8_t DF_VOICE_GAME_OVER = 3;

void dfInit();

// Minta musik idle jalan. Command sebenarnya dikirim dari dfTick() supaya
// voice yang lagi bunyi nggak kepotong.
void dfPlayIdleLoop();

void dfStopIdle();
void dfPlayVoice(uint8_t advertTrack);
void dfApplyVolume(uint8_t vol);

// Wajib dipanggil tiap iterasi loop().
void dfTick();
