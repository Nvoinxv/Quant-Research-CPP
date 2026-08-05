#pragma once

#include <cstdint>

#include "core/candle.hpp"

namespace quant::backtest
{

enum class Side
{
    Buy,
    Sell
};

struct Execution
{
    bool success = false;

    Side side = Side::Buy;

    double requestedPrice = 0.0;
    double executedPrice = 0.0;

    double quantity = 0.0;

    double fee = 0.0;

    double cost = 0.0;
};

class Broker
{
public:

    Broker(
        double tradingFee = 0.001,
        double slippage = 0.0005
    );

    Execution executeMarketOrder(
        Side side,
        double quantity,
        const quant::market::Candle& candle
    ) const;

private:

    double tradingFee_;
    double slippage_;
};

}