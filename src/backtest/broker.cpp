#include "backtest/broker.hpp"

namespace quant::backtest
{

Broker::Broker(
    double tradingFee,
    double slippage
)
    :
    tradingFee_(tradingFee),
    slippage_(slippage)
{
}

Execution Broker::executeMarketOrder(
    Side side,
    double quantity,
    const quant::market::Candle& candle
) const
{
    Execution result;

    result.success = true;
    result.side = side;
    result.quantity = quantity;

    result.requestedPrice = candle.close;

    if (side == Side::Buy)
    {
        result.executedPrice =
            candle.close * (1.0 + slippage_);
    }
    else
    {
        result.executedPrice =
            candle.close * (1.0 - slippage_);
    }

    result.cost =
        result.executedPrice * quantity;

    result.fee =
        result.cost * tradingFee_;

    return result;
}

}