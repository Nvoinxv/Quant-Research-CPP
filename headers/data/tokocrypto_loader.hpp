#pragma once

#include "core/candle.hpp"

#include <string>
#include <vector>

namespace quant::loaders
{

class BinanceLoader
{
public:
    BinanceLoader(
        std::string API_KEY_TESTNET_BINANCE_FUTURES,
        std::string SECRET_KEY_TESTNET_BINANCE_FUTURES
    );

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

private:
    std::string m_apiKey;
    std::string m_secretKey;
};

} // namespace quant::loaders