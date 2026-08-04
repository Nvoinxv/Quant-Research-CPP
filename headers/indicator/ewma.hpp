#pragma once

#include <vector>
#include <cstddef>

namespace quant::indicators
{

class EWMA
{
public:
    /**
     * @brief Menghitung Exponentially Weighted Moving Average (EWMA)
     *
     * @param values Data input (biasanya harga Close)
     * @param period Periode EWMA
     * @return std::vector<double>
     */
    static std::vector<double> calculate(
        const std::vector<double>& values,
        std::size_t period
    );

    /**
     * @brief Menghitung nilai EWMA terakhir saja
     *
     * Lebih efisien apabila hanya membutuhkan
     * nilai indikator terkini.
     */
    static double last(
        const std::vector<double>& values,
        std::size_t period
    );

private:
    static double smoothingFactor(std::size_t period);
};

} // namespace quant::indicators