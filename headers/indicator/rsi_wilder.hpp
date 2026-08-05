#pragma once

#include <cstddef>
#include <vector>

namespace quant::indicators
{

class RSI
{
public:
    /**
     * @brief Menghitung seluruh nilai RSI menggunakan metode Wilder.
     *
     * @param values Data harga (biasanya Close Price)
     * @param period Periode RSI (default umum = 14)
     * @return Vector berisi nilai RSI.
     */
    static std::vector<double> calculate(
        const std::vector<double>& values,
        std::size_t period = 14
    );

    /**
     * @brief Menghitung nilai RSI terakhir saja.
     *
     * Lebih efisien untuk live trading.
     */
    static double last(
        const std::vector<double>& values,
        std::size_t period = 14
    );

private:
    static void validateInput(
        const std::vector<double>& values,
        std::size_t period
    );
};

} // namespace quant::indicators