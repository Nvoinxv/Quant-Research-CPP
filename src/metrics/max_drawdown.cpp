#include "metrics/max_drawdown.hpp"
#include <numeric>
#include <algorithm>

namespace quant {
namespace metrics {

std::optional<MaxDrawdownResult> calculate_max_drawdown(
    const std::vector<double>& equity_curve
) {
    // === Edge case: data kosong ===
    if (equity_curve.empty()) {
        return std::nullopt;
    }

    // === Edge case: hanya 1 data point ===
    if (equity_curve.size() == 1) {
        MaxDrawdownResult res;
        res.peak_index = 0;
        res.trough_index = 0;
        res.recovery_index = 0;
        return res;
    }

    MaxDrawdownResult result;
    double peak_value = equity_curve[0];
    std::size_t peak_idx = 0;

    // Accumulator untuk average drawdown (setiap bar di bawah running peak)
    double sum_drawdown = 0.0;
    double sum_drawdown_pct = 0.0;
    std::size_t underwater_count = 0;

    // === Single Pass: O(n) ===
    for (std::size_t i = 1; i < equity_curve.size(); ++i) {
        const double current = equity_curve[i];

        if (current >= peak_value) {
            // New high (or equal) — reset running peak
            peak_value = current;
            peak_idx = i;
        } 
        else {
            // Sedang dalam drawdown dari peak_idx
            const double dd = peak_value - current;
            const double dd_pct = (peak_value != 0.0) 
                                  ? (dd / peak_value) * 100.0 
                                  : 0.0;

            // Akumulasi untuk rata-rata
            sum_drawdown += dd;
            sum_drawdown_pct += dd_pct;
            ++underwater_count;

            // Update Max Drawdown jika ini yang terbesar sejauh ini
            if (dd > result.max_drawdown) {
                result.max_drawdown = dd;
                result.max_drawdown_pct = dd_pct;
                result.peak_index = peak_idx;
                result.trough_index = i;
                result.peak_to_trough = i - peak_idx;
            }
        }
    }

    // === Cari Recovery Point untuk Max Drawdown terbesar ===
    // Recovery = index pertama setelah trough yang menyentuh atau melebihi peak asli
    if (result.max_drawdown > 0.0) {
        const double original_peak = equity_curve[result.peak_index];
        
        for (std::size_t i = result.trough_index + 1; i < equity_curve.size(); ++i) {
            if (equity_curve[i] >= original_peak) {
                result.recovery_index = i;
                result.peak_to_recovery = i - result.peak_index;
                break;
            }
        }
    }

    // === Hitung Rata-rata Drawdown ===
    if (underwater_count > 0) {
        result.avg_drawdown = sum_drawdown / static_cast<double>(underwater_count);
        result.avg_drawdown_pct = sum_drawdown_pct / static_cast<double>(underwater_count);
        result.underwater_bars = underwater_count;
    }

    return result;
}

std::optional<MaxDrawdownResult> calculate_max_drawdown(
    const std::vector<TradeRecord>& trades,
    double initial_capital
) {
    if (trades.empty()) {
        return std::nullopt;
    }

    // Build equity curve: [initial_capital, initial+pnl1, initial+pnl1+pnl2, ...]
    std::vector<double> equity;
    equity.reserve(trades.size() + 1);
    equity.push_back(initial_capital);

    double cumulative = initial_capital;
    for (const auto& trade : trades) {
        cumulative += trade.pnl;
        equity.push_back(cumulative);
    }

    return calculate_max_drawdown(equity);
}

} // namespace metrics
} // namespace quant