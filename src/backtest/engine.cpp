#include "backtest/engine.hpp"
#include "core/dataset.hpp"

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

void Engine::run(const quant::strategy::Strategy& strategy)
{
    if (empty())
    {
        throw std::runtime_error(
            "Cannot run backtest because dataset is empty."
        );
    }

    reset();

    // Build Dataset untuk strategi (batch processing)
    quant::core::Dataset dataset;
    dataset.symbol   = "BACKTEST";
    dataset.interval = "5m";
    dataset.candles  = candles_;

    // Generate semua sinyal sekali (O(n))
    auto signals = strategy.generate_signals(dataset);

    std::size_t signal_idx = 0;

    for (current_index_ = 0;
         current_index_ < candles_.size();
         ++current_index_)
    {
        quant::strategy::Signal current_signal;
        current_signal.type = quant::strategy::SignalType::HOLD;

        // Advance ke signal yang cocok dengan bar saat ini
        while (signal_idx < signals.size() && 
               signals[signal_idx].index < current_index_)
        {
            ++signal_idx;
        }

        if (signal_idx < signals.size() && 
            signals[signal_idx].index == current_index_)
        {
            current_signal = signals[signal_idx];
        }

        processCandle(candles_[current_index_], current_signal);
    }
}

void Engine::processCandle(
    const quant::market::Candle& candle,
    const quant::strategy::Signal& signal
)
{
    // 1. Update unrealized PnL portfolio
    portfolio_.updateMarketPrice(candle.close);

    // 2. Eksekusi order kalau ada sinyal
    if (signal.type != quant::strategy::SignalType::HOLD)
    {
        // TODO: Position sizing bisa pakai signal.confidence
        double quantity = 1.0;
        
        Side side = (signal.type == quant::strategy::SignalType::BUY) 
                    ? Side::Buy 
                    : Side::Sell;
        
        // 3. Eksekusi via Broker (simulasi slippage + fee)
        auto execution = broker_.executeMarketOrder(side, quantity, candle);

        if (execution.success)
        {
            // 4. Buat record Order
            Order order;
            order.id = next_order_id_++;
            order.side = (side == Side::Buy) ? OrderSide::Buy : OrderSide::Sell;
            order.type = OrderType::Market;
            order.status = OrderStatus::Filled;
            order.price = execution.executedPrice;
            order.quantity = execution.quantity;
            order.fee = execution.fee;
            order.timestamp = candle.closeTime;

            // 5. Update Portfolio (cash, position, avg price, equity curve)
            portfolio_.executeOrder(order);
        }
    }
}

} // namespace quant::backtest