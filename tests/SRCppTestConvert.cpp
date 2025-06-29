#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <gtest/gtest.h>
#include <numbers>
#include <ranges>

TEST(SRCpp, Resample) {
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto&& hz : { std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();
                    auto input = makeSin(hz, 48000.0, frames);

#if SRCPP_USE_CPP23
                    {
                        auto reference = CreateOneShotReference(
                            input, channels, factor, type);
                        auto output = SRCpp::Convert_expected(
                            input, type, channels, factor);
                        EXPECT_EQ(output, reference);
                    }
#endif // SRCPP_USE_CPP23
                    {
                        auto reference = CreateOneShotReference(
                            input, channels, factor, type);
                        auto [output, error]
                            = SRCpp::Convert(input, type, channels, factor);
                        EXPECT_EQ(*output, reference);
                    }
                }
            }
        }
    }
}
