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
};

extern AppConfig appConfig;

constexpr uint32_t COUNTDOWN_MIN_MS = 1000UL;
constexpr uint32_t COUNTDOWN_MAX_MS = 5999999UL;  // 99:59:999, batas format MM:SS:mmm
constexpr uint8_t DF_VOLUME_MAX = 30;
constexpr uint8_t DF_TRACK_MIN = 1;
// Lantai brightness biar display nggak bisa disetel gelap total lewat web.
constexpr uint8_t BRIGHTNESS_MIN = 10;

// Password WPA2 = satu-satunya lapis auth halaman settings, ganti sebelum
// dipakai di lapangan. Minimal 8 karakter.
constexpr char AP_SSID[] = "P5-TIMER";
constexpr char AP_PASSWORD[] = "@Quantum2022";

void loadConfig(AppConfig &cfg);
void saveConfig(const AppConfig &cfg);

// Paksa semua field ke rentang aman. Return true kalau ada yang diubah.
bool clampConfig(AppConfig &cfg);
