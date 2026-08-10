#include "state.h"

#include "pins.h"

RuntimeState runtime;

void triggerInit()
{
  pinMode(PIN_TRIGGER_BTN, INPUT_PULLUP);
  runtime.btnLastReading = HIGH;
  runtime.btnStableReading = HIGH;
  runtime.btnLastDebounceMs = millis();
}

bool triggerPressed()
{
  const bool reading = (digitalRead(PIN_TRIGGER_BTN) == LOW);
  Serial.println("button trigger:" + String(reading));
  if (reading != runtime.btnLastReading)
  {
    runtime.btnLastReading = reading;
    runtime.btnLastDebounceMs = millis();
    return false;
  }

  if (millis() - runtime.btnLastDebounceMs <= BTN_DEBOUNCE_MS)
    return false;
  if (reading == runtime.btnStableReading)
    return false;

  runtime.btnStableReading = reading;
  return !reading; // tombol aktif LOW, jadi press = transisi ke LOW
}

void timerStart(uint32_t durationMs)
{
  runtime.state = TimerState::RUNNING;
  runtime.remainingMs = durationMs;
  runtime.lastTickMs = millis();
}

void timerStop()
{
  runtime.state = TimerState::IDLE;
  runtime.remainingMs = 0;
}

bool timerTick()
{
  if (runtime.state != TimerState::RUNNING || runtime.remainingMs == 0)
    return false;

  const unsigned long now = millis();
  const unsigned long elapsed = now - runtime.lastTickMs;
  runtime.lastTickMs = now;

  if (elapsed >= runtime.remainingMs)
  {
    runtime.remainingMs = 0;
    return true;
  }

  runtime.remainingMs -= elapsed;
  return false;
}
