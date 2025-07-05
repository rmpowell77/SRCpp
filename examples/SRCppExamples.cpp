#include <SRCpp/SRCpp.hpp>
#include <algorithm>
#include <cmath>
#include <expected>
#include <numbers>
#include <print>
#include <ranges>
#include <vector>

auto makeSin([[maybe_unused]] float hz, [[maybe_unused]] float sr,
    [[maybe_unused]] size_t len)
{
    using namespace std::numbers;
    std::vector<float> data(len);
    for (auto i : std::views::iota(0UL, len)) {
        data.at(i) = std::sin(hz * i * 2 * pi / sr);
    }
    return data;
}

int exampleConverter()
{
    // Input audio data (e.g., a sine wave)
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };
    auto ratio = 1.5;
    auto channels = 1;

    // Perform sample rate conversion with a ratio of 1.5
    auto [output, error] = SRCpp::Convert<float>(
        input, SRCpp::Type::Sinc_MediumQuality, channels, ratio);

    // Output the converted data
    if (output) {
        std::print("Converted audio data: [");
        for (const auto& sample : *output) {
            std::print("{}, ", sample);
        }
        std::println("]");
    } else {
        std::println("Error: {}", error);
    }
    return 0;
}

#if SRCPP_USE_CPP23
int examplePushConverter()
{
    // Input audio data (e.g., a sine wave)
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };
    auto ratio = 1.5;
    auto channels = 1;

    auto converter = SRCpp::PushConverter(
        SRCpp::Type::Sinc_MediumQuality, channels, ratio);
    auto output = converter.convert_expected<float>(input).and_then(
        [&converter](
            auto data) -> std::expected<std::vector<float>, std::string> {
            auto flush = converter.flush_expected<float>();
            if (flush.has_value()) {
                data.insert(data.end(), flush->begin(), flush->end());
            }
            return data;
        });

    if (output.has_value()) {
        std::print("Push Converted audio data: ");
        for (const auto& sample : *output) {
            std::print("{}, ", sample);
        }
        std::println("]");
    } else {
        std::println("Error: {}", output.error());
    }
    return 0;
}
#endif // SRCPP_USE_CPP23

int examplePullConverter()
{
    // Input audio data (e.g., a sine wave)
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };

    auto ratio = 1.5;
    auto channels = 1;

    auto frames_left = input.size();
    auto callback =
        [&frames_left, input_span = std::span { input }]() -> std::span<float> {
        if (frames_left == 0) {
            return {};
        }
        frames_left -= input_span.size();
        return input_span;
    };
    auto puller = SRCpp::PullConverter<float>(
        callback, SRCpp::Type::Sinc_MediumQuality, channels, ratio);

    auto buffer = std::vector<float>(input.size() * ratio);

    auto [output, error] = puller.convert(buffer);
    if (output.has_value()) {
        std::print("Pull Converted audio data: [");
        for (const auto& sample : *output) {
            std::print("{}, ", sample);
        }
        std::println("]");
    } else {
        std::println("Error: {}", error);
    }
    return 0;
}

int try_simple()
{
    auto data = makeSin(3000.0, 48000.0, 128);

    std::println("data");
    for (auto i : data) {
        std::print("{} ", i);
    }
    std::println("");
    {
        auto [output, error] = SRCpp::Convert<float>(
            data, SRCpp::Type::Sinc_MediumQuality, 1, 0.1);

        if (output.has_value()) {
            std::println("data");
            for (auto i : *output) {
                std::print("{} ", i);
            }
            std::println("");
        }
    }
    return 0;
}

int try_normal()
{
    auto data = makeSin(3000.0, 48000.0, 128);

    std::println("data");
    for (auto i : data) {
        std::print("{} ", i);
    }
    std::println("");
    {
        auto src
            = SRCpp::PushConverter(SRCpp::Type::Sinc_MediumQuality, 1, 0.1);
        {
            auto [output, error] = src.convert<float>(data);
            if (output.has_value()) {
                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
        {
            auto [output, error] = src.flush<float>();
            if (output.has_value()) {
                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
    }

    return 0;
}

int main()
{
    exampleConverter();
#if SRCPP_USE_CPP23
    examplePushConverter();
#endif // SRCPP_USE_CPP23
    examplePullConverter();
    try_simple();
    try_normal();
    return 0;
}