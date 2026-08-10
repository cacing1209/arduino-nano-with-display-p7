#pragma once

#include <Arduino.h>

struct AppConfig {
  uint32_t countdownMs;
  uint8_t marginTop;
  uint8_t marginBottom;
  uint8_t marginLeft;
  uint8_t marginRight;
  uint8_t dfVolume;          // 0-30
  uint8_t idleTrack;         // nomor track di folder /MP3
  bool idleMusicEnabled;
  uint8_t brightness;        // 0-255, dipetakan ke setBrightness8()
  uint32_t colorIdle;        // 0xRRGGBB, angka di layar tunggu
  uint32_t colorRun;         // 0xRRGGBB, countdown jalan
  uint32_t colorUrgent;      // 0xRRGGBB, 10 detik terakhir
  uint8_t textThickness;     // 1 = normal, 2 = tebal, 3 = ekstra tebal
};

extern AppConfig appConfig;

constexpr uint32_t COUNTDOWN_MIN_MS = 1000UL;
constexpr uint32_t COUNTDOWN_MAX_MS = 5999990UL;  // 99:59:99, batas format MM:SS:cc
constexpr uint8_t DF_VOLUME_MAX = 30;
constexpr uint8_t DF_TRACK_MIN = 1;
// Lantai brightness biar display nggak bisa disetel gelap total lewat web.
constexpr uint8_t BRIGHTNESS_MIN = 10;

constexpr uint32_t COLOR_MASK = 0xFFFFFFUL;
// Lantai warna, alasannya sama kayak BRIGHTNESS_MIN: angka nggak boleh bisa
// disetel jadi hitam (alias hilang) lewat web.
constexpr uint8_t COLOR_MIN_LEVEL = 32;

constexpr uint8_t TEXT_THICKNESS_MIN = 1;
constexpr uint8_t TEXT_THICKNESS_MAX = 3;

constexpr uint32_t COLOR_IDLE_DEFAULT = 0x006E82UL;
constexpr uint32_t COLOR_RUN_DEFAULT = 0xFFAA00UL;
constexpr uint32_t COLOR_URGENT_DEFAULT = 0xFF1E00UL;

// Password WPA2 = satu-satunya lapis auth halaman settings, ganti sebelum
// dipakai di lapangan. Minimal 8 karakter.
constexpr char AP_SSID[] = "P5-TIMER";
constexpr char AP_PASSWORD[] = "@Quantum2022";

void loadConfig(AppConfig &cfg);
void saveConfig(const AppConfig &cfg);

// Paksa semua field ke rentang aman. Return true kalau ada yang diubah.
bool clampConfig(AppConfig &cfg);
