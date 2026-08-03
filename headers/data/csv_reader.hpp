#pragma once

#include "data/csv_loader.hpp"

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