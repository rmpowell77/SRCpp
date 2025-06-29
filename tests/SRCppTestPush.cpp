#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <gtest/gtest.h>
#include <numbers>
#include <ranges>

auto ConvertWithPush([[maybe_unused]] bool cpp_20, std::vector<float> input,
    size_t channels, double factor, SRCpp::Type type, size_t input_frames)
{
    auto output = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto framesLeft = input.size() / channels;
    while (framesLeft) {
        auto framesForThis = std::min(input_frames, input.size() / channels);
        auto data = [&] {
#if SRCPP_USE_CPP23
            if (cpp_20) {
#endif // SRCPP_USE_CPP23
                auto [data, error] = pusher.convert({ input.begin(),
                    input.begin() + framesForThis * channels });
                if (!data.has_value()) {
                    throw error;
                }
                return *data;
#if SRCPP_USE_CPP23
            } else {
                auto data = pusher.convert_expected({ input.begin(),
                    input.begin() + framesForThis * channels });
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
            auto [data, error] = pusher.flush();
            if (!data.has_value()) {
                throw error;
            }
            return *data;
#if SRCPP_USE_CPP23
        } else {
            auto data = pusher.flush_expected();
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

auto ConvertWithPushReuseMemory([[maybe_unused]] bool cpp_20,
    std::vector<float> input, size_t channels, double factor, SRCpp::Type type,
    size_t input_frames)
{
    auto framesLeft = input.size() / channels;
    auto output = std::vector<float>(framesLeft * channels * factor * 2);
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
                    { input.begin(), input.begin() + framesForThis * channels },
                    outputSpan);
                if (!data.has_value()) {
                    throw error;
                }
                return *data;
#if SRCPP_USE_CPP23
            } else {
                auto data = pusher.convert_expected(
                    { input.begin(), input.begin() + framesForThis * channels },
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
            auto [data, error] = pusher.convert({}, outputSpan);
            if (!data.has_value()) {
                throw error;
            }
            return *data;
#if SRCPP_USE_CPP23
        } else {
            auto data = pusher.convert_expected({}, outputSpan);
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

TEST(SRCppPush, PushConverter) {
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
                        auto input = makeSin(hz, 48000.0, frames);

                        auto reference = CreatePushReference(
                            input, channels, factor, type);
                        {
                            auto output = ConvertWithPush(
                                cpp_20, input, channels, factor, type, frames);

                            EXPECT_EQ(output, reference);
                        }
                        {
                            auto output = ConvertWithPushReuseMemory(
                                cpp_20, input, channels, factor, type, frames);

                            EXPECT_EQ(output, reference);
                        }
                        for (auto input_size : { 4, 8, 16, 32, 64 }) {
                            auto output = ConvertWithPush(cpp_20, input,
                                channels, factor, type, input_size);

                            if (type == SRCpp::Type::ZeroOrderHold) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                                EXPECT_EQ(output, mangledReference);
                            } else {
                                EXPECT_EQ(output, reference);
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST(SRCppPush, CopyingConverter) {
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
        auto [data, error] = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
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
        auto [data, error] = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
        std::tie(data, error) = pusher2.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        output2.insert(
            output2.end(), data->begin(), data->end()); // append flush to data
    }

    {
        auto [flush, error] = pusher.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }
    {
        auto [flush, error] = pusher2.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(output2.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output1, reference);
    EXPECT_EQ(output2, reference);
}

TEST(SRCppPush, MovingConverter) {
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
        auto [data, error] = pusher.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
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
        auto [data, error] = pusher2.convert(
            { input.begin(), input.begin() + framesForThis * channels });
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
    }
    {
        auto [flush, error] = pusher2.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output1, reference);
}

TEST(SRCppPush, PushAfterFlush) {
    auto frames = 64;
    auto firstPush = 10;
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.9;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();
    auto input1 = makeSin(hz, 48000.0, frames);
    auto input2 = input1;

    auto reference = CreatePushReference(input1, channels, factor, type);

    auto output1 = std::vector<float> {};
    auto output2 = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);

    {
        auto framesForThis = firstPush;
        auto [data, error] = pusher.convert(
            { input1.begin(), input1.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        std::tie(data, error) = pusher.flush();
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
    }

    // now convert the other input, should match reference
    {
        auto [data, error] = pusher.convert(input2);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(
            output2.end(), data->begin(), data->end()); // append flush to data
    }

    {
        auto [flush, error] = pusher.flush();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(output2.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output2, reference);
}
