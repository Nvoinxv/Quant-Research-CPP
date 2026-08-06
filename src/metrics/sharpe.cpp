#include "metrics/sharpe.hpp"
#include <cmath>
#include <numeric>
#include <limits>

namespace quant {
namespace metrics {

namespace {
    /**
     * @brief Hitung sample standard deviation (denominator n-1).
     * Precondition: data.size() >= 2
     */
    inline double sample_std_dev(const std::vector<double>& data, double mean) {
        double sq_sum = 0.0;
        for (double x : data) {
            double diff = x - mean;
            sq_sum += diff * diff;
        }
        return std::sqrt(sq_sum / static_cast<double>(data.size() - 1));
    }
}

std::optional<SharpeResult> calculate_sharpe(
    const std::vector<double>& period_returns,
    double risk_free_annual,
    double periods_per_year
) {
    // === Guard: butuh min. 2 periode untuk sample std dev ===
    if (period_returns.size() < 2) {
        return std::nullopt;
    }

    SharpeResult result;
    result.num_periods = period_returns.size();
    result.periods_per_year = periods_per_year;
    result.risk_free_per_period = risk_free_annual / periods_per_year;

    // === Rata-rata return ===
    double sum = std::accumulate(period_returns.begin(), period_returns.end(), 0.0);
    result.avg_return = sum / static_cast<double>(result.num_periods);

    // === Total return kumulatif (compounded) ===
    double compounded = 1.0;
    for (double r : period_returns) {
        compounded *= (1.0 + r);
    }
    result.total_return = compounded - 1.0;

    // === Volatilitas (sample std dev) ===
    result.volatility = sample_std_dev(period_returns, result.avg_return);

    // === Sharpe Ratio ===
    double excess = result.avg_return - result.risk_free_per_period;

    if (result.volatility > 0.0) {
        result.sharpe_ratio = excess / result.volatility;
    } else {
        // Zero volatility edge case
        if (excess > 0.0) {
            result.sharpe_ratio = std::numeric_limits<double>::infinity();
        } else if (excess < 0.0) {
            result.sharpe_ratio = -std::numeric_limits<double>::infinity();
        } else {
            result.sharpe_ratio = std::numeric_limits<double>::quiet_NaN();
        }
    }

    // === Annualisasi ===
    result.sharpe_annualized = result.sharpe_ratio * std::sqrt(periods_per_year);

    return result;
}

std::optional<SharpeResult> calculate_sharpe_from_equity(
    const std::vector<double>& equity_curve,
    double risk_free_annual,
    double periods_per_year
) {
    // === Guard: butuh min. 3 poin untuk menghasilkan 2 return ===
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

    return calculate_sharpe(returns, risk_free_annual, periods_per_year);
}

std::optional<SharpeResult> calculate_sharpe(
    const std::vector<TradeRecord>& trades,
    double initial_capital,
    double risk_free_annual,
    double periods_per_year
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

    return calculate_sharpe_from_equity(equity, risk_free_annual, periods_per_year);
}

} // namespace metrics
} // namespace quant