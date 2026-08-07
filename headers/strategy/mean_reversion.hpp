#pragma once

#include <cstddef>
#include "strategy/strategy.hpp"
#include "strategy/signal.hpp"

namespace quant::strategy {

/**
 * @brief Strategi Mean Reversion berbasis RSI Wilder + Choppiness Index Filter.
 * 
 * Optimized untuk crypto timeframe 5m (Bitcoin).
 * 
 * Logika entry:
 * - BUY: RSI di bawah oversold_threshold + market choppy (CHOP cukup tinggi).
 * - SELL: RSI di atas overbought_threshold + market choppy.
 * 
 * Filter CHOP dipakai untuk avoid entry saat market strong trending
 * (mean reversion berbahaya di trending market).
 * 
 * Default parameter dikencengin buat crypto 5m:
 * - RSI period 9 (lebih responsif dari 14, kurangi lagging).
 * - Threshold 25/75 (lebih agresif, catch extreme move).
 * - Confirmation bars default 0 (entry cepat, nggak nunggu-nunggu).
 */
class MeanReversionStrategy : public Strategy {
public:
    /**
     * @param rsi_period Periode RSI (default: 9 untuk crypto 5m).
     * @param oversold_threshold Threshold RSI oversold (default: 25).
     * @param overbought_threshold Threshold RSI overbought (default: 75).
     * @param chop_period Periode Choppiness Index (default: 14).
     * @param chop_threshold Threshold CHOP minimum (default: 50). 
     *                       CHOP > threshold = market choppy (ideal mean reversion).
     * @param confirmation_bars Bar konfirmasi RSI berturut-turut (default: 0).
     * @param use_volume_confirmation Filter volume (default: false).
     */
    explicit MeanReversionStrategy(std::size_t rsi_period = 9,
                                   double oversold_threshold = 25.0,
                                   double overbought_threshold = 75.0,
                                   std::size_t chop_period = 14,
                                   double chop_threshold = 50.0,
                                   std::size_t confirmation_bars = 0,
                                   bool use_volume_confirmation = false);

    [[nodiscard]]
    std::vector<Signal> generate_signals(const core::Dataset& dataset) const override;

    [[nodiscard]]
    Signal generate_signal_at(const core::Dataset& dataset, std::size_t index) const override;

    // Getters
    std::size_t rsi_period() const noexcept;
    double oversold_threshold() const noexcept;
    double overbought_threshold() const noexcept;
    std::size_t chop_period() const noexcept;
    double chop_threshold() const noexcept;
    std::size_t confirmation_bars() const noexcept;
    bool volume_confirmation_enabled() const noexcept;

    // Setters
    void set_rsi_period(std::size_t period);
    void set_oversold_threshold(double threshold);
    void set_overbought_threshold(double threshold);
    void set_chop_period(std::size_t period);
    void set_chop_threshold(double threshold);
    void set_confirmation_bars(std::size_t bars);
    void set_volume_confirmation(bool enabled);

private:
    std::size_t rsi_period_;
    double oversold_threshold_;
    double overbought_threshold_;
    std::size_t chop_period_;
    double chop_threshold_;
    std::size_t confirmation_bars_;
    bool use_volume_confirmation_;

    bool confirm_volume(const core::Dataset& dataset, std::size_t index) const;
    double compute_confidence(double rsi_value, double threshold, bool is_oversold) const;
};

} // namespace quant::strategy