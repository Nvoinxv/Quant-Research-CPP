#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/dataset.hpp"

namespace quant::plot
{

/**
 * @brief Visual representation type for an indicator series.
 */
enum class IndicatorType
{
    Line,       ///< Overlay line on main price chart (e.g., EWMA, EMA)
    Histogram,  ///< Bar histogram in sub-panel (e.g., Volume-like indicator)
    Oscillator, ///< Oscillator in sub-panel with fixed range (e.g., RSI 0-100)
    Band,       ///< Upper/lower band (e.g., Bollinger Bands, ATR bands)
    Dot,        ///< Scatter dots on main chart (e.g., entry/exit signals)
    Area        ///< Filled area under line (e.g., MACD histogram)
};

/**
 * @brief Target panel for rendering the indicator.
 */
enum class IndicatorPanel
{
    Main, ///< Overlay on the candlestick price chart
    Sub1, ///< First sub-panel below main chart
    Sub2, ///< Second sub-panel (if needed)
};

/**
 * @brief Configuration for a single indicator series to be plotted.
 */
struct IndicatorSeries
{
    std::string name;              ///< Display name (for legend)
    std::vector<double> values;    ///< Must align 1:1 with Dataset candles
    IndicatorType type = IndicatorType::Line;
    IndicatorPanel panel = IndicatorPanel::Main;

    std::string color = "#2563eb"; ///< Line/bar color
    std::string fillColor;         ///< For Area/Band fill (empty = no fill)
    double lineWidth = 1.5;        ///< Stroke width for Line/Band/Dot
    double opacity = 1.0;          ///< 0.0 - 1.0

    // Oscillator-specific bounds (ignored for other types)
    double oscMin = 0.0;
    double oscMax = 100.0;

    // Band-specific second series (upper band values; values = lower band)
    std::vector<double> bandUpper; ///< Only used when type == Band
};

/**
 * @brief Generates a professional SVG chart with candlesticks + indicator overlays.
 *
 * Supports:
 * - Multiple line/histogram/oscillator indicators
 * - Auto-scaled sub-panels for oscillators
 * - Legend with color swatches
 * - Zero-dependency pure C++17 SVG output
 */
class IndicatorPlotter
{
public:
    struct Config
    {
        std::size_t width = 1400;
        std::size_t height = 900;
        std::string title = "Price & Indicators";

        // Colors
        std::string upColor = "#26a69a";
        std::string downColor = "#ef5350";
        std::string wickColor = "#374151";
        std::string backgroundColor = "#ffffff";
        std::string gridColor = "#e5e7eb";
        std::string textColor = "#374151";
        std::string axisColor = "#9ca3af";
        std::string legendBg = "#f9fafb";
        std::string crosshairColor = "#9ca3af";

        // Layout
        std::size_t paddingLeft = 80;
        std::size_t paddingRight = 50;
        std::size_t paddingTop = 60;
        std::size_t paddingBottom = 100;

        // Sub-panel heights (ratios of remaining height after main chart)
        double mainPanelRatio = 0.55;
        double subPanel1Ratio = 0.22;
        double subPanel2Ratio = 0.23;

        // Volume
        bool showVolume = true;
        double volumeHeightRatio = 0.12; // Within main panel

        // Legend
        bool showLegend = true;
        std::size_t legendItemWidth = 140;

        // Labels
        std::size_t maxPriceLabels = 8;
        std::size_t maxTimeLabels = 6;
    };

    explicit IndicatorPlotter(const Config& config = Config{});

    void loadDataset(const quant::core::Dataset& dataset) noexcept;
    void addIndicator(const IndicatorSeries& indicator);

    [[nodiscard]] bool renderToFile(const std::string& filepath) const;
    [[nodiscard]] std::string renderToString() const;

private:
    Config config_;
    quant::core::Dataset dataset_;
    std::vector<IndicatorSeries> indicators_;

    struct PanelGeometry
    {
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    [[nodiscard]] PanelGeometry computeMainPanel(double totalPlotHeight) const noexcept;
    [[nodiscard]] PanelGeometry computeSubPanel1(double totalPlotHeight) const noexcept;
    [[nodiscard]] PanelGeometry computeSubPanel2(double totalPlotHeight) const noexcept;
    [[nodiscard]] PanelGeometry computeVolumePanel(const PanelGeometry& main) const noexcept;

    [[nodiscard]] double mapPriceToY(double price, double minPrice, double maxPrice, const PanelGeometry& panel) const noexcept;
    [[nodiscard]] double mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept;
    [[nodiscard]] double mapValueToY(double value, double minVal, double maxVal, const PanelGeometry& panel) const noexcept;
    [[nodiscard]] double mapVolumeToHeight(double volume, double maxVolume, double volHeight) const noexcept;

    [[nodiscard]] std::string escapeXml(std::string_view text) const;
    [[nodiscard]] std::string formatTimestamp(std::int64_t ms) const;
    [[nodiscard]] std::string formatPrice(double price) const;

    void appendSvgHeader(std::string& out) const;
    void appendSvgFooter(std::string& out) const;
    void appendBackground(std::string& out) const;
    void appendTitle(std::string& out) const;

    void appendPanelBorder(std::string& out, const PanelGeometry& panel) const;
    void appendGridAndPriceLabels(std::string& out, double minPrice, double maxPrice, const PanelGeometry& panel) const;
    void appendTimeLabels(std::string& out, double plotWidth, double bottomY) const;

    void appendCandlesticks(std::string& out, double minPrice, double maxPrice, double plotWidth, const PanelGeometry& mainPanel) const;
    void appendVolumes(std::string& out, double maxVolume, double plotWidth, const PanelGeometry& volPanel) const;

    void appendIndicatorLine(std::string& out, const IndicatorSeries& ind, double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const;
    void appendIndicatorHistogram(std::string& out, const IndicatorSeries& ind, double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const;
    void appendIndicatorOscillator(std::string& out, const IndicatorSeries& ind, double plotWidth, const PanelGeometry& panel) const;
    void appendIndicatorBand(std::string& out, const IndicatorSeries& ind, double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const;
    void appendIndicatorDots(std::string& out, const IndicatorSeries& ind, double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const;
    void appendIndicatorArea(std::string& out, const IndicatorSeries& ind, double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const;

    void appendLegend(std::string& out) const;
    void appendZeroLine(std::string& out, const PanelGeometry& panel, double minVal, double maxVal) const;
};

} // namespace quant::plot