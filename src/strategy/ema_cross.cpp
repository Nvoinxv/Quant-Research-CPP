#include "strategy/ema_cross.hpp"
#include "core/dataset.hpp"
#include "core/candle.hpp"
#include "indicators/ewma.hpp"
#include <stdexcept>
#include <cmath>

namespace quant::strategy {

EmaCrossStrategy::EmaCrossStrategy(std::size_t short_period,
                                   std::size_t long_period,
                                   std::size_t confirmation_bars,
                                   double threshold_pct,
                                   bool use_volume_confirmation)
    : short_period_(short_period),
      long_period_(long_period),
      confirmation_bars_(confirmation_bars),
      threshold_pct_(threshold_pct),
      use_volume_confirmation_(use_volume_confirmation) {
    
    if (short_period_ == 0 || long_period_ == 0) {
        throw std::invalid_argument("EmaCrossStrategy: period harus > 0");
    }
    if (short_period_ >= long_period_) {
        throw std::invalid_argument("EmaCrossStrategy: short_period harus < long_period");
    }
    if (threshold_pct_ < 0.0) {
        throw std::invalid_argument("EmaCrossStrategy: threshold_pct tidak boleh negatif");
    }
}

std::vector<Signal> EmaCrossStrategy::generate_signals(const core::Dataset& dataset) const {
    std::vector<Signal> signals;
    
    if (dataset.empty() || dataset.size() <= long_period_ + confirmation_bars_ + 1) {
        return signals;
    }
    
    // Extract close prices
    std::vector<double> closes;
    closes.reserve(dataset.size());
    for (const auto& candle : dataset.candles) {
        closes.push_back(candle.close);
    }
    
    // Pre-calculate EMA vectors sekali (O(n))
    std::vector<double> ema_short = indicators::EWMA::calculate(closes, short_period_);
    std::vector<double> ema_long = indicators::EWMA::calculate(closes, long_period_);
    
    // Scan untuk cross events
    for (std::size_t i = long_period_ + 1; i < dataset.size(); ++i) {
        bool cross_over = (ema_short[i] > ema_long[i]) && (ema_short[i-1] <= ema_long[i-1]);
        bool cross_under = (ema_short[i] < ema_long[i]) && (ema_short[i-1] >= ema_long[i-1]);
        
        if (!cross_over && !cross_under) {
            continue;
        }
        
        // Apply confirmation bars: cross di bar i, sinyal di bar i + confirmation_bars
        std::size_t signal_idx = i + confirmation_bars_;
        if (signal_idx >= dataset.size()) {
            continue;
        }
        
        // Cek apakah arah masih valid setelah confirmation
        bool still_above = ema_short[signal_idx] > ema_long[signal_idx];
        bool still_below = ema_short[signal_idx] < ema_long[signal_idx];
        
        Signal sig{SignalType::HOLD, signal_idx, dataset.candles[signal_idx].close, 0.0, ""};
        
        if (cross_over && still_above) {
            double diff = std::abs(ema_short[signal_idx] - ema_long[signal_idx]) / ema_long[signal_idx];
            if (diff < threshold_pct_) {
                continue;
            }
            
            if (use_volume_confirmation_ && !confirm_volume(dataset, signal_idx)) {
                continue;
            }
            
            sig.type = SignalType::BUY;
            sig.confidence = compute_confidence(ema_short[signal_idx], ema_long[signal_idx]);
            sig.reason = "EMA" + std::to_string(short_period_) + " cross above EMA" + 
                        std::to_string(long_period_) + " at bar " + std::to_string(i);
            signals.push_back(sig);
        }
        else if (cross_under && still_below) {
            double diff = std::abs(ema_short[signal_idx] - ema_long[signal_idx]) / ema_long[signal_idx];
            if (diff < threshold_pct_) {
                continue;
            }
            
            if (use_volume_confirmation_ && !confirm_volume(dataset, signal_idx)) {
                continue;
            }
            
            sig.type = SignalType::SELL;
            sig.confidence = compute_confidence(ema_short[signal_idx], ema_long[signal_idx]);
            sig.reason = "EMA" + std::to_string(short_period_) + " cross below EMA" + 
                        std::to_string(long_period_) + " at bar " + std::to_string(i);
            signals.push_back(sig);
        }
    }
    
    return signals;
}

Signal EmaCrossStrategy::generate_signal_at(const core::Dataset& dataset, std::size_t index) const {
    if (dataset.empty() || index < long_period_ + confirmation_bars_ + 1 || index >= dataset.size()) {
        return Signal{SignalType::HOLD, index, 0.0, 0.0, "Data tidak cukup atau index invalid"};
    }
    
    std::size_t cross_idx = index - confirmation_bars_;
    if (cross_idx < long_period_ + 1) {
        return Signal{SignalType::HOLD, index, 0.0, 0.0, "Cross index terlalu awal"};
    }
    
    // --- Hitung EMA di bar cross_idx ---
    std::vector<double> closes_cross;
    closes_cross.reserve(cross_idx + 1);
    for (std::size_t i = 0; i <= cross_idx; ++i) {
        closes_cross.push_back(dataset.candles[i].close);
    }
    double ema_short_cross = indicators::EWMA::last(closes_cross, short_period_);
    double ema_long_cross = indicators::EWMA::last(closes_cross, long_period_);
    
    // --- Hitung EMA di bar cross_idx - 1 ---
    std::vector<double> closes_prev(closes_cross.begin(), closes_cross.end() - 1);
    double ema_short_prev = indicators::EWMA::last(closes_prev, short_period_);
    double ema_long_prev = indicators::EWMA::last(closes_prev, long_period_);
    
    bool cross_over = (ema_short_cross > ema_long_cross) && (ema_short_prev <= ema_long_prev);
    bool cross_under = (ema_short_cross < ema_long_cross) && (ema_short_prev >= ema_long_prev);
    
    if (!cross_over && !cross_under) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "No cross detected"};
    }
    
    // --- Cek apakah arah masih valid di bar index (setelah confirmation) ---
    std::vector<double> closes_now;
    closes_now.reserve(index + 1);
    for (std::size_t i = 0; i <= index; ++i) {
        closes_now.push_back(dataset.candles[i].close);
    }
    double ema_short_now = indicators::EWMA::last(closes_now, short_period_);
    double ema_long_now = indicators::EWMA::last(closes_now, long_period_);
    
    bool still_above = ema_short_now > ema_long_now;
    bool still_below = ema_short_now < ema_long_now;
    
    if (cross_over && !still_above) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Cross not confirmed"};
    }
    if (cross_under && !still_below) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Cross not confirmed"};
    }
    
    // --- Apply threshold ---
    double diff = std::abs(ema_short_now - ema_long_now) / ema_long_now;
    if (diff < threshold_pct_) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Below threshold"};
    }
    
    // --- Volume confirmation ---
    if (use_volume_confirmation_ && !confirm_volume(dataset, index)) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Volume not confirmed"};
    }
    
    Signal sig{SignalType::HOLD, index, dataset.candles[index].close, 0.0, ""};
    if (cross_over) {
        sig.type = SignalType::BUY;
        sig.confidence = compute_confidence(ema_short_now, ema_long_now);
        sig.reason = "EMA" + std::to_string(short_period_) + " cross above EMA" + 
                    std::to_string(long_period_) + " (confirmed)";
    } else {
        sig.type = SignalType::SELL;
        sig.confidence = compute_confidence(ema_short_now, ema_long_now);
        sig.reason = "EMA" + std::to_string(short_period_) + " cross below EMA" + 
                    std::to_string(long_period_) + " (confirmed)";
    }
    
    return sig;
}

bool EmaCrossStrategy::confirm_volume(const core::Dataset& dataset, std::size_t index) const {
    if (index < long_period_) {
        return false;
    }
    
    double sum_volume = 0.0;
    for (std::size_t i = index - long_period_; i < index; ++i) {
        sum_volume += dataset.candles[i].volume;
    }
    const double avg_volume = sum_volume / static_cast<double>(long_period_);
    return dataset.candles[index].volume > avg_volume;
}

double EmaCrossStrategy::compute_confidence(double ema_short, double ema_long) const {
    if (ema_long == 0.0) return 0.0;
    
    const double distance = std::abs(ema_short - ema_long) / ema_long;
    // Scaling: 0.5% diff = 0.1 confidence, 5% atau lebih = 1.0 (max)
    return std::min(distance * 20.0, 1.0);
}

// ==================== GETTERS ====================
std::size_t EmaCrossStrategy::short_period() const noexcept { return short_period_; }
std::size_t EmaCrossStrategy::long_period() const noexcept { return long_period_; }
std::size_t EmaCrossStrategy::confirmation_bars() const noexcept { return confirmation_bars_; }
double EmaCrossStrategy::threshold_pct() const noexcept { return threshold_pct_; }
bool EmaCrossStrategy::volume_confirmation_enabled() const noexcept { return use_volume_confirmation_; }

// ==================== SETTERS ====================
void EmaCrossStrategy::set_short_period(std::size_t period) {
    if (period == 0) throw std::invalid_argument("short_period harus > 0");
    if (period >= long_period_) throw std::invalid_argument("short_period harus < long_period");
    short_period_ = period;
}

void EmaCrossStrategy::set_long_period(std::size_t period) {
    if (period == 0) throw std::invalid_argument("long_period harus > 0");
    if (short_period_ >= period) throw std::invalid_argument("long_period harus > short_period");
    long_period_ = period;
}

void EmaCrossStrategy::set_confirmation_bars(std::size_t bars) { confirmation_bars_ = bars; }

void EmaCrossStrategy::set_threshold_pct(double threshold) {
    if (threshold < 0.0) throw std::invalid_argument("threshold_pct tidak boleh negatif");
    threshold_pct_ = threshold;
}

void EmaCrossStrategy::set_volume_confirmation(bool enabled) { use_volume_confirmation_ = enabled; }

} // namespace quant::strategy