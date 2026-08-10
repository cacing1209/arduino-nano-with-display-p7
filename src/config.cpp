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

// Buang bit di luar 0xRRGGBB, lalu naikin warna yang terlalu gelap sampai kanal
// paling terang menyentuh COLOR_MIN_LEVEL. Rasio antar kanal (alias hue-nya)
// tetap kejaga, cuma dinaikin levelnya.
bool clampColor(uint32_t &color) {
  const uint32_t masked = color & COLOR_MASK;
  const uint8_t r = static_cast<uint8_t>(masked >> 16);
  const uint8_t g = static_cast<uint8_t>(masked >> 8);
  const uint8_t b = static_cast<uint8_t>(masked);
  const uint8_t peak = max(r, max(g, b));

  uint32_t fixed;
  if (peak == 0) {
    // Hitam total nggak punya rasio buat dipertahanin, jadi dijadiin abu-abu.
    fixed = (static_cast<uint32_t>(COLOR_MIN_LEVEL) << 16) |
            (static_cast<uint32_t>(COLOR_MIN_LEVEL) << 8) | COLOR_MIN_LEVEL;
  } else if (peak < COLOR_MIN_LEVEL) {
    fixed = (static_cast<uint32_t>(r * COLOR_MIN_LEVEL / peak) << 16) |
            (static_cast<uint32_t>(g * COLOR_MIN_LEVEL / peak) << 8) |
            (b * COLOR_MIN_LEVEL / peak);
  } else {
    fixed = masked;
  }

  if (fixed == color) return false;
  color = fixed;
  return true;
}

bool clampU8(uint8_t &value, uint8_t min, uint8_t max) {
  if (value < min) {
    value = min;
    return true;
  }
  if (value > max) {
    value = max;
    return true;
  }
  return false;
}

}  // namespace

bool clampConfig(AppConfig &cfg) {
  bool changed = false;

  // Layar cuma nampilin MM:SS, jadi pecahan detik dibuang: sisa 50 ms dari
  // setting versi lama bikin layar tunggu nampilin satu detik lebih banyak dari
  // yang di-set (detik dibulatkan ke atas pas digambar).
  if (cfg.countdownMs % 1000 != 0) {
    cfg.countdownMs -= cfg.countdownMs % 1000;
    changed = true;
  }

  if (cfg.countdownMs < COUNTDOWN_MIN_MS) {
    cfg.countdownMs = COUNTDOWN_MIN_MS;
    changed = true;
  } else if (cfg.countdownMs > COUNTDOWN_MAX_MS) {
    cfg.countdownMs = COUNTDOWN_MAX_MS;
    changed = true;
  }

  changed |= clampMarginPair(cfg.marginLeft, cfg.marginRight, MARGIN_MAX_X);
  changed |= clampMarginPair(cfg.marginTop, cfg.marginBottom, MARGIN_MAX_Y);

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

  changed |= clampColor(cfg.colorIdle);
  changed |= clampColor(cfg.colorRun);
  changed |= clampColor(cfg.colorUrgent);
  changed |= clampU8(cfg.textThickness, TEXT_THICKNESS_MIN, TEXT_THICKNESS_MAX);

  // Background cuma dipotong ke 24 bit, tanpa lantai: hitam nilai yang sah.
  if (cfg.colorBg != (cfg.colorBg & COLOR_MASK)) {
    cfg.colorBg &= COLOR_MASK;
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
  cfg.colorIdle = prefs.getULong("colorIdle", COLOR_IDLE_DEFAULT);
  cfg.colorRun = prefs.getULong("colorRun", COLOR_RUN_DEFAULT);
  cfg.colorUrgent = prefs.getULong("colorUrgent", COLOR_URGENT_DEFAULT);
  cfg.textThickness = prefs.getUChar("thickness", TEXT_THICKNESS_MIN);
  cfg.bgEnabled = prefs.getBool("bgEnabled", false);
  cfg.colorBg = prefs.getULong("colorBg", COLOR_BG_DEFAULT);

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
  prefs.putULong("colorIdle", cfg.colorIdle);
  prefs.putULong("colorRun", cfg.colorRun);
  prefs.putULong("colorUrgent", cfg.colorUrgent);
  prefs.putUChar("thickness", cfg.textThickness);
  prefs.putBool("bgEnabled", cfg.bgEnabled);
  prefs.putULong("colorBg", cfg.colorBg);

  prefs.end();
}
