#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <ranges>

TEST_CASE("Resample", "[SRCpp]")
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
                        = CreateOneShotReference(input, channels, factor, type);
                    auto output = SRCpp::Convert(input, type, channels, factor);

                    REQUIRE(output == reference);
                }
            }
        }
    }
}
