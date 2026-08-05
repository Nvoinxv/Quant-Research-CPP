#pragma once

#include <cstddef>
#include <vector>

#include "core/candle.hpp"
#include "backtest/broker.hpp"
#include "backtest/portofolio.hpp"

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

    void setData(
        const std::vector<quant::market::Candle>& candles
    );

    void reset();

    void run();

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    std::size_t size() const noexcept;

    [[nodiscard]]
    std::size_t currentIndex() const noexcept;

    [[nodiscard]]
    const quant::market::Candle& currentCandle() const;

private:
    void processCandle(
        const quant::market::Candle& candle
    );

private:
    std::vector<quant::market::Candle> candles_;
    Portfolio portfolio_;
    Broker broker_{portfolio_};
    std::uint64_t next_order_id_{1};
    std::size_t current_index_{0};
};

} // namespace quant::backtest