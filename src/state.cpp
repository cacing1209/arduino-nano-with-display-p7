#include "state.h"

#include "pins.h"

RuntimeState runtime;

void triggerInit()
{
  pinMode(PIN_TRIGGER_BTN, INPUT_PULLUP);
  runtime.btnLastReading = HIGH;
  runtime.btnStableReading = HIGH;
  runtime.btnLastDebounceMs = millis();

  // Idle harus kebaca 1 (ketarik pullup internal). Kalau di sini sudah 0,
  // berarti tombol nyangkut ke gnd atau kabelnya salah pin, bukan soal debounce.
  Serial.printf("[btn] GPIO%u level awal=%d (harusnya 1)\n", PIN_TRIGGER_BTN,
                digitalRead(PIN_TRIGGER_BTN));
}

bool triggerPressed()
{
  // Tombol aktif LOW, jadi level HIGH = lepas. Nilai mentahnya yang disimpan di
  // runtime supaya sama dengan nilai awal HIGH yang diset triggerInit().
  const bool level = (digitalRead(PIN_TRIGGER_BTN) == HIGH);

  if (level != runtime.btnLastReading)
  {
    // Cuma di-print pas ada perubahan. Kalau tiap loop, serial kebanjiran
    // sampai nggak kebaca dan malah ikut ganggu timing.
    Serial.printf("[btn] level=%d\n", level);
    runtime.btnLastReading = level;
    runtime.btnLastDebounceMs = millis();
    return false;
  }

  if (millis() - runtime.btnLastDebounceMs <= BTN_DEBOUNCE_MS)
    return false;
  if (level == runtime.btnStableReading)
    return false;

  runtime.btnStableReading = level;
  Serial.printf("[btn] stabil: %s\n", level ? "lepas" : "TEKAN");
  return !level; // press = transisi HIGH -> LOW
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
