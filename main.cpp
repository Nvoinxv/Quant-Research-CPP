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

#include "plot/candlestick.hpp"
#include "plot/equity_curver.hpp"
#include "plot/indicator_plot.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
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

// ═════════════════════════════════════════════════════════════
// ADAPTER: Wrapper biar Breakout & EMA Cross bisa masuk Engine
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

// ═════════════════════════════════════════════════════════════
// PLOT HELPERS  (with debug path & fallback)
// ═════════════════════════════════════════════════════════════
static std::filesystem::path getOutputDir()
{
    // Prefer project root (where CSV is), fallback to CWD
    std::filesystem::path csvDir = "/home/nvoinxv/Documents/Quant_Research_C++";
    if (std::filesystem::exists(csvDir) && std::filesystem::is_directory(csvDir))
        return csvDir;
    return std::filesystem::current_path();
}

static bool savePlot(const std::string& filename,
                     const std::function<bool(const std::string&)>& renderFn)
{
    std::filesystem::path outDir = getOutputDir();
    std::filesystem::path outPath = outDir / filename;

    std::cout << "  [SAVE] Attempting: " << outPath.string() << '\n';

    if (renderFn(outPath.string()))
    {
        std::cout << "  [OK]   Saved to " << outPath.string() << '\n';
        return true;
    }

    // Fallback: try CWD
    std::filesystem::path cwdPath = std::filesystem::current_path() / filename;
    std::cout << "  [WARN] Primary path failed. Retrying: " << cwdPath.string() << '\n';

    if (renderFn(cwdPath.string()))
    {
        std::cout << "  [OK]   Saved to " << cwdPath.string() << '\n';
        return true;
    }

    std::cout << "  [ERR]  Failed to save " << filename << " (check permissions)\n";
    return false;
}

static void renderEquityPlot(
    const quant::core::Dataset& dataset,
    const quant::backtest::Portfolio& portfolio,
    const std::string& filename,
    const std::string& title)
{
    const auto& equity = portfolio.equityCurve();
    if (equity.empty())
    {
        std::cout << "  [SKIP] Equity curve empty for: " << title << '\n';
        return;
    }

    quant::plot::EquityCurvePlotter::Config cfg;
    cfg.title = title;
    cfg.showDrawdownPanel = true;
    cfg.showStatsBox = true;

    quant::plot::EquityCurvePlotter plotter(cfg);

    if (equity.size() == dataset.size())
    {
        plotter.loadFromDataset(dataset, equity);
    }
    else
    {
        std::vector<quant::plot::EquityPoint> points;
        points.reserve(equity.size());
        for (std::size_t i = 0; i < equity.size(); ++i)
        {
            std::int64_t ts = (i < dataset.size()) ? dataset.candles[i].openTime : 0;
            points.push_back({ts, equity[i]});
        }
        plotter.loadEquityData(points);
    }

    savePlot(filename, [&](const std::string& path) {
        return plotter.renderToFile(path);
    });
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

        std::cout << "[INFO] CWD: " << std::filesystem::current_path().string() << "\n\n";

        // =========================================================
        // 2. LOAD DATA
        // =========================================================
        quant::data::CSVReader reader;
        auto candles = reader.read(csvPath);

        if (candles.empty())
        {
            throw std::runtime_error("CSV file is empty or failed to load.");
        }

        quant::core::Dataset dataset;
        dataset.symbol   = "BTCUSDT";
        dataset.interval = "5m";
        dataset.candles  = candles;

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

        printHeader("LATEST INDICATOR VALUES");
        std::cout << "  " << std::left << std::setw(28) << "EWMA (" + std::to_string(ewmaPeriod) + ")"
                  << ": " << fmt4(lastEWMA) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "RSI Wilder (" + std::to_string(rsiPeriod) + ")"
                  << ": " << fmt(lastRSI) << '\n';
        std::cout << "  " << std::left << std::setw(28) << "Choppiness Index"
                  << ": " << fmt(lastCHOP) << '\n';
        std::cout << '\n';

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

        quant::strategy::BreakoutStrategy breakout(20, 1, 0.001, false);
        BreakoutAdapter breakoutAdapter(breakout);
        quant::backtest::Engine engineBreakout(candles, initialCash);
        engineBreakout.run(breakoutAdapter);
        printBacktestResults("BREAKOUT (Donchian 20)", engineBreakout.portfolio(), initialCash);

        quant::strategy::EmaCrossStrategy emaCross(9, 21, 1, 0.0, false);
        EmaCrossAdapter emaAdapter(emaCross);
        quant::backtest::Engine engineEma(candles, initialCash);
        engineEma.run(emaAdapter);
        printBacktestResults("EMA CROSS (9/21)", engineEma.portfolio(), initialCash);

        quant::strategy::MeanReversionStrategy meanRev(9, 25.0, 75.0, 14, 50.0, 0, false);
        quant::backtest::Engine engineMr(candles, initialCash);
        engineMr.run(meanRev);
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
        std::cout << '\n';

        // =========================================================
        // 7. VISUALIZATION — CANDLESTICK CHART
        // =========================================================
        printHeader("VISUALIZATION: CANDLESTICK CHART");
        {
            quant::plot::CandlestickPlotter::Config cfg;
            cfg.title = dataset.symbol + " " + dataset.interval;
            quant::plot::CandlestickPlotter candlePlotter(cfg);
            candlePlotter.loadDataset(dataset);

            savePlot("output_candlestick.svg", [&](const std::string& path) {
                return candlePlotter.renderToFile(path);
            });
        }
        std::cout << '\n';

        // =========================================================
        // 8. VISUALIZATION — EQUITY CURVES
        // =========================================================
        printHeader("VISUALIZATION: EQUITY CURVES");
        renderEquityPlot(dataset, engineBreakout.portfolio(),
                         "output_equity_breakout.svg", "Breakout Equity Curve");
        renderEquityPlot(dataset, engineEma.portfolio(),
                         "output_equity_emacross.svg", "EMA Cross Equity Curve");
        renderEquityPlot(dataset, engineMr.portfolio(),
                         "output_equity_meanrev.svg", "Mean Reversion Equity Curve");
        std::cout << '\n';

        // =========================================================
        // 9. VISUALIZATION — INDICATOR OVERLAY
        // =========================================================
        printHeader("VISUALIZATION: INDICATOR OVERLAY");
        {
            quant::plot::IndicatorPlotter indPlotter;
            indPlotter.loadDataset(dataset);

            if (!ewmaValues.empty())
            {
                quant::plot::IndicatorSeries ewmaSeries;
                ewmaSeries.name      = "EWMA(" + std::to_string(ewmaPeriod) + ")";
                ewmaSeries.values    = ewmaValues;
                ewmaSeries.type      = quant::plot::IndicatorType::Line;
                ewmaSeries.panel     = quant::plot::IndicatorPanel::Main;
                ewmaSeries.color     = "#f59e0b";
                ewmaSeries.lineWidth = 2.0;
                indPlotter.addIndicator(ewmaSeries);
            }

            if (!rsiValues.empty())
            {
                quant::plot::IndicatorSeries rsiSeries;
                rsiSeries.name    = "RSI(" + std::to_string(rsiPeriod) + ")";
                rsiSeries.values  = rsiValues;
                rsiSeries.type    = quant::plot::IndicatorType::Oscillator;
                rsiSeries.panel   = quant::plot::IndicatorPanel::Sub1;
                rsiSeries.color   = "#8b5cf6";
                rsiSeries.oscMin  = 0.0;
                rsiSeries.oscMax  = 100.0;
                indPlotter.addIndicator(rsiSeries);
            }

            if (!chopValues.empty())
            {
                quant::plot::IndicatorSeries chopSeries;
                chopSeries.name      = "CHOP(" + std::to_string(chopPeriod) + ")";
                chopSeries.values    = chopValues;
                chopSeries.type      = quant::plot::IndicatorType::Line;
                chopSeries.panel     = quant::plot::IndicatorPanel::Sub2;
                chopSeries.color     = "#10b981";
                chopSeries.lineWidth = 1.5;
                indPlotter.addIndicator(chopSeries);
            }

            savePlot("output_indicators.svg", [&](const std::string& path) {
                return indPlotter.renderToFile(path);
            });
        }
        std::cout << '\n';

        printSeparator('=');
        std::cout << "           ALL VISUALIZATIONS EXPORTED\n";
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
