#include "buzzer_ctrl.h"

#include "pins.h"

namespace {

struct BuzzStep {
  uint16_t freqHz;  // 0 = jeda diam
  uint16_t durMs;
};

// Pakai tone() (LEDC di balik layar), bukan digitalWrite. Buzzer pasif butuh
// square wave ini; buzzer aktif yang sudah punya oscillator tetap bunyi karena
// pin-nya tetap digetarkan, cuma nadanya nurut oscillator sendiri.
constexpr BuzzStep kStart[] = {{2400, 90}};
constexpr BuzzStep kTick[] = {{2000, 45}};
constexpr BuzzStep kTimeUp[] = {{2600, 160}, {0, 90}, {2600, 160}, {0, 90}, {2600, 700}};
constexpr BuzzStep kGameOver[] = {{1900, 180}, {0, 60}, {1300, 420}};

const BuzzStep *steps = nullptr;
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
unsigned long stepUntilMs = 0;

void applyStep() {
  const BuzzStep &s = steps[stepIndex];
  if (s.freqHz == 0) {
    noTone(PIN_BUZZER);
  } else {
    tone(PIN_BUZZER, s.freqHz);
  }
  stepUntilMs = millis() + s.durMs;
}

}  // namespace

void buzzerInit() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  buzzerStop();
}

void buzzerPlay(BuzzPattern pattern) {
  switch (pattern) {
    case BuzzPattern::START:
      steps = kStart;
      stepCount = sizeof(kStart) / sizeof(kStart[0]);
      break;
    case BuzzPattern::TICK:
      steps = kTick;
      stepCount = sizeof(kTick) / sizeof(kTick[0]);
      break;
    case BuzzPattern::TIME_UP:
      steps = kTimeUp;
      stepCount = sizeof(kTimeUp) / sizeof(kTimeUp[0]);
      break;
    case BuzzPattern::GAME_OVER:
      steps = kGameOver;
      stepCount = sizeof(kGameOver) / sizeof(kGameOver[0]);
      break;
  }

  stepIndex = 0;
  applyStep();
}

void buzzerStop() {
  steps = nullptr;
  stepCount = 0;
  stepIndex = 0;
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

void buzzerTick() {
  if (steps == nullptr) return;
  if (static_cast<long>(millis() - stepUntilMs) < 0) return;

  if (++stepIndex >= stepCount) {
    buzzerStop();
    return;
  }

  applyStep();
}
