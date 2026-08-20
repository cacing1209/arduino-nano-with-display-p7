# Prompt: bikin manual penggunaan P5 Countdown Timer (output PDF)

Copy semua isi blok di bawah, paste ke Claude / ChatGPT / Claude Code.
Bagian `FAKTA ALAT` udah diisi dari kode, jangan diubah kecuali firmware berubah.

---

## PROMPT (copy dari sini)

Kamu technical writer yang biasa nulis buku manual alat elektronik untuk operator
lapangan. Tugas kamu: bikin **Buku Panduan Penggunaan** untuk alat "P5 Countdown
Timer", output akhir berupa **file PDF ukuran A4**.

### Pembaca
Operator acara / panitia lomba yang **bukan orang teknik**. Mereka cuma mau tahu
cara nyalain, cara setel waktu, cara mulai, dan apa yang harus dilakukan kalau
alatnya rewel. Hindari istilah teknis; kalau terpaksa dipakai, jelaskan sekali
di dalam kurung dengan bahasa awam.

### Bahasa & gaya
- Bahasa Indonesia, formal tapi santai, kalimat pendek.
- Semua instruksi pakai **langkah bernomor**, satu langkah satu tindakan.
- Sebut nama tombol/menu persis seperti yang tertulis di alat dan di halaman web.
- Jangan pakai kata "user", "device", "config" — pakai "pengguna", "alat", "setelan".

### Struktur wajib
1. **Sampul** — nama alat, subjudul "Panduan Penggunaan", versi dokumen, tanggal.
2. **Daftar isi** dengan nomor halaman.
3. **Kenali alatnya** — bagian-bagian alat (panel angka, tombol, speaker, colokan
   listrik) + tabel isi paket. Sisipkan placeholder gambar (lihat aturan gambar).
4. **Persiapan pertama kali** — cara pasang, urutan nyalain, apa yang muncul di
   layar kalau normal.
5. **Cara pakai harian** — alur satu ronde dari awal sampai selesai. Ini bab
   paling penting, tulis paling detail.
6. **Mengubah setelan lewat HP/laptop** — sambung Wi-Fi, buka halaman setelan,
   penjelasan tiap setelan satu per satu dalam bentuk tabel (nama setelan,
   fungsinya, rentang nilai, kapan berlakunya), lalu cara simpan.
7. **Suara & musik** — apa saja yang bunyi, kapan bunyinya, cara ganti lagu di
   kartu memori (termasuk aturan penamaan folder/file).
8. **Perawatan & penyimpanan** — singkat.
9. **Kalau ada masalah** — tabel gejala → kemungkinan penyebab → langkah
   perbaikan. Minimal 8 baris, ambil dari daftar masalah di FAKTA ALAT.
10. **Spesifikasi singkat** — tabel.
11. **Catatan keselamatan** — listrik 5V terpisah, jangan colok panel ke pin board,
    kabel ground harus nyambung.

### FAKTA ALAT (sumber kebenaran, dilarang ngarang di luar ini)

**Identitas**
- Nama: P5 Countdown Timer. Otak: ESP32 DevKit v1.
- Layar: panel LED P5 ukuran 64x32 titik, nampilin angka mundur format MM:SS.
- Kendali: **satu tombol fisik** (tombol trigger). Tidak ada tombol lain.
- Suara: modul pemutar MP3 (DFPlayer Mini) + speaker, plus buzzer.
- Listrik: panel LED wajib pakai adaptor 5V sendiri, jangan diambil dari pin 5V
  board ESP32. Ground adaptor panel dan ground ESP32 harus disambung.

**Alur kerja satu ronde**
1. Nyala → layar tunggu nampilin durasi yang sudah disetel (misal 05:00), musik
   idle diputar berulang.
2. Tekan tombol → terdengar suara "mulai", buzzer bunyi, hitung mundur jalan.
3. Selama berjalan, angka pakai warna "berjalan".
4. Masuk 10 detik terakhir → warna angka ganti jadi warna "10 detik akhir" dan
   buzzer bunyi tik tiap detik.
5. Waktu habis → layar nampilin 00:00 dan **ditahan 10 detik** (tidak langsung
   balik ke layar tunggu), terdengar suara "waktu habis", lalu musik idle jalan
   lagi dan layar balik ke durasi awal.
6. Kalau tombol ditekan saat hitung mundur masih jalan → ronde dihentikan paksa,
   angka terakhir **dibekukan di layar selama 10 detik** (biar sisa waktu sempat
   dicatat), buzzer bunyi tanda berhenti, musik idle jalan lagi.
7. Tombol punya jeda anti-dobel 50 milidetik, jadi sekali tekan = satu perintah.

**Setelan di halaman web**
- Cara masuk: sambungkan HP/laptop ke Wi-Fi alat, nama Wi-Fi `P5-TIMER-XXXXXX`
  (XXXXXX = kode unik tiap alat, dicetak di stiker unit), password `@Quantum2022`,
  lalu buka `http://192.168.4.1` di browser.
- Nama Wi-Fi tiap unit beda-beda supaya beberapa alat di satu lokasi tidak bentrok.
- Daftar setelan:
  | Setelan | Keterangan | Rentang |
  |---|---|---|
  | Durasi (MM:SS) | lama hitung mundur | 00:01 sampai 99:59, dibulatkan ke detik penuh |
  | Margin atas/bawah/kiri/kanan | jarak angka ke tepi panel, satuan titik | mulai 0 |
  | Kecerahan | terang panel | minimal 10, maksimal 255 |
  | Warna layar tunggu | warna angka saat menunggu | bebas, warna terlalu gelap dinaikkan otomatis |
  | Warna berjalan | warna angka saat hitung mundur | sama |
  | Warna 10 detik akhir | warna angka di 10 detik terakhir | sama |
  | Ketebalan angka | Normal / Tebal / Ekstra tebal | 3 pilihan |
  | Pakai background warna | isi latar area angka, area margin tetap hitam | nyala/mati |
  | Warna background | warna latar, boleh hitam | bebas |
  | Volume | keras suara MP3 | 0 sampai 30 |
  | Musik idle | musik latar saat menunggu | nyala/mati |
  | Track idle | nomor lagu di folder MP3 | mulai 1 |
- Tekan tombol **Simpan** untuk menyimpan. Setelan disimpan di dalam alat, tidak
  hilang walau listrik dimatikan.
- **Durasi baru baru berlaku di ronde berikutnya**, tidak memotong ronde yang
  sedang berjalan. Margin, warna, ketebalan, kecerahan, dan volume langsung terasa.
- Kalau nilai yang diisi di luar batas, alat otomatis membetulkan ke nilai
  terdekat yang aman dan halaman menampilkan nilai hasil koreksinya.

**Kartu memori suara (FAT32)**
- `/MP3/0001.mp3` musik latar saat menunggu (diputar berulang)
- `/01/001.mp3` suara "mulai"
- `/01/002.mp3` suara "waktu habis"
- `/01/003.mp3` suara "game selesai" (tersedia di kartu, di firmware saat ini
  belum dipakai — sebutkan apa adanya, jangan dijanjikan bunyi)
- Nama folder dan file harus persis: `/01` bukan `/1`, `001.mp3` bukan `1.mp3`.
  Urutan menyalin file ke kartu tidak berpengaruh.
- Untuk ganti musik/suara: matikan alat, cabut kartu, timpa file dengan nama yang
  sama persis, format MP3.

**Daftar masalah untuk bab troubleshooting**
- Panel tidak menyala sama sekali → adaptor 5V panel belum tercolok / ground belum
  nyambung.
- Alat tidak mau menyala atau layar acak setelah dicolok → panel masih terpasang
  saat proses isi ulang program; cabut panel dulu.
- Angka terlihat redup atau gelap → kecerahan disetel terlalu rendah.
- Angka terpotong di tepi panel → margin kebesaran, kecilkan.
- Hitung mundur jalan sendiri / dobel jalan begitu ditekan → kabel tombol
  terjepit atau korslet ke ground.
- Tidak ada suara sama sekali → kartu memori belum terpasang, format bukan FAT32,
  atau volume 0.
- Suara "mulai" tidak bunyi tapi musik latar bunyi → nama file di folder `/01`
  salah.
- Wi-Fi `P5-TIMER-XXXXXX` tidak muncul di HP → alat belum menyala penuh, tunggu
  sekitar 10 detik lalu refresh daftar Wi-Fi.
- Sudah tersambung Wi-Fi tapi halaman tidak terbuka → pastikan alamat diketik
  `http://192.168.4.1` (bukan lewat kolom pencarian Google), dan matikan data
  seluler sementara.
- Layar diam di 00:00 agak lama → itu normal, angka akhir memang ditahan 10 detik.
- Setelan balik lagi setelah disimpan → tombol Simpan belum ditekan.

### Aturan gambar
Kamu tidak punya foto alatnya. Jangan bikin gambar palsu. Sediakan kotak
placeholder bergaris putus-putus dengan keterangan di dalamnya, misal
`[FOTO: tampak depan alat, tombol trigger dilingkari merah]`. Minimal ada
placeholder untuk: tampak depan alat, posisi tombol, colokan listrik, tampilan
halaman setelan di HP, dan susunan folder di kartu memori.

### Aturan isi
- Dilarang menambah fitur, tombol, menu, atau angka yang tidak ada di FAKTA ALAT.
- Kalau ada informasi yang kamu butuh tapi tidak tersedia (contoh: dimensi fisik
  boks, berat, panjang kabel, isi paket penjualan), tulis `[ISI SENDIRI: ...]`
  dengan warna merah, jangan dikarang.
- Ulangi informasi penting di tempat yang tepat; pembaca tidak baca dari awal.
- Jangan bahas kode program, nama file program, GPIO, atau cara flash firmware.
  Manual ini untuk pemakai, bukan teknisi.

### Cara menghasilkan PDF-nya
Prioritas 1 — kalau kamu bisa menjalankan kode:
1. Tulis manual sebagai satu file HTML mandiri (CSS ditulis di dalam file, tanpa
   link ke internet).
2. Konversi ke PDF pakai tool yang tersedia, simpan sebagai
   `Panduan-Penggunaan-P5-Timer.pdf`.
3. Laporkan lokasi file dan jumlah halamannya.

Prioritas 2 — kalau tidak bisa menjalankan kode: keluarkan satu file HTML mandiri
yang **siap dicetak jadi PDF**, lalu beri tahu caranya (buka di Chrome →
Ctrl+P → Tujuan "Save as PDF" → ukuran A4 → margin Default → centang
"Background graphics").

Ketentuan tata letak:
- Ukuran A4, `@page { size: A4; margin: 18mm 16mm; }`
- Huruf isi 11pt, jarak baris 1.5, judul bab jelas dan bernomor.
- Setiap bab utama mulai di halaman baru (`page-break-before: always`).
- Tabel dan langkah bernomor jangan terpotong tengah halaman
  (`page-break-inside: avoid`).
- Aman dicetak hitam putih: jangan mengandalkan warna sebagai satu-satunya
  penanda, kasih label teks juga.
- Nomor halaman di kanan bawah, nama alat di kiri bawah.
- Kotak khusus dengan latar abu untuk "Catatan" dan latar kuning untuk "Perhatian".

### Sebelum menyerahkan, cek sendiri
- [ ] Semua langkah bisa diikuti tanpa perlu tanya orang lain.
- [ ] Tidak ada fitur yang tidak ada di FAKTA ALAT.
- [ ] Semua nilai angka (durasi maks, volume, kecerahan) sesuai FAKTA ALAT.
- [ ] Password dan alamat `http://192.168.4.1` tertulis benar.
- [ ] Bab troubleshooting minimal 8 baris.
- [ ] Bagian yang belum diketahui ditandai `[ISI SENDIRI: ...]`.
- [ ] Tidak ada bab yang terpotong aneh saat dicetak A4.

Mulai kerjakan. Kalau ada yang benar-benar menghalangi, tanya maksimal 3
pertanyaan dulu; selebihnya pakai asumsi wajar dan tulis asumsinya di akhir
dokumen.

## PROMPT (sampai sini)

---

## Catatan pemakaian

- Kalau prompt ini dijalanin di Claude Code dalam repo ini, tambahin satu kalimat
  di awal: "Baca juga `wirring.txt` dan `data/index.html` buat mastiin nama
  setelan persis sama."
- Kalau firmware berubah (durasi, jumlah tombol, alur ronde), update bagian
  `FAKTA ALAT` dulu sebelum prompt dipakai lagi.
- Kode unik Wi-Fi tiap unit muncul di serial monitor waktu alat baru nyala —
  catat dan tempel di stiker unit sebelum dibagikan ke operator.
