#pragma once

#include <vector>
#include <cstddef>
#include "strategy/signal.hpp"

namespace quant::core { struct Dataset; }

namespace quant::strategy {

/**
 * @brief Strategi EMA Cross (Golden Cross / Death Cross).
 * 
 * Optimized untuk market crypto timeframe 5m (Bitcoin, dsb).
 * Default EMA 9/21 untuk entry yang responsif dan mengurangi lagging.
 * 
 * Logika sinyal:
 * - BUY: EMA short cross ABOVE EMA long (golden cross).
 * - SELL: EMA short cross BELOW EMA long (death cross).
 * - HOLD: Tidak ada cross, atau cross tidak lolos filter.
 * 
 * Fitur:
 * - Confirmation bars: tunggu N bar setelah cross untuk validasi.
 * - Threshold: selisih EMA harus minimal X% untuk dianggap valid.
 * - Volume confirmation: volume di bar sinyal harus di atas rata-rata.
 */
class EmaCrossStrategy {
public:
    /**
     * @param short_period Periode EMA cepat (default: 9 untuk crypto 5m).
     * @param long_period Periode EMA lambat (default: 21 untuk crypto 5m).
     * @param confirmation_bars Jumlah bar konfirmasi (default: 1).
     * @param threshold_pct Selisih minimum EMA (default: 0.0 = 0%).
     * @param use_volume_confirmation Aktifkan filter volume (default: false).
     */
    explicit EmaCrossStrategy(std::size_t short_period = 9,
                              std::size_t long_period = 21,
                              std::size_t confirmation_bars = 1,
                              double threshold_pct = 0.0,
                              bool use_volume_confirmation = false);

    // --- Batch processing: generate semua sinyal dari dataset (O(n)) ---
    std::vector<Signal> generate_signals(const core::Dataset& dataset) const;

    // --- Per-bar processing: dipanggil oleh backtest engine per iterasi ---
    // Note: Method ini melakukan re-calculation EMA per bar. Untuk dataset besar,
    // gunakan generate_signals() untuk performa optimal.
    Signal generate_signal_at(const core::Dataset& dataset, std::size_t index) const;

    // --- Getters ---
    std::size_t short_period() const noexcept;
    std::size_t long_period() const noexcept;
    std::size_t confirmation_bars() const noexcept;
    double threshold_pct() const noexcept;
    bool volume_confirmation_enabled() const noexcept;

    // --- Setters ---
    void set_short_period(std::size_t period);
    void set_long_period(std::size_t period);
    void set_confirmation_bars(std::size_t bars);
    void set_threshold_pct(double threshold);
    void set_volume_confirmation(bool enabled);

private:
    std::size_t short_period_;
    std::size_t long_period_;
    std::size_t confirmation_bars_;
    double threshold_pct_;
    bool use_volume_confirmation_;

    // Konfirmasi volume: volume saat ini > rata-rata volume long_period
    bool confirm_volume(const core::Dataset& dataset, std::size_t index) const;

    // Hitung confidence berdasarkan jarak relatif kedua EMA
    double compute_confidence(double ema_short, double ema_long) const;
};

} // namespace quant::strategy