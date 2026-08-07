#include "plot/candlestick.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace quant::plot
{

CandlestickPlotter::CandlestickPlotter(const Config& config)
    : config_(config)
{
}

void CandlestickPlotter::loadDataset(const quant::core::Dataset& dataset) noexcept
{
    dataset_ = dataset;
}

bool CandlestickPlotter::renderToFile(const std::string& filepath) const
{
    const std::string svg = renderToString();
    if (svg.empty())
        return false;

    std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
        return false;

    ofs << svg;
    return ofs.good();
}

std::string CandlestickPlotter::renderToString() const
{
    if (dataset_.empty())
        return {};

    const double plotWidth = static_cast<double>(config_.width - config_.paddingLeft - config_.paddingRight);
    const double plotHeight = static_cast<double>(config_.height - config_.paddingTop - config_.paddingBottom);

    if (plotWidth <= 0.0 || plotHeight <= 0.0)
        return {};

    // Compute price range
    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();
    double maxVolume = 0.0;

    for (const auto& c : dataset_.candles)
    {
        minPrice = std::min(minPrice, c.low);
        maxPrice = std::max(maxPrice, c.high);
        maxVolume = std::max(maxVolume, c.volume);
    }

    // Add small padding to price range
    const double priceRange = maxPrice - minPrice;
    if (priceRange > 0.0)
    {
        minPrice -= priceRange * 0.02;
        maxPrice += priceRange * 0.02;
    }
    else
    {
        // All prices identical
        minPrice -= 1.0;
        maxPrice += 1.0;
    }

    const double mainPlotHeight = config_.showVolume
        ? plotHeight * (1.0 - config_.volumeHeightRatio)
        : plotHeight;

    const double volumePlotHeight = config_.showVolume
        ? plotHeight * config_.volumeHeightRatio
        : 0.0;

    const double volumePlotY = config_.paddingTop + mainPlotHeight + 10.0; // 10px gap

    std::string svg;
    svg.reserve(1024 * 1024); // Pre-allocate ~1MB

    appendSvgHeader(svg);
    appendBackground(svg);
    appendTitle(svg);
    appendGridAndAxes(svg, minPrice, maxPrice, plotWidth, mainPlotHeight);
    appendCandlesticks(svg, minPrice, maxPrice, plotWidth, mainPlotHeight);

    if (config_.showVolume && maxVolume > 0.0)
        appendVolumes(svg, maxVolume, plotWidth, volumePlotHeight, volumePlotY);

    appendSvgFooter(svg);
    return svg;
}

// ---------------------------------------------------------------------
// Coordinate Mapping
// ---------------------------------------------------------------------

double CandlestickPlotter::mapPriceToY(double price, double minPrice, double maxPrice, double plotHeight) const noexcept
{
    if (maxPrice <= minPrice)
        return config_.paddingTop + plotHeight / 2.0;

    const double ratio = (price - minPrice) / (maxPrice - minPrice);
    return config_.paddingTop + (1.0 - ratio) * plotHeight;
}

double CandlestickPlotter::mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept
{
    if (total == 0)
        return config_.paddingLeft;

    const double slotWidth = plotWidth / static_cast<double>(total);
    return config_.paddingLeft + static_cast<double>(index) * slotWidth + slotWidth / 2.0;
}

double CandlestickPlotter::mapVolumeToHeight(double volume, double maxVolume, double volumePlotHeight) const noexcept
{
    if (maxVolume <= 0.0)
        return 0.0;

    return (volume / maxVolume) * volumePlotHeight;
}

// ---------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------

std::string CandlestickPlotter::escapeXml(std::string_view text) const
{
    std::string out;
    out.reserve(text.size());
    for (char ch : text)
    {
        switch (ch)
        {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += ch;       break;
        }
    }
    return out;
}

std::string CandlestickPlotter::formatTimestamp(std::int64_t ms) const
{
    const std::time_t sec = static_cast<std::time_t>(ms / 1000);
    const std::tm* tm = std::localtime(&sec);
    if (!tm)
        return "N/A";

    std::ostringstream oss;
    oss << std::put_time(tm, "%m-%d %H:%M");
    return oss.str();
}

std::string CandlestickPlotter::formatPrice(double price) const
{
    std::ostringstream oss;
    if (price >= 1000.0)
        oss << std::fixed << std::setprecision(2) << price;
    else if (price >= 1.0)
        oss << std::fixed << std::setprecision(4) << price;
    else
        oss << std::fixed << std::setprecision(6) << price;
    return oss.str();
}

// ---------------------------------------------------------------------
// SVG Builders
// ---------------------------------------------------------------------

void CandlestickPlotter::appendSvgHeader(std::string& out) const
{
    out += R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)";
    out += "\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"";
    out += std::to_string(config_.width);
    out += "\" height=\"";
    out += std::to_string(config_.height);
    out += "\" viewBox=\"0 0 ";
    out += std::to_string(config_.width);
    out += " ";
    out += std::to_string(config_.height);
    out += "\">\n";
}

void CandlestickPlotter::appendSvgFooter(std::string& out) const
{
    out += "</svg>\n";
}

void CandlestickPlotter::appendBackground(std::string& out) const
{
    out += "<rect x=\"0\" y=\"0\" width=\"";
    out += std::to_string(config_.width);
    out += "\" height=\"";
    out += std::to_string(config_.height);
    out += "\" fill=\"";
    out += config_.backgroundColor;
    out += "\"/>\n";
}

void CandlestickPlotter::appendTitle(std::string& out) const
{
    if (dataset_.symbol.empty() && config_.title.empty())
        return;

    std::string title = config_.title;
    if (title.empty())
    {
        title = dataset_.symbol;
        if (!dataset_.interval.empty())
            title += " (" + dataset_.interval + ")";
    }

    out += "<text x=\"";
    out += std::to_string(config_.width / 2);
    out += "\" y=\"35\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"18\" font-weight=\"bold\" fill=\"";
    out += config_.textColor;
    out += "\">";
    out += escapeXml(title);
    out += "</text>\n";
}

void CandlestickPlotter::appendGridAndAxes(std::string& out, double minPrice, double maxPrice, double plotWidth, double mainPlotHeight) const
{
    const double left = static_cast<double>(config_.paddingLeft);
    const double right = static_cast<double>(config_.width - config_.paddingRight);
    const double top = static_cast<double>(config_.paddingTop);
    const double bottom = top + mainPlotHeight;

    // Main plot border
    out += "<rect x=\"";
    out += std::to_string(left);
    out += "\" y=\"";
    out += std::to_string(top);
    out += "\" width=\"";
    out += std::to_string(plotWidth);
    out += "\" height=\"";
    out += std::to_string(mainPlotHeight);
    out += "\" fill=\"none\" stroke=\"";
    out += config_.axisColor;
    out += "\" stroke-width=\"1\"/>\n";

    // Horizontal grid lines + price labels
    const std::size_t steps = config_.maxPriceLabels;
    for (std::size_t i = 0; i <= steps; ++i)
    {
        const double ratio = static_cast<double>(i) / static_cast<double>(steps);
        const double price = minPrice + ratio * (maxPrice - minPrice);
        const double y = mapPriceToY(price, minPrice, maxPrice, mainPlotHeight);

        // Grid line
        out += "<line x1=\"";
        out += std::to_string(left);
        out += "\" y1=\"";
        out += std::to_string(y);
        out += "\" x2=\"";
        out += std::to_string(right);
        out += "\" y2=\"";
        out += std::to_string(y);
        out += "\" stroke=\"";
        out += config_.gridColor;
        out += "\" stroke-width=\"1\"/>\n";

        // Price label (left side)
        out += "<text x=\"";
        out += std::to_string(left - 10);
        out += "\" y=\"";
        out += std::to_string(y + 4);
        out += "\" text-anchor=\"end\" font-family=\"Arial, sans-serif\" font-size=\"11\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += formatPrice(price);
        out += "</text>\n";
    }

    // Time labels (bottom)
    const std::size_t n = dataset_.size();
    const std::size_t timeSteps = std::min(config_.maxTimeLabels, n);
    if (timeSteps > 0 && n > 0)
    {
        const std::size_t step = n / timeSteps;
        for (std::size_t i = 0; i < timeSteps; ++i)
        {
            const std::size_t idx = i * step;
            const double x = mapIndexToX(idx, n, plotWidth);
            const std::string label = formatTimestamp(dataset_.candles[idx].openTime);

            out += "<text x=\"";
            out += std::to_string(x);
            out += "\" y=\"";
            out += std::to_string(static_cast<std::size_t>(bottom) + 20);
            out += "\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"10\" fill=\"";
            out += config_.textColor;
            out += "\">";
            out += escapeXml(label);
            out += "</text>\n";
        }

        // Last candle time
        const double lastX = mapIndexToX(n - 1, n, plotWidth);
        const std::string lastLabel = formatTimestamp(dataset_.candles[n - 1].openTime);
        out += "<text x=\"";
        out += std::to_string(lastX);
        out += "\" y=\"";
        out += std::to_string(static_cast<std::size_t>(bottom) + 20);
        out += "\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"10\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += escapeXml(lastLabel);
        out += "</text>\n";
    }
}

void CandlestickPlotter::appendCandlesticks(std::string& out, double minPrice, double maxPrice, double plotWidth, double mainPlotHeight) const
{
    const std::size_t n = dataset_.size();
    if (n == 0)
        return;

    const double slotWidth = plotWidth / static_cast<double>(n);
    const double bodyWidth = std::max(1.0, slotWidth * 0.75);

    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& c = dataset_.candles[i];
        const double xCenter = mapIndexToX(i, n, plotWidth);
        const double yHigh = mapPriceToY(c.high, minPrice, maxPrice, mainPlotHeight);
        const double yLow = mapPriceToY(c.low, minPrice, maxPrice, mainPlotHeight);
        const double yOpen = mapPriceToY(c.open, minPrice, maxPrice, mainPlotHeight);
        const double yClose = mapPriceToY(c.close, minPrice, maxPrice, mainPlotHeight);

        const bool isUp = c.close >= c.open;
        const std::string& color = isUp ? config_.upColor : config_.downColor;

        const double bodyTop = std::min(yOpen, yClose);
        const double bodyBottom = std::max(yOpen, yClose);
        const double bodyH = std::max(1.0, bodyBottom - bodyTop);

        // Wick (high to low)
        out += "<line x1=\"";
        out += std::to_string(xCenter);
        out += "\" y1=\"";
        out += std::to_string(yHigh);
        out += "\" x2=\"";
        out += std::to_string(xCenter);
        out += "\" y2=\"";
        out += std::to_string(yLow);
        out += "\" stroke=\"";
        out += config_.wickColor;
        out += "\" stroke-width=\"1\"/>\n";

        // Body
        out += "<rect x=\"";
        out += std::to_string(xCenter - bodyWidth / 2.0);
        out += "\" y=\"";
        out += std::to_string(bodyTop);
        out += "\" width=\"";
        out += std::to_string(bodyWidth);
        out += "\" height=\"";
        out += std::to_string(bodyH);
        out += "\" fill=\"";
        out += color;
        out += "\" stroke=\"";
        out += color;
        out += "\" stroke-width=\"1\"/>\n";
    }
}

void CandlestickPlotter::appendVolumes(std::string& out, double maxVolume, double plotWidth, double volumePlotHeight, double volumePlotY) const
{
    const std::size_t n = dataset_.size();
    if (n == 0 || maxVolume <= 0.0)
        return;

    const double slotWidth = plotWidth / static_cast<double>(n);
    const double barWidth = std::max(1.0, slotWidth * 0.75);

    // Volume panel border
    const double left = static_cast<double>(config_.paddingLeft);
    out += "<rect x=\"";
    out += std::to_string(left);
    out += "\" y=\"";
    out += std::to_string(volumePlotY);
    out += "\" width=\"";
    out += std::to_string(plotWidth);
    out += "\" height=\"";
    out += std::to_string(volumePlotHeight);
    out += "\" fill=\"none\" stroke=\"";
    out += config_.axisColor;
    out += "\" stroke-width=\"1\"/>\n";

    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& c = dataset_.candles[i];
        const double xCenter = mapIndexToX(i, n, plotWidth);
        const double barH = mapVolumeToHeight(c.volume, maxVolume, volumePlotHeight);
        const double barY = volumePlotY + volumePlotHeight - barH;

        const bool isUp = c.close >= c.open;
        const std::string& color = isUp ? config_.upColor : config_.downColor;

        out += "<rect x=\"";
        out += std::to_string(xCenter - barWidth / 2.0);
        out += "\" y=\"";
        out += std::to_string(barY);
        out += "\" width=\"";
        out += std::to_string(barWidth);
        out += "\" height=\"";
        out += std::to_string(barH);
        out += "\" fill=\"";
        out += color;
        out += "\" opacity=\"0.6\"/>\n";
    }
}

} // namespace quant::plot