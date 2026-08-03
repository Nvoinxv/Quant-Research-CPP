#include "data/binance_loader.hpp"
#include "data/csv_loader.hpp"
#include "data/csv_reader.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
    const char* apiKey = std::getenv("API_KEY_TESTNET_BINANCE_FUTURES");
    const char* secretKey = std::getenv("SECRET_KEY_TESTNET_BINANCE_FUTURES");

    if (!apiKey || !secretKey)
    {
        std::cerr << "Environment variable belum ditemukan.\n";
        return 1;
    }

    quant::loaders::BinanceLoader loader(
        apiKey,
        secretKey
    );

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

    std::cout << "Jumlah candle : "
              << result.size()
              << '\n';

    return 0;
}