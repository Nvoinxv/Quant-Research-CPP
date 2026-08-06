#include "metrics/expectancy.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <limits>

namespace quant {
namespace metrics {

std::optional<ExpectancyResult> calculate_expectancy(const std::vector<TradeRecord>& trades) {
    if (trades.empty()) {
        return std::nullopt;
    }

    ExpectancyResult result;
    result.total_trades = trades.size();
    
    double total_risk = 0.0;
    std::size_t trades_with_risk = 0;
    
    // === Pass 1: Aggregation ===
    for (const auto& trade : trades) {
        result.net_profit += trade.pnl;
        
        if (trade.risk_amount > 0.0) {
            total_risk += trade.risk_amount;
            ++trades_with_risk;
        }
        
        if (trade.pnl > 0.0) {
            ++result.winning_trades;
            result.gross_profit += trade.pnl;
        } else if (trade.pnl < 0.0) {
            ++result.losing_trades;
            result.gross_loss += std::abs(trade.pnl);
        } else {
            ++result.break_even_trades;
        }
    }
    
    // === Pass 2: Averages ===
    if (result.winning_trades > 0) {
        result.avg_win = result.gross_profit / static_cast<double>(result.winning_trades);
    }
    
    if (result.losing_trades > 0) {
        result.avg_loss = result.gross_loss / static_cast<double>(result.losing_trades);
    }
    
    if (result.total_trades > 0) {
        result.avg_trade = result.net_profit / static_cast<double>(result.total_trades);
    }
    
    // === Pass 3: Rates & Core Expectancy ===
    const double total = static_cast<double>(result.total_trades);
    result.win_rate = static_cast<double>(result.winning_trades) / total;
    result.loss_rate = static_cast<double>(result.losing_trades) / total;
    result.break_even_rate = static_cast<double>(result.break_even_trades) / total;
    
    // Expectancy = (WinRate × AvgWin) − (LossRate × AvgLoss)
    result.expectancy = (result.win_rate * result.avg_win) - (result.loss_rate * result.avg_loss);
    
    // === Pass 4: Percentage Expectancy ===
    const double total_pnl_pct = std::accumulate(trades.begin(), trades.end(), 0.0,
        [](double sum, const TradeRecord& t) { return sum + t.pnl_pct; });
    result.expectancy_pct = total_pnl_pct / total;
    
    // === Pass 5: R-Multiple Expectancy (Van Tharp style) ===
    if (trades_with_risk > 0) {
        double total_r_multiple = 0.0;
        for (const auto& trade : trades) {
            if (trade.risk_amount > 0.0) {
                total_r_multiple += trade.pnl / trade.risk_amount;
            }
        }
        result.expectancy_r = total_r_multiple / static_cast<double>(trades_with_risk);
    }
    
    // === Pass 6: Profit Factor ===
    if (result.gross_loss > 0.0) {
        result.profit_factor = result.gross_profit / result.gross_loss;
    } else if (result.gross_profit > 0.0) {
        result.profit_factor = std::numeric_limits<double>::infinity();
    } // else stays 0.0
    
    // === Pass 7: Payoff Ratio ===
    if (result.avg_loss > 0.0) {
        result.payoff_ratio = result.avg_win / result.avg_loss;
    } else if (result.avg_win > 0.0) {
        result.payoff_ratio = std::numeric_limits<double>::infinity();
    } // else stays 0.0
    
    return result;
}

std::optional<ExpectancyResult> calculate_expectancy(const std::vector<double>& pnl_values) {
    std::vector<TradeRecord> trades;
    trades.reserve(pnl_values.size());
    
    for (double pnl : pnl_values) {
        trades.emplace_back(pnl);
    }
    
    return calculate_expectancy(trades);
}

std::optional<ExpectancyResult> calculate_expectancy_by_symbol(
    const std::vector<TradeRecord>& trades, 
    const std::string& symbol
) {
    std::vector<TradeRecord> filtered;
    filtered.reserve(trades.size());
    
    std::copy_if(trades.begin(), trades.end(), std::back_inserter(filtered),
        [&symbol](const TradeRecord& t) { return t.symbol == symbol; });
    
    return calculate_expectancy(filtered);
}

} // namespace metrics
} // namespace quant