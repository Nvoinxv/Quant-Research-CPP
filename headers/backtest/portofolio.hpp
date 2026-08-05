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

}#include "backtest/portofolio.hpp"

namespace quant::backtest
{

Portfolio::Portfolio(
    double initial_cash
)
    :
    initial_cash_(initial_cash),
    cash_(initial_cash),
    position_(0.0),
    average_price_(0.0),
    last_price_(0.0)
{
    recordEquity();
}

void Portfolio::reset()
{
    cash_ = initial_cash_;

    position_ = 0.0;

    average_price_ = 0.0;

    last_price_ = 0.0;

    equity_curve_.clear();

    recordEquity();
}

void Portfolio::executeOrder(
    const Order& order
)
{
    if (order.isBuy())
    {
        cash_ -= order.value() + order.fee;

        average_price_ = order.price;

        position_ += order.quantity;
    }
    else
    {
        cash_ += order.value() - order.fee;

        position_ -= order.quantity;

        if (position_ <= 0.0)
        {
            position_ = 0.0;

            average_price_ = 0.0;
        }
    }

    last_price_ = order.price;

    recordEquity();
}

void Portfolio::updateMarketPrice(
    double price
)
{
    last_price_ = price;

    recordEquity();
}

double Portfolio::cash() const noexcept
{
    return cash_;
}

double Portfolio::position() const noexcept
{
    return position_;
}

double Portfolio::averagePrice() const noexcept
{
    return average_price_;
}

double Portfolio::equity() const noexcept
{
    return cash_ + (position_ * last_price_);
}

double Portfolio::unrealizedPnL() const noexcept
{
    return (last_price_ - average_price_) * position_;
}

const std::vector<double>&
Portfolio::equityCurve() const noexcept
{
    return equity_curve_;
}

void Portfolio::recordEquity()
{
    equity_curve_.push_back(
        equity()
    );
}

}