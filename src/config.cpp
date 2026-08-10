#include "config.h"

#include <Preferences.h>

#include "pins.h"

AppConfig appConfig;

namespace {

constexpr char kPrefsNamespace[] = "p5cfg";

Preferences prefs;

// Kalau total margin lewat `limit`, kecilin dua-duanya proporsional biar rasio
// yang diminta user tetap kejaga.
bool clampMarginPair(uint8_t &a, uint8_t &b, uint16_t limit) {
  const uint16_t total = static_cast<uint16_t>(a) + static_cast<uint16_t>(b);
  if (total <= limit) return false;

  const uint16_t scaledA = static_cast<uint16_t>(a) * limit / total;
  a = static_cast<uint8_t>(scaledA);
  b = static_cast<uint8_t>(limit - scaledA);
  return true;
}

}  // namespace

bool clampConfig(AppConfig &cfg) {
  bool changed = false;

  if (cfg.countdownMs < COUNTDOWN_MIN_MS) {
    cfg.countdownMs = COUNTDOWN_MIN_MS;
    changed = true;
  } else if (cfg.countdownMs > COUNTDOWN_MAX_MS) {
    cfg.countdownMs = COUNTDOWN_MAX_MS;
    changed = true;
  }

  changed |= clampMarginPair(cfg.marginLeft, cfg.marginRight, PANEL_RES_X - PANEL_MIN_DRAW_W);
  changed |= clampMarginPair(cfg.marginTop, cfg.marginBottom, PANEL_RES_Y - PANEL_MIN_DRAW_H);

  if (cfg.dfVolume > DF_VOLUME_MAX) {
    cfg.dfVolume = DF_VOLUME_MAX;
    changed = true;
  }

  if (cfg.idleTrack < DF_TRACK_MIN) {
    cfg.idleTrack = DF_TRACK_MIN;
    changed = true;
  }

  if (cfg.brightness < BRIGHTNESS_MIN) {
    cfg.brightness = BRIGHTNESS_MIN;
    changed = true;
  }

  return changed;
}

void loadConfig(AppConfig &cfg) {
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);

  cfg.countdownMs = prefs.getULong("countdownMs", 300000UL);
  cfg.marginTop = prefs.getUChar("marginTop", 0);
  cfg.marginBottom = prefs.getUChar("marginBottom", 0);
  cfg.marginLeft = prefs.getUChar("marginLeft", 0);
  cfg.marginRight = prefs.getUChar("marginRight", 0);
  cfg.dfVolume = prefs.getUChar("dfVolume", 20);
  cfg.idleTrack = prefs.getUChar("idleTrack", 1);
  cfg.idleMusicEnabled = prefs.getBool("idleMusic", true);
  cfg.brightness = prefs.getUChar("brightness", 150);

  prefs.end();

  // NVS bisa nyimpen nilai dari versi firmware lama, tetap divalidasi.
  clampConfig(cfg);
}

void saveConfig(const AppConfig &cfg) {
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);

  prefs.putULong("countdownMs", cfg.countdownMs);
  prefs.putUChar("marginTop", cfg.marginTop);
  prefs.putUChar("marginBottom", cfg.marginBottom);
  prefs.putUChar("marginLeft", cfg.marginLeft);
  prefs.putUChar("marginRight", cfg.marginRight);
  prefs.putUChar("dfVolume", cfg.dfVolume);
  prefs.putUChar("idleTrack", cfg.idleTrack);
  prefs.putBool("idleMusic", cfg.idleMusicEnabled);
  prefs.putUChar("brightness", cfg.brightness);

  prefs.end();
}
