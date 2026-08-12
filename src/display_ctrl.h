#pragma once

#include <Arduino.h>

constexpr unsigned long DISPLAY_FINAL_HOLD_MS = 5000;

void displayInit();

// Layar tunggu: durasi yang lagi ke-arm. Cuma redraw kalau config berubah.
void displayIdle();

// Countdown MM:SS. Aman dipanggil tiap loop(), throttle diurus di dalam.
void displayCountdown(uint32_t remainingMs);

// Gambar frame terakhir ronde dan tahan selama DISPLAY_FINAL_HOLD_MS.
void displayFreezeFinal(uint32_t remainingMs);

void displayApplyBrightness(uint8_t brightness);
