# Quant Research C++

Framework riset kuantitatif berbasis C++ untuk melakukan **backtesting strategi trading**, **evaluasi indikator statistik**, dan **manajemen portofolio** secara terintegrasi. Proyek ini dibangun untuk keperluan eksplorasi dan validasi ide trading sistematis (systematic trading) menggunakan data historis (candle/OHLCV), sebelum ide tersebut diuji lebih lanjut di lingkungan live atau paper trading.

> **Status:** Proyek ini masih dalam tahap pengembangan aktif (work in progress). Beberapa modul source (`src/`) belum memiliki header (`headers/`) yang bersesuaian, dan build system formal (CMake/Makefile) belum tersedia di repo. Lihat bagian [Roadmap & Known Issues](#roadmap--known-issues) untuk detail.

---

## Daftar Isi

- [Fitur Utama](#fitur-utama)
- [Struktur Proyek](#struktur-proyek)
- [Arsitektur Modul](#arsitektur-modul)
- [Cara Build & Menjalankan](#cara-build--menjalankan)
- [Alur Kerja (Workflow)](#alur-kerja-workflow)
- [Roadmap & Known Issues](#roadmap--known-issues)
- [Kontribusi](#kontribusi)
- [Lisensi](#lisensi)

---

## Fitur Utama

- **Backtesting Engine** — simulasi eksekusi order, broker, dan tracking portofolio berbasis data historis.
- **Indikator Statistik/Quant** — implementasi indikator seperti EWMA, RSI, dan Volatility sebagai basis sinyal strategi.
- **Strategi Trading Modular** — contoh implementasi strategi (breakout, EMA cross, mean reversion) yang bisa dijadikan template untuk strategi baru.
- **Metrik Evaluasi Performa** — perhitungan Sharpe Ratio, Max Drawdown, Expectancy, dan metrik risk-adjusted lainnya.
- **Data Loader Fleksibel** — mendukung data dari CSV maupun Binance API (untuk data crypto OHLCV).
- **Visualisasi** — modul plotting untuk candlestick chart, equity curve, dan overlay indikator.

---

## Struktur Proyek

```
Quant_Research_C++/
├── headers/                    # Header (.hpp) — kontrak/interface modul
│   ├── core/                   # Struktur data inti (candle, dataset, timeseries)
│   ├── data/                   # Interface loader data (CSV, Binance)
│   └── indicator/               # Interface indikator statistik
├── src/                        # Implementasi (.cpp)
│   ├── backtest/                # Engine simulasi, broker, order, portofolio
│   ├── data/                    # Implementasi loader data
│   ├── indicator/                # Implementasi indikator
│   ├── metrics/                  # Perhitungan metrik performa
│   ├── plot/                     # Visualisasi hasil backtest
│   └── strategy/                  # Implementasi strategi trading
├── main.cpp                    # Entry point aplikasi
├── quant_research               # (belum dikonfirmasi — kemungkinan binary/output build)
└── README.md
```

---

## Arsitektur Modul

### `headers/core/`
Struktur data fundamental yang dipakai di seluruh sistem:
- `candle.hpp` — representasi satu candle/OHLCV.
- `dataset.hpp` — kumpulan data (kemungkinan kumpulan candle atau time series multi-asset).
- `timeseries.hpp` — struktur data time series generik.

### `headers/data/` & `src/data/`
Layer akuisisi data:
- `csv_loader.hpp/.cpp` & `csv_reader.hpp/.cpp` — membaca data historis dari file CSV.
- `binance_loader.hpp/.cpp` — mengambil data OHLCV langsung dari Binance API.

### `headers/indicator/` & `src/indicator/`
Indikator statistik sebagai basis sinyal:
- `ewma.hpp/.cpp` — Exponentially Weighted Moving Average.
- `rsi.hpp/.cpp` — Relative Strength Index.
- `volatility.hpp/.cpp` — pengukuran volatilitas (kemungkinan realized/historical volatility).

### `src/backtest/`
Inti dari mesin backtesting:
- `engine.cpp` — orkestrasi utama simulasi (loop candle → sinyal → eksekusi → update portofolio).
- `broker.cpp` — simulasi eksekusi order (slippage, fee, dll — perlu dikonfirmasi).
- `order.cpp` — representasi order (market/limit, buy/sell).
- `portofolio.cpp` — tracking posisi, cash, dan equity.

> ⚠️ Catatan: nama file `portofolio.cpp` sebaiknya diganti ke `portfolio.cpp` (ejaan baku Bahasa Inggris) agar konsisten dan tidak membingungkan kontributor lain atau tooling (grep/IDE search).

### `src/metrics/`
Evaluasi performa strategi:
- `sharpe.cpp` — Sharpe Ratio.
- `max_drawdown.cpp` — Maximum Drawdown.
- `expectancy.cpp` — expectancy per trade.
- `sorting.cpp` — **diasumsikan** ini adalah **Sortino Ratio** (metrik risk-adjusted return berbasis downside deviation), bukan algoritma sorting. Mohon dikoreksi/di-rename ke `sortino.cpp` jika asumsi ini benar, untuk menghindari kebingungan.

### `src/plot/`
Visualisasi hasil:
- `candlestick.cpp` — chart candlestick.
- `equity_curver.cpp` — **kemungkinan typo** dari `equity_curve.cpp` (kurva ekuitas portofolio dari waktu ke waktu). Disarankan rename untuk konsistensi.
- `indicator_plot.cpp` — overlay indikator pada chart.

### `src/strategy/`
Contoh strategi trading yang siap pakai atau jadi template:
- `breakout.cpp` — strategi breakout.
- `ema_cross.cpp` — strategi persilangan EMA (golden/death cross).
- `mean_reversion.cpp` — strategi mean reversion.

---

## Cara Build & Menjalankan

> Belum ada `CMakeLists.txt` atau `Makefile` di repo saat ini, sehingga instruksi di bawah bersifat **contoh umum**, bukan perintah yang sudah teruji di proyek ini. Sesuaikan dengan compiler flags dan dependency aktual (misalnya library HTTP untuk Binance loader, atau library plotting yang dipakai).

### Prasyarat
- Compiler C++ dengan dukungan minimal C++17 (`g++` atau `clang++`).
- (Opsional, jika `binance_loader` menggunakan HTTP request) library seperti `libcurl`.
- (Opsional, jika `plot/` merender ke file/window) library plotting yang relevan (mis. matplotlib-cpp, atau export ke format lain).

### Build manual (contoh, perlu disesuaikan)
```bash
g++ -std=c++17 -Iheaders \
    main.cpp \
    src/backtest/*.cpp \
    src/data/*.cpp \
    src/indicator/*.cpp \
    src/metrics/*.cpp \
    src/plot/*.cpp \
    src/strategy/*.cpp \
    -o quant_research
```

### Menjalankan
```bash
./quant_research
```

**Rekomendasi:** tambahkan `CMakeLists.txt` agar proses build reproducible dan mudah diintegrasikan ke CI, terutama karena jumlah source file akan terus bertambah seiring strategi/indikator baru ditambahkan.

---

## Alur Kerja (Workflow)

Alur konseptual penggunaan framework ini (berdasarkan struktur modul yang ada):

1. **Load data** — ambil data historis via `csv_loader`/`csv_reader` atau `binance_loader`.
2. **Hitung indikator** — terapkan `ewma`, `rsi`, `volatility`, atau indikator custom pada dataset.
3. **Jalankan strategi** — logic strategi (`breakout`, `ema_cross`, `mean_reversion`) menghasilkan sinyal beli/jual berdasarkan indikator.
4. **Backtest** — `engine` mensimulasikan eksekusi sinyal melalui `broker` dan `order`, lalu memperbarui `portofolio`.
5. **Evaluasi performa** — hitung `sharpe`, `max_drawdown`, `expectancy`, dan metrik risk-adjusted lainnya.
6. **Visualisasi** — tampilkan hasil melalui `candlestick`, `equity_curver` (equity curve), dan `indicator_plot`.

---

## Roadmap & Known Issues

Daftar ini ditulis secara jujur agar kontributor/pembaca tahu status sebenarnya dari proyek:

- [ ] Tambahkan `CMakeLists.txt`/`Makefile` untuk build yang reproducible.
- [ ] Lengkapi header (`.hpp`) untuk modul `backtest/`, `metrics/`, `plot/`, dan `strategy/` yang saat ini hanya punya `.cpp` tanpa interface eksplisit.
- [ ] Perbaiki penamaan file: `portofolio.cpp` → `portfolio.cpp`, `equity_curver.cpp` → `equity_curve.cpp`, konfirmasi `sorting.cpp` → `sortino.cpp` (jika memang dimaksudkan sebagai Sortino Ratio).
- [ ] Klarifikasi fungsi file `quant_research` di root (apakah ini binary hasil build yang ter-commit tanpa sengaja?).
- [ ] Tambahkan unit test (mis. dengan Catch2/GoogleTest) untuk indikator dan metrics, mengingat perhitungan statistik rawan off-by-one error dan edge case (data kosong, NaN, dsb).
- [ ] Tambahkan contoh dataset kecil (`sample_data/`) agar orang lain bisa langsung mencoba tanpa perlu koneksi ke Binance API.
- [ ] Dokumentasikan parameter tiap indikator dan strategi (periode EWMA, threshold RSI, dll).

---

## Kontribusi

Kontribusi, saran, dan laporan bug sangat diterima. Silakan buka issue atau pull request. Untuk perubahan besar pada arsitektur (misalnya penambahan header baru), mohon diskusikan dulu melalui issue agar konsisten dengan struktur modul yang sudah ada.

---

## Lisensi

*(Belum ditentukan — tambahkan file `LICENSE` sesuai preferensi, misalnya MIT, Apache 2.0, atau proprietary jika ini riset privat.)*