#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <cassert>
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>
#include <numbers>
#include <ranges>
#include <span>

namespace {
template <typename To, typename From> void RunResampleTest()
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto&& hz : { std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();

                    auto inputFloat = makeSin(hz, 48000.0, frames);
                    auto input = ConvertTo<From>(inputFloat);
                    auto referenceFloat = CreateOneShotReference(
                        inputFloat, channels, factor, type);
                    auto reference = ConvertTo<To>(referenceFloat);

#if SRCPP_USE_CPP23
                    {
                        auto output = SRCpp::Convert_expected<To, From>(
                            input, type, channels, factor);
                        auto output_value = output.value();
                        CheckRMS<To, From>(reference, output_value);
                        auto output_span = SRCpp::Convert_expected(
                            input, output_value, type, channels, factor);
                        CheckRMS<To, From>(reference, output_span.value());
                    }
#endif // SRCPP_USE_CPP23
                    {
                        auto [output, error] = SRCpp::Convert<To, From>(
                            input, type, channels, factor);
                        auto output_value = output.value();
                        CheckRMS<To, From>(reference, output_value);
                        auto [output_span, error2] = SRCpp::Convert(
                            input, output_value, type, channels, factor);
                        CheckRMS<To, From>(reference, output_span.value());
                    }
                }
            }
        }
    }
}

template <typename To, typename From>
auto doUnsafeTest(std::vector<From> const& input,
    std::vector<To> const& reference, SRCpp::Type type, size_t channels,
    float factor)
{
#if SRCPP_USE_CPP23
    {
        auto output_bytes
            = SRCpp::Convert_unsafe_expected(SRCpp::SampleTypeToFormat<From>(),
                input.data(), input.size() * sizeof(From),
                SRCpp::SampleTypeToFormat<To>(), type, channels, factor);

        auto output_value = output_bytes.value();

        std::vector<To> output(output_value.size() / sizeof(To));
        std::memcpy(output.data(), output_value.data(), output_value.size());

        CheckRMS<To, From>(reference, output);
        auto output_size = SRCpp::Convert_unsafe_expected(
            SRCpp::SampleTypeToFormat<From>(), input.data(),
            input.size() * sizeof(From), SRCpp::SampleTypeToFormat<To>(),
            output.data(), output.size() * sizeof(To), type, channels, factor);
        output.resize(output_size.value() / sizeof(To));
        CheckRMS<To, From>(reference, output);
    }
#endif // SRCPP_USE_CPP23
    {
        auto [output_bytes, error]
            = SRCpp::Convert_unsafe(SRCpp::SampleTypeToFormat<From>(),
                input.data(), input.size() * sizeof(From),
                SRCpp::SampleTypeToFormat<To>(), type, channels, factor);

        auto output_value = output_bytes.value();

        std::vector<To> output(output_value.size() / sizeof(To));
        std::memcpy(output.data(), output_value.data(), output_value.size());

        CheckRMS<To, From>(reference, output);
        auto [output_size, error2] = SRCpp::Convert_unsafe(
            SRCpp::SampleTypeToFormat<From>(), input.data(),
            input.size() * sizeof(From), SRCpp::SampleTypeToFormat<To>(),
            output.data(), output.size() * sizeof(To), type, channels, factor);
        output.resize(output_size.value() / sizeof(To));
        CheckRMS<To, From>(reference, output);
    }
}

}

TEST(SRCpp, ResampleShortShort) { RunResampleTest<short, short>(); }
TEST(SRCpp, ResampleShortInt) { RunResampleTest<short, int>(); }
TEST(SRCpp, ResampleShortFloat) { RunResampleTest<short, float>(); }

TEST(SRCpp, ResampleIntShort) { RunResampleTest<int, short>(); }
TEST(SRCpp, ResampleIntInt) { RunResampleTest<int, int>(); }
TEST(SRCpp, ResampleIntFloat) { RunResampleTest<int, float>(); }

TEST(SRCpp, ResampleFloatShort) { RunResampleTest<float, short>(); }
TEST(SRCpp, ResampleFloatInt) { RunResampleTest<float, int>(); }
TEST(SRCpp, ResampleFloatFloat) { RunResampleTest<float, float>(); }

TEST(SRCpp, UnsafeConvert)
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
