#include "metrics/sortino_ratio.hpp"
#include <cmath>
#include <numeric>
#include <limits>

namespace quant {
namespace metrics {

namespace {
    /**
     * @brief Hitung downside deviation (σd).
     * 
     * Hanya return yang di bawah target (MAR) yang dihitung.
     * Denominator = N (total observasi), bukan k (cuma yang downside).
     * Ini yang paling umum dipakai di industri quant.
     * 
     * Precondition: returns.size() >= 1
     */
    inline double downside_deviation(const std::vector<double>& returns, double target) {
        double sq_sum = 0.0;
        for (double r : returns) {
            if (r < target) {
                double diff = r - target;   // Selalu negatif, tapi kita kuadratkan
                sq_sum += diff * diff;
            }
        }
        return std::sqrt(sq_sum / static_cast<double>(returns.size()));
    }
}

std::optional<SortinoResult> calculate_sortino(
    const std::vector<double>& period_returns,
    double risk_free_annual,
    double periods_per_year,
    double target_return
) {
    // === Guard: butuh min. 2 periode ===
    if (period_returns.size() < 2) {
        return std::nullopt;
    }

    SortinoResult result;
    result.num_periods = period_returns.size();
    result.periods_per_year = periods_per_year;
    result.risk_free_per_period = risk_free_annual / periods_per_year;
    result.target_return = target_return;

    // === Rata-rata return ===
    double sum = std::accumulate(period_returns.begin(), period_returns.end(), 0.0);
    result.avg_return = sum / static_cast<double>(result.num_periods);

    // === Total return kumulatif ===
    double compounded = 1.0;
    for (double r : period_returns) {
        compounded *= (1.0 + r);
    }
    result.total_return = compounded - 1.0;

    // === Hitung berapa periode yang di bawah target ===
    for (double r : period_returns) {
        if (r < target_return) {
            ++result.downside_periods;
        }
    }

    // === Downside deviation (cuma dari return < target) ===
    result.downside_deviation = downside_deviation(period_returns, target_return);

    // === Sortino Ratio ===
    double excess = result.avg_return - result.risk_free_per_period;

    if (result.downside_deviation > 0.0) {
        result.sortino_ratio = excess / result.downside_deviation;
    } else {
        // Zero downside deviation edge case
        if (excess > 0.0) {
            result.sortino_ratio = std::numeric_limits<double>::infinity();
        } else if (excess < 0.0) {
            result.sortino_ratio = -std::numeric_limits<double>::infinity();
        } else {
            result.sortino_ratio = std::numeric_limits<double>::quiet_NaN();
        }
    }

    // === Annualisasi ===
    result.sortino_annualized = result.sortino_ratio * std::sqrt(periods_per_year);

    return result;
}

std::optional<SortinoResult> calculate_sortino_from_equity(
    const std::vector<double>& equity_curve,
    double risk_free_annual,
    double periods_per_year,
    double target_return
) {
    // === Guard: butuh min. 3 poin untuk hasilkan 2 return ===
    if (equity_curve.size() < 3) {
        return std::nullopt;
    }

    std::vector<double> returns;
    returns.reserve(equity_curve.size() - 1);

    for (std::size_t i = 1; i < equity_curve.size(); ++i) {
        double prev = equity_curve[i - 1];
        if (prev == 0.0) {
            returns.push_back(0.0);  // Hindari div by zero
        } else {
            returns.push_back((equity_curve[i] - prev) / prev);
        }
    }

    return calculate_sortino(returns, risk_free_annual, periods_per_year, target_return);
}

std::optional<SortinoResult> calculate_sortino(
    const std::vector<TradeRecord>& trades,
    double initial_capital,
    double risk_free_annual,
    double periods_per_year,
    double target_return
) {
    // === Guard: butuh modal valid dan min. 2 trade ===
    if (trades.size() < 2 || initial_capital <= 0.0) {
        return std::nullopt;
    }

    // Build equity curve dari trade P&L
    std::vector<double> equity;
    equity.reserve(trades.size() + 1);
    equity.push_back(initial_capital);

    double cumulative = initial_capital;
    for (const auto& trade : trades) {
        cumulative += trade.pnl;
        equity.push_back(cumulative);
    }

    return calculate_sortino_from_equity(equity, risk_free_annual, periods_per_year, target_return);
}

} // namespace metrics
} // namespace quant