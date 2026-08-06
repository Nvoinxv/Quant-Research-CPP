#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstddef>

namespace quant {
namespace metrics {

/**
 * @brief Represents a single completed trade for metric calculations.
 * 
 * Sengaja di-decouple dari struct internal backtest/ agar modul metrics
 * bisa dipakai untuk data dari mana pun (backtest, live trading, import CSV).
 */
struct TradeRecord {
    double pnl{0.0};                    ///< Profit/Loss absolut (positif = profit)
    double pnl_pct{0.0};                ///< Profit/Loss dalam persentase
    double risk_amount{0.0};            ///< Risk amount per trade (untuk R-multiple expectancy)
    std::string symbol{};               ///< Simbol/asset (opsional, untuk filtering)
    
    TradeRecord() = default;
    TradeRecord(double pnl_, double pnl_pct_ = 0.0, double risk_ = 0.0, std::string sym_ = "")
        : pnl(pnl_), pnl_pct(pnl_pct_), risk_amount(risk_), symbol(std::move(sym_)) {}
};

/**
 * @brief Hasil perhitungan expectancy dan statistik trade terkait.
 * 
 * Expectancy = (WinRate × AvgWin) − (LossRate × AvgLoss)
 * 
 * Semua nilai moneter menggunakan unit yang sama dengan input TradeRecord::pnl.
 */
struct ExpectancyResult {
    // === Core Expectancy ===
    double expectancy{0.0};             ///< Expectancy per trade (nominal)
    double expectancy_pct{0.0};         ///< Expectancy per trade (persentase)
    double expectancy_r{0.0};           ///< Expectancy dalam R-multiple (per unit risk)
    
    // === Trade Distribution ===
    double win_rate{0.0};               ///< Probabilitas win [0.0, 1.0]
    double loss_rate{0.0};              ///< Probabilitas loss [0.0, 1.0]
    double break_even_rate{0.0};        ///< Probabilitas break-even [0.0, 1.0]
    
    // === Averages ===
    double avg_win{0.0};                ///< Rata-rata profit trade yang win
    double avg_loss{0.0};               ///< Rata-rata loss trade yang loss (nilai positif)
    double avg_trade{0.0};              ///< Rata-rata P&L seluruh trade
    
    // === Risk Metrics ===
    double profit_factor{0.0};          ///< Gross Profit / Gross Loss
    double payoff_ratio{0.0};           ///< AvgWin / AvgLoss
    
    // === Counts ===
    std::size_t total_trades{0};
    std::size_t winning_trades{0};
    std::size_t losing_trades{0};
    std::size_t break_even_trades{0};
    
    // === Totals ===
    double gross_profit{0.0};
    double gross_loss{0.0};
    double net_profit{0.0};
};

/**
 * @brief Hitung expectancy dan statistik lengkap dari daftar trade.
 * @param trades Vector trade record yang sudah complete (closed)
 * @return ExpectancyResult jika trades tidak kosong, std::nullopt jika kosong
 */
[[nodiscard]] std::optional<ExpectancyResult> calculate_expectancy(const std::vector<TradeRecord>& trades);

/**
 * @brief Simplified interface: hitung expectancy dari raw P&L values.
 * @param pnl_values Vector profit/loss per trade
 * @return ExpectancyResult jika tidak kosong, std::nullopt jika kosong
 */
[[nodiscard]] std::optional<ExpectancyResult> calculate_expectancy(const std::vector<double>& pnl_values);

/**
 * @brief Hitung expectancy untuk trade dengan symbol tertentu (multi-asset filtering).
 * @param trades Semua trade record
 * @param symbol Symbol yang ingin difilter
 * @return ExpectancyResult untuk symbol tersebut, std::nullopt jika tidak ada match
 */
[[nodiscard]] std::optional<ExpectancyResult> calculate_expectancy_by_symbol(
    const std::vector<TradeRecord>& trades, 
    const std::string& symbol
);

} // namespace metrics
} // namespace quant