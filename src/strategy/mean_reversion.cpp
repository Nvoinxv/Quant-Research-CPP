#include "strategy/mean_reversion.hpp"
#include "core/dataset.hpp"
#include "core/candle.hpp"
#include "indicators/rsi_wilder.hpp"
#include "indicators/choppiness_index.hpp"
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace quant::strategy {

MeanReversionStrategy::MeanReversionStrategy(std::size_t rsi_period,
                                             double oversold_threshold,
                                             double overbought_threshold,
                                             std::size_t chop_period,
                                             double chop_threshold,
                                             std::size_t confirmation_bars,
                                             bool use_volume_confirmation)
    : rsi_period_(rsi_period),
      oversold_threshold_(oversold_threshold),
      overbought_threshold_(overbought_threshold),
      chop_period_(chop_period),
      chop_threshold_(chop_threshold),
      confirmation_bars_(confirmation_bars),
      use_volume_confirmation_(use_volume_confirmation) {
    
    if (rsi_period_ == 0 || chop_period_ == 0) {
        throw std::invalid_argument("MeanReversionStrategy: period harus > 0");
    }
    if (oversold_threshold_ >= overbought_threshold_) {
        throw std::invalid_argument("MeanReversionStrategy: oversold harus < overbought");
    }
    if (oversold_threshold_ <= 0.0 || overbought_threshold_ >= 100.0) {
        throw std::invalid_argument("MeanReversionStrategy: RSI threshold harus antara 0 dan 100");
    }
    if (chop_threshold_ < 0.0 || chop_threshold_ > 100.0) {
        throw std::invalid_argument("MeanReversionStrategy: chop_threshold harus antara 0 dan 100");
    }
}

std::vector<Signal> MeanReversionStrategy::generate_signals(const core::Dataset& dataset) const {
    std::vector<Signal> signals;
    const std::size_t min_bars = std::max(rsi_period_, chop_period_) + 1;
    
    if (dataset.empty() || dataset.size() <= min_bars + confirmation_bars_) {
        return signals;
    }
    
    // Extract close prices
    std::vector<double> closes;
    closes.reserve(dataset.size());
    for (const auto& candle : dataset.candles) {
        closes.push_back(candle.close);
    }
    
    // Pre-calculate indicators (O(n))
    std::vector<double> rsi_values = indicators::RSI::calculate(closes, rsi_period_);
    std::vector<double> chop_values = indicators::ChoppinessIndex::calculate(dataset.candles, chop_period_);
    
    for (std::size_t i = min_bars; i < dataset.size(); ++i) {
        // --- Confirmation: RSI harus di threshold selama confirmation_bars + 1 bar berturut-turut ---
        bool oversold_confirmed = true;
        bool overbought_confirmed = true;
        
        for (std::size_t j = 0; j <= confirmation_bars_; ++j) {
            if (i < j) {
                oversold_confirmed = false;
                overbought_confirmed = false;
                break;
            }
            std::size_t check_idx = i - j;
            
            if (rsi_values[check_idx] >= oversold_threshold_) {
                oversold_confirmed = false;
            }
            if (rsi_values[check_idx] <= overbought_threshold_) {
                overbought_confirmed = false;
            }
        }
        
        if (!oversold_confirmed && !overbought_confirmed) {
            continue;
        }
        
        // --- Choppiness filter: hanya entry di market choppy (sideways) ---
        if (chop_values[i] < chop_threshold_) {
            continue; // Market lagi trending kuat, skip mean reversion
        }
        
        // --- Volume confirmation ---
        if (use_volume_confirmation_ && !confirm_volume(dataset, i)) {
            continue;
        }
        
        Signal sig{SignalType::HOLD, i, dataset.candles[i].close, 0.0, ""};
        
        if (oversold_confirmed) {
            sig.type = SignalType::BUY;
            sig.confidence = compute_confidence(rsi_values[i], oversold_threshold_, true);
            sig.reason = "Mean reversion: RSI oversold (" + std::to_string(static_cast<int>(rsi_values[i])) + ")";
            signals.push_back(sig);
        } else if (overbought_confirmed) {
            sig.type = SignalType::SELL;
            sig.confidence = compute_confidence(rsi_values[i], overbought_threshold_, false);
            sig.reason = "Mean reversion: RSI overbought (" + std::to_string(static_cast<int>(rsi_values[i])) + ")";
            signals.push_back(sig);
        }
    }
    
    return signals;
}

Signal MeanReversionStrategy::generate_signal_at(const core::Dataset& dataset, std::size_t index) const {
    const std::size_t min_bars = std::max(rsi_period_, chop_period_) + 1;
    
    if (dataset.empty() || index < min_bars + confirmation_bars_ || index >= dataset.size()) {
        return Signal{SignalType::HOLD, index, 0.0, 0.0, "Data tidak cukup atau index invalid"};
    }
    
    // Build sub-vectors [0..index] untuk perhitungan indikator
    std::vector<double> closes;
    closes.reserve(index + 1);
    for (std::size_t i = 0; i <= index; ++i) {
        closes.push_back(dataset.candles[i].close);
    }
    
    std::vector<quant::market::Candle> sub_candles(dataset.candles.begin(), dataset.candles.begin() + index + 1);
    
    // Calculate indicators
    std::vector<double> rsi_values = indicators::RSI::calculate(closes, rsi_period_);
    std::vector<double> chop_values = indicators::ChoppinessIndex::calculate(sub_candles, chop_period_);
    
    // --- Confirmation check ---
    bool oversold_confirmed = true;
    bool overbought_confirmed = true;
    
    for (std::size_t j = 0; j <= confirmation_bars_; ++j) {
        std::size_t check_idx = rsi_values.size() - 1 - j;
        if (rsi_values[check_idx] >= oversold_threshold_) oversold_confirmed = false;
        if (rsi_values[check_idx] <= overbought_threshold_) overbought_confirmed = false;
    }
    
    if (!oversold_confirmed && !overbought_confirmed) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "RSI tidak lolos konfirmasi"};
    }
    
    // --- Choppiness filter ---
    if (chop_values.back() < chop_threshold_) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Market trending (CHOP too low)"};
    }
    
    // --- Volume confirmation ---
    if (use_volume_confirmation_ && !confirm_volume(dataset, index)) {
        return Signal{SignalType::HOLD, index, dataset.candles[index].close, 0.0, "Volume tidak dikonfirmasi"};
    }
    
    Signal sig{SignalType::HOLD, index, dataset.candles[index].close, 0.0, ""};
    
    if (oversold_confirmed) {
        sig.type = SignalType::BUY;
        sig.confidence = compute_confidence(rsi_values.back(), oversold_threshold_, true);
        sig.reason = "Mean reversion: RSI oversold (" + std::to_string(static_cast<int>(rsi_values.back())) + ")";
    } else if (overbought_confirmed) {
        sig.type = SignalType::SELL;
        sig.confidence = compute_confidence(rsi_values.back(), overbought_threshold_, false);
        sig.reason = "Mean reversion: RSI overbought (" + std::to_string(static_cast<int>(rsi_values.back())) + ")";
    }
    
    return sig;
}

bool MeanReversionStrategy::confirm_volume(const core::Dataset& dataset, std::size_t index) const {
    if (index < rsi_period_) {
        return false;
    }
    
    double sum_volume = 0.0;
    for (std::size_t i = index - rsi_period_; i < index; ++i) {
        sum_volume += dataset.candles[i].volume;
    }
    const double avg_volume = sum_volume / static_cast<double>(rsi_period_);
    return dataset.candles[index].volume > avg_volume;
}

double MeanReversionStrategy::compute_confidence(double rsi_value, double threshold, bool is_oversold) const {
    // Semakin ekstrem RSI, semakin tinggi confidence
    // Oversold: RSI 25 = 0.0, RSI 0 = 1.0
    // Overbought: RSI 75 = 0.0, RSI 100 = 1.0
    if (is_oversold) {
        return std::min((threshold - rsi_value) / threshold, 1.0);
    } else {
        return std::min((rsi_value - threshold) / (100.0 - threshold), 1.0);
    }
}

// ==================== GETTERS ====================
std::size_t MeanReversionStrategy::rsi_period() const noexcept { return rsi_period_; }
double MeanReversionStrategy::oversold_threshold() const noexcept { return oversold_threshold_; }
double MeanReversionStrategy::overbought_threshold() const noexcept { return overbought_threshold_; }
std::size_t MeanReversionStrategy::chop_period() const noexcept { return chop_period_; }
double MeanReversionStrategy::chop_threshold() const noexcept { return chop_threshold_; }
std::size_t MeanReversionStrategy::confirmation_bars() const noexcept { return confirmation_bars_; }
bool MeanReversionStrategy::volume_confirmation_enabled() const noexcept { return use_volume_confirmation_; }

// ==================== SETTERS ====================
void MeanReversionStrategy::set_rsi_period(std::size_t period) {
    if (period == 0) throw std::invalid_argument("rsi_period harus > 0");
    rsi_period_ = period;
}

void MeanReversionStrategy::set_oversold_threshold(double threshold) {
    if (threshold <= 0.0 || threshold >= overbought_threshold_) {
        throw std::invalid_argument("oversold_threshold invalid");
    }
    oversold_threshold_ = threshold;
}

void MeanReversionStrategy::set_overbought_threshold(double threshold) {
    if (threshold >= 100.0 || threshold <= oversold_threshold_) {
        throw std::invalid_argument("overbought_threshold invalid");
    }
    overbought_threshold_ = threshold;
}

void MeanReversionStrategy::set_chop_period(std::size_t period) {
    if (period == 0) throw std::invalid_argument("chop_period harus > 0");
    chop_period_ = period;
}

void MeanReversionStrategy::set_chop_threshold(double threshold) {
    if (threshold < 0.0 || threshold > 100.0) {
        throw std::invalid_argument("chop_threshold harus antara 0 dan 100");
    }
    chop_threshold_ = threshold;
}

void MeanReversionStrategy::set_confirmation_bars(std::size_t bars) { confirmation_bars_ = bars; }
void MeanReversionStrategy::set_volume_confirmation(bool enabled) { use_volume_confirmation_ = enabled; }

} // namespace quant::strategy