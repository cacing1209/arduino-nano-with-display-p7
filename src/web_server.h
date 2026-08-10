#pragma once

// Halaman settings lokal di SoftAP, http://192.168.4.1
//   GET  /        -> index.html dari LittleFS
//   GET  /config  -> AppConfig sebagai JSON
//   POST /config  -> update field yang dikirim, clamp, simpan, balikin nilai
//                    efektif + flag "clamped"

void webServerInit();
