#include "plot/indicator_plot.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>

namespace quant::plot
{

IndicatorPlotter::IndicatorPlotter(const Config& config)
    : config_(config)
{
}

void IndicatorPlotter::loadDataset(const quant::core::Dataset& dataset) noexcept
{
    dataset_ = dataset;
}

void IndicatorPlotter::addIndicator(const IndicatorSeries& indicator)
{
    if (indicator.values.size() == dataset_.size())
        indicators_.push_back(indicator);
}

bool IndicatorPlotter::renderToFile(const std::string& filepath) const
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

std::string IndicatorPlotter::renderToString() const
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

    const double priceRange = maxPrice - minPrice;
    if (priceRange > 0.0)
    {
        minPrice -= priceRange * 0.02;
        maxPrice += priceRange * 0.02;
    }
    else
    {
        minPrice -= 1.0;
        maxPrice += 1.0;
    }

    const PanelGeometry mainPanel = computeMainPanel(plotHeight);
    const PanelGeometry volPanel = computeVolumePanel(mainPanel);
    const PanelGeometry sub1Panel = computeSubPanel1(plotHeight);
    const PanelGeometry sub2Panel = computeSubPanel2(plotHeight);

    std::string svg;
    svg.reserve(2 * 1024 * 1024); // ~2MB

    appendSvgHeader(svg);
    appendBackground(svg);
    appendTitle(svg);

    // Main panel
    appendPanelBorder(svg, mainPanel);
    appendGridAndPriceLabels(svg, minPrice, maxPrice, mainPanel);
    appendTimeLabels(svg, plotWidth, mainPanel.y + mainPanel.height);
    appendCandlesticks(svg, minPrice, maxPrice, plotWidth, mainPanel);

    if (config_.showVolume && maxVolume > 0.0)
    {
        appendPanelBorder(svg, volPanel);
        appendVolumes(svg, maxVolume, plotWidth, volPanel);
    }

    // Main panel overlays (Line, Band, Dot, Area)
    for (const auto& ind : indicators_)
    {
        if (ind.panel != IndicatorPanel::Main || ind.values.size() != dataset_.size())
            continue;

        double minVal = *std::min_element(ind.values.begin(), ind.values.end());
        double maxVal = *std::max_element(ind.values.begin(), ind.values.end());
        if (ind.type == IndicatorType::Band && !ind.bandUpper.empty())
        {
            minVal = std::min(minVal, *std::min_element(ind.bandUpper.begin(), ind.bandUpper.end()));
            maxVal = std::max(maxVal, *std::max_element(ind.bandUpper.begin(), ind.bandUpper.end()));
        }
        const double range = maxVal - minVal;
        if (range > 0.0) { minVal -= range * 0.02; maxVal += range * 0.02; }
        else { minVal -= 1.0; maxVal += 1.0; }

        switch (ind.type)
        {
            case IndicatorType::Line:   appendIndicatorLine(svg, ind, minVal, maxVal, plotWidth, mainPanel); break;
            case IndicatorType::Band:   appendIndicatorBand(svg, ind, minVal, maxVal, plotWidth, mainPanel); break;
            case IndicatorType::Dot:    appendIndicatorDots(svg, ind, minVal, maxVal, plotWidth, mainPanel); break;
            case IndicatorType::Area:   appendIndicatorArea(svg, ind, minVal, maxVal, plotWidth, mainPanel); break;
            default: break;
        }
    }

    // Sub-panel 1
    if (sub1Panel.height > 0.0)
    {
        appendPanelBorder(svg, sub1Panel);
        appendTimeLabels(svg, plotWidth, sub1Panel.y + sub1Panel.height);

        for (const auto& ind : indicators_)
        {
            if (ind.panel != IndicatorPanel::Sub1 || ind.values.size() != dataset_.size())
                continue;

            if (ind.type == IndicatorType::Oscillator)
                appendIndicatorOscillator(svg, ind, plotWidth, sub1Panel);
            else if (ind.type == IndicatorType::Histogram)
            {
                double minVal = *std::min_element(ind.values.begin(), ind.values.end());
                double maxVal = *std::max_element(ind.values.begin(), ind.values.end());
                const double range = maxVal - minVal;
                if (range > 0.0) { minVal -= range * 0.02; maxVal += range * 0.02; }
                else { minVal -= 1.0; maxVal += 1.0; }
                appendIndicatorHistogram(svg, ind, minVal, maxVal, plotWidth, sub1Panel);
            }
            else if (ind.type == IndicatorType::Line)
            {
                double minVal = *std::min_element(ind.values.begin(), ind.values.end());
                double maxVal = *std::max_element(ind.values.begin(), ind.values.end());
                const double range = maxVal - minVal;
                if (range > 0.0) { minVal -= range * 0.02; maxVal += range * 0.02; }
                else { minVal -= 1.0; maxVal += 1.0; }
                appendIndicatorLine(svg, ind, minVal, maxVal, plotWidth, sub1Panel);
            }
        }
    }

    // Sub-panel 2
    if (sub2Panel.height > 0.0)
    {
        appendPanelBorder(svg, sub2Panel);
        appendTimeLabels(svg, plotWidth, sub2Panel.y + sub2Panel.height);

        for (const auto& ind : indicators_)
        {
            if (ind.panel != IndicatorPanel::Sub2 || ind.values.size() != dataset_.size())
                continue;

            if (ind.type == IndicatorType::Oscillator)
                appendIndicatorOscillator(svg, ind, plotWidth, sub2Panel);
            else if (ind.type == IndicatorType::Histogram)
            {
                double minVal = *std::min_element(ind.values.begin(), ind.values.end());
                double maxVal = *std::max_element(ind.values.begin(), ind.values.end());
                const double range = maxVal - minVal;
                if (range > 0.0) { minVal -= range * 0.02; maxVal += range * 0.02; }
                else { minVal -= 1.0; maxVal += 1.0; }
                appendIndicatorHistogram(svg, ind, minVal, maxVal, plotWidth, sub2Panel);
            }
            else if (ind.type == IndicatorType::Line)
            {
                double minVal = *std::min_element(ind.values.begin(), ind.values.end());
                double maxVal = *std::max_element(ind.values.begin(), ind.values.end());
                const double range = maxVal - minVal;
                if (range > 0.0) { minVal -= range * 0.02; maxVal += range * 0.02; }
                else { minVal -= 1.0; maxVal += 1.0; }
                appendIndicatorLine(svg, ind, minVal, maxVal, plotWidth, sub2Panel);
            }
        }
    }

    if (config_.showLegend)
        appendLegend(svg);

    appendSvgFooter(svg);
    return svg;
}

// ---------------------------------------------------------------------
// Panel Geometry
// ---------------------------------------------------------------------

IndicatorPlotter::PanelGeometry IndicatorPlotter::computeMainPanel(double totalPlotHeight) const noexcept
{
    const double gap = 8.0;
    const double h = totalPlotHeight * config_.mainPanelRatio - gap;
    return PanelGeometry{
        static_cast<double>(config_.paddingLeft),
        static_cast<double>(config_.paddingTop),
        static_cast<double>(config_.width - config_.paddingLeft - config_.paddingRight),
        std::max(50.0, h)
    };
}

IndicatorPlotter::PanelGeometry IndicatorPlotter::computeSubPanel1(double totalPlotHeight) const noexcept
{
    const double gap = 8.0;
    const PanelGeometry main = computeMainPanel(totalPlotHeight);
    const double h = totalPlotHeight * config_.subPanel1Ratio - gap;
    return PanelGeometry{
        static_cast<double>(config_.paddingLeft),
        main.y + main.height + gap,
        static_cast<double>(config_.width - config_.paddingLeft - config_.paddingRight),
        std::max(40.0, h)
    };
}

IndicatorPlotter::PanelGeometry IndicatorPlotter::computeSubPanel2(double totalPlotHeight) const noexcept
{
    const double gap = 8.0;
    const PanelGeometry sub1 = computeSubPanel1(totalPlotHeight);
    const double h = totalPlotHeight * config_.subPanel2Ratio - gap;
    return PanelGeometry{
        static_cast<double>(config_.paddingLeft),
        sub1.y + sub1.height + gap,
        static_cast<double>(config_.width - config_.paddingLeft - config_.paddingRight),
        std::max(40.0, h)
    };
}

IndicatorPlotter::PanelGeometry IndicatorPlotter::computeVolumePanel(const PanelGeometry& main) const noexcept
{
    const double h = main.height * config_.volumeHeightRatio;
    return PanelGeometry{
        main.x,
        main.y + main.height - h,
        main.width,
        h
    };
}

// ---------------------------------------------------------------------
// Coordinate Mapping
// ---------------------------------------------------------------------

double IndicatorPlotter::mapPriceToY(double price, double minPrice, double maxPrice, const PanelGeometry& panel) const noexcept
{
    if (maxPrice <= minPrice)
        return panel.y + panel.height / 2.0;
    const double ratio = (price - minPrice) / (maxPrice - minPrice);
    return panel.y + (1.0 - ratio) * panel.height;
}

double IndicatorPlotter::mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept
{
    if (total == 0)
        return config_.paddingLeft;
    const double slotWidth = plotWidth / static_cast<double>(total);
    return config_.paddingLeft + static_cast<double>(index) * slotWidth + slotWidth / 2.0;
}

double IndicatorPlotter::mapValueToY(double value, double minVal, double maxVal, const PanelGeometry& panel) const noexcept
{
    if (maxVal <= minVal)
        return panel.y + panel.height / 2.0;
    const double ratio = (value - minVal) / (maxVal - minVal);
    return panel.y + (1.0 - ratio) * panel.height;
}

double IndicatorPlotter::mapVolumeToHeight(double volume, double maxVolume, double volHeight) const noexcept
{
    if (maxVolume <= 0.0)
        return 0.0;
    return (volume / maxVolume) * volHeight;
}

// ---------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------

std::string IndicatorPlotter::escapeXml(std::string_view text) const
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

std::string IndicatorPlotter::formatTimestamp(std::int64_t ms) const
{
    const std::time_t sec = static_cast<std::time_t>(ms / 1000);
    const std::tm* tm = std::localtime(&sec);
    if (!tm)
        return "N/A";
    std::ostringstream oss;
    oss << std::put_time(tm, "%m-%d %H:%M");
    return oss.str();
}

std::string IndicatorPlotter::formatPrice(double price) const
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

void IndicatorPlotter::appendSvgHeader(std::string& out) const
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

void IndicatorPlotter::appendSvgFooter(std::string& out) const
{
    out += "</svg>\n";
}

void IndicatorPlotter::appendBackground(std::string& out) const
{
    out += "<rect x=\"0\" y=\"0\" width=\"";
    out += std::to_string(config_.width);
    out += "\" height=\"";
    out += std::to_string(config_.height);
    out += "\" fill=\"";
    out += config_.backgroundColor;
    out += "\"/>\n";
}

void IndicatorPlotter::appendTitle(std::string& out) const
{
    std::string title = config_.title;
    if (title.empty() && !dataset_.symbol.empty())
    {
        title = dataset_.symbol;
        if (!dataset_.interval.empty())
            title += " (" + dataset_.interval + ")";
    }
    if (title.empty())
        return;

    out += "<text x=\"";
    out += std::to_string(config_.width / 2);
    out += "\" y=\"35\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"18\" font-weight=\"bold\" fill=\"";
    out += config_.textColor;
    out += "\">";
    out += escapeXml(title);
    out += "</text>\n";
}

void IndicatorPlotter::appendPanelBorder(std::string& out, const PanelGeometry& panel) const
{
    out += "<rect x=\"";
    out += std::to_string(panel.x);
    out += "\" y=\"";
    out += std::to_string(panel.y);
    out += "\" width=\"";
    out += std::to_string(panel.width);
    out += "\" height=\"";
    out += std::to_string(panel.height);
    out += "\" fill=\"none\" stroke=\"";
    out += config_.axisColor;
    out += "\" stroke-width=\"1\"/>\n";
}

void IndicatorPlotter::appendGridAndPriceLabels(std::string& out, double minPrice, double maxPrice, const PanelGeometry& panel) const
{
    const double left = panel.x;
    const double right = panel.x + panel.width;

    const std::size_t steps = config_.maxPriceLabels;
    for (std::size_t i = 0; i <= steps; ++i)
    {
        const double ratio = static_cast<double>(i) / static_cast<double>(steps);
        const double price = minPrice + ratio * (maxPrice - minPrice);
        const double y = mapPriceToY(price, minPrice, maxPrice, panel);

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
}

void IndicatorPlotter::appendTimeLabels(std::string& out, double plotWidth, double bottomY) const
{
    const std::size_t n = dataset_.size();
    const std::size_t timeSteps = std::min(config_.maxTimeLabels, n);
    if (timeSteps == 0 || n == 0)
        return;

    const std::size_t step = n / timeSteps;
    for (std::size_t i = 0; i < timeSteps; ++i)
    {
        const std::size_t idx = i * step;
        const double x = mapIndexToX(idx, n, plotWidth);
        const std::string label = formatTimestamp(dataset_.candles[idx].openTime);

        out += "<text x=\"";
        out += std::to_string(x);
        out += "\" y=\"";
        out += std::to_string(static_cast<std::size_t>(bottomY) + 20);
        out += "\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"10\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += escapeXml(label);
        out += "</text>\n";
    }

    const double lastX = mapIndexToX(n - 1, n, plotWidth);
    const std::string lastLabel = formatTimestamp(dataset_.candles[n - 1].openTime);
    out += "<text x=\"";
    out += std::to_string(lastX);
    out += "\" y=\"";
    out += std::to_string(static_cast<std::size_t>(bottomY) + 20);
    out += "\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"10\" fill=\"";
    out += config_.textColor;
    out += "\">";
    out += escapeXml(lastLabel);
    out += "</text>\n";
}

void IndicatorPlotter::appendCandlesticks(std::string& out, double minPrice, double maxPrice, double plotWidth, const PanelGeometry& panel) const
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
        const double yHigh = mapPriceToY(c.high, minPrice, maxPrice, panel);
        const double yLow = mapPriceToY(c.low, minPrice, maxPrice, panel);
        const double yOpen = mapPriceToY(c.open, minPrice, maxPrice, panel);
        const double yClose = mapPriceToY(c.close, minPrice, maxPrice, panel);

        const bool isUp = c.close >= c.open;
        const std::string& color = isUp ? config_.upColor : config_.downColor;

        const double bodyTop = std::min(yOpen, yClose);
        const double bodyBottom = std::max(yOpen, yClose);
        const double bodyH = std::max(1.0, bodyBottom - bodyTop);

        // Wick
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

void IndicatorPlotter::appendVolumes(std::string& out, double maxVolume, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = dataset_.size();
    if (n == 0 || maxVolume <= 0.0)
        return;

    const double slotWidth = plotWidth / static_cast<double>(n);
    const double barWidth = std::max(1.0, slotWidth * 0.75);

    for (std::size_t i = 0; i < n; ++i)
    {
        const auto& c = dataset_.candles[i];
        const double xCenter = mapIndexToX(i, n, plotWidth);
        const double barH = mapVolumeToHeight(c.volume, maxVolume, panel.height);
        const double barY = panel.y + panel.height - barH;

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
        out += "\" opacity=\"0.5\"/>\n";
    }
}

// ---------------------------------------------------------------------
// Indicator Renderers
// ---------------------------------------------------------------------

void IndicatorPlotter::appendIndicatorLine(std::string& out, const IndicatorSeries& ind,
    double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n < 2)
        return;

    std::string path;
    path.reserve(n * 32);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapValueToY(ind.values[i], minVal, maxVal, panel);
        path += (i == 0 ? "M" : "L");
        path += std::to_string(x);
        path += ",";
        path += std::to_string(y);
        path += " ";
    }

    out += "<path d=\"";
    out += path;
    out += "\" fill=\"none\" stroke=\"";
    out += ind.color;
    out += "\" stroke-width=\"";
    out += std::to_string(ind.lineWidth);
    out += "\" stroke-linejoin=\"round\" opacity=\"";
    out += std::to_string(ind.opacity);
    out += "\"/>\n";
}

void IndicatorPlotter::appendIndicatorHistogram(std::string& out, const IndicatorSeries& ind,
    double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n == 0)
        return;

    const double slotWidth = plotWidth / static_cast<double>(n);
    const double barWidth = std::max(1.0, slotWidth * 0.70);
    const double zeroY = mapValueToY(0.0, minVal, maxVal, panel);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double xCenter = mapIndexToX(i, n, plotWidth);
        const double yVal = mapValueToY(ind.values[i], minVal, maxVal, panel);
        const double barH = std::abs(yVal - zeroY);
        const double barY = std::min(yVal, zeroY);

        out += "<rect x=\"";
        out += std::to_string(xCenter - barWidth / 2.0);
        out += "\" y=\"";
        out += std::to_string(barY);
        out += "\" width=\"";
        out += std::to_string(barWidth);
        out += "\" height=\"";
        out += std::to_string(barH);
        out += "\" fill=\"";
        out += ind.color;
        out += "\" opacity=\"";
        out += std::to_string(ind.opacity);
        out += "\"/>\n";
    }
}

void IndicatorPlotter::appendIndicatorOscillator(std::string& out, const IndicatorSeries& ind,
    double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n < 2)
        return;

    // Grid lines for oscillator bounds
    const double yMin = mapValueToY(ind.oscMin, ind.oscMin, ind.oscMax, panel);
    const double yMax = mapValueToY(ind.oscMax, ind.oscMin, ind.oscMax, panel);
    const double yMid = mapValueToY((ind.oscMin + ind.oscMax) / 2.0, ind.oscMin, ind.oscMax, panel);

    const double left = panel.x;
    const double right = panel.x + panel.width;

    // Bound lines
    auto appendHLine = [&](double y, const std::string& color, const std::string& label)
    {
        out += "<line x1=\"";
        out += std::to_string(left);
        out += "\" y1=\"";
        out += std::to_string(y);
        out += "\" x2=\"";
        out += std::to_string(right);
        out += "\" y2=\"";
        out += std::to_string(y);
        out += "\" stroke=\"";
        out += color;
        out += "\" stroke-width=\"1\" stroke-dasharray=\"4,3\"/>\n";

        out += "<text x=\"";
        out += std::to_string(left - 8);
        out += "\" y=\"";
        out += std::to_string(y + 4);
        out += "\" text-anchor=\"end\" font-family=\"Arial, sans-serif\" font-size=\"9\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += label;
        out += "</text>\n";
    };

    appendHLine(yMax, config_.gridColor, std::to_string(static_cast<int>(ind.oscMax)));
    appendHLine(yMid, config_.gridColor, std::to_string(static_cast<int>((ind.oscMin + ind.oscMax) / 2.0)));
    appendHLine(yMin, config_.gridColor, std::to_string(static_cast<int>(ind.oscMin)));

    // Overbought/oversold thresholds (RSI style: 70/30)
    if (ind.oscMin == 0.0 && ind.oscMax == 100.0)
    {
        const double y70 = mapValueToY(70.0, 0.0, 100.0, panel);
        const double y30 = mapValueToY(30.0, 0.0, 100.0, panel);
        appendHLine(y70, "#ef5350", "70");
        appendHLine(y30, "#26a69a", "30");
    }

    // Line path
    std::string path;
    path.reserve(n * 32);
    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapValueToY(ind.values[i], ind.oscMin, ind.oscMax, panel);
        path += (i == 0 ? "M" : "L");
        path += std::to_string(x);
        path += ",";
        path += std::to_string(y);
        path += " ";
    }

    out += "<path d=\"";
    out += path;
    out += "\" fill=\"none\" stroke=\"";
    out += ind.color;
    out += "\" stroke-width=\"";
    out += std::to_string(ind.lineWidth);
    out += "\" stroke-linejoin=\"round\" opacity=\"";
    out += std::to_string(ind.opacity);
    out += "\"/>\n";
}

void IndicatorPlotter::appendIndicatorBand(std::string& out, const IndicatorSeries& ind,
    double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n < 2 || ind.bandUpper.size() != n)
        return;

    // Fill area between upper and lower
    if (!ind.fillColor.empty())
    {
        std::string points;
        points.reserve(n * 64);

        // Upper line forward
        for (std::size_t i = 0; i < n; ++i)
        {
            const double x = mapIndexToX(i, n, plotWidth);
            const double y = mapValueToY(ind.bandUpper[i], minVal, maxVal, panel);
            points += (i == 0 ? "" : " ");
            points += std::to_string(x);
            points += ",";
            points += std::to_string(y);
        }

        // Lower line backward
        for (std::size_t i = n; i-- > 0;)
        {
            const double x = mapIndexToX(i, n, plotWidth);
            const double y = mapValueToY(ind.values[i], minVal, maxVal, panel);
            points += " ";
            points += std::to_string(x);
            points += ",";
            points += std::to_string(y);
        }

        out += "<polygon points=\"";
        out += points;
        out += "\" fill=\"";
        out += ind.fillColor;
        out += "\" opacity=\"0.2\"/>\n";
    }

    // Upper line
    IndicatorSeries upper = ind;
    upper.values = ind.bandUpper;
    appendIndicatorLine(out, upper, minVal, maxVal, plotWidth, panel);

    // Lower line
    IndicatorSeries lower = ind;
    lower.values = ind.values;
    appendIndicatorLine(out, lower, minVal, maxVal, plotWidth, panel);
}

void IndicatorPlotter::appendIndicatorDots(std::string& out, const IndicatorSeries& ind,
    double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n == 0)
        return;

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapValueToY(ind.values[i], minVal, maxVal, panel);

        out += "<circle cx=\"";
        out += std::to_string(x);
        out += "\" cy=\"";
        out += std::to_string(y);
        out += "\" r=\"3\" fill=\"";
        out += ind.color;
        out += "\" opacity=\"";
        out += std::to_string(ind.opacity);
        out += "\"/>\n";
    }
}

void IndicatorPlotter::appendIndicatorArea(std::string& out, const IndicatorSeries& ind,
    double minVal, double maxVal, double plotWidth, const PanelGeometry& panel) const
{
    const std::size_t n = ind.values.size();
    if (n < 2)
        return;

    const double bottomY = mapValueToY(minVal, minVal, maxVal, panel);

    std::string points;
    points.reserve(n * 64);

    // Start at bottom-left
    points += std::to_string(mapIndexToX(0, n, plotWidth));
    points += ",";
    points += std::to_string(bottomY);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapValueToY(ind.values[i], minVal, maxVal, panel);
        points += " ";
        points += std::to_string(x);
        points += ",";
        points += std::to_string(y);
    }

    // Close at bottom-right
    points += " ";
    points += std::to_string(mapIndexToX(n - 1, n, plotWidth));
    points += ",";
    points += std::to_string(bottomY);

    out += "<polygon points=\"";
    out += points;
    out += "\" fill=\"";
    out += ind.fillColor.empty() ? ind.color : ind.fillColor;
    out += "\" opacity=\"";
    out += std::to_string(ind.opacity * 0.3);
    out += "\"/>\n";

    // Also draw line on top
    appendIndicatorLine(out, ind, minVal, maxVal, plotWidth, panel);
}

void IndicatorPlotter::appendZeroLine(std::string& out, const PanelGeometry& panel, double minVal, double maxVal) const
{
    if (minVal > 0.0 || maxVal < 0.0)
        return;

    const double y = mapValueToY(0.0, minVal, maxVal, panel);
    out += "<line x1=\"";
    out += std::to_string(panel.x);
    out += "\" y1=\"";
    out += std::to_string(y);
    out += "\" x2=\"";
    out += std::to_string(panel.x + panel.width);
    out += "\" y2=\"";
    out += std::to_string(y);
    out += "\" stroke=\"";
    out += config_.axisColor;
    out += "\" stroke-width=\"1\" stroke-dasharray=\"4,3\"/>\n";
}

void IndicatorPlotter::appendLegend(std::string& out) const
{
    if (indicators_.empty())
        return;

    const std::size_t itemH = 22;
    const std::size_t boxW = config_.legendItemWidth;
    const std::size_t boxH = indicators_.size() * itemH + 10;
    const std::size_t boxX = config_.width - config_.paddingRight - boxW - 10;
    const std::size_t boxY = config_.paddingTop + 10;

    out += "<rect x=\"";
    out += std::to_string(boxX);
    out += "\" y=\"";
    out += std::to_string(boxY);
    out += "\" width=\"";
    out += std::to_string(boxW);
    out += "\" height=\"";
    out += std::to_string(boxH);
    out += "\" fill=\"";
    out += config_.legendBg;
    out += "\" stroke=\"";
    out += config_.gridColor;
    out += "\" stroke-width=\"1\" rx=\"3\"/>\n";

    for (std::size_t i = 0; i < indicators_.size(); ++i)
    {
        const auto& ind = indicators_[i];
        const std::size_t ty = boxY + 18 + i * itemH;

        // Color swatch
        out += "<rect x=\"";
        out += std::to_string(boxX + 8);
        out += "\" y=\"";
        out += std::to_string(ty - 8);
        out += "\" width=\"12\" height=\"12\" fill=\"";
        out += ind.color;
        out += "\" rx=\"2\"/>\n";

        // Name + panel
        std::string label = ind.name;
        if (ind.panel == IndicatorPanel::Sub1)
            label += " [S1]";
        else if (ind.panel == IndicatorPanel::Sub2)
            label += " [S2]";

        out += "<text x=\"";
        out += std::to_string(boxX + 26);
        out += "\" y=\"";
        out += std::to_string(ty);
        out += "\" font-family=\"Arial, sans-serif\" font-size=\"11\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += escapeXml(label);
        out += "</text>\n";
    }
}

} // namespace quant::plot