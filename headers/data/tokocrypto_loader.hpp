#pragma once

#include "core/candle.hpp"

#include <string>
#include <vector>

namespace quant::loaders
{

class TokocryptoLoader
{
public:
    TokocryptoLoader() = default;

    std::vector<market::Candle> load(
        const std::string& symbol,
        const std::string& interval,
        int limit = 1000
    ) const;

private:
    std::string buildURL(
        const std::string& symbol,
        const std::string& interval,
        int limit
    ) const;

    std::string performRequest(
        const std::string& url
    ) const;

    std::vector<market::Candle> parseResponse(
        const std::string& body,
        const std::string& symbol
    ) const;
};

} // namespace quant::loaders