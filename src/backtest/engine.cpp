#include "backtest/engine.hpp"

#include <iostream>
#include <stdexcept>

namespace quant::backtest
{

Engine::Engine(
    const std::vector<quant::market::Candle>& candles
)
    : candles_(candles)
{
}

void Engine::setData(
    const std::vector<quant::market::Candle>& candles
)
{
    candles_ = candles;
    reset();
}

void Engine::reset()
{
    current_index_ = 0;
}

bool Engine::empty() const noexcept
{
    return candles_.empty();
}

std::size_t Engine::size() const noexcept
{
    return candles_.size();
}

std::size_t Engine::currentIndex() const noexcept
{
    return current_index_;
}

const quant::market::Candle&
Engine::currentCandle() const
{
    if (empty())
    {
        throw std::runtime_error(
            "Backtest engine has no candle data."
        );
    }

    return candles_.at(current_index_);
}

void Engine::run()
{
    if (empty())
    {
        throw std::runtime_error(
            "Cannot run backtest because dataset is empty."
        );
    }

    reset();

    for (current_index_ = 0;
         current_index_ < candles_.size();
         ++current_index_)
    {
        processCandle(
            candles_[current_index_]
        );
    }
}

void Engine::processCandle(
    const quant::market::Candle& candle
)
{
    /*
        Workflow Engine

        Candle
            ↓
        Strategy
            ↓
        Signal
            ↓
        Broker
            ↓
        Order
            ↓
        Portfolio
            ↓
        Equity Curve

        Untuk sementara fungsi ini hanya
        menjadi placeholder sampai seluruh
        modul backtest selesai dibuat.
    */

    (void)candle;
}

} // namespace quant::backtest