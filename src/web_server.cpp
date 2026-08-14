#include "web_server.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "config.h"
#include "debug.h"
#include "dfplayer_ctrl.h"
#include "display_ctrl.h"
#include "pins.h"

namespace {

AsyncWebServer server(80);
bool fsMounted = false;

// Potong ke jangkauan tipe; batas "masuk akal"-nya diurus clampConfig().
bool takeNumber(JsonVariantConst src, const char *key, double min, double max, double &out) {
  JsonVariantConst v = src[key];
  if (v.isNull() || !v.is<double>()) return false;

  double raw = v.as<double>();
  if (raw < min) raw = min;
  if (raw > max) raw = max;
  out = raw;
  return true;
}

bool takeU32(JsonVariantConst src, const char *key, uint32_t &dest) {
  double raw = 0;
  if (!takeNumber(src, key, 0, 4294967295.0, raw)) return false;
  dest = static_cast<uint32_t>(raw);
  return true;
}

bool takeU8(JsonVariantConst src, const char *key, uint8_t &dest) {
  double raw = 0;
  if (!takeNumber(src, key, 0, 255, raw)) return false;
  dest = static_cast<uint8_t>(raw);
  return true;
}

bool takeBool(JsonVariantConst src, const char *key, bool &dest) {
  JsonVariantConst v = src[key];
  if (v.isNull() || !v.is<bool>()) return false;
  dest = v.as<bool>();
  return true;
}

void fillConfigJson(JsonObject obj, const AppConfig &cfg) {
  obj["countdownMs"] = cfg.countdownMs;
  obj["marginTop"] = cfg.marginTop;
  obj["marginBottom"] = cfg.marginBottom;
  obj["marginLeft"] = cfg.marginLeft;
  obj["marginRight"] = cfg.marginRight;
  obj["dfVolume"] = cfg.dfVolume;
  obj["idleTrack"] = cfg.idleTrack;
  obj["idleMusicEnabled"] = cfg.idleMusicEnabled;
  obj["brightness"] = cfg.brightness;
  obj["colorIdle"] = cfg.colorIdle;
  obj["colorRun"] = cfg.colorRun;
  obj["colorUrgent"] = cfg.colorUrgent;
  obj["textThickness"] = cfg.textThickness;
  obj["bgEnabled"] = cfg.bgEnabled;
  obj["colorBg"] = cfg.colorBg;

  // Batas-batas ikut dikirim biar UI nggak perlu hardcode. Batas margin sudah
  // dipotong offset perataan panel, jadi UI nggak perlu tahu soal itu.
  obj["marginMaxX"] = MARGIN_MAX_X;
  obj["marginMaxY"] = MARGIN_MAX_Y;
  obj["countdownMinMs"] = COUNTDOWN_MIN_MS;
  obj["countdownMaxMs"] = COUNTDOWN_MAX_MS;
  obj["volumeMax"] = DF_VOLUME_MAX;
  obj["brightnessMin"] = BRIGHTNESS_MIN;
  obj["thicknessMin"] = TEXT_THICKNESS_MIN;
  obj["thicknessMax"] = TEXT_THICKNESS_MAX;
}

String configJson(const AppConfig &cfg, const bool *clamped) {
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  fillConfigJson(obj, cfg);
  if (clamped != nullptr) obj["clamped"] = *clamped;

  String out;
  serializeJson(doc, out);
  return out;
}

void handleConfigPost(AsyncWebServerRequest *request, JsonVariant &json) {
  if (!json.is<JsonObject>()) {
    request->send(400, "application/json", "{\"error\":\"body harus object JSON\"}");
    return;
  }

  // appConfig baru ditimpa setelah semuanya valid.
  AppConfig next = appConfig;
  JsonVariantConst src = json;

  takeU32(src, "countdownMs", next.countdownMs);
  takeU8(src, "marginTop", next.marginTop);
  takeU8(src, "marginBottom", next.marginBottom);
  takeU8(src, "marginLeft", next.marginLeft);
  takeU8(src, "marginRight", next.marginRight);
  takeU8(src, "dfVolume", next.dfVolume);
  takeU8(src, "idleTrack", next.idleTrack);
  takeBool(src, "idleMusicEnabled", next.idleMusicEnabled);
  takeU8(src, "brightness", next.brightness);
  takeU32(src, "colorIdle", next.colorIdle);
  takeU32(src, "colorRun", next.colorRun);
  takeU32(src, "colorUrgent", next.colorUrgent);
  takeU8(src, "textThickness", next.textThickness);
  takeBool(src, "bgEnabled", next.bgEnabled);
  takeU32(src, "colorBg", next.colorBg);

  const bool clamped = clampConfig(next);

  const bool volumeChanged = next.dfVolume != appConfig.dfVolume;
  const bool brightnessChanged = next.brightness != appConfig.brightness;

  // countdownMs baru efektif ronde berikutnya, nggak motong yang lagi jalan.
  // Margin, warna & ketebalan langsung kepakai karena renderer baca appConfig
  // tiap frame.
  appConfig = next;
  saveConfig(appConfig);

  if (volumeChanged) dfApplyVolume(appConfig.dfVolume);
  if (brightnessChanged) displayApplyBrightness(appConfig.brightness);

  request->send(200, "application/json", configJson(appConfig, &clamped));
}

}  // namespace

void webServerInit() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid(), AP_PASSWORD);
  DBG_PRINTF("[web] SoftAP \"%s\" di http://%s\n", apSsid(), WiFi.softAPIP().toString().c_str());

  fsMounted = LittleFS.begin(/*formatOnFail=*/true);
  if (!fsMounted) DBG_PRINTLN(F("[web] LittleFS gagal dimount"));

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", configJson(appConfig, nullptr));
  });

  AsyncCallbackJsonWebHandler *postConfig =
      new AsyncCallbackJsonWebHandler("/config", handleConfigPost);
  postConfig->setMethod(HTTP_POST);
  server.addHandler(postConfig);

  if (fsMounted) server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    // Firmware bisa jalan duluan tanpa isi LittleFS.
    if (request->url() == "/") {
      request->send(200, "text/html",
                    F("<h3>UI belum diupload</h3><p>Jalanin <code>pio run -t uploadfs</code> "
                      "buat ngirim folder <code>data/</code> ke LittleFS.</p>"));
      return;
    }
    request->send(404, "text/plain", "not found");
  });

  server.begin();
}
