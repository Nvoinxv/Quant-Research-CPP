#include "plot/equity_curver.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>

namespace quant::plot
{

EquityCurvePlotter::EquityCurvePlotter(const Config& config)
    : config_(config)
{
}

void EquityCurvePlotter::loadEquityData(const std::vector<EquityPoint>& equityData) noexcept
{
    equityData_ = equityData;
    drawdownsComputed_ = false;
    drawdowns_.clear();
}

void EquityCurvePlotter::loadFromDataset(const quant::core::Dataset& dataset, const std::vector<double>& equityValues)
{
    if (dataset.size() != equityValues.size())
        return;

    equityData_.clear();
    equityData_.reserve(dataset.size());

    for (std::size_t i = 0; i < dataset.size(); ++i)
    {
        equityData_.push_back(EquityPoint{
            dataset.candles[i].openTime,
            equityValues[i]
        });
    }

    drawdownsComputed_ = false;
    drawdowns_.clear();
}

void EquityCurvePlotter::loadBenchmarkData(const std::vector<double>& benchmarkData) noexcept
{
    benchmarkData_ = benchmarkData;
}

bool EquityCurvePlotter::renderToFile(const std::string& filepath) const
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

std::string EquityCurvePlotter::renderToString() const
{
    if (equityData_.size() < 2)
        return {};

    const double plotWidth = static_cast<double>(config_.width - config_.paddingLeft - config_.paddingRight);
    const double plotHeight = static_cast<double>(config_.height - config_.paddingTop - config_.paddingBottom);

    if (plotWidth <= 0.0 || plotHeight <= 0.0)
        return {};

    // Compute equity range
    double minEquity = std::numeric_limits<double>::max();
    double maxEquity = std::numeric_limits<double>::lowest();

    for (const auto& pt : equityData_)
    {
        minEquity = std::min(minEquity, pt.equity);
        maxEquity = std::max(maxEquity, pt.equity);
    }

    // Include benchmark in scale if present
    if (config_.showBenchmark && benchmarkData_.size() == equityData_.size())
    {
        for (double b : benchmarkData_)
        {
            minEquity = std::min(minEquity, b);
            maxEquity = std::max(maxEquity, b);
        }
    }

    // Add padding
    const double equityRange = maxEquity - minEquity;
    if (equityRange > 0.0)
    {
        minEquity -= equityRange * 0.05;
        maxEquity += equityRange * 0.05;
    }
    else
    {
        minEquity -= 1.0;
        maxEquity += 1.0;
    }

    const double mainPlotHeight = config_.showDrawdownPanel
        ? plotHeight * (1.0 - config_.drawdownPanelRatio)
        : plotHeight;

    const double ddPanelHeight = config_.showDrawdownPanel
        ? plotHeight * config_.drawdownPanelRatio
        : 0.0;

    const double ddPanelY = config_.paddingTop + mainPlotHeight + 10.0; // 10px gap

    computeDrawdowns();
    const double maxDrawdown = drawdowns_.empty() ? 0.0 : *std::max_element(drawdowns_.begin(), drawdowns_.end());

    std::string svg;
    svg.reserve(1024 * 1024);

    appendSvgHeader(svg);
    appendBackground(svg);
    appendTitle(svg);
    appendGridAndAxes(svg, minEquity, maxEquity, plotWidth, mainPlotHeight);

    if (config_.showAreaFill)
        appendEquityArea(svg, minEquity, maxEquity, plotWidth, mainPlotHeight);

    if (config_.showBenchmark && benchmarkData_.size() == equityData_.size())
        appendBenchmarkLine(svg, minEquity, maxEquity, plotWidth, mainPlotHeight);

    appendEquityLine(svg, minEquity, maxEquity, plotWidth, mainPlotHeight);

    if (config_.showDrawdownPanel && !drawdowns_.empty() && maxDrawdown > 0.0)
        appendDrawdownPanel(svg, maxDrawdown, plotWidth, ddPanelHeight, ddPanelY);

    if (config_.showStatsBox)
        appendStatsBox(svg);

    appendSvgFooter(svg);
    return svg;
}

// ---------------------------------------------------------------------
// Coordinate Mapping
// ---------------------------------------------------------------------

double EquityCurvePlotter::mapEquityToY(double equity, double minEquity, double maxEquity, double plotHeight) const noexcept
{
    if (maxEquity <= minEquity)
        return config_.paddingTop + plotHeight / 2.0;

    const double ratio = (equity - minEquity) / (maxEquity - minEquity);
    return config_.paddingTop + (1.0 - ratio) * plotHeight;
}

double EquityCurvePlotter::mapIndexToX(std::size_t index, std::size_t total, double plotWidth) const noexcept
{
    if (total == 0)
        return config_.paddingLeft;

    const double slotWidth = plotWidth / static_cast<double>(total);
    return config_.paddingLeft + static_cast<double>(index) * slotWidth + slotWidth / 2.0;
}

double EquityCurvePlotter::mapDrawdownToHeight(double drawdown, double maxDrawdown, double panelHeight) const noexcept
{
    if (maxDrawdown <= 0.0)
        return 0.0;

    return (drawdown / maxDrawdown) * panelHeight;
}

// ---------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------

std::string EquityCurvePlotter::escapeXml(std::string_view text) const
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

std::string EquityCurvePlotter::formatTimestamp(std::int64_t ms) const
{
    const std::time_t sec = static_cast<std::time_t>(ms / 1000);
    const std::tm* tm = std::localtime(&sec);
    if (!tm)
        return "N/A";

    std::ostringstream oss;
    oss << std::put_time(tm, "%m-%d %H:%M");
    return oss.str();
}

std::string EquityCurvePlotter::formatEquity(double equity) const
{
    std::ostringstream oss;
    if (std::abs(equity) >= 1'000'000.0)
        oss << std::fixed << std::setprecision(0) << equity;
    else if (std::abs(equity) >= 10'000.0)
        oss << std::fixed << std::setprecision(1) << equity;
    else if (std::abs(equity) >= 1.0)
        oss << std::fixed << std::setprecision(2) << equity;
    else
        oss << std::fixed << std::setprecision(6) << equity;
    return oss.str();
}

void EquityCurvePlotter::computeDrawdowns() const
{
    if (drawdownsComputed_ || equityData_.empty())
        return;

    drawdowns_.resize(equityData_.size());
    double peak = equityData_[0].equity;

    for (std::size_t i = 0; i < equityData_.size(); ++i)
    {
        peak = std::max(peak, equityData_[i].equity);
        if (peak > 0.0)
            drawdowns_[i] = ((peak - equityData_[i].equity) / peak) * 100.0;
        else
            drawdowns_[i] = 0.0;
    }

    drawdownsComputed_ = true;
}

// ---------------------------------------------------------------------
// SVG Builders
// ---------------------------------------------------------------------

void EquityCurvePlotter::appendSvgHeader(std::string& out) const
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

void EquityCurvePlotter::appendSvgFooter(std::string& out) const
{
    out += "</svg>\n";
}

void EquityCurvePlotter::appendBackground(std::string& out) const
{
    out += "<rect x=\"0\" y=\"0\" width=\"";
    out += std::to_string(config_.width);
    out += "\" height=\"";
    out += std::to_string(config_.height);
    out += "\" fill=\"";
    out += config_.backgroundColor;
    out += "\"/>\n";
}

void EquityCurvePlotter::appendTitle(std::string& out) const
{
    std::string title = config_.title;
    if (title.empty())
        title = "Equity Curve";

    out += "<text x=\"";
    out += std::to_string(config_.width / 2);
    out += "\" y=\"35\" text-anchor=\"middle\" font-family=\"Arial, sans-serif\" font-size=\"18\" font-weight=\"bold\" fill=\"";
    out += config_.textColor;
    out += "\">";
    out += escapeXml(title);
    out += "</text>\n";
}

void EquityCurvePlotter::appendGridAndAxes(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const
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

    // Horizontal grid + equity labels
    const std::size_t steps = config_.maxYLabels;
    for (std::size_t i = 0; i <= steps; ++i)
    {
        const double ratio = static_cast<double>(i) / static_cast<double>(steps);
        const double equity = minEquity + ratio * (maxEquity - minEquity);
        const double y = mapEquityToY(equity, minEquity, maxEquity, mainPlotHeight);

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
        out += formatEquity(equity);
        out += "</text>\n";
    }

    // Time labels
    const std::size_t n = equityData_.size();
    const std::size_t timeSteps = std::min(config_.maxTimeLabels, n);
    if (timeSteps > 0 && n > 0)
    {
        const std::size_t step = n / timeSteps;
        for (std::size_t i = 0; i < timeSteps; ++i)
        {
            const std::size_t idx = i * step;
            const double x = mapIndexToX(idx, n, plotWidth);
            const std::string label = formatTimestamp(equityData_[idx].timestamp);

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

        // Last point label
        const double lastX = mapIndexToX(n - 1, n, plotWidth);
        const std::string lastLabel = formatTimestamp(equityData_[n - 1].timestamp);
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

void EquityCurvePlotter::appendEquityArea(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const
{
    const std::size_t n = equityData_.size();
    if (n < 2)
        return;

    const double bottomY = mapEquityToY(minEquity, minEquity, maxEquity, mainPlotHeight);

    std::string points;
    points.reserve(n * 32);

    // Start at bottom-left
    points += std::to_string(mapIndexToX(0, n, plotWidth));
    points += ",";
    points += std::to_string(bottomY);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapEquityToY(equityData_[i].equity, minEquity, maxEquity, mainPlotHeight);
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
    out += config_.areaColor;
    out += "\" opacity=\"0.5\"/>\n";
}

void EquityCurvePlotter::appendEquityLine(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const
{
    const std::size_t n = equityData_.size();
    if (n < 2)
        return;

    std::string path;
    path.reserve(n * 32);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapEquityToY(equityData_[i].equity, minEquity, maxEquity, mainPlotHeight);
        path += (i == 0 ? "M" : "L");
        path += std::to_string(x);
        path += ",";
        path += std::to_string(y);
        path += " ";
    }

    out += "<path d=\"";
    out += path;
    out += "\" fill=\"none\" stroke=\"";
    out += config_.equityColor;
    out += "\" stroke-width=\"2\" stroke-linejoin=\"round\"/>\n";
}

void EquityCurvePlotter::appendBenchmarkLine(std::string& out, double minEquity, double maxEquity, double plotWidth, double mainPlotHeight) const
{
    const std::size_t n = benchmarkData_.size();
    if (n < 2 || n != equityData_.size())
        return;

    std::string path;
    path.reserve(n * 32);

    for (std::size_t i = 0; i < n; ++i)
    {
        const double x = mapIndexToX(i, n, plotWidth);
        const double y = mapEquityToY(benchmarkData_[i], minEquity, maxEquity, mainPlotHeight);
        path += (i == 0 ? "M" : "L");
        path += std::to_string(x);
        path += ",";
        path += std::to_string(y);
        path += " ";
    }

    out += "<path d=\"";
    out += path;
    out += "\" fill=\"none\" stroke=\"";
    out += config_.benchmarkColor;
    out += "\" stroke-width=\"1.5\" stroke-dasharray=\"6,4\" stroke-linejoin=\"round\"/>\n";
}

void EquityCurvePlotter::appendDrawdownPanel(std::string& out, double maxDrawdown, double plotWidth, double panelHeight, double panelY) const
{
    const std::size_t n = drawdowns_.size();
    if (n == 0 || panelHeight <= 0.0)
        return;

    const double left = static_cast<double>(config_.paddingLeft);
    const double slotWidth = plotWidth / static_cast<double>(n);
    const double barWidth = std::max(1.0, slotWidth * 0.80);

    // Panel border
    out += "<rect x=\"";
    out += std::to_string(left);
    out += "\" y=\"";
    out += std::to_string(panelY);
    out += "\" width=\"";
    out += std::to_string(plotWidth);
    out += "\" height=\"";
    out += std::to_string(panelHeight);
    out += "\" fill=\"none\" stroke=\"";
    out += config_.axisColor;
    out += "\" stroke-width=\"1\"/>\n";

    // Label
    out += "<text x=\"";
    out += std::to_string(left - 5);
    out += "\" y=\"";
    out += std::to_string(static_cast<std::size_t>(panelY) + 12);
    out += "\" text-anchor=\"end\" font-family=\"Arial, sans-serif\" font-size=\"10\" fill=\"";
    out += config_.textColor;
    out += "\">DD%</text>\n";

    for (std::size_t i = 0; i < n; ++i)
    {
        const double xCenter = mapIndexToX(i, n, plotWidth);
        const double barH = mapDrawdownToHeight(drawdowns_[i], maxDrawdown, panelHeight);
        const double barY = panelY + panelHeight - barH;

        out += "<rect x=\"";
        out += std::to_string(xCenter - barWidth / 2.0);
        out += "\" y=\"";
        out += std::to_string(barY);
        out += "\" width=\"";
        out += std::to_string(barWidth);
        out += "\" height=\"";
        out += std::to_string(barH);
        out += "\" fill=\"";
        out += config_.drawdownColor;
        out += "\" opacity=\"0.7\"/>\n";
    }
}

void EquityCurvePlotter::appendStatsBox(std::string& out) const
{
    if (equityData_.size() < 2)
        return;

    const double startEquity = equityData_.front().equity;
    const double finalEquity = equityData_.back().equity;
    const double totalReturn = (startEquity > 0.0)
        ? ((finalEquity - startEquity) / startEquity) * 100.0
        : 0.0;

    computeDrawdowns();
    const double maxDD = drawdowns_.empty() ? 0.0 : *std::max_element(drawdowns_.begin(), drawdowns_.end());

    const std::size_t boxW = 200;
    const std::size_t boxH = 95;
    const std::size_t boxX = config_.width - config_.paddingRight - boxW - 10;
    const std::size_t boxY = config_.paddingTop + 10;

    // Background
    out += "<rect x=\"";
    out += std::to_string(boxX);
    out += "\" y=\"";
    out += std::to_string(boxY);
    out += "\" width=\"";
    out += std::to_string(boxW);
    out += "\" height=\"";
    out += std::to_string(boxH);
    out += "\" fill=\"";
    out += config_.statsBoxBg;
    out += "\" stroke=\"";
    out += config_.gridColor;
    out += "\" stroke-width=\"1\" rx=\"4\"/>\n";

    const std::size_t textX = boxX + 10;
    const std::size_t lineH = 18;
    std::size_t ty = boxY + 18;

    auto appendLine = [&](const std::string& label, const std::string& value, const std::string& color)
    {
        out += "<text x=\"";
        out += std::to_string(textX);
        out += "\" y=\"";
        out += std::to_string(ty);
        out += "\" font-family=\"Arial, sans-serif\" font-size=\"12\" fill=\"";
        out += config_.textColor;
        out += "\">";
        out += escapeXml(label);
        out += "</text>\n";

        out += "<text x=\"";
        out += std::to_string(boxX + boxW - 10);
        out += "\" y=\"";
        out += std::to_string(ty);
        out += "\" text-anchor=\"end\" font-family=\"Arial, sans-serif\" font-size=\"12\" font-weight=\"bold\" fill=\"";
        out += color;
        out += "\">";
        out += escapeXml(value);
        out += "</text>\n";

        ty += lineH;
    };

    appendLine("Start Equity", formatEquity(startEquity), config_.textColor);
    appendLine("Final Equity", formatEquity(finalEquity), config_.textColor);

    const std::string retStr = (totalReturn >= 0.0 ? "+" : "") + formatEquity(totalReturn) + "%";
    const std::string retColor = totalReturn >= 0.0 ? "#059669" : "#dc2626";
    appendLine("Total Return", retStr, retColor);

    const std::string ddStr = "-" + formatEquity(maxDD) + "%";
    appendLine("Max Drawdown", ddStr, config_.drawdownColor);
}

} // namespace quant::plot