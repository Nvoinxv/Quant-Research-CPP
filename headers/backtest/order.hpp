#pragma once

#include <cstdint>

namespace quant::backtest
{

enum class OrderSide
{
    Buy,
    Sell
};

enum class OrderType
{
    Market,
    Limit
};

enum class OrderStatus
{
    Pending,
    Filled,
    Cancelled
};

struct Order
{
    std::uint64_t id{0};

    OrderSide side{OrderSide::Buy};

    OrderType type{OrderType::Market};

    OrderStatus status{OrderStatus::Pending};

    double price{0.0};

    double quantity{0.0};

    double fee{0.0};

    std::int64_t timestamp{0};

    [[nodiscard]]
    double value() const noexcept;

    [[nodiscard]]
    bool isBuy() const noexcept;

    [[nodiscard]]
    bool isSell() const noexcept;

    [[nodiscard]]
    bool isFilled() const noexcept;
};

} // namespace quant::backtest