#include <SRCpp/SRCpp.hpp>
#include <algorithm>
#include <cmath>
#include <expected>
#include <numbers>
#include <print>
#include <ranges>
#include <vector>

auto makeSin([[maybe_unused]] float hz, [[maybe_unused]] float sr,
    [[maybe_unused]] size_t len)
{
    using namespace std::numbers;
    std::vector<float> data(len);
    for (auto i : std::views::iota(0UL, len)) {
        data.at(i) = std::sin(hz * i * 2 * pi / sr);
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
            auto output = src.push(data);
            if (output.has_value()) {

                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
        {
            auto output = src.push({});
            if (output.has_value()) {

                std::println("output({})", output->size());
                for (auto i : *output) {
                    std::print("{} ", i);
                }
                std::println("");
            }
        }
    }

    return 0;
}

int main()
{
    try_simple();
    try_normal();
    return 0;
}