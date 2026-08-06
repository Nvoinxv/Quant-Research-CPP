#pragma once

#include <cstddef>
#include <vector>

#include "core/candle.hpp"

namespace quant::indicators
{

class ChoppinessIndex
{
public:
    /**
     * @brief Menghitung seluruh nilai Choppiness Index.
     *
     * @param candles Data OHLC.
     * @param period Periode CHOP (default 14).
     * @return Vector nilai CHOP.
     */
    static std::vector<double> calculate(
        const std::vector<quant::market::Candle>& candles,
        std::size_t period = 14
    );

    /**
     * @brief Menghitung nilai CHOP terakhir.
     */
    static double last(
        const std::vector<quant::market::Candle>& candles,
        std::size_t period = 14
    );

private:
    static double trueRange(
        const quant::market::Candle& current,
        const quant::market::Candle& previous
    );

    static void validateInput(
        const std::vector<quant::market::Candle>& candles,
        std::size_t period
    );
};

}