#include "data/csv_loader.hpp"

#include <fstream>
#include <iomanip>

namespace quant::data
{

void CSVWriter::writeHeader(std::ofstream& file) const
{
    file
        << "symbol,"
        << "open_time,"
        << "close_time,"
        << "open,"
        << "high,"
        << "low,"
        << "close,"
        << "volume,"
        << "quote_volume,"
        << "trade_count,"
        << "taker_buy_base_volume,"
        << "taker_buy_quote_volume\n";
}

bool CSVWriter::write(
    const std::filesystem::path& filepath,
    const std::vector<quant::market::Candle>& candles,
    bool overwrite
) const
{
    std::ios::openmode mode = std::ios::out;

    if (!overwrite)
        mode |= std::ios::app;

    std::ofstream file(filepath, mode);

    if (!file.is_open())
        return false;

    if (overwrite)
        writeHeader(file);

    file << std::fixed << std::setprecision(8);

    for (const auto& candle : candles)
    {
        file
            << candle.symbol << ","
            << candle.openTime << ","
            << candle.closeTime << ","
            << candle.open << ","
            << candle.high << ","
            << candle.low << ","
            << candle.close << ","
            << candle.volume << ","
            << candle.quoteVolume << ","
            << candle.tradeCount << ","
            << candle.takerBuyBaseVolume << ","
            << candle.takerBuyQuoteVolume
            << '\n';
    }

    return true;
}

}