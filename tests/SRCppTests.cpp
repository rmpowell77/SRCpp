#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ranges>

TEST_CASE("std::format with SRC_DATA", "[SRC_DATA]")
{
    auto* badPtr = reinterpret_cast<float*>(0xdeadbeef);
    auto* badPtr2 = reinterpret_cast<float*>(0xCAFEBABE);

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
        == "SRC@1.5 in: 0xdeadbeef[10, 42), out: 0xcafebabe[15, 314)");

    src_data.end_of_input = 1;
    REQUIRE(std::format("{}", src_data)
        == "SRC@1.5 in(eof): 0xdeadbeef[10, 42), out: 0xcafebabe[15, 314)");
}

auto makeSin(const std::vector<float>& hz, float sr, size_t len)
{
    size_t channels = hz.size();
    std::vector<float> data(len * channels);

    for (size_t i : std::views::iota(0UL, len)) {
        for (size_t ch = 0; ch < channels; ++ch) {
            data.at(i * channels + ch) = std::sin(hz[ch] * i * 2 * M_PI / sr);
        }
    }

    return data;
}

int try_simple(const std::vector<float>& hz, double factor, SRCpp::Type type)
{
    auto channels = hz.size();
    auto input = makeSin(hz, 48000.0, 16);

    auto reference = [&] {
        std::vector<float> output(input.size() * factor * channels * 2);
        auto src_data = SRC_DATA {
            input.data(),
            output.data(),
            static_cast<long>(input.size() / channels),
            static_cast<long>(output.size() / channels),
            0,
            0,
            1,
            factor,
        };
        if (auto result
            = src_simple(&src_data, static_cast<int>(type), channels);
            result != 0) {
            throw std::runtime_error(src_strerror(result));
        }

        output.resize(src_data.output_frames_gen);
        return output;
    }();
    auto output = SRCpp::Resample(input, type, channels, factor);

    REQUIRE(output == reference);
    return 0;
}

TEST_CASE("AudioProcessor Sample Rate", "[AudioProcessor]")
{
    for (auto type : { SRCpp::Type::Sinc_BestQuality,
             SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
             SRCpp::Type::ZeroOrderHold/*,
             SRCpp::Type::Linear*/ }) {
        for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
            try_simple({ 3000.0 }, factor, type);
            // try_simple({ 3000.0, 40 }, factor, type);
            // try_simple({ 3000.0, 40, 1004.0 }, factor, type);
        }
    }
}
