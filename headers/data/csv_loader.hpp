#pragma once

#include "market/candle.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

namespace quant::data
{

class CSVWriter
{
public:
    CSVWriter() = default;

    bool write(
        const std::filesystem::path& filepath,
        const std::vector<quant::market::Candle>& candles,
        bool overwrite = true
    ) const;

private:
    void writeHeader(std::ofstream& file) const;
};

}