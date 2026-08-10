#pragma once

#include <Arduino.h>

// --- Panel HUB75 (mapping PCB, ringkasannya di wirring.txt) ---
constexpr uint8_t PIN_HUB75_R1 = 2;
constexpr uint8_t PIN_HUB75_G1 = 15;
constexpr uint8_t PIN_HUB75_B1 = 4;
constexpr uint8_t PIN_HUB75_R2 = 16;
constexpr uint8_t PIN_HUB75_G2 = 27;
constexpr uint8_t PIN_HUB75_B2 = 17;
constexpr uint8_t PIN_HUB75_A = 5;
constexpr uint8_t PIN_HUB75_B = 18;
constexpr uint8_t PIN_HUB75_C = 19;
constexpr uint8_t PIN_HUB75_D = 21;
// GPIO12 = strapping pin MTDI, nggak boleh ketarik HIGH pas boot (ESP32 gagal
// boot, flash voltage ke-set 1.8V). Scan 1/16 nggak pakai jalur E, tapi library
// tetap ngedrive pin-nya, jadi kasih pulldown 10K di jalur ini.
constexpr uint8_t PIN_HUB75_E = 12;
constexpr uint8_t PIN_HUB75_LAT = 26;
constexpr uint8_t PIN_HUB75_OE = 25;
constexpr uint8_t PIN_HUB75_CLK = 22;

// --- DFPlayer Mini (HardwareSerial 2, 9600 8N1) ---
constexpr uint8_t PIN_DFPLAYER_RX = 13;  // RX ESP32 <- TX DFPlayer
constexpr uint8_t PIN_DFPLAYER_TX = 14;  // TX ESP32 -> RX DFPlayer

constexpr uint8_t PIN_BUZZER = 23;

// GPIO32 = pad RTC SCL di PCB, kepakai buat tombol karena RTC nggak dipasang.
// GPIO16 nggak bisa: sudah jadi R2 panel.
constexpr uint8_t PIN_TRIGGER_BTN = 32;  // INPUT_PULLUP, aktif LOW

// --- Spec panel: P5 SMD2121, 320x160mm, scan 1/16, driver FM6126A ---
constexpr uint16_t PANEL_RES_X = 64;
constexpr uint16_t PANEL_RES_Y = 32;
constexpr uint8_t PANEL_CHAIN = 1;

// Titik tengah panel ini meleset ke kanan: dicoba langsung di panelnya, angka
// baru kelihatan pas di tengah kalau area gambar mulai dari x=2. Offset ini
// dipakai sebelum margin dari web, jadi margin 0 = sudah pas tengah dan user
// nggak perlu nambal pakai margin kiri 2.
constexpr uint8_t PANEL_X_OFFSET = 2;

// Sisa area gambar minimal setelah offset + margin.
constexpr uint8_t PANEL_MIN_DRAW_W = 16;
constexpr uint8_t PANEL_MIN_DRAW_H = 8;

// Batas total margin yang masih nyisain area gambar minimal.
constexpr uint8_t MARGIN_MAX_X = PANEL_RES_X - PANEL_X_OFFSET - PANEL_MIN_DRAW_W;
constexpr uint8_t MARGIN_MAX_Y = PANEL_RES_Y - PANEL_MIN_DRAW_H;
