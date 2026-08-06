#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include <limits>

#include "metrics/expectancy.hpp"   // Untuk reuse struct TradeRecord

namespace quant {
namespace metrics {

/**
 * @brief Hasil perhitungan Maximum Drawdown dan statistik turunannya.
 * 
 * Semua index mengacu pada posisi di vector equity_curve yang diberikan.
 * Jika recovery_index == npos (max value), artinya drawdown belum pulih 
 * sampai akhir data yang tersedia.
 */
struct MaxDrawdownResult {
    // === Core Drawdown ===
    double max_drawdown{0.0};               ///< Besar drawdown absolut (peak - trough)
    double max_drawdown_pct{0.0};           ///< Drawdown dalam persen terhadap peak
    
    // === Lokasi Event ===
    std::size_t peak_index{0};              ///< Index peak sebelum drawdown terbesar
    std::size_t trough_index{0};            ///< Index titik terendah (trough)
    std::size_t recovery_index{std::numeric_limits<std::size_t>::max()}; 
                                            ///< Index recovery ke new high (npos = belum pulih)
    
    // === Durasi ===
    std::size_t peak_to_trough{0};          ///< Jumlah bar dari peak sampai trough
    std::size_t peak_to_recovery{0};        ///< Jumlah bar dari peak sampai recovery (0 jika belum)
    std::size_t underwater_bars{0};         ///< Total bar yang dihabiskan di bawah peak manapun
    
    // === Rata-rata ===
    double avg_drawdown{0.0};               ///< Rata-rata drawdown per bar (dari running peak)
    double avg_drawdown_pct{0.0};           ///< Rata-rata drawdown % per bar
    
    // === Helpers ===
    [[nodiscard]] bool has_recovery() const noexcept {
        return recovery_index != std::numeric_limits<std::size_t>::max();
    }
};

/**
 * @brief Hitung Max Drawdown dari equity curve (time series nilai portofolio).
 * @param equity_curve Vector nilai portofolio per bar/timestamp. 
 *                     Index 0 = starting equity, index N = equity terakhir.
 * @return MaxDrawdownResult jika data tidak kosong, nullopt jika kosong.
 * 
 * @note Algoritma O(n) — single pass dengan tracking running peak.
 */
[[nodiscard]] std::optional<MaxDrawdownResult> calculate_max_drawdown(
    const std::vector<double>& equity_curve
);

/**
 * @brief Hitung Max Drawdown dari daftar trade (cumulative P&L).
 * @param trades Daftar trade yang sudah close (gunakan TradeRecord dari expectancy).
 * @param initial_capital Modal awal. Default 0.0 (pure cumulative P&L curve).
 * @return MaxDrawdownResult jika ada trade, nullopt jika kosong.
 * 
 * Contoh: initial_capital = 10000.0, maka equity curve = [10000, 10000+pnl1, ...]
 */
[[nodiscard]] std::optional<MaxDrawdownResult> calculate_max_drawdown(
    const std::vector<TradeRecord>& trades,
    double initial_capital = 0.0
);

} // namespace metrics
} // namespace quant