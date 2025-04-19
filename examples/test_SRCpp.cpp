#include <SRCpp/SRCpp.hpp>
#include <algorithm>
#include <cmath>
#include <expected>
#include <print>
#include <ranges>
#include <vector>

auto makeSin([[maybe_unused]] float hz, [[maybe_unused]] float sr,
    [[maybe_unused]] size_t len)
{
    std::vector<float> data(len);
    for (auto i : std::views::iota(0UL, len)) {
        data.at(i) = std::sin(hz * i * 2 * M_PI / sr);
    }
    return data;
}

int try_simple()
{
    auto data = makeSin(3000.0, 48000.0, 128);

    std::println("data");
    for (auto i : data) {
        std::print("{} ", i);
    }
    std::println("");
    {
        auto output
            = SRCpp::Resample(data, SRCpp::Type::Sinc_MediumQuality, 1, 0.1);

        if (output.has_value()) {

            std::println("data");
            for (auto i : *output) {
                std::print("{} ", i);
            }
            std::println("");
        }
    }
    return 0;
}

int try_normal()
{
    auto data = makeSin(3000.0, 48000.0, 128);

    std::println("data");
    for (auto i : data) {
        std::print("{} ", i);
    }
    std::println("");
    {
        auto src
            = SRCpp::PushConverter(SRCpp::Type::Sinc_MediumQuality, 1, 0.1);
        {
            auto output = src(data);
            if (output.has_value()) {

                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
        {
            auto output = src({});
            if (output.has_value()) {

                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
    }
    std::vector<float> output(128);
    int error = 0;
    auto src = SRCpp::PushConverter(SRCpp::Type::Sinc_MediumQuality, 1, 0.1);
    std::println("error {}", error);

    auto src_data = SRC_DATA {
        data.data(),
        output.data(),

        128,
        128,
        0,
        0,

        0,

        0.1,
    };
    std::println("{}", src_data);
    auto result = src_process(src.get(), &src_data);
    std::println("result {} {}", result, src_strerror(result));
    std::println("data");
    for (auto i : output) {
        std::print("{} ", i);
    }
    std::println("");
    std::println("{}", src_data);
    src_data.end_of_input = 1;
    src_data.input_frames_used = 0;
    std::println("{}", src_data);

    result = src_process(src.get(), &src_data);
    std::println("result {} {}", result, src_strerror(result));
    std::println("data");
    for (auto i : output) {
        std::print("{} ", i);
    }
    std::println("");
    std::println("{}", src_data);
    return 0;
}

int main()
{
    try_simple();
    try_normal();
    return 0;
}