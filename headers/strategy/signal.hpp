#pragma once

#include <string>
#include <cstddef>

namespace quant::strategy {

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

} // namespace quant::strategy