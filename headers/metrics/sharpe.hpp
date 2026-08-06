#pragma once

#include <vector>
#include <optional>
#include <cstddef>
#include "metrics/expectancy.hpp"   // Untuk TradeRecord

namespace quant {
namespace metrics {

/**
 * @brief Hasil perhitungan Sharpe Ratio dan komponennya.
 * 
 * Sharpe Ratio = (Rp − Rf) / σp
 *   Rp  = rata-rata return per periode
 *   Rf  = risk-free rate per periode  
 *   σp  = standar deviasi sample return (volatilitas)
 * 
 * Annualized Sharpe = Sharpe × √periods_per_year
 */
struct SharpeResult {
    double sharpe_ratio{0.0};           ///< Sharpe per periode (raw)
    double sharpe_annualized{0.0};      ///< Sharpe annualized
    double avg_return{0.0};             ///< Rata-rata return per periode
    double volatility{0.0};             ///< Volatilitas per periode (sample std dev)
    double total_return{0.0};           ///< Total return kumulatif (compounded)
    std::size_t num_periods{0};         ///< Jumlah periode return
    double risk_free_per_period{0.0};   ///< Risk-free rate yang dipakai per periode
    double periods_per_year{252.0};     ///< Faktor annualisasi
};

/**
 * @brief Hitung Sharpe dari vector return per periode (sudah dalam desimal).
 * @param period_returns Return per periode, e.g., 0.01 = +1%
 * @param risk_free_annual Risk-free rate annual (desimal), default 0.0
 * @param periods_per_year Jumlah periode dalam 1 tahun, default 252 (daily)
 * @return SharpeResult jika data ≥ 2 periode, nullopt jika kurang
 */
[[nodiscard]] std::optional<SharpeResult> calculate_sharpe(
    const std::vector<double>& period_returns,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0
);

/**
 * @brief Hitung Sharpe dari equity curve (nilai portofolio per bar).
 * @param equity_curve Time series nilai portofolio. Index 0 = starting equity.
 * @param risk_free_annual Risk-free rate annual (desimal)
 * @param periods_per_year Faktor annualisasi
 * @return SharpeResult jika data ≥ 3 poin (min. 2 return), nullopt jika kurang
 */
[[nodiscard]] std::optional<SharpeResult> calculate_sharpe_from_equity(
    const std::vector<double>& equity_curve,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0
);

/**
 * @brief Hitung Sharpe dari daftar trade (auto-build equity curve).
 * @param trades Daftar trade closed
 * @param initial_capital Modal awal (harus > 0 untuk bisa hitung % return)
 * @param risk_free_annual Risk-free rate annual (desimal)
 * @param periods_per_year Faktor annualisasi
 * @return SharpeResult jika trade ≥ 2 dan initial_capital > 0, nullopt jika tidak
 */
[[nodiscard]] std::optional<SharpeResult> calculate_sharpe(
    const std::vector<TradeRecord>& trades,
    double initial_capital,
    double risk_free_annual = 0.0,
    double periods_per_year = 252.0
);

} // namespace metrics
} // namespace quant