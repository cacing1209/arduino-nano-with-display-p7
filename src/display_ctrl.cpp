#include "display_ctrl.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <cstring>

#include "config.h"
#include "pins.h"

namespace {

// Font bawaan Adafruit_GFX (classic 5x7). Advance sudah termasuk spasi `size`
// px antar karakter, jadi lebar yang kelihatan = chars*6*size - size.
constexpr int16_t kCharAdvance = 6;
constexpr int16_t kGlyphHeight = 7;

constexpr int16_t textVisWidth(uint8_t chars, uint8_t size) {
  return static_cast<int16_t>(chars) * kCharAdvance * size - size;
}
constexpr int16_t textHeight(uint8_t size) { return kGlyphHeight * size; }

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Warna config disimpan 0xRRGGBB biar gampang bolak-balik ke <input type=color>.
constexpr uint16_t rgb565From(uint32_t rgb) {
  return rgb565(static_cast<uint8_t>(rgb >> 16), static_cast<uint8_t>(rgb >> 8),
                static_cast<uint8_t>(rgb));
}

constexpr uint16_t kColorMsAccent = rgb565(150, 150, 150);  // milidetik, sengaja diredam

constexpr uint32_t kUrgentThresholdMs = 10000;

// Font classic cuma punya stroke 1 px dan nggak ada varian bold-nya. "Tebal"
// dipalsuin dengan nge-print teks yang sama beberapa kali digeser 1 px, jadi
// nggak perlu nambah font ke flash. Geseran tetap 1 px di semua text size:
// yang dicari efek stroke lebih gemuk, bukan skala huruf.
struct BoldOffset {
  int16_t dx, dy;
};
constexpr BoldOffset kBoldOffsets[] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

constexpr uint8_t boldPasses(uint8_t thickness) {
  return thickness >= 3 ? 4 : (thickness >= 2 ? 2 : 1);
}
// Smear-nya nambahin footprint teks, jadi ukur & posisi teks ikut nambah.
constexpr int16_t boldExtraW(uint8_t thickness) { return thickness >= 2 ? 1 : 0; }
constexpr int16_t boldExtraH(uint8_t thickness) { return thickness >= 3 ? 1 : 0; }

// ~50 fps. Lebih cepat cuma bikin digit milidetik blur dan makan CPU.
constexpr unsigned long kRenderIntervalMs = 20;

// Kotak area gambar setelah margin diterapkan.
struct Box {
  int16_t x, y, w, h;
};

// Bikin margin jadi clipping beneran: gambar Adafruit_GFX (termasuk teks)
// nggak bisa tembus keluar kotak.
class ClippedMatrix : public MatrixPanel_I2S_DMA {
 public:
  using MatrixPanel_I2S_DMA::MatrixPanel_I2S_DMA;
  // Biar overload RGB888 dari base nggak ketutup sama override di bawah.
  using MatrixPanel_I2S_DMA::drawPixel;
  using MatrixPanel_I2S_DMA::fillRect;

  void setClipBox(const Box &b) {
    clipX0_ = b.x;
    clipY0_ = b.y;
    clipX1_ = b.x + b.w - 1;
    clipY1_ = b.y + b.h - 1;
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < clipX0_ || x > clipX1_ || y < clipY0_ || y > clipY1_) return;
    MatrixPanel_I2S_DMA::drawPixel(x, y, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    int16_t x0 = x > clipX0_ ? x : clipX0_;
    int16_t y0 = y > clipY0_ ? y : clipY0_;
    int16_t x1 = (x + w - 1) < clipX1_ ? (x + w - 1) : clipX1_;
    int16_t y1 = (y + h - 1) < clipY1_ ? (y + h - 1) : clipY1_;
    if (x1 < x0 || y1 < y0) return;
    MatrixPanel_I2S_DMA::fillRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, color);
  }

 private:
  int16_t clipX0_ = 0;
  int16_t clipY0_ = 0;
  int16_t clipX1_ = PANEL_RES_X - 1;
  int16_t clipY1_ = PANEL_RES_Y - 1;
};

ClippedMatrix *matrix = nullptr;

// Cache layar idle, dipakai buat nentuin perlu redraw atau nggak.
struct IdleSnapshot {
  bool valid = false;
  uint32_t countdownMs = 0;
  uint8_t marginTop = 0, marginBottom = 0, marginLeft = 0, marginRight = 0;
  uint32_t colorIdle = 0;
  uint8_t textThickness = 0;
  bool bgEnabled = false;
  uint32_t colorBg = 0;
};
IdleSnapshot lastIdle;

unsigned long lastRenderMs = 0;

// Selama masih aktif, displayIdle() nggak boleh nimpa frame terakhir ronde.
bool finalHoldActive = false;
unsigned long finalHoldUntilMs = 0;

Box currentBox() {
  Box b;
  b.x = appConfig.marginLeft;
  b.y = appConfig.marginTop;
  b.w = static_cast<int16_t>(PANEL_RES_X) - appConfig.marginLeft - appConfig.marginRight;
  b.h = static_cast<int16_t>(PANEL_RES_Y) - appConfig.marginTop - appConfig.marginBottom;
  return b;
}

// Ukuran teks apa adanya sesuai ketebalan yang lagi kepakai. Semua perhitungan
// layout lewat sini biar teks tebal nggak meleset dari tengah / kepotong clip.
int16_t textW(size_t chars, uint8_t size) {
  return textVisWidth(static_cast<uint8_t>(chars), size) + boldExtraW(appConfig.textThickness);
}
int16_t textH(uint8_t size) { return textHeight(size) + boldExtraH(appConfig.textThickness); }

void drawText(int16_t x, int16_t y, const char *text, uint8_t size, uint16_t color) {
  matrix->setTextSize(size);
  matrix->setTextColor(color);

  const uint8_t passes = boldPasses(appConfig.textThickness);
  for (uint8_t i = 0; i < passes; ++i) {
    // print() majuin cursor, jadi tiap pass harus di-set ulang.
    matrix->setCursor(x + kBoldOffsets[i].dx, y + kBoldOffsets[i].dy);
    matrix->print(text);
  }
}

void drawOneLine(const Box &b, const char *text, uint8_t size, uint16_t color) {
  drawText(b.x + (b.w - textW(strlen(text), size)) / 2, b.y + (b.h - textH(size)) / 2, text, size,
           color);
}

void drawTwoLine(const Box &b, const char *top, uint8_t topSize, uint16_t topColor,
                 const char *bottom, uint8_t bottomSize, uint16_t bottomColor, int16_t gap) {
  const int16_t topH = textH(topSize);
  const int16_t totalH = topH + gap + textH(bottomSize);
  const int16_t y0 = b.y + (b.h - totalH) / 2;

  drawText(b.x + (b.w - textW(strlen(top), topSize)) / 2, y0, top, topSize, topColor);
  drawText(b.x + (b.w - textW(strlen(bottom), bottomSize)) / 2, y0 + topH + gap, bottom, bottomSize,
           bottomColor);
}

// Pilih layout terbesar yang masih muat di kotak, dari MM:SS gede 2 baris
// sampai MM:SS aja. Kalau semua kesempitan, opsi terakhir kepotong clip box.
void drawCountdownLayout(const Box &b, const char *mmss, const char *msPart, const char *full,
                         uint16_t color) {
  if (b.w >= textW(5, 2) && b.h >= textH(2) + 3 + textH(1)) {
    drawTwoLine(b, mmss, 2, color, msPart, 1, kColorMsAccent, 3);
  } else if (b.w >= textW(8, 1) && b.h >= textH(1)) {  // "MM:SS:cc" = 8 karakter
    drawOneLine(b, full, 1, color);
  } else if (b.w >= textW(5, 1) && b.h >= textH(1) * 2 + 1) {
    drawTwoLine(b, mmss, 1, color, msPart, 1, kColorMsAccent, 1);
  } else {
    drawOneLine(b, mmss, 1, color);
  }
}

uint16_t runningColor(uint32_t remainingMs) {
  return rgb565From(remainingMs <= kUrgentThresholdMs ? appConfig.colorUrgent : appConfig.colorRun);
}

void formatTime(uint32_t ms, char *mmss, size_t mmssLen, char *msPart, size_t msPartLen,
                char *full, size_t fullLen) {
  const uint32_t totalSec = ms / 1000;
  const unsigned long mm = totalSec / 60;
  const unsigned long ss = totalSec % 60;
  // Milidetik dipotong (bukan dibulatin) ke 2 digit: dibulatin bisa nongol "100"
  // dan bikin angka kelihatan nambah pas sisa waktu justru berkurang.
  const unsigned long cs = (ms % 1000) / 10;

  snprintf(mmss, mmssLen, "%02lu:%02lu", mm, ss);
  snprintf(msPart, msPartLen, "%02lu", cs);
  snprintf(full, fullLen, "%02lu:%02lu:%02lu", mm, ss, cs);
}

// Background cuma ngisi area gambar, bukan sepanjang panel: bagian margin tetap
// hitam supaya tetap kelihatan sebagai bezel, bukan bingkai warna.
void drawBackground(const Box &b) {
  if (!appConfig.bgEnabled) return;
  matrix->fillRect(b.x, b.y, b.w, b.h, rgb565From(appConfig.colorBg));
}

// bothBuffers=true buat frame diam (idle / freeze): kalau cuma satu buffer yang
// keisi, layar balik ke frame lama begitu ada flip berikutnya.
void renderFrame(uint32_t ms, uint16_t color, bool bothBuffers) {
  char mmss[8], msPart[4], full[16];
  formatTime(ms, mmss, sizeof(mmss), msPart, sizeof(msPart), full, sizeof(full));

  const Box box = currentBox();
  matrix->setClipBox(box);

  matrix->clearScreen();
  drawBackground(box);
  drawCountdownLayout(box, mmss, msPart, full, color);
  matrix->flipDMABuffer();

  if (bothBuffers) {
    matrix->clearScreen();
    drawBackground(box);
    drawCountdownLayout(box, mmss, msPart, full, color);
  }
}

}  // namespace

void displayInit() {
  HUB75_I2S_CFG::i2s_pins pins = {PIN_HUB75_R1, PIN_HUB75_G1, PIN_HUB75_B1, PIN_HUB75_R2,
                                  PIN_HUB75_G2, PIN_HUB75_B2, PIN_HUB75_A,  PIN_HUB75_B,
                                  PIN_HUB75_C,  PIN_HUB75_D,  PIN_HUB75_E,  PIN_HUB75_LAT,
                                  PIN_HUB75_OE, PIN_HUB75_CLK};

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, pins);
  mxconfig.driver = HUB75_I2S_CFG::FM6126A;
  mxconfig.double_buff = true;  // countdown redraw penuh tiap frame, tanpa ini kedip

  matrix = new ClippedMatrix(mxconfig);
  matrix->begin();
  matrix->setBrightness8(appConfig.brightness);
  matrix->setTextWrap(false);

  // Buffer belakang isinya masih sampah setelah alokasi.
  matrix->clearScreen();
  matrix->flipDMABuffer();
  matrix->clearScreen();
}

void displayIdle() {
  if (matrix == nullptr) return;

  if (finalHoldActive) {
    if (static_cast<long>(millis() - finalHoldUntilMs) < 0) return;
    finalHoldActive = false;
  }

  if (lastIdle.valid && lastIdle.countdownMs == appConfig.countdownMs &&
      lastIdle.marginTop == appConfig.marginTop && lastIdle.marginBottom == appConfig.marginBottom &&
      lastIdle.marginLeft == appConfig.marginLeft && lastIdle.marginRight == appConfig.marginRight &&
      lastIdle.colorIdle == appConfig.colorIdle &&
      lastIdle.textThickness == appConfig.textThickness &&
      lastIdle.bgEnabled == appConfig.bgEnabled && lastIdle.colorBg == appConfig.colorBg) {
    return;
  }

  renderFrame(appConfig.countdownMs, rgb565From(appConfig.colorIdle), /*bothBuffers=*/true);

  lastIdle.valid = true;
  lastIdle.countdownMs = appConfig.countdownMs;
  lastIdle.marginTop = appConfig.marginTop;
  lastIdle.marginBottom = appConfig.marginBottom;
  lastIdle.marginLeft = appConfig.marginLeft;
  lastIdle.marginRight = appConfig.marginRight;
  lastIdle.colorIdle = appConfig.colorIdle;
  lastIdle.textThickness = appConfig.textThickness;
  lastIdle.bgEnabled = appConfig.bgEnabled;
  lastIdle.colorBg = appConfig.colorBg;
}

void displayCountdown(uint32_t remainingMs) {
  if (matrix == nullptr) return;
  if (millis() - lastRenderMs < kRenderIntervalMs) return;
  lastRenderMs = millis();

  lastIdle.valid = false;   // paksa idle digambar ulang pas balik ke IDLE
  finalHoldActive = false;  // ronde baru jalan, sisa hold ronde lalu dibatalin

  renderFrame(remainingMs, runningColor(remainingMs), /*bothBuffers=*/false);
}

void displayFreezeFinal(uint32_t remainingMs) {
  if (matrix == nullptr) return;

  lastIdle.valid = false;
  renderFrame(remainingMs, runningColor(remainingMs), /*bothBuffers=*/true);

  finalHoldActive = true;
  finalHoldUntilMs = millis() + DISPLAY_FINAL_HOLD_MS;
}

void displayApplyBrightness(uint8_t brightness) {
  if (matrix == nullptr) return;
  matrix->setBrightness8(brightness);
}
