// ESP32 HUB75 P5 32x64 countdown timer + DFPlayer + web config.
// File ini cuma orkestrasi modul + transisi state machine.

#include <Arduino.h>

#include "buzzer_ctrl.h"
#include "config.h"
#include "debug.h"
#include "dfplayer_ctrl.h"
#include "display_ctrl.h"
#include "pins.h"
#include "state.h"
#include "web_server.h"

namespace
{

  constexpr uint32_t kBuzzLastSeconds = 10;

  // Detik yang terakhir dibunyiin, biar satu detik cuma dapat satu beep.
  uint32_t lastBeepSec = 0;

  void beepLastSeconds()
  {
    // Dibulatin ke atas biar sama dengan angka yang lagi kelihatan di panel.
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
  DBG_BEGIN(115200);
  DBG_PRINTLN("SYSTEM BEGIN");
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
      dfPlayVoice(DF_VOICE_START);
      buzzerPlay(BuzzPattern::START);
      timerStart(appConfig.countdownMs);
      // Reset di sini biar durasi yang lebih pendek dari ambang beep tetap
      // kebunyiin dari detik pertama.
      lastBeepSec = 0;
    }
    break;

  case TimerState::RUNNING:
    if (trigger)
    {
      // Stop paksa: sisa waktu dibekuin biar kelihatan berhenti di angka berapa.
      displayFreezeFinal(runtime.remainingMs);
      timerStop();
      // Sengaja tanpa voice, cuma buzzer (lihat DF_VOICE_GAME_OVER).
      buzzerPlay(BuzzPattern::GAME_OVER);
      dfPlayIdleLoop();
    }
    else if (timerTick())
    {
      displayFreezeFinal(0);
      timerStop();
      // dfPlayVoice(DF_VOICE_TIME_UP);
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
