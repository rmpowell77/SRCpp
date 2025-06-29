#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <gtest/gtest.h>
#include <numbers>
#include <ranges>

TEST(SRCppPush, CopyingConverter)
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
        auto [data, error] = pusher.convert<float>(std::span<float> {
            input.begin(), input.begin() + framesForThis * channels });
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
        auto [data, error] = pusher.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
        std::tie(data, error) = pusher2.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        output2.insert(
            output2.end(), data->begin(), data->end()); // append flush to data
    }

    {
        auto [flush, error] = pusher.flush<float>();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }
    {
        auto [flush, error] = pusher2.flush<float>();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(output2.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output1, reference);
    EXPECT_EQ(output2, reference);
}

TEST(SRCppPush, MovingConverter)
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
        auto [data, error] = pusher.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
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
        auto [data, error] = pusher2.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
        output1.insert(
            output1.end(), data->begin(), data->end()); // append flush to data
    }
    {
        auto [flush, error] = pusher2.flush<float>();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output1.insert(output1.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output1, reference);
}

TEST(SRCppPush, PushAfterFlush)
{
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
        auto [data, error] = pusher.convert<float>(std::span {
            input1.begin(), input1.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        std::tie(data, error) = pusher.flush<float>();
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
    }

    // now convert the other input, should match reference
    {
        auto [data, error] = pusher.convert<float>(input2);
        if (!data.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(
            output2.end(), data->begin(), data->end()); // append flush to data
    }

    {
        auto [flush, error] = pusher.flush<float>();
        if (!flush.has_value()) {
            throw std::runtime_error(error);
        }
        output2.insert(output2.end(), flush->begin(),
            flush->end()); // append flush to data
    }

    EXPECT_EQ(output2, reference);
}
