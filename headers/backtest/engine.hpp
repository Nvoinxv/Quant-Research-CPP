#pragma once

#include <cstddef>
#include <vector>

#include "core/candle.hpp"

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

    std::size_t current_index_{0};
};

} // namespace quant::backtest