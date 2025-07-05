#pragma once
#include <SRCpp/SRCpp.hpp>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <numeric>
#include <ranges>
#include <vector>

inline auto makeSin(const std::vector<float>& hz, float sr, size_t len)
{
    size_t channels = hz.size();
    std::vector<float> data(len * channels);

    using namespace std::numbers;
    for (size_t i : std::views::iota(0UL, len)) {
        for (size_t ch = 0; ch < channels; ++ch) {
            data.at(i * channels + ch) = std::sin(hz[ch] * i * 2 * pi / sr);
        }
    }

    return data;
}

auto CreateOneShotReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>;
auto CreatePushReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>;
auto CreatePullReference(std::vector<float> input, size_t channels,
    double factor, SRCpp::Type type, size_t in_size, size_t out_size)
    -> std::vector<float>;

template <typename To>
std::vector<To> ConvertTo(const std::vector<float>& input)
{
    if constexpr (std::is_same_v<To, short>) {
        std::vector<To> output(input.size());
        src_float_to_short_array(input.data(), output.data(), input.size());
        return output;
    } else if constexpr (std::is_same_v<To, int>) {
        std::vector<To> output(input.size());
        src_float_to_int_array(input.data(), output.data(), input.size());
        return output;
    } else {
        return input;
    }
}

template <typename T>
auto CalculateRMSError(std::span<const T> a, std::span<const T> b) -> double
{
    assert(a.size() == b.size());
    return std::sqrt(
        std::inner_product(a.begin(), a.end(), b.begin(), 0.0, std::plus<>(),
            [](const T& x, const T& y) {
                double diff = static_cast<double>(x) - static_cast<double>(y);
                return diff * diff;
            })
        / a.size());
}

inline auto ToDecibels(double value, double ref = 1.0) -> double
{
    return 20.0 * std::log10(std::abs(value / ref));
}

template <typename To, typename From>
auto CheckRMS(std::span<const To> reference, std::span<const To> output)
{
    constexpr auto compare = []() {
        if constexpr (std::is_same_v<From, short>
            || std::is_same_v<To, short>) {
            return -80;
        } else if constexpr (std::is_same_v<From, int>
            || std::is_same_v<To, int>) {
            return -160;
        } else {
            return -300;
        }
    }();
    if constexpr (!std::is_same_v<To, float>) {
        auto rms = ToDecibels(CalculateRMSError<To>(reference, output),
            std::numeric_limits<To>::max());
        EXPECT_LE(rms, compare);
    } else {
        auto rms = ToDecibels(CalculateRMSError<To>(reference, output));
        EXPECT_LE(rms, compare);
    }
}

template <typename To = float, typename From = float>
auto ConvertWithOnePull([[maybe_unused]] bool use_cpp20,
    std::vector<From> input, size_t channels, double factor, SRCpp::Type type,
    std::optional<size_t> input_frames = std::nullopt) -> std::vector<To>
{
    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<From>(input);
    // pull more than we need, make sure to run the input dry
    std::vector<To> output(framesExpected * channels * 2);
    auto callback = [&]() -> std::span<From> {
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
    auto puller = SRCpp::PullConverter<From>(callback, type, channels, factor);
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

// What happens is that when we're pulling and input
template <typename To = float, typename From = float>
auto ConvertWithPullOutputFrames([[maybe_unused]] bool use_cpp20,
    std::vector<From> input, size_t channels, double factor, SRCpp::Type type,
    size_t output_frames, size_t input_frames, size_t framesExpected)
    -> std::vector<To>
{
    auto input_span = std::span<From>(input);
    std::vector<To> output(framesExpected * channels * 2);
    auto callback = [&]() -> std::span<From> {
        auto input_samples = [&]() {
            return std::min(input_frames * channels, input_span.size());
        }();

        auto result = input_span.subspan(0, input_samples);
        input_span = input_span.subspan(input_samples);
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto framesProduced = 0UL;
    while (framesProduced < framesExpected) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
#if SRCPP_USE_CPP23
        if (use_cpp20) {
#endif // SRCPP_USE_CPP23
            auto [data, error] = puller.convert(pullBuffer);
            if (!data.has_value()) {
                throw std::runtime_error(error);
            }
            framesProduced += data->size() / channels;
#if SRCPP_USE_CPP23
        } else {
            auto data = puller.convert_expected(pullBuffer);
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

template <typename To, typename From> void RunPullConvertTest()
{
    for (auto cpp_20 : { false, true }) {
        for (auto frames : { 16, 256, 257, 500 }) {
            for (auto type : {
                     SRCpp::Type::ZeroOrderHold,
                     SRCpp::Type::Linear,
                     SRCpp::Type::Sinc_Fastest,
                     SRCpp::Type::Sinc_BestQuality,
                     SRCpp::Type::Sinc_MediumQuality,
                 }) {
                for (auto factor : { 0.5, 1.5, 0.1, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                    for (auto hz : {
                             std::vector<float> { 3000.0f, 40.0f, 1004.0f },
                             std::vector<float> { 3000.0f },
                             std::vector<float> { 3000.0f, 40.0f },
                         }) {
                        auto channels = hz.size();

                        auto inputFloat = makeSin(hz, 48000.0, frames);
                        auto input = ConvertTo<From>(inputFloat);
                        auto referenceFloat = CreatePushReference(
                            inputFloat, channels, factor, type);
                        auto reference = ConvertTo<To>(referenceFloat);

                        {
                            auto output = ConvertWithOnePull<To, From>(
                                cpp_20, input, channels, factor, type);
                            EXPECT_GT(output.size(), 0);
                            // fudge factor for pull
                            if (std::abs(static_cast<int>(reference.size())
                                    - static_cast<int>(output.size()))
                                == static_cast<int>(channels)) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                CheckRMS<To, From>(mangledReference, output);
                            } else {
                                CheckRMS<To, From>(reference, output);
                            }
                        }
                        for (auto input_size : { 4, 32, 33, 128 }) {
                            auto output = ConvertWithOnePull<To, From>(cpp_20,
                                input, channels, factor, type, input_size);
                            EXPECT_GT(output.size(), 0);

                            // fudge factor for pull
                            if (std::abs(static_cast<int>(reference.size())
                                    - static_cast<int>(output.size()))
                                == static_cast<int>(channels)) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                CheckRMS<To, From>(mangledReference, output);
                            } else {
                                CheckRMS<To, From>(reference, output);
                            }
                        }
                        for (auto output_frames : { 4, 32, 33, 128 }) {
                            for (auto input_frames : { 4, 32, 33, 128 }) {
                                auto referenceFloat2 = CreatePullReference(
                                    inputFloat, channels, factor, type,
                                    input_frames, output_frames);
                                auto reference2
                                    = ConvertTo<To>(referenceFloat2);
                                auto output
                                    = ConvertWithPullOutputFrames<To, From>(
                                        cpp_20, input, channels, factor, type,
                                        output_frames, input_frames,
                                        reference2.size() / channels);
                                EXPECT_GT(output.size(), 0);

                                CheckRMS<To, From>(reference2, output);
                            }
                        }
                    }
                }
            }
        }
    }
}

template <typename To, typename From>
auto ConvertWithPush([[maybe_unused]] bool cpp_20, std::vector<From> input,
    size_t channels, double factor, SRCpp::Type type, size_t input_frames)
    -> std::vector<To>
{
    auto output = std::vector<To> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto framesLeft = input.size() / channels;
    while (framesLeft) {
        auto framesForThis = std::min(input_frames, input.size() / channels);
        auto data = [&]() -> std::vector<To> {
#if SRCPP_USE_CPP23
            if (cpp_20) {
#endif // SRCPP_USE_CPP23
                auto [data, error] = pusher.convert<To>(std::span<From> {
                    input.begin(), input.begin() + framesForThis * channels });
                if (!data.has_value()) {
                    throw error;
                }
                return *data;
#if SRCPP_USE_CPP23
            } else {
                auto data = pusher.convert_expected<To>(std::span<From> {
                    input.begin(), input.begin() + framesForThis * channels });
                if (!data.has_value()) {
                    throw std::runtime_error(data.error());
                }
                return data.value();
            }
#endif //  SRCPP_USE_CPP23
        }();
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        framesLeft -= framesForThis;
        output.insert(
            output.end(), data.begin(), data.end()); // append flush to data
    }
    auto flush = [&] {
#if SRCPP_USE_CPP23
        if (cpp_20) {
#endif // SRCPP_USE_CPP23
            auto [data, error] = pusher.flush<To>();
            if (!data.has_value()) {
                throw error;
            }
            return *data;
#if SRCPP_USE_CPP23
        } else {
            auto data = pusher.flush_expected<To>();
            if (!data.has_value()) {
                throw std::runtime_error(data.error());
            }
            return data.value();
        }
#endif //  SRCPP_USE_CPP23
    }();
    output.insert(
        output.end(), flush.begin(), flush.end()); // append flush to data
    return output;
}

template <typename To, typename From>
auto ConvertWithPushReuseMemory([[maybe_unused]] bool cpp_20,
    std::vector<From> input, size_t channels, double factor, SRCpp::Type type,
    size_t input_frames)
{
    auto framesLeft = input.size() / channels;
    auto output = std::vector<To>(framesLeft * channels * factor * 2);
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto outputSpan = std::span { output };
    auto samplesProduced = 0;
    while (framesLeft) {
        auto framesForThis = std::min(input_frames, input.size() / channels);
        auto data = [&] {
#if SRCPP_USE_CPP23
            if (cpp_20) {
#endif // SRCPP_USE_CPP23
                auto [data, error] = pusher.convert(
                    std::span { input.begin(),
                        input.begin() + framesForThis * channels },
                    outputSpan);
                if (!data.has_value()) {
                    throw error;
                }
                return *data;
#if SRCPP_USE_CPP23
            } else {
                auto data = pusher.convert_expected(
                    std::span { input.begin(),
                        input.begin() + framesForThis * channels },
                    outputSpan);
                if (!data.has_value()) {
                    throw std::runtime_error(data.error());
                }
                return data.value();
            }
#endif // SRCPP_USE_CPP23
        }();
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        auto size = data.size();
        samplesProduced += size;
        framesLeft -= framesForThis;
        outputSpan = outputSpan.subspan(size);
    }
    auto flush = [&] {
#if SRCPP_USE_CPP23
        if (cpp_20) {
#endif // SRCPP_USE_CPP23
            auto [data, error] = pusher.convert(std::span<From> {}, outputSpan);
            if (!data.has_value()) {
                throw error;
            }
            return *data;
#if SRCPP_USE_CPP23
        } else {
            auto data = pusher.convert_expected(std::span<From> {}, outputSpan);
            if (!data.has_value()) {
                throw std::runtime_error(data.error());
            }
            return data.value();
        }
#endif // SRCPP_USE_CPP23
    }();
    auto size = flush.size();
    samplesProduced += size;
    output.resize(samplesProduced);
    return output;
}

template <typename To, typename From> void RunPushConvertTest()
{
    for (auto cpp_20 : { false, true }) {
        for (auto frames : { 16, 256, 257, 500 }) {
            for (auto type : { SRCpp::Type::Sinc_BestQuality,
                     SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                     SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
                for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                    for (auto hz : { std::vector<float> { 3000.0f },
                             std::vector<float> { 3000.0f, 40.0f },
                             std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                        auto channels = hz.size();

                        auto inputFloat = makeSin(hz, 48000.0, frames);
                        auto input = ConvertTo<From>(inputFloat);
                        auto referenceFloat = CreatePushReference(
                            inputFloat, channels, factor, type);
                        auto reference = ConvertTo<To>(referenceFloat);

                        {
                            auto output = ConvertWithPush<To>(
                                cpp_20, input, channels, factor, type, frames);
                            CheckRMS<To, From>(reference, output);
                        }
                        {
                            auto output = ConvertWithPushReuseMemory<To>(
                                cpp_20, input, channels, factor, type, frames);
                            CheckRMS<To, From>(reference, output);
                        }
                        for (auto input_size : { 4, 8, 16, 32, 64 }) {
                            auto output = ConvertWithPush<To>(cpp_20, input,
                                channels, factor, type, input_size);

                            if (type == SRCpp::Type::ZeroOrderHold) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                CheckRMS<To, From>(mangledReference, output);
                            } else {
                                CheckRMS<To, From>(reference, output);
                            }
                        }
                    }
                }
            }
        }
    }
}
