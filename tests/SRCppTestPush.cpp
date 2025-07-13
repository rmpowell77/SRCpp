#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>
#include <numbers>
#include <ranges>

namespace {
template <typename To, typename From>
auto doUnsafeTest(std::vector<From> const& input,
    std::vector<To> const& reference, SRCpp::Type type, size_t channels,
    float factor)
{
#if SRCPP_USE_CPP23
    {
        auto output = std::vector<To> {};
        auto pusher = SRCpp::PushConverter(type, channels, factor);
        auto data = [&]() -> std::vector<To> {
            auto output_bytes = pusher.convert_unsafe_expected(
                SRCpp::SampleTypeToFormat<From>(), input.data(),
                input.size() * sizeof(From), SRCpp::SampleTypeToFormat<To>());
            if (!output_bytes.has_value()) {
                throw std::runtime_error(output_bytes.error());
            }
            auto output_value = output_bytes.value();
            std::vector<To> output(output_value.size() / sizeof(To));
            std::memcpy(
                output.data(), output_value.data(), output_value.size());
            return output;
        }();
        output.insert(
            output.end(), data.begin(), data.end()); // append flush to data
        auto flush = [&]() -> std::vector<To> {
            auto output_bytes = pusher.convert_unsafe_expected(
                SRCpp::SampleTypeToFormat<From>(), nullptr, 0,
                SRCpp::SampleTypeToFormat<To>());
            if (!output_bytes.has_value()) {
                throw std::runtime_error(output_bytes.error());
            }
            auto output_value = output_bytes.value();
            std::vector<To> output(output_value.size() / sizeof(To));
            std::memcpy(
                output.data(), output_value.data(), output_value.size());
            return output;
        }();
        output.insert(output.end(), flush.begin(),
            flush.end()); // append flush to data

        // now do the same operation, but fill in output
        auto pusher2 = SRCpp::PushConverter(type, channels, factor);
        auto output_ptr = reinterpret_cast<std::byte*>(output.data());
        auto output_bytes = output.size() * sizeof(To);
        auto output_size
            = pusher.convert_unsafe_expected(SRCpp::SampleTypeToFormat<From>(),
                input.data(), input.size() * sizeof(From),
                SRCpp::SampleTypeToFormat<To>(), output_ptr, output_bytes);
        if (!output_size.has_value()) {
            throw std::runtime_error(output_size.error());
        }
        output_ptr += output_size.value();
        output_bytes -= output_size.value();
        output_size = pusher.convert_unsafe_expected(
            SRCpp::SampleTypeToFormat<From>(), nullptr, 0,
            SRCpp::SampleTypeToFormat<To>(), output_ptr, output_bytes);
        if (!output_size.has_value()) {
            throw std::runtime_error(output_size.error());
        }
        EXPECT_EQ(output_bytes, output_size.value());
        CheckRMS<To, From>(reference, output);
    }
#endif // SRCPP_USE_CPP23
    {
        auto output = std::vector<To> {};
        auto pusher = SRCpp::PushConverter(type, channels, factor);
        auto data = [&]() -> std::vector<To> {
            auto [output_bytes, error] = pusher.convert_unsafe(
                SRCpp::SampleTypeToFormat<From>(), input.data(),
                input.size() * sizeof(From), SRCpp::SampleTypeToFormat<To>());
            if (!output_bytes.has_value()) {
                throw std::runtime_error(error);
            }
            auto output_value = output_bytes.value();
            std::vector<To> output(output_value.size() / sizeof(To));
            std::memcpy(
                output.data(), output_value.data(), output_value.size());
            return output;
        }();
        output.insert(
            output.end(), data.begin(), data.end()); // append flush to data
        auto flush = [&]() -> std::vector<To> {
            auto [output_bytes, error]
                = pusher.convert_unsafe(SRCpp::SampleTypeToFormat<From>(),
                    nullptr, 0, SRCpp::SampleTypeToFormat<To>());
            if (!output_bytes.has_value()) {
                throw std::runtime_error(error);
            }
            auto output_value = output_bytes.value();
            std::vector<To> output(output_value.size() / sizeof(To));
            std::memcpy(
                output.data(), output_value.data(), output_value.size());
            return output;
        }();
        output.insert(output.end(), flush.begin(),
            flush.end()); // append flush to data
        CheckRMS<To, From>(reference, output);

        // now do the same operation, but fill in output
        auto pusher2 = SRCpp::PushConverter(type, channels, factor);
        auto output_ptr = reinterpret_cast<std::byte*>(output.data());
        auto output_bytes = output.size() * sizeof(To);
        auto [output_size, error]
            = pusher.convert_unsafe(SRCpp::SampleTypeToFormat<From>(),
                input.data(), input.size() * sizeof(From),
                SRCpp::SampleTypeToFormat<To>(), output_ptr, output_bytes);
        if (!output_size.has_value()) {
            throw std::runtime_error(error);
        }
        output_ptr += output_size.value();
        output_bytes -= output_size.value();
        std::tie(output_size, error)
            = pusher.convert_unsafe(SRCpp::SampleTypeToFormat<From>(), nullptr,
                0, SRCpp::SampleTypeToFormat<To>(), output_ptr, output_bytes);
        if (!output_size.has_value()) {
            throw std::runtime_error(error);
        }
        EXPECT_EQ(output_bytes, output_size.value());
        CheckRMS<To, From>(reference, output);
    }
}

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
        output1.insert(output1.end(), data->begin(),
            data->end()); // append flush to data
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
        output1.insert(output1.end(), data->begin(),
            data->end()); // append flush to data
        std::tie(data, error) = pusher2.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        output2.insert(output2.end(), data->begin(),
            data->end()); // append flush to data
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
        output1.insert(output1.end(), data->begin(),
            data->end()); // append flush to data
    }

    // moving into a new pusher. It should still continue to work.
    auto pusher2 = std::move(pusher);

    // push the rest
    {
        auto framesForThis = framesLeft;
        auto [data, error] = pusher2.convert<float>(std::span {
            input.begin(), input.begin() + framesForThis * channels });
        output1.insert(output1.end(), data->begin(),
            data->end()); // append flush to data
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
        output2.insert(output2.end(), data->begin(),
            data->end()); // append flush to data
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

TEST(SRCppPush, Push1)
{
    auto frames = 64;
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

        auto output = std::vector<float> {};
        auto pusher = SRCpp::PushConverter(type, channels, factor);

        auto input_span = std::span { input };
        auto framesLeft = input_span.size() / channels;
        while (framesLeft) {
            auto framesForThis = 1;
            auto [data, error]
                = pusher.convert<float>(std::span<float> { input_span.begin(),
                    input_span.begin() + framesForThis * channels });
            if (!data.has_value()) {
                throw std::runtime_error(error);
            }
            input_span = input_span.subspan(framesForThis * channels);
            framesLeft -= framesForThis;
            output.insert(output.end(), data->begin(),
                data->end()); // append flush to data
        }

        {
            auto [flush, error] = pusher.flush<float>();
            if (!flush.has_value()) {
                throw std::runtime_error(error);
            }
            output.insert(output.end(), flush->begin(),
                flush->end()); // append flush to data
        }

        EXPECT_EQ(output, reference);
    }
}

TEST(SRCppPush, UnsafeConvert)
{
    auto frames = 256;
    auto type = SRCpp::Type::Sinc_BestQuality;
    auto factor = 0.5;
    auto hz = std::vector<float> { 3000.0f, 40.0f };
    auto channels = hz.size();

    auto inputFloat = makeSin(hz, 48000.0, frames);
    auto inputShort = ConvertTo<short>(inputFloat);
    auto inputInt = ConvertTo<int>(inputFloat);
    auto referenceFloat
        = CreateOneShotReference(inputFloat, channels, factor, type);
    auto referenceShort = ConvertTo<short>(referenceFloat);
    auto referenceInt = ConvertTo<int>(referenceFloat);
    doUnsafeTest(inputShort, referenceShort, type, channels, factor);
    doUnsafeTest(inputInt, referenceShort, type, channels, factor);
    doUnsafeTest(inputFloat, referenceShort, type, channels, factor);
    doUnsafeTest(inputShort, referenceInt, type, channels, factor);
    doUnsafeTest(inputInt, referenceInt, type, channels, factor);
    doUnsafeTest(inputFloat, referenceInt, type, channels, factor);
    doUnsafeTest(inputShort, referenceFloat, type, channels, factor);
    doUnsafeTest(inputInt, referenceFloat, type, channels, factor);
    doUnsafeTest(inputFloat, referenceFloat, type, channels, factor);
}
}
