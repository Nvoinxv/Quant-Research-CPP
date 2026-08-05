#include "data/tokocrypto_loader.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>
#include <utility>

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
    const auto total = size * nmemb;

    static_cast<std::string*>(userp)->append(
        static_cast<char*>(contents),
        total
    );

    return total;
}

} // anonymous namespace

std::vector<market::Candle>
TokocryptoLoader::load(
    const std::string& symbol,
    const std::string& interval,
    int limit
) const
{
    const auto url = buildURL(
        symbol,
        interval,
        limit
    );

    const auto response = performRequest(
        url
    );

    return parseResponse(
        response,
        symbol
    );
}

std::string
TokocryptoLoader::buildURL(
    const std::string& symbol,
    const std::string& interval,
    int limit
) const
{
    std::stringstream ss;

    ss
        << "https://www.tokocrypto.site/api/v3/klines?"
        << "symbol=" << symbol
        << "&interval=" << interval
        << "&limit=" << limit;

    return ss.str();
}

std::string
TokocryptoLoader::performRequest(
    const std::string& url
) const
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        throw std::runtime_error(
            "Failed to initialize CURL."
        );
    }

    std::string response;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

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

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        30L
    );

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "QuantResearchPlatform/1.0"
    );

    const auto result =
        curl_easy_perform(curl);

    long httpCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpCode
    );

    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
    {
        throw std::runtime_error(
            curl_easy_strerror(result)
        );
    }

    if (httpCode != 200)
    {
        throw std::runtime_error(
            "Tokocrypto API returned HTTP " +
            std::to_string(httpCode)
        );
    }

    return response;
}

std::vector<market::Candle>
TokocryptoLoader::parseResponse(
    const std::string& body,
    const std::string& symbol
) const
{
    json data = json::parse(body);

    if (!data.is_array())
    {
        throw std::runtime_error(
            "Invalid Tokocrypto response."
        );
    }

    std::vector<market::Candle> candles;

    candles.reserve(
        data.size()
    );

    for (const auto& row : data)
    {
        market::Candle candle;

        candle.symbol = symbol;

        candle.openTime =
            row[0].get<std::int64_t>();

        candle.open =
            std::stod(
                row[1].get<std::string>()
            );

        candle.high =
            std::stod(
                row[2].get<std::string>()
            );

        candle.low =
            std::stod(
                row[3].get<std::string>()
            );

        candle.close =
            std::stod(
                row[4].get<std::string>()
            );

        candle.volume =
            std::stod(
                row[5].get<std::string>()
            );

        candle.closeTime =
            row[6].get<std::int64_t>();

        candle.quoteVolume =
            std::stod(
                row[7].get<std::string>()
            );

        candle.tradeCount =
            row[8].get<std::uint64_t>();

        candle.takerBuyBaseVolume =
            std::stod(
                row[9].get<std::string>()
            );

        candle.takerBuyQuoteVolume =
            std::stod(
                row[10].get<std::string>()
            );

        candles.emplace_back(
            std::move(candle)
        );
    }

    return candles;
}

} // namespace quant::loaders