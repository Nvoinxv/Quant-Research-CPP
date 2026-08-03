#pragma once

#include <cstdint>
#include <string>

namespace quant::market
{

struct Candle
{
    std::int64_t openTime = 0;
    std::int64_t closeTime = 0;

    std::string symbol;

    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;

    double volume = 0.0;

    double quoteVolume = 0.0;

    std::uint64_t tradeCount = 0;

    double takerBuyBaseVolume = 0.0;
    double takerBuyQuoteVolume = 0.0;
};

} // namespace quant::market