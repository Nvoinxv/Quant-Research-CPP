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
        std::size_t width;
        std::size_t height;
        std::string title;

        std::string equityColor;
        std::string areaColor;
        std::string benchmarkColor;
        std::string drawdownColor;
        std::string backgroundColor;
        std::string gridColor;
        std::string textColor;
        std::string axisColor;
        std::string statsBoxBg;

        std::size_t paddingLeft;
        std::size_t paddingRight;
        std::size_t paddingTop;
        std::size_t paddingBottom;

        bool showAreaFill;
        bool showBenchmark;
        bool showDrawdownPanel;
        bool showStatsBox;
        double drawdownPanelRatio;

        std::size_t maxYLabels;
        std::size_t maxTimeLabels;

        Config()
            : width(1200)
            , height(700)
            , title("Equity Curve")
            , equityColor("#2563eb")
            , areaColor("#dbeafe")
            , benchmarkColor("#9ca3af")
            , drawdownColor("#ef5350")
            , backgroundColor("#ffffff")
            , gridColor("#e5e7eb")
            , textColor("#374151")
            , axisColor("#9ca3af")
            , statsBoxBg("#f9fafb")
            , paddingLeft(90)
            , paddingRight(40)
            , paddingTop(60)
            , paddingBottom(100)
            , showAreaFill(true)
            , showBenchmark(false)
            , showDrawdownPanel(true)
            , showStatsBox(true)
            , drawdownPanelRatio(0.18)
            , maxYLabels(8)
            , maxTimeLabels(6)
        {}
    };

    explicit EquityCurvePlotter(const Config& config = Config{});

    void loadEquityData(const std::vector<EquityPoint>& equityData) noexcept;

    void loadFromDataset(const quant::core::Dataset& dataset, const std::vector<double>& equityValues);

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
