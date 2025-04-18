#pragma once
/*
MIT License

Copyright (c) 2025 Richard Powell

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <expected>
#include <format>
#include <samplerate.h>
#include <span>
#include <string>
#include <vector>

namespace SRCpp {

enum struct Type : int {
    Sinc_BestQuality = SRC_SINC_BEST_QUALITY,
    Sinc_MediumQuality = SRC_SINC_MEDIUM_QUALITY,
    Sinc_Fastest = SRC_SINC_FASTEST,
    ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR
};

auto Resample(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>
{
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
    if (auto result = src_simple(&src_data, static_cast<int>(type), channels);
        result != 0) {
        return std::unexpected(src_strerror(result));
    }

    return std::span { output.data(),
        static_cast<size_t>(src_data.output_frames_gen) };
}

auto Resample(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::expected<std::vector<float>, std::string>
{
    std::vector<float> output(input.size() * factor * channels);
    auto result = Resample(input, output, type, channels, factor);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    output.resize(result->size());
    return output;
}

class SampleRateConverter {
public:
    SampleRateConverter(SRCpp::Type type, int channels, double factor)
        : factor_ { factor }
    {
        auto error = 0;
        state_ = src_new(static_cast<int>(type), channels, &error);
        if (error != 0) {
            throw std::runtime_error(src_strerror(error));
        }
    }
    ~SampleRateConverter() { src_delete(state_); }

    auto operator()(std::span<const float> input, std::span<float> output)
        -> std::expected<std::span<float>, std::string>
    {
        float dummy = 0.f;
        auto src_data = SRC_DATA {
            input.empty() ? &dummy : input.data(),
            output.data(),
            static_cast<long>(input.size()),
            static_cast<long>(output.size()),
            0,
            0,
            input.empty(),
            factor_,
        };
        if (auto result = src_process(state_, &src_data); result != 0) {
            return std::unexpected(src_strerror(result));
        }
        outstanding_ += src_data.input_frames_used * factor_
            - src_data.output_frames_gen;

        return std::span { output.data(),
            static_cast<size_t>(src_data.output_frames_gen) };
    }

    auto operator()(std::span<const float> input)
        -> std::expected<std::vector<float>, std::string>
    {
        std::vector<float> output(
            (input.empty()) ? outstanding_ : input.size() * factor_);
        auto result = this->operator()(input, output);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        output.resize(result->size());
        return output;
    }

    SRC_STATE* get() { return state_; }

private:
    SRC_STATE* state_ { nullptr };
    double factor_ { 1.0 };
    size_t outstanding_ { 0 };
};

}
// Custom formatter specialization for SRC_DATA
template <> struct std::formatter<SRC_DATA> {
    // No parse function needed since we have no format specifiers
    static constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    // Formats the CalChart::Coord object
    template <typename FormatContext>
    static auto format(const SRC_DATA& data, FormatContext& ctx)
    {
        return std::format_to(ctx.out(),
            "SRC@{} in{}: {}[{}, {}), out: {}[{}, {})", data.src_ratio,
            data.end_of_input ? "(eof)" : "",
            static_cast<const void*>(data.data_in), data.input_frames_used,
            data.input_frames, static_cast<void*>(data.data_out),
            data.output_frames_gen, data.output_frames);
    }
};
