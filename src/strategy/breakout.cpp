#include "strategy/breakout.hpp"
#include "core/dataset.hpp"
#include "core/candle.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cmath>

namespace quant::strategy {

BreakoutStrategy::BreakoutStrategy(std::size_t lookback_period,
                                   std::size_t confirmation_bars,
                                   double breakout_threshold_pct,
                                   bool use_volume_confirmation)
    : lookback_period_(lookback_period),
      confirmation_bars_(confirmation_bars),
      breakout_threshold_pct_(breakout_threshold_pct),
      use_volume_confirmation_(use_volume_confirmation) {
    
    if (lookback_period_ == 0) {
        throw std::invalid_argument("BreakoutStrategy: lookback_period harus > 0");
    }
    if (breakout_threshold_pct_ < 0.0) {
        throw std::invalid_argument("BreakoutStrategy: breakout_threshold_pct tidak boleh negatif");
    }
}

std::vector<Signal> BreakoutStrategy::generate_signals(const core::Dataset& dataset) const {
    std::vector<Signal> signals;
    const std::size_t min_required = lookback_period_ + confirmation_bars_;
    
    if (dataset.empty() || dataset.size() <= min_required) {
        return signals; // Dataset terlalu pendek
    }
    
    // Reserve perkiraan kapasitas (breakout jarang terjadi, ~5-10% dari data)
    signals.reserve(dataset.size() / lookback_period_);
    
    for (std::size_t i = min_required; i < dataset.size(); ++i) {
        Signal sig = generate_signal_at(dataset, i);
        if (sig.type != SignalType::HOLD) {
            signals.push_back(sig);
        }
    }
    
    return signals;
}

Signal BreakoutStrategy::generate_signal_at(const core::Dataset& dataset,
                                            std::size_t index) const {
    const std::size_t min_required = lookback_period_ + confirmation_bars_;
    
    if (index < min_required || index >= dataset.size()) {
        return Signal{SignalType::HOLD, index, 0.0, 0.0, "Data tidak cukup atau index invalid"};
    }
    
    // Channel dihitung dari data historis SEBELUM confirmation period
    const std::size_t channel_end = index - confirmation_bars_;
    auto [support, resistance] = calculate_channel(dataset, channel_end);
    
    const market::Candle& current = dataset.candles[index];
    Signal signal{SignalType::HOLD, index, current.close, 0.0, ""};
    
    // --- Breakout ke atas (BUY) ---
    if (is_breakout_above(dataset, index, resistance)) {
        if (!use_volume_confirmation_ || confirm_volume(dataset, index)) {
            signal.type = SignalType::BUY;
            signal.confidence = compute_confidence(current.close, resistance);
            signal.reason = "Breakout above resistance " + std::to_string(resistance);
        }
    }
    // --- Breakout ke bawah (SELL) ---
    else if (is_breakout_below(dataset, index, support)) {
        if (!use_volume_confirmation_ || confirm_volume(dataset, index)) {
            signal.type = SignalType::SELL;
            signal.confidence = compute_confidence(current.close, support);
            signal.reason = "Breakout below support " + std::to_string(support);
        }
    }
    
    return signal;
}

std::pair<double, double> BreakoutStrategy::calculate_channel(const core::Dataset& dataset,
                                                               std::size_t end_index) const {
    if (end_index + 1 < lookback_period_) {
        throw std::invalid_argument("BreakoutStrategy: data tidak cukup untuk menghitung channel");
    }
    
    const std::size_t start = end_index - lookback_period_ + 1;
    double highest_high = dataset.candles[start].high;
    double lowest_low = dataset.candles[start].low;
    
    for (std::size_t i = start + 1; i <= end_index; ++i) {
        highest_high = std::max(highest_high, dataset.candles[i].high);
        lowest_low = std::min(lowest_low, dataset.candles[i].low);
    }
    
    return {lowest_low, highest_high};
}

bool BreakoutStrategy::is_breakout_above(const core::Dataset& dataset,
                                         std::size_t index,
                                         double resistance) const {
    const double threshold = resistance * (1.0 + breakout_threshold_pct_);
    return dataset.candles[index].close > threshold;
}

bool BreakoutStrategy::is_breakout_below(const core::Dataset& dataset,
                                         std::size_t index,
                                         double support) const {
    const double threshold = support * (1.0 - breakout_threshold_pct_);
    return dataset.candles[index].close < threshold;
}

bool BreakoutStrategy::confirm_volume(const core::Dataset& dataset,
                                      std::size_t index) const {
    if (index < lookback_period_) {
        return false;
    }
    
    double sum_volume = 0.0;
    for (std::size_t i = index - lookback_period_; i < index; ++i) {
        sum_volume += dataset.candles[i].volume;
    }
    const double avg_volume = sum_volume / static_cast<double>(lookback_period_);
    
    // Breakout valid kalau volume di atas rata-rata (konfirmasi partisipasi pasar)
    return dataset.candles[index].volume > avg_volume;
}

double BreakoutStrategy::compute_confidence(double price, double level) const {
    if (level == 0.0) return 0.0;
    
    const double distance = std::abs(price - level) / level;
    // Scaling: 1% breakout = 0.2 confidence, 5% atau lebih = 1.0 (max)
    return std::min(distance * 20.0, 1.0);
}

// ==================== GETTERS ====================
std::size_t BreakoutStrategy::lookback_period() const noexcept { 
    return lookback_period_; 
}

std::size_t BreakoutStrategy::confirmation_bars() const noexcept { 
    return confirmation_bars_; 
}

double BreakoutStrategy::breakout_threshold_pct() const noexcept { 
    return breakout_threshold_pct_; 
}

bool BreakoutStrategy::volume_confirmation_enabled() const noexcept { 
    return use_volume_confirmation_; 
}

// ==================== SETTERS ====================
void BreakoutStrategy::set_lookback_period(std::size_t period) {
    if (period == 0) {
        throw std::invalid_argument("lookback_period harus > 0");
    }
    lookback_period_ = period;
}

void BreakoutStrategy::set_confirmation_bars(std::size_t bars) { 
    confirmation_bars_ = bars; 
}

void BreakoutStrategy::set_breakout_threshold_pct(double threshold) {
    if (threshold < 0.0) {
        throw std::invalid_argument("breakout_threshold_pct tidak boleh negatif");
    }
    breakout_threshold_pct_ = threshold;
}

void BreakoutStrategy::set_volume_confirmation(bool enabled) { 
    use_volume_confirmation_ = enabled; 
}

} // namespace quant::strategy