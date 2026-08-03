#include "config/config.hpp"
#include "data/binance_loader.hpp"
#include "data/csv_loader.hpp"
#include "data/csv_reader.hpp"

#include <iostream>

int main()
{
    try
    {
        //--------------------------------------------------
        // Read API Key
        //--------------------------------------------------

        quant::Config config(".env");

        //--------------------------------------------------
        // Create Binance Loader
        //--------------------------------------------------

        quant::loaders::BinanceLoader loader(
            config.API_KEY_TESNET_BINANCE_FUTURES(),
            config.SECRET_KEY_TESTNET_BINANCE_FUTURES()
        );

        //--------------------------------------------------
        // Download Candle
        //--------------------------------------------------

        auto candles = loader.load(
            "BTCUSDT",
            "15m",
            20000
        );

        std::cout
            << "Downloaded "
            << candles.size()
            << " candles.\n";

        //--------------------------------------------------
        // Save CSV
        //--------------------------------------------------

        quant::data::CSVWriter writer;

        writer.write(
            "datasets/BTCUSDT_1m.csv",
            candles
        );

        std::cout
            << "CSV Saved.\n";

        //--------------------------------------------------
        // Load CSV
        //--------------------------------------------------

        quant::data::CSVReader reader;

        auto loaded = reader.read(
            "datasets/BTCUSDT_1m.csv"
        );

        std::cout
            << "CSV Loaded : "
            << loaded.size()
            << '\n';

        //--------------------------------------------------
        // Print First Candle
        //--------------------------------------------------

        if(!loaded.empty())
        {
            const auto& c = loaded.front();

            std::cout << "\n========== FIRST CANDLE ==========\n";

            std::cout
                << "Open Time : "
                << c.openTime
                << '\n';

            std::cout
                << "Open      : "
                << c.open
                << '\n';

            std::cout
                << "High      : "
                << c.high
                << '\n';

            std::cout
                << "Low       : "
                << c.low
                << '\n';

            std::cout
                << "Close     : "
                << c.close
                << '\n';

            std::cout
                << "Volume    : "
                << c.volume
                << '\n';
        }

        std::cout
            << "\nTesting Finished.\n";
    }
    catch(const std::exception& e)
    {
        std::cerr
            << "Error : "
            << e.what()
            << '\n';
    }
}