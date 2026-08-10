#pragma once

#include <Arduino.h>

// --- Panel HUB75 ---
constexpr uint8_t PIN_HUB75_R1 = 2;
constexpr uint8_t PIN_HUB75_G1 = 15;
constexpr uint8_t PIN_HUB75_B1 = 4;
constexpr uint8_t PIN_HUB75_R2 = 16;
// GPIO12 = strapping pin MTDI, nggak boleh ketarik HIGH pas boot (ESP32 gagal
// boot, flash voltage ke-set 1.8V). Pin dikunci spec hardware.
constexpr uint8_t PIN_HUB75_G2 = 27;
constexpr uint8_t PIN_HUB75_B2 = 17;
constexpr uint8_t PIN_HUB75_A = 5;
constexpr uint8_t PIN_HUB75_B = 18;
constexpr uint8_t PIN_HUB75_C = 19;
constexpr uint8_t PIN_HUB75_D = 21;
constexpr uint8_t PIN_HUB75_E = 12;
constexpr uint8_t PIN_HUB75_LAT = 26;
constexpr uint8_t PIN_HUB75_OE = 25;
constexpr uint8_t PIN_HUB75_CLK = 22;

// --- DFPlayer Mini (HardwareSerial 2, 9600 8N1) ---
constexpr uint8_t PIN_DFPLAYER_RX = 13;  // RX ESP32 <- TX DFPlayer
constexpr uint8_t PIN_DFPLAYER_TX = 14;  // TX ESP32 -> RX DFPlayer

constexpr uint8_t PIN_TRIGGER_BTN = 16;  // INPUT_PULLUP, aktif LOW

// --- Spec panel: P5 SMD2121, 320x160mm, scan 1/16, driver FM6126A ---
constexpr uint16_t PANEL_RES_X = 64;
constexpr uint16_t PANEL_RES_Y = 32;
constexpr uint8_t PANEL_CHAIN = 1;

// Sisa area gambar minimal setelah margin.
constexpr uint8_t PANEL_MIN_DRAW_W = 16;
constexpr uint8_t PANEL_MIN_DRAW_H = 8;

constexpr uint8_t BUZZER_P = 23;