#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace quant::data
{

struct Candle
{
    std::string timestamp;

    double open;
    double high;
    double low;
    double close;

    double volume;
};

class CSVWriter
{
public:
    CSVWriter() = default;

    bool write(
        const std::filesystem::path& filepath,
        const std::vector<Candle>& candles,
        bool overwrite = true
    ) const;

private:
    void writeHeader(std::ofstream& file) const;
};

}