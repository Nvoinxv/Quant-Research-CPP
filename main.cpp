#include "data/csv_reader.hpp"
#include "core/candle.hpp"

#include "indicator/choppiness_index.hpp"
#include "indicator/ewma.hpp"
#include "indicator/rsi_wilder.hpp"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <vector>

// ─────────────────────────────────────────────────────────────
// Helper: Format angka dengan 2 desimal
// ─────────────────────────────────────────────────────────────
static std::string fmt(double value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    return oss.str();
}

// ─────────────────────────────────────────────────────────────
// Helper: Format angka dengan 4 desimal (untuk harga)
// ─────────────────────────────────────────────────────────────
static std::string fmt4(double value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << value;
    return oss.str();
}

// ─────────────────────────────────────────────────────────────
// Helper: Cetak garis pemisah tabel
// ─────────────────────────────────────────────────────────────
static void printSeparator(char fill = '-')
{
    std::cout << std::string(70, fill) << '\n';
}

// ─────────────────────────────────────────────────────────────
// Helper: Cetak header tabel indikator
// ─────────────────────────────────────────────────────────────
static void printIndicatorHeader(const std::string& name)
{
    printSeparator('=');
    std::cout << "  " << name << "\n";
    printSeparator('=');
}

// ─────────────────────────────────────────────────────────────
// Helper: Cetak baris data indikator
// ─────────────────────────────────────────────────────────────
static void printIndicatorRow(const std::string& label, const std::string& value)
{
    std::cout << "  " << std::left << std::setw(30) << label
              << ": " << value << '\n';
}

int main()
{
    try
    {
        // =========================================================
        // 1. KONFIGURASI
        // =========================================================
        const std::string csvPath   = "/home/nvoinxv/Documents/Quant_Research_C++/BTCUSDT.csv";
        const std::size_t ewmaPeriod = 20;
        const std::size_t rsiPeriod  = 14;
        const std::size_t chopPeriod = 14;

        // =========================================================
        // 2. LOAD DATA DARI CSV
        // =========================================================
        quant::data::CSVReader reader;
        auto candles = reader.read(csvPath);

        if (candles.empty())
        {
            throw std::runtime_error("CSV file is empty or failed to load.");
        }

        // Ekstrak Close Prices untuk indikator berbasis harga
        std::vector<double> closePrices;
        closePrices.reserve(candles.size());

        for (const auto& candle : candles)
        {
            closePrices.push_back(candle.close);
        }

        // =========================================================
        // 3. HITUNG INDIKATOR
        // =========================================================
        auto ewmaValues = quant::indicators::EWMA::calculate(closePrices, ewmaPeriod);
        auto rsiValues  = quant::indicators::RSI::calculate(closePrices, rsiPeriod);
        auto chopValues = quant::indicators::ChoppinessIndex::calculate(candles, chopPeriod);

        // Nilai terakhir (terkini)
        double lastEWMA = ewmaValues.empty() ? 0.0 : ewmaValues.back();
        double lastRSI  = rsiValues.empty()  ? 0.0 : rsiValues.back();
        double lastCHOP = chopValues.empty() ? 0.0 : chopValues.back();

        // =========================================================
        // 4. OUTPUT PROFESIONAL
        // =========================================================
        
        // ── Header Program ──
        printSeparator('=');
        std::cout << "           QUANTITATIVE INDICATOR ANALYSIS ENGINE\n";
        std::cout << "                     BTCUSDT / Spot Market\n";
        printSeparator('=');
        std::cout << '\n';

        // ── Informasi Dataset ──
        printIndicatorHeader("DATASET INFORMATION");
        printIndicatorRow("CSV Source", csvPath);
        printIndicatorRow("Total Candles Loaded", std::to_string(candles.size()));
        printIndicatorRow("First Close Price", fmt4(closePrices.front()));
        printIndicatorRow("Last Close Price", fmt4(closePrices.back()));
        printIndicatorRow("Price Range", fmt4(*std::min_element(closePrices.begin(), closePrices.end())) 
                        + " - " + fmt4(*std::max_element(closePrices.begin(), closePrices.end())));
        std::cout << '\n';

        // ── Konfigurasi Indikator ──
        printIndicatorHeader("INDICATOR CONFIGURATION");
        printIndicatorRow("EWMA Period", std::to_string(ewmaPeriod));
        printIndicatorRow("RSI Period (Wilder)", std::to_string(rsiPeriod));
        printIndicatorRow("Choppiness Index Period", std::to_string(chopPeriod));
        std::cout << '\n';

        // ── Hasil Perhitungan ──
        printIndicatorHeader("LATEST INDICATOR VALUES");
        printIndicatorRow("EWMA (" + std::to_string(ewmaPeriod) + ")", fmt4(lastEWMA));
        printIndicatorRow("RSI Wilder (" + std::to_string(rsiPeriod) + ")", fmt(lastRSI));
        printIndicatorRow("Choppiness Index (" + std::to_string(chopPeriod) + ")", fmt(lastCHOP));
        std::cout << '\n';

        // ── Interpretasi Market ──
        printIndicatorHeader("MARKET INTERPRETATION");
        
        // RSI Interpretation
        std::string rsiSignal;
        if (lastRSI > 70.0)      rsiSignal = "OVERBOUGHT (Bearish Bias)";
        else if (lastRSI < 30.0) rsiSignal = "OVERSOLD (Bullish Bias)";
        else                     rsiSignal = "NEUTRAL";
        printIndicatorRow("RSI Signal", rsiSignal);

        // EWMA vs Price
        std::string ewmaSignal;
        if (closePrices.back() > lastEWMA) ewmaSignal = "Price Above EWMA (Bullish)";
        else                               ewmaSignal = "Price Below EWMA (Bearish)";
        printIndicatorRow("EWMA Signal", ewmaSignal);

        // Choppiness Index Interpretation
        std::string chopSignal;
        if (lastCHOP > 61.8)      chopSignal = "CHOPPY MARKET (Sideways)";
        else if (lastCHOP < 38.2) chopSignal = "TRENDING MARKET";
        else                      chopSignal = "TRANSITION PHASE";
        printIndicatorRow("Choppiness Signal", chopSignal);
        std::cout << '\n';

        // ── Tabel History (10 nilai terakhir) ──
        printIndicatorHeader("RECENT HISTORY (Last 10 Candles)");
        printSeparator('-');
        std::cout << "  " << std::left 
                  << std::setw(6)  << "Index"
                  << std::setw(16) << "Close"
                  << std::setw(16) << "EWMA"
                  << std::setw(16) << "RSI"
                  << std::setw(16) << "CHOP"
                  << '\n';
        printSeparator('-');

        std::size_t displayCount = std::min<std::size_t>(10, candles.size());
        std::size_t offset       = candles.size() - displayCount;

        for (std::size_t i = 0; i < displayCount; ++i)
        {
            std::size_t idx = offset + i;
            std::cout << "  " << std::left
                      << std::setw(6)  << (idx + 1)
                      << std::setw(16) << fmt4(closePrices[idx])
                      << std::setw(16) << fmt4(ewmaValues[idx])
                      << std::setw(16) << fmt(rsiValues[idx])
                      << std::setw(16) << fmt(chopValues[idx])
                      << '\n';
        }
        printSeparator('-');
        std::cout << '\n';

        // ── Footer ──
        printSeparator('=');
        std::cout << "           ANALYSIS COMPLETED SUCCESSFULLY\n";
        printSeparator('=');

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << '\n';
        printSeparator('!');
        std::cerr << "  CRITICAL ERROR\n";
        printSeparator('!');
        std::cerr << "  Reason: " << e.what() << '\n';
        printSeparator('!');
        return EXIT_FAILURE;
    }
}