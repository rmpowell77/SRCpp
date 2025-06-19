#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <ranges>

TEST_CASE("std::format with SRC_DATA", "[SRC_DATA]")
{
    auto* badPtr = reinterpret_cast<float*>(0xbeef);
    auto* badPtr2 = reinterpret_cast<float*>(0xBABE);

    auto src_data = SRC_DATA {
        badPtr,
        badPtr2,
        42,
        314,
        10,
        15,
        0,
        1.5,
    };

    REQUIRE(std::format("{}", src_data)
        == "SRC@1.5 in: 0xbeef[10, 42), out: 0xbabe[15, 314)");

    src_data.end_of_input = 1;
    REQUIRE(std::format("{}", src_data)
        == "SRC@1.5 in(eof): 0xbeef[10, 42), out: 0xbabe[15, 314)");
}
