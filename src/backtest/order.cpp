#include "backtest/order.hpp"

namespace quant::backtest
{

double Order::value() const noexcept
{
    return price * quantity;
}

bool Order::isBuy() const noexcept
{
    return side == OrderSide::Buy;
}

bool Order::isSell() const noexcept
{
    return side == OrderSide::Sell;
}

bool Order::isFilled() const noexcept
{
    return status == OrderStatus::Filled;
}

} // namespace quant::backtest