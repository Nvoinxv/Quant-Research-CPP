#pragma once

#include <vector>

#include "backtest/order.hpp"

namespace quant::backtest
{

class Portfolio
{
public:

    explicit Portfolio(
        double initial_cash = 10000.0
    );

    void reset();

    void executeOrder(
        const Order& order
    );

    void updateMarketPrice(
        double price
    );

    [[nodiscard]]
    double cash() const noexcept;

    [[nodiscard]]
    double position() const noexcept;

    [[nodiscard]]
    double averagePrice() const noexcept;

    [[nodiscard]]
    double equity() const noexcept;

    [[nodiscard]]
    double unrealizedPnL() const noexcept;

    [[nodiscard]]
    const std::vector<double>& equityCurve() const noexcept;

private:

    void recordEquity();

private:

    double initial_cash_;

    double cash_;

    double position_;

    double average_price_;

    double last_price_;

    std::vector<double> equity_curve_;
};

}