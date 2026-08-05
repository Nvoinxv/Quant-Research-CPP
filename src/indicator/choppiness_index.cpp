#include "indicators/choppiness_index.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace quant::indicators
{

void ChoppinessIndex::validateInput(
    const std::vector<quant::market::Candle>& candles,
    std::size_t period
)
{
    if (period < 2)
    {
        throw std::invalid_argument(
            "CHOP period must be at least 2."
        );
    }

    if (candles.size() <= period)
    {
        throw std::invalid_argument(
            "Not enough candle data."
        );
    }
}

double ChoppinessIndex::trueRange(
    const quant::market::Candle& current,
    const quant::market::Candle& previous
)
{
    const double hl =
        current.high - current.low;

    const double hc =
        std::abs(current.high - previous.close);

    const double lc =
        std::abs(current.low - previous.close);

    return std::max(
        {hl, hc, lc}
    );
}

std::vector<double> ChoppinessIndex::calculate(
    const std::vector<quant::market::Candle>& candles,
    std::size_t period
)
{
    validateInput(candles, period);

    std::vector<double> result(
        candles.size(),
        std::numeric_limits<double>::quiet_NaN()
    );

    std::vector<double> tr(
        candles.size(),
        0.0
    );

    tr[0] =
        candles[0].high - candles[0].low;

    for (std::size_t i = 1; i < candles.size(); ++i)
    {
        tr[i] =
            trueRange(
                candles[i],
                candles[i - 1]
            );
    }

    for (std::size_t i = period; i < candles.size(); ++i)
    {
        double sumTR = 0.0;

        double highest =
            candles[i - period + 1].high;

        double lowest =
            candles[i - period + 1].low;

        for (std::size_t j = i - period + 1; j <= i; ++j)
        {
            sumTR += tr[j];

            highest =
                std::max(
                    highest,
                    candles[j].high
                );

            lowest =
                std::min(
                    lowest,
                    candles[j].low
                );
        }

        const double range =
            highest - lowest;

        if (range <= 0.0)
        {
            result[i] = 100.0;
            continue;
        }

        result[i] =
            100.0 *
            std::log10(sumTR / range) /
            std::log10(
                static_cast<double>(period)
            );
    }

    return result;
}

double ChoppinessIndex::last(
    const std::vector<quant::market::Candle>& candles,
    std::size_t period
)
{
    validateInput(candles, period);

    std::vector<double> tr(
        candles.size(),
        0.0
    );

    tr[0] =
        candles[0].high - candles[0].low;

    for (std::size_t i = 1; i < candles.size(); ++i)
    {
        tr[i] =
            trueRange(
                candles[i],
                candles[i - 1]
            );
    }

    double sumTR = 0.0;

    double highest =
        candles[candles.size() - period].high;

    double lowest =
        candles[candles.size() - period].low;

    for (
        std::size_t i = candles.size() - period;
        i < candles.size();
        ++i
    )
    {
        sumTR += tr[i];

        highest =
            std::max(
                highest,
                candles[i].high
            );

        lowest =
            std::min(
                lowest,
                candles[i].low
            );
    }

    const double range =
        highest - lowest;

    if (range <= 0.0)
    {
        return 100.0;
    }

    return
        100.0 *
        std::log10(sumTR / range) /
        std::log10(
            static_cast<double>(period)
        );
}

}