#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/dataset.hpp"

namespace quant::plot
{

/**
 * @brief Single point in an equity curve.
 */
struct EquityPoint
{
    std::int64_t timestamp = 0; ///< Epoch milliseconds
    double equity = 0.0;
};

/**
 * @brief Generates a professional SVG equity curve chart.
 *
 * Features:
 * - Equity line with optional area fill
 * - Optional benchmark overlay (e.g., buy & hold)
 * - Optional drawdown panel
 * - Stats annotation box (start equity, final equity, return%, max DD%)
 *
 * Pure C++17 implementation with zero external dependencies.
 */
class EquityCurvePlotter
{
public:
    struct Config
    {
        std::size_t width = 1200;
        std::size_t height = 700;
        std::string title = "Equity Curve";

        // Colors
        std::string equityColor = "#2563eb";      ///< Equity line (blue)
        std::string areaColor = "#dbeafe";      ///< Area fill under curve
        std::string benchmarkColor = "#9ca3af";   ///< Benchmark line (gray)
        std::string drawdownColor = "#ef5350";    ///< Drawdown bars (red)
        std::string backgroundColor = "#ffffff";
        std::string gridColor = "#e5e7eb";
        std::string textColor = "#374151";
        std::string axisColor = "#9ca3af";
        std::string statsBoxBg = "#f9fafb";

        // Layout
        std::size_t paddingLeft = 90;
        std::size_t paddingRight = 40;
        std::size_t paddingTop = 60;
        std::size_t paddingBottom = 100;

        // Features
        bool showAreaFill = true;
        bool showBenchmark = false;
        bool showDrawdownPanel = true;
        bool showStatsBox = true;
        double drawdownPanelRatio = 0.18; ///< 18% of plot height for drawdown

        // Labels
        std::size_t maxYLabels = 8;
        std::size_t maxTimeLabels = 6;
    };

    explicit EquityCurvePlotter(const Config& config = Config{});

    /**
     * @brief Load equity points directly (e.g., from Portfolio history).
     */
    void loadEquityData(const std::vector<EquityPoint>& equityData) noexcept;

    /**
     * @brief Convenience: build equity points from a Dataset + equity values.
     *
     * Uses Dataset candle timestamps for the X-axis. The size of
     * \p equityValues must match dataset.size().
     */
    void loadFromDataset(const quant::core::Dataset& dataset, const std::vector<double>& equityValues);

    /**
     * @brief Load optional benchmark data aligned by index with equity data.
     *
     * Typically normalized buy-and-hold values. Must match equity data size.
     */
    void loadBenchmarkData(const std::vector<double>& benchmarkData) noexcept;

    [[nodiscard]] bool renderToFile(const std::string& filepath) const;
    [[nodiscard]] std::string renderToString() const;

private:
    Config config_;
    std::vector<EquityPoint> equityData_;
    std::vector<double> benchmarkData_;

    mutable std::vector<double> drawdowns_;
    mutable bool drawdownsComputed_ = false;

    [[nodiscard]] double mapEquityToY(double equity, double minEquity, double maxEquity, double plotHeight) const noexcept;
    [[nodiscard]] double mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept;
    [[nodiscard]] double mapDrawdownToHeight(double drawdown, double maxDrawdown, double panelHeight) const noexcept;

    [[nodiscard]] std::string escapeXml(std::string_view text) const;
    [[nodiscard]] std::string formatTimestamp(std::int64_t ms) const;
    [[nodiscard]] std::string formatEquity(double equity) const;

    void computeDrawdowns() const;

    void appendSvgHeader(std::string& out) const;
    void appendSvgFooter(std::string& out) const;
    void appendBackground(std::string& out) const;
    void appendTitle(std::string& out) const;
    void appendGridAndAxes(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const;
    void appendEquityArea(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const;
    void appendEquityLine(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const;
    void appendBenchmarkLine(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const;
    void appendDrawdownPanel(std::string& out, double maxDrawdown, double plotWidth, double panelHeight, double panelY) const;
    void appendStatsBox(std::string& out) const;
};

} // namespace quant::plot