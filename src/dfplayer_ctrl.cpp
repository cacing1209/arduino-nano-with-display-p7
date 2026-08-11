#include "dfplayer_ctrl.h"

#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

#include "config.h"
#include "debug.h"
#include "pins.h"

namespace
{

  // Bukan playAdvertisement() (butuh lagu lagi jalan) atau play() index global
  // (nurut urutan fisik file di kartu).
  constexpr uint8_t kVoiceFolder = 1;

  // Cadangan kalau notifikasi "play finished" nggak pernah datang, misal RX
  // nggak disambung.
  constexpr unsigned long kVoiceGuardMs = 6000;

  enum class DfNowPlaying : uint8_t
  {
    NONE,
    IDLE_MUSIC,
    VOICE
  };

  HardwareSerial dfSerial(2);
  DFRobotDFPlayerMini dfPlayer;

  bool dfReady = false;
  bool idleWanted = false;
  DfNowPlaying nowPlaying = DfNowPlaying::NONE;
  unsigned long voiceGuardUntilMs = 0;
  uint8_t playingIdleTrack = 0;

  void startIdleMusic()
  {
    playingIdleTrack = appConfig.idleTrack;
    dfPlayer.playMp3Folder(playingIdleTrack);
    // Firmware clone kadang nggak dukung, cadangannya restart di dfTick().
    dfPlayer.enableLoop();
    nowPlaying = DfNowPlaying::IDLE_MUSIC;
  }

  void stopIdleMusic()
  {
    dfPlayer.disableLoop();
    dfPlayer.stop();
    nowPlaying = DfNowPlaying::NONE;
  }

} // namespace

void dfInit()
{
  dfSerial.begin(9600, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);

  // isACK=false: ACK bikin tiap command nunggu balasan sampai ratusan ms.
  dfReady = dfPlayer.begin(dfSerial, /*isACK=*/false, /*doReset=*/true);
  if (!dfReady)
  {
    DBG_PRINTLN(F("[df] modul tidak merespon, audio dilewati"));
    return;
  }

  dfApplyVolume(appConfig.dfVolume);
}

void dfApplyVolume(uint8_t vol)
{
  if (!dfReady)
    return;
  if (vol > DF_VOLUME_MAX)
    vol = DF_VOLUME_MAX;
  dfPlayer.volume(vol);
}

void dfPlayIdleLoop()
{
  idleWanted = true;
}

void dfStopIdle()
{
  idleWanted = false;
  if (!dfReady)
    return;

  if (nowPlaying == DfNowPlaying::IDLE_MUSIC)
    stopIdleMusic();
}

void dfPlayVoice(uint8_t advertTrack)
{
  if (!dfReady)
    return;

  dfPlayer.disableLoop();
  dfPlayer.playFolder(kVoiceFolder, advertTrack);
  nowPlaying = DfNowPlaying::VOICE;
  voiceGuardUntilMs = millis() + kVoiceGuardMs;
}

void dfTick()
{
  if (!dfReady)
    return;

  while (dfPlayer.available())
  {
    const uint8_t type = dfPlayer.readType();
    const uint16_t value = dfPlayer.read();

    if (type == DFPlayerPlayFinished)
    {
      nowPlaying = DfNowPlaying::NONE;
    }
    else if (type == DFPlayerError)
    {
      DBG_PRINTF("[df] error %u\n", value);
    }
  }

  if (nowPlaying == DfNowPlaying::VOICE &&
      static_cast<long>(millis() - voiceGuardUntilMs) >= 0)
  {
    nowPlaying = DfNowPlaying::NONE;
  }

  // Setting berubah dari web pas musik jalan: matiin dulu, blok di bawah yang
  // nyalain ulang kalau masih perlu.
  if (nowPlaying == DfNowPlaying::IDLE_MUSIC &&
      (!appConfig.idleMusicEnabled || playingIdleTrack != appConfig.idleTrack))
  {
    stopIdleMusic();
  }

  if (idleWanted && appConfig.idleMusicEnabled && nowPlaying == DfNowPlaying::NONE)
  {
    startIdleMusic();
  }
}
