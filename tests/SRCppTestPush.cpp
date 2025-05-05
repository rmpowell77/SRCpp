#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <ranges>

auto ConvertWithPush(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type)
{
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto data = pusher.convert(input);
    if (!data.has_value()) {
        throw std::runtime_error(data.error());
    }
    auto flush = pusher.flush();
    if (!flush.has_value()) {
        throw std::runtime_error(flush.error());
    }
    data->insert(
        data->end(), flush->begin(), flush->end()); // append flush to data
    return *data;
}

auto ConvertWithPush(std::vector<float> input, size_t channels, double factor,
    SRCpp::Type type, size_t input_frames)
{
    auto output = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto framesLeft = input.size() / channels;
    while (framesLeft) {
        auto framesForThis = std::min(input_frames, input.size() / channels);
        auto data = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        framesLeft -= framesForThis;
        output.insert(
            output.end(), data->begin(), data->end()); // append flush to data
    }
    auto flush = pusher.flush();
    if (!flush.has_value()) {
        throw std::runtime_error(flush.error());
    }
    output.insert(
        output.end(), flush->begin(), flush->end()); // append flush to data
    return output;
}

TEST_CASE("PushConverter", "[SRCpp]")
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto hz : { std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();
                    auto input = makeSin(hz, 48000.0, frames);

                    auto reference
                        = CreatePushReference(input, channels, factor, type);
                    {
                        auto output = ConvertWithPush(
                            input, channels, factor, type, frames);

                        REQUIRE(output == reference);
                    }
                    for (auto input_size : { 4, 8, 16, 32, 64 }) {
                        auto output = ConvertWithPush(
                            input, channels, factor, type, input_size);

                        if (type == SRCpp::Type::ZeroOrderHold) {
                            auto mangledReference = reference;
                            mangledReference.resize(output.size());
                            REQUIRE(output == mangledReference);
                        } else {
                            REQUIRE(output == reference);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("Test copying a convert allows both to create the right results.",
    "[SRCpp]")
{
    auto frames = 64;
    auto firstPush = 10;
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, frames);

    auto reference = CreatePushReference(input, channels, factor, type);

    auto output1 = std::vector<float> {};
    auto output2 = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);

    // push 100
    auto framesLeft = input.size() / channels;
    {
        auto framesForThis = firstPush;
        auto data = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        framesLeft -= framesForThis;
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
        output2 = output1;
    }

    auto pusher2 = pusher;

    // push the rest
    {
        auto framesForThis = framesLeft;
        auto data = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
        data = pusher2.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        output2.insert(
            output2.end(), data->begin(), data->end()); // append flush to data
    }

    {
        auto flush = pusher.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(flush.error());
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }
    {
        auto flush = pusher2.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(flush.error());
        }
        output2.insert(output2.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    REQUIRE(output1 == reference);
    REQUIRE(output2 == reference);
}

TEST_CASE("Test moving a pull converters work", "[SRCpp]")
{
    auto frames = 64;
    auto firstPush = 10;
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, frames);

    auto reference = CreatePushReference(input, channels, factor, type);

    auto output1 = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);

    // push 100
    auto framesLeft = input.size() / channels;
    {
        auto framesForThis = firstPush;
        auto data = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        framesLeft -= framesForThis;
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
    }

    // moving into a new pusher. It should still continue to work.
    auto pusher2 = std::move(pusher);

    // push the rest
    {
        auto framesForThis = framesLeft;
        auto data = pusher2.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
    }
    {
        auto flush = pusher2.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(flush.error());
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    REQUIRE(output1 == reference);
}
