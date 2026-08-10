// ESP32 HUB75 P5 32x64 countdown timer + DFPlayer + web config.
// File ini cuma orkestrasi antar modul + transisi state machine.

#include <Arduino.h>

#include "buzzer_ctrl.h"
#include "config.h"
#include "dfplayer_ctrl.h"
#include "display_ctrl.h"
#include "pins.h"
#include "state.h"
#include "web_server.h"

namespace
{

  // Beep tiap detik selama sisa waktu di bawah ambang ini.
  constexpr uint32_t kBuzzLastSeconds = 3;

  // Detik yang terakhir dibunyiin, biar satu detik cuma dapat satu beep.
  uint32_t lastBeepSec = 0;

  void beepLastSeconds()
  {
    // Dibulatkan ke atas biar sinkron sama angka yang lagi kelihatan di panel.
    const uint32_t sec = (runtime.remainingMs + 999) / 1000;
    if (sec == lastBeepSec)
      return;

    lastBeepSec = sec;
    if (sec > 0 && sec <= kBuzzLastSeconds)
      buzzerPlay(BuzzPattern::TICK);
  }

} // namespace

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("SYSTEM BEGIN");
  loadConfig(appConfig);
  triggerInit();
  buzzerInit();
  displayInit();
  dfInit();
  webServerInit();

  dfPlayIdleLoop();
}

void loop()
{
  dfTick();
  buzzerTick();

  const bool trigger = triggerPressed();

  switch (runtime.state)
  {
  case TimerState::IDLE:
    if (trigger)
    {
      dfStopIdle();
      dfPlayVoice(DF_VOICE_START);
      buzzerPlay(BuzzPattern::START);
      timerStart(appConfig.countdownMs);
      // Reset di sini, bukan di beepLastSeconds(), biar durasi yang lebih pendek
      // dari ambang beep tetap kebunyiin dari detik pertama.
      lastBeepSec = 0;
    }
    break;

  case TimerState::RUNNING:
    if (trigger)
    {
      // Stop paksa: sisa waktu dibekuin biar kelihatan berhenti di angka berapa.
      displayFreezeFinal(runtime.remainingMs);
      timerStop();
      // Voice 003 dinonaktifkan: stop sebelum waktu habis sengaja nggak ada
      // voice, cuma buzzer. Hapus '//' di baris bawah kalau mau dipakai lagi.
      // dfPlayVoice(DF_VOICE_GAME_OVER);
      buzzerPlay(BuzzPattern::GAME_OVER);
      dfPlayIdleLoop();
    }
    else if (timerTick())
    {
      displayFreezeFinal(0);
      timerStop();
      dfPlayVoice(DF_VOICE_TIME_UP);
      buzzerPlay(BuzzPattern::TIME_UP);
      dfPlayIdleLoop();
    }
    else
    {
      beepLastSeconds();
    }
    break;
  }

  if (runtime.state == TimerState::RUNNING)
  {
    displayCountdown(runtime.remainingMs);
  }
  else
  {
    displayIdle();
  }
}
