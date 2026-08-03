#include "data/csv_reader.hpp"

#include <fstream>
#include <sstream>

namespace quant::data
{

std::vector<Candle> CSVReader::read(
    const std::filesystem::path& filepath
) const
{
    std::vector<Candle> candles;

    std::ifstream file(filepath);

    if (!file.is_open())
        return candles;

    std::string line;

    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);

        Candle candle;

        std::string value;

        std::getline(ss, candle.timestamp, ',');

        std::getline(ss, value, ',');
        candle.open = std::stod(value);

        std::getline(ss, value, ',');
        candle.high = std::stod(value);

        std::getline(ss, value, ',');
        candle.low = std::stod(value);

        std::getline(ss, value, ',');
        candle.close = std::stod(value);

        std::getline(ss, value, ',');
        candle.volume = std::stod(value);

        candles.push_back(candle);
    }

    return candles;
}

}