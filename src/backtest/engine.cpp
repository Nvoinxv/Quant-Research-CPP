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

    // Create a temporary Dataset for the strategy batch processing
    quant::core::Dataset dataset;
    dataset.candles = candles_;
    
    // Generate signals efficiently (batch process)
    auto signals = strategy.generate_signals(dataset);
    std::size_t signal_idx = 0;

    for (current_index_ = 0;
         current_index_ < candles_.size();
         ++current_index_)
    {
        quant::strategy::Signal current_signal;
        current_signal.type = quant::strategy::SignalType::HOLD;

        // Advance to current candle's signal if any
        while (signal_idx < signals.size() && signals[signal_idx].index < current_index_)
        {
            ++signal_idx;
        }

        if (signal_idx < signals.size() && signals[signal_idx].index == current_index_)
        {
            current_signal = signals[signal_idx];
            // Don't auto-increment here in case there are multiple signals per candle, 
            // though normally it's 1-to-1, the loop advancement will handle it.
        }

        processCandle(candles_[current_index_], current_signal);
    }
}

void Engine::processCandle(
    const quant::market::Candle& candle,
    const quant::strategy::Signal& signal
)
{
    // 1. Update Market Price so portfolio can calculate Unrealized PnL correctly
    portfolio_.updateMarketPrice(candle.close);

    // 2. Execute Orders based on Strategy Signal
    if (signal.type != quant::strategy::SignalType::HOLD)
    {
        // Fixed quantity of 1.0 for now, as Position Sizing isn't implemented
        double quantity = 1.0;
        
        Side side = (signal.type == quant::strategy::SignalType::BUY) ? Side::Buy : Side::Sell;
        
        // 3. Broker Execution
        auto execution = broker_.executeMarketOrder(side, quantity, candle);

        if (execution.success)
        {
            // 4. Create Order record
            Order order;
            order.id = next_order_id_++;
            order.side = (side == Side::Buy) ? OrderSide::Buy : OrderSide::Sell;
            order.type = OrderType::Market;
            order.status = OrderStatus::Filled;
            order.price = execution.executedPrice;
            order.quantity = execution.quantity;
            order.fee = execution.fee;
            order.timestamp = candle.closeTime;

            // 5. Update Portfolio
            portfolio_.executeOrder(order);
        }
    }
}

} // namespace quant::backtest