#ifndef QUANT_STRATEGY_BREAKOUT_HPP
#define QUANT_STRATEGY_BREAKOUT_HPP

#include <vector>
#include <string>
#include <utility>
#include <cstddef>

// Forward declaration untuk mengurangi compile-time dependency
namespace quant {
namespace core {
    class Dataset;
    struct Candle;
} // namespace core

namespace strategy {

/**
 * @brief Jenis sinyal yang dihasilkan strategi.
 * 
 * Engine backtest akan membaca sinyal ini pada setiap iterasi candle
 * untuk menentukan eksekusi order (BUY/SELL) atau hold posisi.
 */
enum class SignalType {
    HOLD = 0,
    BUY = 1,
    SELL = -1
};

/**
 * @brief Struct sinyal trading yang dikonsumsi oleh backtest engine.
 */
struct Signal {
    SignalType type = SignalType::HOLD;
    std::size_t index = 0;          // Index candle di dataset
    double price = 0.0;             // Harga referensi (biasanya close)
    double confidence = 0.0;        // 0.0 - 1.0, opsional untuk position sizing
    std::string reason;             // Deskripsi sinyal untuk logging/debug
};

/**
 * @brief Strategi Breakout berbasis Price Channel (Donchian-style).
 * 
 * Strategi ini menghitung highest high dan lowest low selama periode lookback.
 * Jika harga close menembus resistance (high tertinggi) → sinyal BUY.
 * Jika harga close menembus support (low terendah) → sinyal SELL.
 * 
 * Fitur opsional:
 * - Confirmation bars: menunggu N bar sebelum channel di-reset.
 * - Breakout threshold: persentase minimum di luar level untuk validasi breakout.
 * - Volume confirmation: volume harus di atas rata-rata lookback.
 */
class BreakoutStrategy {
public:
    /**
     * @param lookback_period Periode untuk menghitung channel (default: 20).
     * @param confirmation_bars Jumlah bar konfirmasi sebelum channel dihitung ulang (default: 1).
     * @param breakout_threshold_pct Persentase threshold di luar level (default: 0.0 = 0%).
     * @param use_volume_confirmation Aktifkan filter volume (default: false).
     */
    explicit BreakoutStrategy(std::size_t lookback_period = 20,
                              std::size_t confirmation_bars = 1,
                              double breakout_threshold_pct = 0.0,
                              bool use_volume_confirmation = false);

    // --- Batch processing: generate semua sinyal dari dataset ---
    std::vector<Signal> generate_signals(const core::Dataset& dataset) const;

    // --- Per-bar processing: dipanggil oleh backtest engine per iterasi ---
    Signal generate_signal_at(const core::Dataset& dataset, std::size_t index) const;

    // --- Getters ---
    std::size_t lookback_period() const noexcept;
    std::size_t confirmation_bars() const noexcept;
    double breakout_threshold_pct() const noexcept;
    bool volume_confirmation_enabled() const noexcept;

    // --- Setters ---
    void set_lookback_period(std::size_t period);
    void set_confirmation_bars(std::size_t bars);
    void set_breakout_threshold_pct(double threshold);
    void set_volume_confirmation(bool enabled);

private:
    std::size_t lookback_period_;
    std::size_t confirmation_bars_;
    double breakout_threshold_pct_;   // e.g., 0.005 = 0.5% di atas/bawah level
    bool use_volume_confirmation_;

    // Hitung channel [lowest_low, highest_high] untuk periode ending di end_index
    std::pair<double, double> calculate_channel(const core::Dataset& dataset,
                                                 std::size_t end_index) const;

    // Validasi breakout dengan threshold
    bool is_breakout_above(const core::Dataset& dataset,
                           std::size_t index,
                           double resistance) const;
    bool is_breakout_below(const core::Dataset& dataset,
                           std::size_t index,
                           double support) const;

    // Konfirmasi volume: volume saat ini > rata-rata volume lookback
    bool confirm_volume(const core::Dataset& dataset, std::size_t index) const;

    // Hitung confidence score berdasarkan jarak breakout dari level
    double compute_confidence(double price, double level) const;
};

} // namespace strategy
} // namespace quant

#endif // QUANT_STRATEGY_BREAKOUT_HPP