#include "buzzer_ctrl.h"

#include "pins.h"

namespace {

struct BuzzStep {
  bool on;
  uint16_t durMs;
};

// Buzzer aktif: cuma digital on/off, nadanya dari oscillator buzzer sendiri.
// Driver S8050 NPN = base ketarik HIGH bikin transistor nyala, jadi aktif HIGH.
// Kalau modul buzzer lu ternyata aktif LOW, tinggal balik dua konstanta ini.
constexpr uint8_t kBuzzOn = HIGH;
constexpr uint8_t kBuzzOff = LOW;

// Nada nggak bisa dibedain, jadi tiap event dibedain lewat pola durasi.
constexpr BuzzStep kStart[] = {{true, 90}};
constexpr BuzzStep kTick[] = {{true, 45}};
constexpr BuzzStep kTimeUp[] = {{true, 160}, {false, 90}, {true, 160}, {false, 90}, {true, 700}};
constexpr BuzzStep kGameOver[] = {{true, 180}, {false, 60}, {true, 420}};

const BuzzStep *steps = nullptr;
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
unsigned long stepUntilMs = 0;

void applyStep() {
  const BuzzStep &s = steps[stepIndex];
  digitalWrite(PIN_BUZZER, s.on ? kBuzzOn : kBuzzOff);
  stepUntilMs = millis() + s.durMs;
}

}  // namespace

void buzzerInit() {
  pinMode(PIN_BUZZER, OUTPUT);
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
  digitalWrite(PIN_BUZZER, kBuzzOff);
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
