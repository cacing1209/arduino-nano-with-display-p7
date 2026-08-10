// ESP32 HUB75 P5 32x64 countdown timer + DFPlayer + web config.
// File ini cuma orkestrasi antar modul + transisi state machine.

#include <Arduino.h>

#include "config.h"
#include "dfplayer_ctrl.h"
#include "display_ctrl.h"
#include "pins.h"
#include "state.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);

  loadConfig(appConfig);
  triggerInit();
  displayInit();
  dfInit();
  webServerInit();

  dfPlayIdleLoop();
}

void loop() {
  dfTick();

  const bool trigger = triggerPressed();

  switch (runtime.state) {
    case TimerState::IDLE:
      if (trigger) {
        dfStopIdle();
        dfPlayVoice(DF_VOICE_START);
        timerStart(appConfig.countdownMs);
      }
      break;

    case TimerState::RUNNING:
      if (trigger) {
        // Stop paksa: sisa waktu dibekuin biar kelihatan berhenti di angka berapa.
        displayFreezeFinal(runtime.remainingMs);
        timerStop();
        dfPlayVoice(DF_VOICE_GAME_OVER);
        dfPlayIdleLoop();
      } else if (timerTick()) {
        displayFreezeFinal(0);
        timerStop();
        dfPlayVoice(DF_VOICE_TIME_UP);
        dfPlayIdleLoop();
      }
      break;
  }

  if (runtime.state == TimerState::RUNNING) {
    displayCountdown(runtime.remainingMs);
  } else {
    displayIdle();
  }
}
