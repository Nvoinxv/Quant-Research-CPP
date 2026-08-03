#include "data/binance_loader.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace quant::loaders
{

namespace
{

size_t writeCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp
)
{
    auto total = size * nmemb;

    static_cast<std::string*>(userp)->append(
        static_cast<char*>(contents),
        total
    );

    return total;
}

}

BinanceLoader::BinanceLoader(
    std::string API_KEY_TESNET_BINANCE_FUTURES,
    std::string SECRET_KEY_TESTNET_BINANCE_FUTURES
)
    : m_apiKey(std::move(API_KEY_TESNET_BINANCE_FUTURES)),
      m_secretKey(std::move(SECRET_KEY_TESTNET_BINANCE_FUTURES))
{
}

std::vector<market::Candle>
BinanceLoader::load(
    const std::string& symbol,
    const std::string& interval,
    int limit
) const
{
    auto url = buildURL(
        symbol,
        interval,
        limit
    );

    auto response = performRequest(url);

    return parseResponse(response);
}

std::string BinanceLoader::buildURL(
    const std::string& symbol,
    const std::string& interval,
    int limit
) const
{
    std::stringstream ss;

    ss
        << "https://api.binance.com/api/v3/klines?"
        << "symbol=" << symbol
        << "&interval=" << interval
        << "&limit=" << limit;

    return ss.str();
}

std::string BinanceLoader::performRequest(
    const std::string& url
) const
{
    CURL* curl = curl_easy_init();

    if (!curl)
        throw std::runtime_error("Failed to initialize CURL.");

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    struct curl_slist* headers = nullptr;

    if (!m_apiKey.empty())
    {
        headers = curl_slist_append(
            headers,
            ("X-MBX-APIKEY: " + m_apiKey).c_str()
        );

        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            headers
        );
    }

    auto result = curl_easy_perform(curl);

    if (headers)
        curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
        throw std::runtime_error(
            curl_easy_strerror(result)
        );

    return response;
}

std::vector<market::Candle>
BinanceLoader::parseResponse(
    const std::string& body
) const
{
    json data = json::parse(body);

    std::vector<market::Candle> candles;

    candles.reserve(data.size());

    for (const auto& row : data)
    {
        market::Candle candle;

        candle.timestamp =
            std::to_string(row[0].get<long long>());

        candle.open =
            std::stod(row[1].get<std::string>());

        candle.high =
            std::stod(row[2].get<std::string>());

        candle.low =
            std::stod(row[3].get<std::string>());

        candle.close =
            std::stod(row[4].get<std::string>());

        candle.volume =
            std::stod(row[5].get<std::string>());

        candles.push_back(candle);
    }

    return candles;
}

}