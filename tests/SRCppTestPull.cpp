#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <ranges>

auto ConvertWithPull([[maybe_unused]] bool use_cpp20, std::vector<float> input,
    size_t channels, double factor, SRCpp::Type type,
    std::optional<size_t> input_frames = std::nullopt)
{
    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<float>(input);
    std::vector<float> output(framesExpected * channels);
    auto callback = [&]() -> std::span<float> {
        auto input_samples = [&]() {
            if (input_frames.has_value()) {
                return std::min(
                    input_frames.value() * channels, input_span.size());
            }
            return input_span.size();
        }();

        auto result = input_span.subspan(0, input_samples);
        input_span = input_span.subspan(input_samples);
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
#if SRCPP_USE_CPP23
    if (use_cpp20) {
#endif // SRCPP_USE_CPP23
        auto [data, error] = puller.convert(output);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        output.resize(data->size());
#if SRCPP_USE_CPP23
    } else {
        auto data = puller.convert_expected(output);
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        output.resize(data->size());
    }
#endif // SRCPP_USE_CPP23
    return output;
}

auto ConvertWithPullOutputFrames([[maybe_unused]] bool use_cpp20,
    std::vector<float> input, size_t channels, double factor, SRCpp::Type type,
    size_t output_frames, std::optional<size_t> input_frames = std::nullopt)
{
    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<float>(input);
    std::vector<float> output(framesExpected * channels);
    auto callback = [&]() -> std::span<float> {
        auto input_samples = [&]() {
            if (input_frames.has_value()) {
                return std::min(
                    input_frames.value() * channels, input_span.size());
            }
            return input_span.size();
        }();

        auto result = input_span.subspan(0, input_samples);
        input_span = input_span.subspan(input_samples);
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto framesProduced = 0UL;
    while (framesProduced < framesExpected && input_span.size()) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
#if SRCPP_USE_CPP23
        if (use_cpp20) {
#endif // SRCPP_USE_CPP23
            auto [data, error] = puller.convert(output);
            if (!data.has_value()) {
                throw std::runtime_error(error);
            }
            framesProduced += data->size() / channels;
#if SRCPP_USE_CPP23
        } else {
            auto data = puller.convert_expected(output);
            if (!data.has_value()) {
                throw std::runtime_error(data.error());
            }
            framesProduced += data->size() / channels;
        }
#endif // SRCPP_USE_CPP23
    }
    output.resize(framesProduced * channels);
    return output;
}

TEST_CASE("PullConverter", "[SRCpp]")
{
    for (auto cpp_20 : { false, true }) {
        for (auto frames : { 16, 256, 257, 500 }) {
            for (auto type : { SRCpp::Type::Sinc_BestQuality,
                     SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                     SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
                for (auto factor : { 1.5, 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                    for (auto hz : { std::vector<float> { 3000.0f, 40.0f },
                             std::vector<float> { 3000.0f },
                             std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                        auto channels = hz.size();
                        auto input = makeSin(hz, 48000.0, frames);

                        auto reference = CreatePushReference(
                            input, channels, factor, type);
                        {
                            auto output = ConvertWithPull(
                                cpp_20, input, channels, factor, type);
                            REQUIRE(output.size() > 0);
                            // fudge factor for pull
                            if (std::abs(static_cast<int>(reference.size())
                                    - static_cast<int>(output.size()))
                                == static_cast<int>(channels)) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                REQUIRE(output == mangledReference);
                            } else {
                                REQUIRE(output == reference);
                            }
                        }
                        for (auto input_size : { 4, 32, 33, 128 }) {
                            auto output = ConvertWithPull(cpp_20, input,
                                channels, factor, type, input_size);
                            REQUIRE(output.size() > 0);

                            // fudge factor for pull
                            if (std::abs(static_cast<int>(reference.size())
                                    - static_cast<int>(output.size()))
                                == static_cast<int>(channels)) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                REQUIRE(output == mangledReference);
                            } else {
                                REQUIRE(output == reference);
                            }
                        }
                        for (auto output_frames : { 4, 32, 33, 128 }) {
                            for (auto input_frames : { 4, 32, 33, 128 }) {
                                auto output = ConvertWithPullOutputFrames(
                                    cpp_20, input, channels, factor, type,
                                    output_frames, input_frames);
                                REQUIRE(output.size() > 0);

                                // fudge factor for pull
                                if (std::abs(static_cast<int>(reference.size())
                                        - static_cast<int>(output.size()))
                                    == static_cast<int>(channels)) {
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
    }
}

TEST_CASE("Test moving a pull converters work", "[SRCpp]")
{
    auto frames = 64;
    auto input_frames = 16;
    auto output_frames = size_t(16);
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, frames);

    auto reference = CreatePushReference(input, channels, factor, type);

    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<float>(input);
    std::vector<float> output(framesExpected * channels);
    auto callback = [&]() -> std::span<float> {
        auto input_samples
            = std::min(input_frames * channels, input_span.size());

        auto result = input_span.subspan(0, input_samples);
        input_span = input_span.subspan(input_samples);
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto framesProduced = 0UL;
    {
        auto toPull = 20;
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
        auto [data, error] = puller.convert(output);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        framesProduced += data->size() / channels;
    }
    // moving into a new pusher. It should still continue to work.
    auto puller2 = std::move(puller);
    while (framesProduced < framesExpected && input_span.size()) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
        auto [data, error] = puller2.convert(output);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        framesProduced += data->size() / channels;
    }
    output.resize(framesProduced * channels);

    REQUIRE(output == reference);
}
