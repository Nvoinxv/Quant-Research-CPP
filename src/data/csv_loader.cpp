#include "data/csv_writer.hpp"

#include <fstream>
#include <iomanip>

namespace quant::data
{

void CSVWriter::writeHeader(std::ofstream& file) const
{
    file << "timestamp,open,high,low,close,volume\n";
}

bool CSVWriter::write(
    const std::filesystem::path& filepath,
    const std::vector<Candle>& candles,
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
            << candle.timestamp << ","
            << candle.open << ","
            << candle.high << ","
            << candle.low << ","
            << candle.close << ","
            << candle.volume
            << "\n";
    }

    return true;
}

}