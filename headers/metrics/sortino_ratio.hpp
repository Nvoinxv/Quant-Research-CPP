#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include "metrics/expectancy.hpp"   // Reuse TradeRecord

namespace quant {
namespace metrics {

/**
 * @brief Hasil perhitungan Sortino Ratio dan komponennya.
 * 
 * Sortino = (Rp − Rf) / σd
 *   Rp  = rata-rata return per periode
 *   Rf  = risk-free rate per periode
 *   σd  = downside deviation (volatilitas yang cuma dihitung dari loss)
 * 
 * Bedanya sama Sharpe: Sortino cuma marah kalau lu rugi. 
 * Kalau profit gede-gede, dia acuhkan aja.
 * 
 * @note Downside deviation pakai denominator N (total observasi), bukan N-1.
 *       Ini standar industri Sortino (bukan sample std dev).
 */
struct SortinoResult {
    double sortino_ratio{0.0};          ///< Sortino per periode (raw)
    double sortino_annualized{0.0};     ///< Sortino annualized
    double avg_return{0.0};             ///< Rata-rata return per periode
    double downside_deviation{0.0};     ///< Downside volatility (σd)
    double total_return{0.0};           ///< Total return kumulatif (compounded)
    std::size_t num_periods{0};         ///< Total periode
    std::size_t downside_periods{0};    ///< Berapa periode yang di bawah target
    double target_return{0.0};          ///< Minimum Acceptable Return (MAR) yang dipakai
    double risk_free_per_period{0.0};   ///< Risk-free rate per periode
    double periods_per_year{252.0};     ///< Faktor annualisasi
};

/**
 * @brief Hitung Sortino dari vector return per periode (desimal).
 * @param period_returns Return per periode, e.g. 0.01 = +1%
 * @param risk_free_annual Risk-free rate annual (desimal)
 * @param periods_per_year Jumlah periode per tahun, default 252 (daily)
 * @param target_return Minimum Acceptable Return (MAR) per periode. 
 *                      Default 0.0 (cuma hitung return negatif sebagai downside).
 * @return SortinoResult jika data ≥ 2 periode, nullopt jika kurang.
 */
[[nodiscard]] std::optional<SortinoResult> calculate_sortino(
    const std::vector<double>& period_returns,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0,
    double target_return = 0.0
);

/**
 * @brief Hitung Sortino dari equity curve.
 * @param equity_curve Time series nilai portofolio. Index 0 = starting equity.
 * @param risk_free_annual Risk-free rate annual
 * @param periods_per_year Faktor annualisasi
 * @param target_return MAR per periode
 * @return SortinoResult jika data ≥ 3 poin, nullopt jika kurang.
 */
[[nodiscard]] std::optional<SortinoResult> calculate_sortino_from_equity(
    const std::vector<double>& equity_curve,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0,
    double target_return = 0.0
);

/**
 * @brief Hitung Sortino dari daftar trade (auto-build equity curve).
 * @param trades Daftar trade closed
 * @param initial_capital Modal awal (harus > 0)
 * @param risk_free_annual Risk-free rate annual
 * @param periods_per_year Faktor annualisasi
 * @param target_return MAR per periode
 * @return SortinoResult jika trade ≥ 2 dan initial_capital > 0.
 */
[[nodiscard]] std::optional<SortinoResult> calculate_sortino(
    const std::vector<TradeRecord>& trades,
    double initial_capital,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0,
    double target_return = 0.0
);

} // namespace metrics
} // namespace quant