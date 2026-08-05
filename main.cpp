#include "data/tokocrypto_loader.hpp"
#include "data/csv_loader.hpp"
#include "data/csv_reader.hpp"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        quant::loaders::TokocryptoLoader loader;

        auto candles = loader.load(
            "BTCUSDT",
            "1m",
            100
        );

        quant::data::CSVWriter writer;

        writer.write(
            "BTCUSDT.csv",
            candles
        );

        quant::data::CSVReader reader;

        auto result = reader.read(
            "BTCUSDT.csv"
        );

        std::cout
            << "=========================================\n"
            << "        TOKOCRYPTO DATA LOADER\n"
            << "=========================================\n"
            << "Symbol           : BTCUSDT\n"
            << "Interval         : 1m\n"
            << "Candles Loaded   : "
            << result.size()
            << "\n"
            << "CSV Output       : BTCUSDT.csv\n"
            << "=========================================\n";
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "=========================================\n"
            << "ERROR\n"
            << "=========================================\n"
            << e.what()
            << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}