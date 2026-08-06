#pragma once

#include <cstddef>
#include <vector>

#include "core/candle.hpp"
#include "backtest/broker.hpp"
#include "backtest/portofolio.hpp"
#include "strategy/strategy.hpp"

namespace quant::backtest
{

enum class Signal
{
    Hold,
    Buy,
    Sell
};

class Engine
{
public:
    Engine() = default;

    explicit Engine(
        const std::vector<quant::market::Candle>& candles
    );

    Engine(
        const std::vector<quant::market::Candle>& candles,
        double initial_cash
    );

    void setData(
        const std::vector<quant::market::Candle>& candles
    );

    void reset();

    void run(const quant::strategy::Strategy& strategy);

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    const Portfolio& portfolio() const noexcept;

    [[nodiscard]]
    std::size_t size() const noexcept;

    [[nodiscard]]
    std::size_t currentIndex() const noexcept;

    [[nodiscard]]
    const quant::market::Candle& currentCandle() const;

private:
    void processCandle(
        const quant::market::Candle& candle,
        const quant::strategy::Signal& signal
    );

private:
    std::vector<quant::market::Candle> candles_;
    Portfolio portfolio_;
    Broker broker_;
    std::uint64_t next_order_id_{1};
    std::size_t current_index_{0};
};

} // namespace quant::backtest