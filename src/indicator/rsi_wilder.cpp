#include "rsi_wilder.hpp"

#include <limits>
#include <stdexcept>

namespace quant::indicators
{

void RSI::validateInput(
    const std::vector<double>& values,
    std::size_t period
)
{
    if (period == 0)
    {
        throw std::invalid_argument(
            "RSI period must be greater than zero."
        );
    }

    if (values.size() <= period)
    {
        throw std::invalid_argument(
            "Not enough price data to calculate RSI."
        );
    }
}

std::vector<double> RSI::calculate(
    const std::vector<double>& values,
    std::size_t period
)
{
    validateInput(values, period);

    std::vector<double> rsi(
        values.size(),
        std::numeric_limits<double>::quiet_NaN()
    );

    double gain = 0.0;
    double loss = 0.0;

    // Initial Average Gain / Loss
    for (std::size_t i = 1; i <= period; ++i)
    {
        const double delta = values[i] - values[i - 1];

        if (delta > 0.0)
        {
            gain += delta;
        }
        else
        {
            loss -= delta;
        }
    }

    double averageGain = gain / static_cast<double>(period);
    double averageLoss = loss / static_cast<double>(period);

    if (averageLoss == 0.0)
    {
        rsi[period] = 100.0;
    }
    else
    {
        const double rs = averageGain / averageLoss;
        rsi[period] = 100.0 - (100.0 / (1.0 + rs));
    }

    // Wilder Smoothing
    for (std::size_t i = period + 1; i < values.size(); ++i)
    {
        const double delta = values[i] - values[i - 1];

        const double currentGain = delta > 0.0 ? delta : 0.0;
        const double currentLoss = delta < 0.0 ? -delta : 0.0;

        averageGain =
            ((averageGain * (period - 1)) + currentGain)
            / static_cast<double>(period);

        averageLoss =
            ((averageLoss * (period - 1)) + currentLoss)
            / static_cast<double>(period);

        if (averageLoss == 0.0)
        {
            rsi[i] = 100.0;
            continue;
        }

        const double rs = averageGain / averageLoss;

        rsi[i] =
            100.0 - (100.0 / (1.0 + rs));
    }

    return rsi;
}

double RSI::last(
    const std::vector<double>& values,
    std::size_t period
)
{
    validateInput(values, period);

    double gain = 0.0;
    double loss = 0.0;

    for (std::size_t i = 1; i <= period; ++i)
    {
        const double delta = values[i] - values[i - 1];

        if (delta > 0.0)
        {
            gain += delta;
        }
        else
        {
            loss -= delta;
        }
    }

    double averageGain = gain / static_cast<double>(period);
    double averageLoss = loss / static_cast<double>(period);

    for (std::size_t i = period + 1; i < values.size(); ++i)
    {
        const double delta = values[i] - values[i - 1];

        const double currentGain = delta > 0.0 ? delta : 0.0;
        const double currentLoss = delta < 0.0 ? -delta : 0.0;

        averageGain =
            ((averageGain * (period - 1)) + currentGain)
            / static_cast<double>(period);

        averageLoss =
            ((averageLoss * (period - 1)) + currentLoss)
            / static_cast<double>(period);
    }

    if (averageLoss == 0.0)
    {
        return 100.0;
    }

    const double rs = averageGain / averageLoss;

    return 100.0 - (100.0 / (1.0 + rs));
}

} // namespace quant::indicators