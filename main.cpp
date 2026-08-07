#include "data/csv_reader.hpp"
#include "core/candle.hpp"
#include "core/dataset.hpp"

#include "indicator/ewma.hpp"
#include "indicator/rsi_wilder.hpp"
#include "indicator/choppiness_index.hpp"

#include "strategy/breakout.hpp"
#include "strategy/ema_cross.hpp"
#include "strategy/mean_reversion.hpp"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────
// Helper: Format angka
// ─────────────────────────────────────────────────────────────
static std::string fmt(double value, int prec = 2)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec) << value;
    return oss.str();
}

static std::string fmt4(double value) { return fmt(value, 4); }

// ─────────────────────────────────────────────────────────────
// Helper: Cetak garis pemisah & header
// ─────────────────────────────────────────────────────────────
static void printSeparator(char fill = '-', int width = 80)
{
    std::cout << std::string(width, fill) << '\n';
}

static void printHeader(const std::string& title, char border = '=')
{
    printSeparator(border);
    std::cout << "  " << title << '\n';
    printSeparator(border);
}

// ─────────────────────────────────────────────────────────────
// Helper: Konversi sinyal ke string
// ─────────────────────────────────────────────────────────────
static std::string signalTypeToString(quant::strategy::SignalType type)
{
    switch (type)
    {
        case quant::strategy::SignalType::BUY:  return "BUY ";
        case quant::strategy::SignalType::SELL: return "SELL";
        default: return "HOLD";
    }
}

// ─────────────────────────────────────────────────────────────
// Helper: Cetak tabel sinyal strategi
// ─────────────────────────────────────────────────────────────
static void printSignals(const std::string& strategyName,
                         const std::vector<quant::strategy::Signal>& signals,
                         std::size_t maxRows = 15)
{
    printHeader("STRATEGY: " + strategyName, '-');

    if (signals.empty())
    {
        std::cout << "  [No signals generated — data too short or no trigger]\n\n";
        return;
    }

    std::cout << "  Total Signals: " << signals.size() << '\n';
    printSeparator('-');
    std::cout << "  " << std::left
              << std::setw(8)  << "Bar"
              << std::setw(12) << "Type"
              << std::setw(16) << "Price"
              << std::setw(14) << "Confidence"
              << "Reason\n";
    printSeparator('-');

    std::size_t start = (signals.size() > maxRows) ? signals.size() - maxRows : 0;
    for (std::size_t i = start; i < signals.size(); ++i)
    {
        const auto& s = signals[i];
        std::cout << "  " << std::left
                  << std::setw(8)  << (s.index + 1)
                  << std::setw(12) << signalTypeToString(s.type)
                  << std::setw(16) << fmt4(s.price)
                  << std::setw(14) << fmt(s.confidence)
                  << s.reason << '\n';
    }
    printSeparator('-');
    std::cout << '\n';
}

// ─────────────────────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────────────────────
int main()
{
    try
    {
        // =========================================================
        // 1. KONFIGURASI
        // =========================================================
        const std::string csvPath    = "/home/nvoinxv/Documents/Quant_Research_C++/BTCUSDT.csv";
        const std::size_t ewmaPeriod = 20;
        const std::size_t rsiPeriod  = 14;
        const std::size_t chopPeriod = 14;

        // =========================================================
        // 2. LOAD DATA DARI CSV → BUILD DATASET
        // =========================================================
        quant::data::CSVReader reader;
        auto candles = reader.read(csvPath);

        if (candles.empty())
        {
            throw std::runtime_error("CSV file is empty or failed to load.");
        }

        // Build Dataset (strategi butuh struct Dataset, bukan vector<Candle> mentah)
        quant::core::Dataset dataset;
        dataset.symbol   = "BTCUSDT";
        dataset.interval = "5m";
        dataset.candles  = std::move(candles);

        // Extract Close Prices untuk indikator
        std::vector<double> closePrices;
        closePrices.reserve(dataset.size());

        for (const auto& c : dataset.candles)
        {
            closePrices.push_back(c.close);
        }

        // =========================================================
        // 3. HITUNG INDIKATOR
        // =========================================================
        auto ewmaValues = quant::indicators::EWMA::calculate(closePrices, ewmaPeriod);
        auto rsiValues  = quant::indicators::RSI::calculate(closePrices, rsiPeriod);
        auto chopValues = quant::indicators::ChoppinessIndex::calculate(dataset.candles, chopPeriod);

        double lastEWMA = ewmaValues.empty() ? 0.0 : ewmaValues.back();
        double lastRSI  = rsiValues.empty()  ? 0.0 : rsiValues.back();
        double lastCHOP = chopValues.empty() ? 0.0 : chopValues.back();

        // =========================================================
        // 4. OUTPUT INDIKATOR (Professional Dashboard)
        // =========================================================
        printHeader("QUANTITATIVE INDICATOR & STRATEGY ENGINE");
        std::cout << "  Asset : BTCUSDT | Timeframe : 5m | Spot Market\n";
        printSeparator('=');
        std::cout << '\n';

        // ── Dataset Info ──
        printHeader("DATASET INFORMATION");
        std::cout << "  " << std::left << std::setw(28) << "CSV Source"
                  << ": " << csvPath << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Total Candles Loaded"
                  << ": " << dataset.size() << '\n';
        std::cout << "  " << std::left << std::setw(28) << "First Close Price"
                  << ": " << fmt4(closePrices.front()) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Last Close Price"
                  << ": " << fmt4(closePrices.back()) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Price Range"
                  << ": " << fmt4(*std::min_element(closePrices.begin(), closePrices.end()))
                  << " - " << fmt4(*std::max_element(closePrices.begin(), closePrices.end())) << '\n';
        std::cout << '\n';

        // ── Konfigurasi Indikator ──
        printHeader("INDICATOR CONFIGURATION");
        std::cout << "  " << std::left << std::setw(28) << "EWMA Period"
                  << ": " << ewmaPeriod << '\n';
        std::cout << "  " << std::left << std::setw(28) << "RSI Period (Wilder)"
                  << ": " << rsiPeriod << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Choppiness Index Period"
                  << ": " << chopPeriod << '\n';
        std::cout << '\n';

        // ── Hasil Perhitungan ──
        printHeader("LATEST INDICATOR VALUES");
        std::cout << "  " << std::left << std::setw(28) << "EWMA (" + std::to_string(ewmaPeriod) + ")"
                  << ": " << fmt4(lastEWMA) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "RSI Wilder (" + std::to_string(rsiPeriod) + ")"
                  << ": " << fmt(lastRSI) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Choppiness Index (" + std::to_string(chopPeriod) + ")"
                  << ": " << fmt(lastCHOP) << '\n';
        std::cout << '\n';

        // ── Interpretasi Market ──
        printHeader("MARKET INTERPRETATION");

        std::string rsiSignal = (lastRSI > 70.0) ? "OVERBOUGHT (Bearish Bias)"
                              : (lastRSI < 30.0) ? "OVERSOLD (Bullish Bias)"
                              : "NEUTRAL";
        std::cout << "  " << std::left << std::setw(28) << "RSI Signal"
                  << ": " << rsiSignal << '\n';

        std::string ewmaSignal = (closePrices.back() > lastEWMA) ? "Price Above EWMA (Bullish)"
                               : "Price Below EWMA (Bearish)";
        std::cout << "  " << std::left << std::setw(28) << "EWMA Signal"
                  << ": " << ewmaSignal << '\n';

        std::string chopSignal = (lastCHOP > 61.8) ? "CHOPPY / SIDEWAYS"
                               : (lastCHOP < 38.2) ? "TRENDING"
                               : "TRANSITION PHASE";
        std::cout << "  " << std::left << std::setw(28) << "Choppiness Signal"
                  << ": " << chopSignal << '\n';
        std::cout << '\n';

        // ── Tabel History ──
        printHeader("RECENT HISTORY (Last 10 Candles)");
        printSeparator('-');
        std::cout << "  " << std::left
                  << std::setw(8)  << "Bar"
                  << std::setw(16) << "Close"
                  << std::setw(16) << "EWMA"
                  << std::setw(16) << "RSI"
                  << std::setw(16) << "CHOP"
                  << '\n';
        printSeparator('-');

        std::size_t displayCount = std::min<std::size_t>(10, dataset.size());
        std::size_t offset       = dataset.size() - displayCount;

        for (std::size_t i = 0; i < displayCount; ++i)
        {
            std::size_t idx = offset + i;
            std::cout << "  " << std::left
                      << std::setw(8)  << (idx + 1)
                      << std::setw(16) << fmt4(closePrices[idx])
                      << std::setw(16) << fmt4(ewmaValues[idx])
                      << std::setw(16) << fmt(rsiValues[idx])
                      << std::setw(16) << fmt(chopValues[idx])
                      << '\n';
        }
        printSeparator('-');
        std::cout << '\n';

        // =========================================================
        // 5. STRATEGY SIGNAL GENERATION (Batch Processing)
        // =========================================================
        printHeader("STRATEGY BACKTEST SIGNALS");

        // ── Breakout: Donchian Channel ──
        quant::strategy::BreakoutStrategy breakout(
            20,     // lookback period
            1,      // confirmation bars
            0.001,  // 0.1% breakout threshold
            false   // volume confirmation off
        );
        auto breakoutSignals = breakout.generate_signals(dataset);
        printSignals("BREAKOUT (Donchian Channel, 20)", breakoutSignals);

        // ── EMA Cross: 9/21 (Crypto 5m Optimized) ──
        quant::strategy::EmaCrossStrategy emaCross(
            9,      // short period
            21,     // long period
            1,      // confirmation bars
            0.0,    // no threshold
            false   // volume confirmation off
        );
        auto emaSignals = emaCross.generate_signals(dataset);
        printSignals("EMA CROSS (9/21 Golden/Death Cross)", emaSignals);

        // ── Mean Reversion: RSI Wilder + Choppiness Filter ──
        quant::strategy::MeanReversionStrategy meanRev(
            9,      // RSI period (responsive, kurangi lagging)
            25.0,   // oversold threshold (agresif)
            75.0,   // overbought threshold (agresif)
            14,     // CHOP period
            50.0,   // CHOP threshold (entry hanya saat choppy)
            0,      // confirmation bars 0 (entry cepat)
            false   // volume confirmation off
        );
        auto mrSignals = meanRev.generate_signals(dataset);
        printSignals("MEAN REVERSION (RSI 9 + CHOP Filter)", mrSignals);

        // =========================================================
        // 6. CROSS-STRATEGY SUMMARY
        // =========================================================
        printHeader("SIGNAL SUMMARY");
        std::cout << "  " << std::left << std::setw(32) << "Breakout Signals"
                  << ": " << breakoutSignals.size() << '\n';
        std::cout << "  " << std::left << std::setw(32) << "EMA Cross Signals"
                  << ": " << emaSignals.size() << '\n';
        std::cout << "  " << std::left << std::setw(32) << "Mean Reversion Signals"
                  << ": " << mrSignals.size() << '\n';
        printSeparator('=');
        std::cout << "           ANALYSIS COMPLETED SUCCESSFULLY\n";
        printSeparator('=');

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << '\n';
        printSeparator('!', 70);
        std::cerr << "  CRITICAL ERROR: " << e.what() << '\n';
        printSeparator('!', 70);
        return EXIT_FAILURE;
    }
}