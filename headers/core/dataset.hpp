#pragma once

#include <string>
#include <vector>

#include "core/candle.hpp"

namespace quant::core
{

struct Dataset
{
    std::string symbol;
    std::string interval;

    std::vector<quant::market::Candle> candles;

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return candles.size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return candles.empty();
    }

    void clear() noexcept
    {
        candles.clear();
    }
};

} // namespace quant::core