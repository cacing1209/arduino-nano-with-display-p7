#pragma once

#include <Arduino.h>

// Lama frame terakhir ditahan sebelum balik ke layar tunggu. Tanpa ini angka
// 00:00:000 nggak sempat kelihatan karena state langsung pindah ke IDLE.
constexpr unsigned long DISPLAY_FINAL_HOLD_MS = 3000;

void displayInit();

// Layar tunggu: durasi yang lagi ke-arm. Cuma redraw kalau config berubah.
void displayIdle();

// Countdown MM:SS:mmm. Aman dipanggil tiap loop(), throttle diurus di dalam.
void displayCountdown(uint32_t remainingMs);

// Gambar frame terakhir ronde dan tahan selama DISPLAY_FINAL_HOLD_MS.
void displayFreezeFinal(uint32_t remainingMs);

void displayApplyBrightness(uint8_t brightness);
