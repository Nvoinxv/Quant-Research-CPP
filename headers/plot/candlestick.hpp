#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/dataset.hpp"

namespace quant::plot
{

/**
 * @brief Generates a professional SVG candlestick chart from a Dataset.
 *
 * Pure C++17 implementation with zero external dependencies.
 * Output is a self-contained SVG file viewable in any modern browser.
 */
class CandlestickPlotter
{
public:
    struct Config
    {
        std::size_t width;
        std::size_t height;
        std::string title;

        std::string upColor;
        std::string downColor;
        std::string wickColor;
        std::string backgroundColor;
        std::string gridColor;
        std::string textColor;
        std::string axisColor;

        std::size_t paddingLeft;
        std::size_t paddingRight;
        std::size_t paddingTop;
        std::size_t paddingBottom;

        bool showVolume;
        double volumeHeightRatio;

        std::size_t maxPriceLabels;
        std::size_t maxTimeLabels;

        Config()
            : width(1200)
            , height(800)
            , title("Candlestick Chart")
            , upColor("#26a69a")
            , downColor("#ef5350")
            , wickColor("#374151")
            , backgroundColor("#ffffff")
            , gridColor("#e5e7eb")
            , textColor("#374151")
            , axisColor("#9ca3af")
            , paddingLeft(80)
            , paddingRight(40)
            , paddingTop(60)
            , paddingBottom(100)
            , showVolume(true)
            , volumeHeightRatio(0.20)
            , maxPriceLabels(8)
            , maxTimeLabels(6)
        {}
    };

    explicit CandlestickPlotter(const Config& config = Config{});

    /**
     * @brief Load dataset to be plotted.
     */
    void loadDataset(const quant::core::Dataset& dataset) noexcept;

    /**
     * @brief Render chart to an SVG file.
     * @return true on success, false on failure.
     */
    [[nodiscard]] bool renderToFile(const std::string& filepath) const;

    /**
     * @brief Render chart to an SVG string.
     */
    [[nodiscard]] std::string renderToString() const;

private:
    Config config_;
    quant::core::Dataset dataset_;

    // Coordinate mapping
    [[nodiscard]] double mapPriceToY(double price, double minPrice, double maxPrice, double plotHeight) const noexcept;
    [[nodiscard]] double mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept;
    [[nodiscard]] double mapVolumeToHeight(double volume, double maxVolume, double volumePlotHeight) const noexcept;

    // Utilities
    [[nodiscard]] std::string escapeXml(std::string_view text) const;
    [[nodiscard]] std::string formatTimestamp(std::int64_t ms) const;
    [[nodiscard]] std::string formatPrice(double price) const;

    // SVG builders
    void appendSvgHeader(std::string& out) const;
    void appendSvgFooter(std::string& out) const;
    void appendBackground(std::string& out) const;
    void appendTitle(std::string& out) const;
    void appendGridAndAxes(std::string& out, double minPrice, double maxPrice, double plotWidth, double mainPlotHeight) const;
    void appendCandlesticks(std::string& out, double minPrice, double maxPrice, double plotWidth, double mainPlotHeight) const;
    void appendVolumes(std::string& out, double maxVolume, double plotWidth, double volumePlotHeight, double volumePlotY) const;
};

} // namespace quant::plot
