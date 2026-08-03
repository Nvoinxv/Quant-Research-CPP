#pragma once

#include "data/csv_writer.hpp"

#include <filesystem>
#include <vector>

namespace quant::data
{

class CSVReader
{
public:
    CSVReader() = default;

    std::vector<Candle> read(
        const std::filesystem::path& filepath
    ) const;
};

}