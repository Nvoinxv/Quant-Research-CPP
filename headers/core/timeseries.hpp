#pragma once

#include <cstddef>
#include <vector>

namespace quant::core
{

template<typename T>
struct TimeSeries
{
    std::vector<T> values;

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return values.size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return values.empty();
    }

    void clear() noexcept
    {
        values.clear();
    }
};

} // namespace quant::core