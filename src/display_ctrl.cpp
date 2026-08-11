#include "display_ctrl.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include <cstring>

#include "config.h"
#include "pins.h"

namespace {

// Font classic 5x7 Adafruit_GFX. Advance sudah termasuk spasi `size` px antar
// karakter, jadi lebar yang kelihatan = chars*6*size - size.
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

constexpr uint32_t kUrgentThresholdMs = 10000;

// Cuma buat jalur fallback font classic (lihat drawCountdownLayout): font-nya
// nggak punya varian bold, jadi "tebal" dipalsuin dengan nge-print teks yang
// sama beberapa kali digeser 1 px.
struct BoldOffset {
  int16_t dx, dy;
};
constexpr BoldOffset kBoldOffsets[] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}};

constexpr uint8_t boldPasses(uint8_t thickness) {
  return thickness >= 3 ? 4 : (thickness >= 2 ? 2 : 1);
}
// Smear nambahin footprint teks, jadi ukuran & posisi ikut nambah.
constexpr int16_t boldExtraW(uint8_t thickness) { return thickness >= 2 ? 1 : 0; }
constexpr int16_t boldExtraH(uint8_t thickness) { return thickness >= 3 ? 1 : 0; }

// ~20 fps. Yang berubah cuma digit detik, jadi lebih cepat cuma nambah beban
// CPU; segini masih cukup rapat biar digit ganti bareng beep buzzer.
constexpr unsigned long kRenderIntervalMs = 50;

// Kotak area gambar setelah margin diterapkan.
struct Box {
  int16_t x, y, w, h;
};

// Bikin margin jadi clipping beneran: gambar Adafruit_GFX (termasuk teks)
// nggak bisa tembus keluar kotak.
class ClippedMatrix : public MatrixPanel_I2S_DMA {
 public:
  using MatrixPanel_I2S_DMA::MatrixPanel_I2S_DMA;
  // Biar overload RGB888 dari base nggak ketutup override di bawah.
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

// Cache layar idle, buat nentuin perlu redraw atau nggak.
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
  b.x = PANEL_X_OFFSET + appConfig.marginLeft;
  b.y = appConfig.marginTop;
  b.w = static_cast<int16_t>(PANEL_RES_X) - PANEL_X_OFFSET - appConfig.marginLeft -
        appConfig.marginRight;
  b.h = static_cast<int16_t>(PANEL_RES_Y) - appConfig.marginTop - appConfig.marginBottom;
  return b;
}

// Semua hitungan layout lewat sini biar teks tebal nggak meleset dari tengah
// atau kepotong clip.
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

// --- Angka 7-segment ---
// Font bitmap nggak kepakai di sini: ditinggiin sampai 32 px, "MM:SS" jadi 87
// px padahal panelnya cuma 64. Digit dari rect bisa disetel lebar dan tingginya
// lepas satu sama lain (13x32 px per angka), dan "ketebalan" di web jadi tebal
// segmen beneran.

// bit0..bit6 = segmen A..G. A=atas, B=kanan atas, C=kanan bawah, D=bawah,
// E=kiri bawah, F=kiri atas, G=tengah.
constexpr uint8_t kSegDigits[10] = {
    0b0111111,  // 0
    0b0000110,  // 1
    0b1011011,  // 2
    0b1001111,  // 3
    0b1100110,  // 4
    0b1101101,  // 5
    0b1111101,  // 6
    0b0000111,  // 7
    0b1111111,  // 8
    0b1101111,  // 9
};

constexpr int16_t kSegMinThickness = 2;
constexpr int16_t kSegGapPreferred = 2;  // celah antar glyph
constexpr int16_t kSegColonMinW = 2;

struct SegLayout {
  int16_t dw;      // lebar satu angka
  int16_t dh;      // tinggi satu angka
  int16_t t;       // tebal segmen
  int16_t colonW;  // lebar titik dua
  int16_t gap;     // celah antar glyph
  int16_t totalW;  // lebar "MM:SS" utuh, buat nengahin
};

// Sengaja lebih sempit dari tebal segmen: tiap px yang nggak kepakai titik dua
// langsung nambah lebar keempat angkanya.
constexpr int16_t segColonW(int16_t t) { return t > 4 ? t - 2 : kSegColonMinW; }

// "MM:SS" = 4 angka + 1 titik dua + 4 celah.
bool segFits(const Box &b, int16_t t, int16_t gap, int16_t colonW, SegLayout &out) {
  const int16_t dw = static_cast<int16_t>((b.w - colonW - 4 * gap) / 4);
  // Celah di tengah angka minimal 2 px, kalau nggak "0" kelihatan kayak balok isi.
  if (dw < 2 * t + 2) return false;
  // 3 batang horizontal + celah minimal 2 px di antaranya.
  if (b.h < 3 * t + 4) return false;

  out.dw = dw;
  out.dh = b.h;
  out.t = t;
  out.colonW = colonW;
  out.gap = gap;
  out.totalW = static_cast<int16_t>(4 * dw + colonW + 4 * gap);
  return true;
}

// Kalau nggak muat, yang dikorbanin berurutan: celah antar angka, lebar titik
// dua, baru tebal segmen — tebal pilihan user dipertahanin selama masih bisa.
// false = kotaknya kekecilan buat 7-segment.
bool segLayout(const Box &b, uint8_t thickness, SegLayout &out) {
  const int16_t want = static_cast<int16_t>(b.h / 7 + thickness - 1);
  for (int16_t t = want; t >= kSegMinThickness; --t) {
    const int16_t colonW = segColonW(t);
    if (segFits(b, t, kSegGapPreferred, colonW, out)) return true;
    if (segFits(b, t, 1, colonW, out)) return true;
    if (segFits(b, t, 1, kSegColonMinW, out)) return true;
  }
  return false;
}

void drawSegDigit(int16_t x, int16_t y, const SegLayout &L, uint8_t digit, uint16_t color) {
  const uint8_t mask = kSegDigits[digit];
  const int16_t yMid = y + (L.dh - L.t) / 2;
  // Vertikal atas ditarik sampai nutup batang tengah, biar sudutnya nyambung.
  const int16_t upperH = yMid + L.t - y;
  const int16_t lowerH = y + L.dh - yMid;
  const int16_t xRight = x + L.dw - L.t;

  if (mask & 0x01) matrix->fillRect(x, y, L.dw, L.t, color);               // A
  if (mask & 0x02) matrix->fillRect(xRight, y, L.t, upperH, color);        // B
  if (mask & 0x04) matrix->fillRect(xRight, yMid, L.t, lowerH, color);     // C
  if (mask & 0x08) matrix->fillRect(x, y + L.dh - L.t, L.dw, L.t, color);  // D
  if (mask & 0x10) matrix->fillRect(x, yMid, L.t, lowerH, color);          // E
  if (mask & 0x20) matrix->fillRect(x, y, L.t, upperH, color);             // F
  if (mask & 0x40) matrix->fillRect(x, yMid, L.dw, L.t, color);            // G
}

// Dua kotak di 1/4 dan 3/4 tinggi angka, sejajar celah antar batang.
void drawSegColon(int16_t x, int16_t y, const SegLayout &L, uint16_t color) {
  matrix->fillRect(x, y + L.dh / 4 - L.colonW / 2, L.colonW, L.colonW, color);
  matrix->fillRect(x, y + (3 * L.dh) / 4 - L.colonW / 2, L.colonW, L.colonW, color);
}

void drawSegTime(const Box &b, const SegLayout &L, const char *text, uint16_t color) {
  int16_t x = b.x + (b.w - L.totalW) / 2;
  const int16_t y = b.y + (b.h - L.dh) / 2;

  for (const char *p = text; *p != '\0'; ++p) {
    if (*p == ':') {
      drawSegColon(x, y, L, color);
      x += L.colonW + L.gap;
    } else if (*p >= '0' && *p <= '9') {
      drawSegDigit(x, y, L, static_cast<uint8_t>(*p - '0'), color);
      x += L.dw + L.gap;
    }
  }
}

void drawCountdownLayout(const Box &b, const char *mmss, uint16_t color) {
  SegLayout seg;
  if (segLayout(b, appConfig.textThickness, seg)) {
    drawSegTime(b, seg, mmss, color);
    return;
  }

  // Margin ekstrem, 7-segment nggak kebentuk lagi. Kalau font classic pun
  // kepotong clip box, biarin — masih lebih baik daripada layar kosong.
  const uint8_t size = (b.w >= textW(5, 2) && b.h >= textH(2)) ? 2 : 1;
  drawOneLine(b, mmss, size, color);
}

uint16_t runningColor(uint32_t remainingMs) {
  return rgb565From(remainingMs <= kUrgentThresholdMs ? appConfig.colorUrgent : appConfig.colorRun);
}

// Detik dibulatkan ke ATAS: kalau dipotong ke bawah, angka awal (misal 05:00)
// cuma nongol sekejap dan 00:00 nongol sedetik penuh sebelum waktunya habis.
void formatTime(uint32_t ms, char *mmss, size_t mmssLen) {
  const uint32_t totalSec = (ms + 999) / 1000;
  const unsigned long mm = totalSec / 60;
  const unsigned long ss = totalSec % 60;

  snprintf(mmss, mmssLen, "%02lu:%02lu", mm, ss);
}

// Cuma ngisi area gambar: margin tetap hitam biar kelihatan sebagai bezel,
// bukan bingkai warna.
void drawBackground(const Box &b) {
  if (!appConfig.bgEnabled) return;
  matrix->fillRect(b.x, b.y, b.w, b.h, rgb565From(appConfig.colorBg));
}

// bothBuffers=true buat frame diam (idle / freeze): kalau cuma satu buffer
// keisi, layar balik ke frame lama begitu ada flip berikutnya.
void renderFrame(uint32_t ms, uint16_t color, bool bothBuffers) {
  char mmss[8];
  formatTime(ms, mmss, sizeof(mmss));

  const Box box = currentBox();
  matrix->setClipBox(box);

  matrix->clearScreen();
  drawBackground(box);
  drawCountdownLayout(box, mmss, color);
  matrix->flipDMABuffer();

  if (bothBuffers) {
    matrix->clearScreen();
    drawBackground(box);
    drawCountdownLayout(box, mmss, color);
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
  mxconfig.double_buff = true;  // redraw penuh tiap frame, tanpa ini kedip

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
  finalHoldActive = false;  // ronde baru jalan, hold ronde lalu dibatalin

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
