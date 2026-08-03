#include "data/csv_reader.hpp"

#include <fstream>
#include <sstream>

namespace quant::data
{

std::vector<quant::market::Candle>
CSVReader::read(
    const std::filesystem::path& filepath
) const
{
    std::vector<quant::market::Candle> candles;

    std::ifstream file(filepath);

    if (!file.is_open())
        return candles;

    std::string line;

    // Skip header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        quant::market::Candle candle;

        std::string value;

        // Symbol
        std::getline(ss, candle.symbol, ',');

        // Open Time
        std::getline(ss, value, ',');
        candle.openTime = std::stoll(value);

        // Close Time
        std::getline(ss, value, ',');
        candle.closeTime = std::stoll(value);

        // OHLC
        std::getline(ss, value, ',');
        candle.open = std::stod(value);

        std::getline(ss, value, ',');
        candle.high = std::stod(value);

        std::getline(ss, value, ',');
        candle.low = std::stod(value);

        std::getline(ss, value, ',');
        candle.close = std::stod(value);

        // Volume
        std::getline(ss, value, ',');
        candle.volume = std::stod(value);

        // Quote Volume
        std::getline(ss, value, ',');
        candle.quoteVolume = std::stod(value);

        // Trade Count
        std::getline(ss, value, ',');
        candle.tradeCount = std::stoull(value);

        // Taker Buy Base Volume
        std::getline(ss, value, ',');
        candle.takerBuyBaseVolume = std::stod(value);

        // Taker Buy Quote Volume
        std::getline(ss, value, ',');
        candle.takerBuyQuoteVolume = std::stod(value);

        candles.emplace_back(std::move(candle));
    }

    return candles;
}

} // namespace quant::data