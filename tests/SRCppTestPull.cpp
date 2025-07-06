#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <numbers>
#include <print>
#include <ranges>

TEST(SRCppPull, MovingConverter)
{
    auto frames = 256;
    auto input_frames = 64;
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
        auto [data, error] = puller.convert(pullBuffer);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        framesProduced += data->size() / channels;
    }
    // moving into a new pusher. It should still continue to work.
    SRCpp::PullConverter puller2 = std::move(puller);
    while (framesProduced < framesExpected) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
        auto [data, error] = puller2.convert(pullBuffer);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        framesProduced += data->size() / channels;
    }
    output.resize(framesProduced * channels);

    EXPECT_EQ(output, reference);
}

TEST(SRCppPull, ReturningNone)
{
    std::vector<float> output(10);
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto channels = 1;
    auto callback = [&]() -> std::span<float> { return {}; };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto [data, error] = puller.convert(output);
    ASSERT_TRUE(data.has_value());
    EXPECT_EQ(data->size(), 0);
}

TEST(SRCppPull, TestFunctionAndContext)
{
    auto frames = 256;
    auto input_frames = 64;
    auto output_frames = size_t(16);
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, frames);

    auto reference = CreatePushReference(input, channels, factor, type);

    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    struct Context {
        std::span<float> input_span;
        int input_frames = 64;
        size_t channels = 2;
    };
    auto context = Context {
        .input_span = std::span<float>(input),
        .input_frames = input_frames,
        .channels = channels,
    };
    std::vector<float> output(framesExpected * channels);
    // yes, there is a + there,
    // https://stackoverflow.com/questions/18889028/a-positive-lambda-what-sorcery-is-this
    auto callback = +[](void* context) -> std::span<float> {
        auto context_ = static_cast<Context*>(context);
        auto input_samples
            = std::min(context_->input_frames * context_->channels,
                context_->input_span.size());

        auto result = context_->input_span.subspan(0, input_samples);
        context_->input_span = context_->input_span.subspan(input_samples);
        return result;
    };
    auto puller
        = SRCpp::PullConverter(callback, &context, type, channels, factor);
    auto framesProduced = 0UL;
    while (framesProduced < framesExpected) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
        auto [data, error] = puller.convert(pullBuffer);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        framesProduced += data->size() / channels;
    }
    output.resize(framesProduced * channels);

    EXPECT_EQ(output, reference);
}

TEST(SRCppPull, Pull1)
{
    auto frames = 64;
    auto input_frames = size_t(1);
    auto output_frames = size_t(16);
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, frames);

    for (auto type : {
             SRCpp::Type::ZeroOrderHold,
             SRCpp::Type::Linear,
             SRCpp::Type::Sinc_Fastest,
             SRCpp::Type::Sinc_BestQuality,
             SRCpp::Type::Sinc_MediumQuality,
         }) {
        auto reference = CreatePushReference(input, channels, factor, type);
        // auto reference = CreatePullReference(
        //     input, channels, factor, type, input_frames, output_frames);
        // EXPECT_EQ(reference2, reference);

        auto framesExpected = static_cast<size_t>(
            std::ceil((input.size() / channels) * factor));
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
        while (framesProduced < framesExpected) {
            auto toPull
                = std::min(output_frames, framesExpected - framesProduced);
            auto pullBuffer
                = std::span { output.data() + framesProduced * channels,
                      toPull * channels };
            auto [data, error] = puller.convert(pullBuffer);
            if (!data.has_value()) {
                throw std::runtime_error(error);
            }
            framesProduced += data->size() / channels;
        }
        output.resize(framesProduced * channels);

        EXPECT_EQ(output, reference);
    }
}

// TEST_CASE("Test returning 0, then returning more", "[SRCpp]")
// {
//     auto frames = 64;
//     auto input_frames = 16;
//     auto output_frames = size_t(16);
//     auto type = SRCpp::Type::Sinc_BestQuality;
//     auto factor = 0.9;
//     auto hz = std::vector<float> { 3000.0f, 40.0f };
//     auto channels = hz.size();
//     auto input = makeSin(hz, 48000.0, frames);

//     auto reference = CreatePushReference(input, channels, factor, type);

//     auto framesExpected
//         = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
//     auto input_span = std::span<float>(input);
//     std::vector<float> output(framesExpected * channels);
//     int allowTimes = 1;
//     auto callback = [&]() -> std::span<float> {
//         if (allowTimes == 0) {
//             return {};
//         }
//         --allowTimes;
//         auto input_samples
//             = std::min(input_frames * channels, input_span.size());

//         auto result = input_span.subspan(0, input_samples);
//         input_span = input_span.subspan(input_samples);
//         return result;
//     };
//     auto puller = SRCpp::PullConverter(callback, type, channels, factor);
//     auto framesProduced = 0UL;
//     {
//         auto toPull = 20;
//         auto pullBuffer = std::span { output.data() + framesProduced *
//         channels,
//             toPull * channels };
//         auto [data, error] = puller.convert(output);
//         if (!data.has_value()) {
//             throw std::runtime_error(error);
//         }
//         framesProduced += data->size() / channels;
//     }
//     allowTimes = 2;
//     // this call returns 0, so we expect this to flush out everything
//     framesProduced = 0UL;
//     {
//         auto toPull = 20;
//         auto pullBuffer = std::span { output.data() + framesProduced *
//         channels,
//             toPull * channels };
//         auto [data, error] = puller.convert(output);
//         if (!data.has_value()) {
//             throw std::runtime_error(error);
//         }
//         framesProduced += data->size() / channels;
//     }

//     while (framesProduced < framesExpected && input_span.size()) {
//         auto toPull = std::min(output_frames, framesExpected -
//         framesProduced); auto pullBuffer = std::span { output.data() +
//         framesProduced * channels,
//             toPull * channels };
//         auto [data, error] = puller.convert(output);
//         if (!data.has_value()) {
//             throw std::runtime_error(error);
//         }
//         framesProduced += data->size() / channels;
//     }
//     output.resize(framesProduced * channels);

//     REQUIRE(output == reference);
// }
