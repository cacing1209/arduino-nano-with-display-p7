#pragma once

#include <Arduino.h>

enum class TimerState : uint8_t { IDLE, RUNNING };

struct RuntimeState {
  TimerState state = TimerState::IDLE;
  uint32_t remainingMs = 0;
  unsigned long lastTickMs = 0;
  bool btnLastReading = HIGH;
  unsigned long btnLastDebounceMs = 0;
  bool btnStableReading = HIGH;  // level stabil terakhir, pembanding debounce
};

extern RuntimeState runtime;

constexpr unsigned long BTN_DEBOUNCE_MS = 50;

void triggerInit();
bool triggerPressed();  // true sekali tiap transisi HIGH->LOW yang sudah stabil

void timerStart(uint32_t durationMs);
void timerStop();
bool timerTick();  // true tepat sekali saat sisa waktu menyentuh 0
