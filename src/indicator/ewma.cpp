#include "ewma.hpp"

#include <stdexcept>
#include <limits>

namespace quant::indicators
{

double EWMA::smoothingFactor(std::size_t period)
{
    if (period == 0)
    {
        throw std::invalid_argument(
            "EWMA period must be greater than zero."
        );
    }

    return 2.0 / (static_cast<double>(period) + 1.0);
}

std::vector<double> EWMA::calculate(
    const std::vector<double>& values,
    std::size_t period
)
{
    if (values.empty())
    {
        return {};
    }

    const double alpha = smoothingFactor(period);

    std::vector<double> ewma(
        values.size(),
        std::numeric_limits<double>::quiet_NaN()
    );

    // Nilai awal menggunakan harga pertama
    ewma[0] = values[0];

    for (std::size_t i = 1; i < values.size(); ++i)
    {
        ewma[i] =
            alpha * values[i]
            + (1.0 - alpha) * ewma[i - 1];
    }

    return ewma;
}

double EWMA::last(
    const std::vector<double>& values,
    std::size_t period
)
{
    if (values.empty())
    {
        throw std::invalid_argument(
            "Input data is empty."
        );
    }

    const double alpha = smoothingFactor(period);

    double ewma = values.front();

    for (std::size_t i = 1; i < values.size(); ++i)
    {
        ewma =
            alpha * values[i]
            + (1.0 - alpha) * ewma;
    }

    return ewma;
}

} // namespace quant::indicators