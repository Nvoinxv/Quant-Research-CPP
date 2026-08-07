#pragma once

#include <vector>
#include <cstddef>
#include "strategy/signal.hpp"

namespace quant::core { struct Dataset; }

namespace quant::strategy {

/**
 * @brief Base class untuk semua strategi trading.
 * 
 * Semua strategi harus implement generate_signals() untuk batch processing
 * dan generate_signal_at() untuk per-bar processing (dipanggil backtest engine).
 */
class Strategy {
public:
    virtual ~Strategy() = default;

    /**
     * @brief Generate semua sinyal dari dataset (batch, lebih efisien).
     */
    [[nodiscard]]
    virtual std::vector<Signal> generate_signals(const core::Dataset& dataset) const = 0;

    /**
     * @brief Generate sinyal untuk bar tertentu (per-bar).
     * 
     * Untuk dataset besar, lebih efisien panggil generate_signals() sekali di awal.
     */
    [[nodiscard]]
    virtual Signal generate_signal_at(const core::Dataset& dataset, std::size_t index) const = 0;
};

} // namespace quant::strategy