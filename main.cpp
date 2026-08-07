#include "data/csv_reader.hpp"
#include "core/candle.hpp"
#include "core/dataset.hpp"

#include "backtest/engine.hpp"
#include "backtest/portofolio.hpp"

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

// ═════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═════════════════════════════════════════════════════════════
static std::string fmt(double value, int prec = 2)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec) << value;
    return oss.str();
}

static std::string fmt4(double value) { return fmt(value, 4); }

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

static std::string signalTypeToString(quant::strategy::SignalType type)
{
    switch (type)
    {
        case quant::strategy::SignalType::BUY:  return "BUY ";
        case quant::strategy::SignalType::SELL: return "SELL";
        default: return "HOLD";
    }
}

// ═════════════════════════════════════════════════════════════
// ADAPTER: Wrapper biar Breakout & EMA Cross bisa masuk Engine
// (Sementara sampai lu update header mereka jadi inherit Strategy)
// ═════════════════════════════════════════════════════════════
namespace {

class BreakoutAdapter : public quant::strategy::Strategy
{
    const quant::strategy::BreakoutStrategy& impl_;
public:
    explicit BreakoutAdapter(const quant::strategy::BreakoutStrategy& impl) 
        : impl_(impl) {}
    
    std::vector<quant::strategy::Signal> generate_signals(
        const quant::core::Dataset& dataset) const override 
    {
        return impl_.generate_signals(dataset);
    }
    
    quant::strategy::Signal generate_signal_at(
        const quant::core::Dataset& dataset, 
        std::size_t index) const override 
    {
        return impl_.generate_signal_at(dataset, index);
    }
};

class EmaCrossAdapter : public quant::strategy::Strategy
{
    const quant::strategy::EmaCrossStrategy& impl_;
public:
    explicit EmaCrossAdapter(const quant::strategy::EmaCrossStrategy& impl) 
        : impl_(impl) {}
    
    std::vector<quant::strategy::Signal> generate_signals(
        const quant::core::Dataset& dataset) const override 
    {
        return impl_.generate_signals(dataset);
    }
    
    quant::strategy::Signal generate_signal_at(
        const quant::core::Dataset& dataset, 
        std::size_t index) const override 
    {
        return impl_.generate_signal_at(dataset, index);
    }
};

// ═════════════════════════════════════════════════════════════
// METRICS: Hitung Max Drawdown dari equity curve
// ═════════════════════════════════════════════════════════════
static double calculateMaxDrawdown(const std::vector<double>& equity)
{
    if (equity.size() < 2) return 0.0;
    
    double peak = equity[0];
    double max_dd = 0.0;
    
    for (const double& val : equity)
    {
        if (val > peak) peak = val;
        double dd = (peak - val) / peak;
        if (dd > max_dd) max_dd = dd;
    }
    return max_dd;
}

static void printBacktestResults(
    const std::string& name,
    const quant::backtest::Portfolio& portfolio,
    double initial_cash)
{
    const auto& equity = portfolio.equityCurve();
    double final_equity = portfolio.equity();
    double total_return = (final_equity - initial_cash) / initial_cash * 100.0;
    double max_dd = calculateMaxDrawdown(equity);
    
    printHeader("BACKTEST RESULT: " + name, '-');
    
    std::cout << "  " << std::left << std::setw(28) << "Initial Cash"
              << ": $" << fmt(initial_cash) << '\n';
    std::cout << "  " << std::left << std::setw(28) << "Final Equity"
              << ": $" << fmt(final_equity) << '\n';
    std::cout << "  " << std::left << std::setw(28) << "Total Return"
              << ": " << fmt(total_return) << "%\n";
    std::cout << "  " << std::left << std::setw(28) << "Max Drawdown"
              << ": " << fmt(max_dd * 100.0) << "%\n";
    std::cout << "  " << std::left << std::setw(28) << "Final Position"
              << ": " << fmt(portfolio.position()) << " BTC\n";
    std::cout << "  " << std::left << std::setw(28) << "Final Cash"
              << ": $" << fmt(portfolio.cash()) << '\n';
    
    // Equity curve snapshot (last 10)
    if (!equity.empty())
    {
        std::cout << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Equity Curve (last 10)"
                  << ": ";
        std::size_t start = (equity.size() > 10) ? equity.size() - 10 : 0;
        for (std::size_t i = start; i < equity.size(); ++i)
        {
            std::cout << fmt(equity[i]) << (i + 1 < equity.size() ? ", " : "");
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════
// MAIN
// ═════════════════════════════════════════════════════════════
int main()
{
    try
    {
        // =========================================================
        // 1. KONFIGURASI
        // =========================================================
        const std::string csvPath     = "/home/nvoinxv/Documents/Quant_Research_C++/BTCUSDT.csv";
        const double initialCash      = 10000.0;
        const std::size_t ewmaPeriod  = 20;
        const std::size_t rsiPeriod   = 14;
        const std::size_t chopPeriod  = 14;

        // =========================================================
        // 2. LOAD DATA
        // =========================================================
        quant::data::CSVReader reader;
        auto candles = reader.read(csvPath);

        if (candles.empty())
        {
            throw std::runtime_error("CSV file is empty or failed to load.");
        }

        // Build Dataset (untuk strategi manual & engine)
        quant::core::Dataset dataset;
        dataset.symbol   = "BTCUSDT";
        dataset.interval = "5m";
        dataset.candles  = candles; // copy untuk dataset

        // Extract closes untuk indikator
        std::vector<double> closePrices;
        closePrices.reserve(dataset.size());
        for (const auto& c : dataset.candles)
            closePrices.push_back(c.close);

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
        // 4. DASHBOARD INDIKATOR
        // =========================================================
        printHeader("QUANTITATIVE INDICATOR & BACKTEST ENGINE");
        std::cout << "  Asset : BTCUSDT | Timeframe : 5m | Spot Market\n";
        std::cout << "  Initial Capital : $" << fmt(initialCash) << '\n';
        printSeparator('=');
        std::cout << '\n';

        // Dataset Info
        printHeader("DATASET INFORMATION");
        std::cout << "  " << std::left << std::setw(28) << "CSV Source"
                  << ": " << csvPath << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Total Candles"
                  << ": " << dataset.size() << '\n';
        std::cout << "  " << std::left << std::setw(28) << "First Close"
                  << ": " << fmt4(closePrices.front()) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Last Close"
                  << ": " << fmt4(closePrices.back()) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Price Range"
                  << ": " << fmt4(*std::min_element(closePrices.begin(), closePrices.end()))
                  << " - " << fmt4(*std::max_element(closePrices.begin(), closePrices.end())) << '\n';
        std::cout << '\n';

        // Latest Values
        printHeader("LATEST INDICATOR VALUES");
        std::cout << "  " << std::left << std::setw(28) << "EWMA (" + std::to_string(ewmaPeriod) + ")"
                  << ": " << fmt4(lastEWMA) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "RSI Wilder (" + std::to_string(rsiPeriod) + ")"
                  << ": " << fmt(lastRSI) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Choppiness Index"
                  << ": " << fmt(lastCHOP) << '\n';
        std::cout << '\n';

        // Market Interpretation
        printHeader("MARKET INTERPRETATION");
        std::string rsiSignal = (lastRSI > 70.0) ? "OVERBOUGHT (Bearish)"
                              : (lastRSI < 30.0) ? "OVERSOLD (Bullish)"
                              : "NEUTRAL";
        std::cout << "  " << std::left << std::setw(28) << "RSI Signal"
                  << ": " << rsiSignal << '\n';
        std::cout << "  " << std::left << std::setw(28) << "EWMA Bias"
                  << ": " << ((closePrices.back() > lastEWMA) ? "Bullish" : "Bearish") << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Market Regime"
                  << ": " << ((lastCHOP > 61.8) ? "Choppy/Sideways" : 
                              (lastCHOP < 38.2) ? "Trending" : "Transition") << '\n';
        std::cout << '\n';

        // Recent History
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
        // 5. STRATEGY BACKTEST VIA ENGINE
        // =========================================================
        printHeader("STRATEGY BACKTEST RESULTS");

        // ── Breakout ──
        quant::strategy::BreakoutStrategy breakout(20, 1, 0.001, false);
        BreakoutAdapter breakoutAdapter(breakout);
        quant::backtest::Engine engineBreakout(candles, initialCash);
        engineBreakout.run(breakoutAdapter);
        printBacktestResults("BREAKOUT (Donchian 20)", engineBreakout.portfolio(), initialCash);

        // ── EMA Cross ──
        quant::strategy::EmaCrossStrategy emaCross(9, 21, 1, 0.0, false);
        EmaCrossAdapter emaAdapter(emaCross);
        quant::backtest::Engine engineEma(candles, initialCash);
        engineEma.run(emaAdapter);
        printBacktestResults("EMA CROSS (9/21)", engineEma.portfolio(), initialCash);

        // ── Mean Reversion ──
        quant::strategy::MeanReversionStrategy meanRev(9, 25.0, 75.0, 14, 50.0, 0, false);
        quant::backtest::Engine engineMr(candles, initialCash);
        engineMr.run(meanRev); // Langsung, karena udah inherit Strategy
        printBacktestResults("MEAN REVERSION (RSI 9 + CHOP)", engineMr.portfolio(), initialCash);

        // =========================================================
        // 6. CROSS-STRATEGY COMPARISON
        // =========================================================
        printHeader("CROSS-STRATEGY COMPARISON");
        std::cout << "  " << std::left << std::setw(24) << "Strategy"
                  << std::setw(16) << "Final Equity"
                  << std::setw(16) << "Return %"
                  << std::setw(16) << "Max DD %"
                  << '\n';
        printSeparator('-');
        
        auto printRow = [&](const std::string& name, const quant::backtest::Portfolio& p) {
            double ret = (p.equity() - initialCash) / initialCash * 100.0;
            double dd = calculateMaxDrawdown(p.equityCurve()) * 100.0;
            std::cout << "  " << std::left << std::setw(24) << name
                      << std::setw(16) << fmt(p.equity())
                      << std::setw(16) << fmt(ret)
                      << std::setw(16) << fmt(dd)
                      << '\n';
        };
        
        printRow("Breakout", engineBreakout.portfolio());
        printRow("EMA Cross", engineEma.portfolio());
        printRow("Mean Reversion", engineMr.portfolio());
        
        printSeparator('=');
        std::cout << "           BACKTEST COMPLETED SUCCESSFULLY\n";
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