#pragma once
#include <SRCpp/SRCpp.hpp>
#include <cmath>
#include <numbers>
#include <ranges>
#include <vector>

inline auto makeSin(const std::vector<float>& hz, float sr, size_t len)
{
    size_t channels = hz.size();
    std::vector<float> data(len * channels);

    using namespace std::numbers;
    for (size_t i : std::views::iota(0UL, len)) {
        for (size_t ch = 0; ch < channels; ++ch) {
            data.at(i * channels + ch) = std::sin(hz[ch] * i * 2 * pi / sr);
        }
    }

    return data;
}

auto CreateOneShotReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>;
auto CreatePushReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>;
auto CreatePullReference(std::vector<float> input, size_t channels,
    double factor, SRCpp::Type type, size_t in_size, size_t out_size)
    -> std::vector<float>;
