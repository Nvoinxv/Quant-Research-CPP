#include "backtest/engine.hpp"

#include <iostream>
#include <stdexcept>

namespace quant::backtest
{

Engine::Engine(
    const std::vector<quant::market::Candle>& candles
)
    :
    candles_(candles),
    portfolio_(),
    broker_()
{
}

void Engine::setData(
    const std::vector<quant::market::Candle>& candles
)
{
    candles_ = candles;
    reset();
}

Engine::Engine(
    const std::vector<quant::market::Candle>& candles,
    double initial_cash
)
    :
    candles_(candles),
    portfolio_(initial_cash),
    broker_()
{
}

const Portfolio&
Engine::portfolio() const noexcept
{
    return portfolio_;
}

void Engine::reset()
{
    current_index_ = 0;

    portfolio_.reset();
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
        Workflow

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

        Saat ini Strategy dan Broker belum
        tersedia sehingga Engine hanya
        memperbarui harga pasar agar equity
        selalu mengikuti harga candle terakhir.
    */

    portfolio_.updateMarketPrice(
        candle.close
    );
}

} // namespace quant::backtest