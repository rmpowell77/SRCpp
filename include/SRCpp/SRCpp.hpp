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

#include <cmath>
#include <format>
#include <functional>
#include <optional>
#include <samplerate.h>
#include <span>
#include <string>
#include <utility>
#include <vector>

#if __cplusplus >= 202302L
#include <expected>
#define SRCPP_USE_CPP23 1
#else
#define SRCPP_USE_CPP23 0
#endif

namespace SRCpp {

enum struct Type : int {
    Sinc_BestQuality = SRC_SINC_BEST_QUALITY,
    Sinc_MediumQuality = SRC_SINC_MEDIUM_QUALITY,
    Sinc_Fastest = SRC_SINC_FASTEST,
    ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR
};

#if SRCPP_USE_CPP23
auto Convert_expected(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>;
auto Convert_expected(
    std::span<const float> input, SRCpp::Type type, int channels, double factor)
    -> std::expected<std::vector<float>, std::string>;
#endif // SRCPP_USE_CPP23

auto Convert(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::span<float>>, std::string>;
auto Convert(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<float>>, std::string>;

class PushConverter {
public:
    PushConverter(SRCpp::Type type, int channels, double factor);
    ~PushConverter();
    PushConverter(const PushConverter& other);
    PushConverter& operator=(const PushConverter& other);
    PushConverter(PushConverter&& other) noexcept;
    PushConverter& operator=(PushConverter&& other) noexcept;

#if SRCPP_USE_CPP23
    auto convert_expected(std::span<const float> input, std::span<float> output)
        -> std::expected<std::span<float>, std::string>;

    auto convert_expected(std::span<const float> input)
        -> std::expected<std::vector<float>, std::string>;

    auto flush_expected() -> std::expected<std::vector<float>, std::string>;
#endif // SRCPP_USE_CPP23

    auto convert(std::span<const float> input, std::span<float> output)
        -> std::pair<std::optional<std::span<float>>, std::string>;

    auto convert(std::span<const float> input)
        -> std::pair<std::optional<std::vector<float>>, std::string>;

    auto flush() -> std::pair<std::optional<std::vector<float>>, std::string>;

private:
    SRC_STATE* state_ { nullptr };
    SRCpp::Type type_ { SRC_SINC_BEST_QUALITY };
    int channels_ { 0 };
    double factor_ { 1.0 };
    const float dummy_ {};
    std::vector<float> reserved_input_;
    std::vector<float> last_input_;
    size_t input_frames_consumed_ { 0 };
    size_t output_frames_produced_ { 0 };

    auto convert(
        std::span<const float> input, std::span<float> output, bool end)
        -> std::pair<
            std::optional<std::pair<std::span<const float>, std::span<float>>>,
            std::string>;
    auto convertWithFixFor208(
        std::span<const float> input, std::span<float> output, bool end)
        -> std::pair<
            std::optional<std::pair<std::span<const float>, std::span<float>>>,
            std::string>;
};

class PullConverter {
public:
    using callback_t = std::function<std::span<float>()>;
    PullConverter(
        callback_t callback, SRCpp::Type type, int channels, double factor);
    ~PullConverter();

    // Copying is dangerous, because then the callback is shared.
    PullConverter(const PullConverter&) = delete;
    PullConverter& operator=(const PullConverter&) = delete;
    PullConverter(PullConverter&& other) noexcept;
    PullConverter& operator=(PullConverter&& other) noexcept;

#if SRCPP_USE_CPP23
    auto convert_expected(std::span<float> output)
        -> std::expected<std::span<float>, std::string>;
#endif // SRCPP_USE_CPP23

    auto convert(std::span<float> output)
        -> std::pair<std::optional<std::span<float>>, std::string>;

private:
    auto handle_calback(float** data) -> long;
    callback_t callback_;
    SRC_STATE* state_ { nullptr };
    int channels_ { 0 };
    double factor_ { 1.0 };
    float dummy_ {};
};

// Implementation details
#if SRCPP_USE_CPP23
inline auto Convert_expected(std::span<const float> input,
    std::span<float> output, SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>
{
    auto [result, error] = Convert(input, output, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto Convert_expected(
    std::span<const float> input, SRCpp::Type type, int channels, double factor)
    -> std::expected<std::vector<float>, std::string>
{
    auto [result, error] = Convert(input, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}
#endif // SRCPP_USE_CPP23

inline auto Convert(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::span<float>>, std::string>
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
        return { std::nullopt, src_strerror(result) };
    }

    return { std::span { output.data(),
                 static_cast<size_t>(src_data.output_frames_gen) },
        {} };
}

inline auto Convert(
    std::span<const float> input, SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::vector<float>>, std::string>
{
    std::vector<float> output(
        std::ceil((input.size() + 1) * factor * channels));
    auto [result, error] = Convert(input, output, type, channels, factor);
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    output.resize(result->size());
    return { output, {} };
}

inline PushConverter::PushConverter(
    SRCpp::Type type, int channels, double factor)
    : type_ { type }
    , channels_ { channels }
    , factor_ { factor }
{
    auto error = 0;
    state_ = src_new(static_cast<int>(type), channels, &error);
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }
}

inline PushConverter::~PushConverter() { src_delete(state_); }

inline PushConverter::PushConverter(const PushConverter& other)
    : type_(other.type_)
    , channels_(other.channels_)
    , factor_(other.factor_)
    , reserved_input_(other.reserved_input_)
    , last_input_(other.last_input_)
    , input_frames_consumed_(other.input_frames_consumed_)
    , output_frames_produced_(other.output_frames_produced_)
{
    auto error = 0;
    state_ = src_clone(other.state_, &error);
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }
}

inline PushConverter& PushConverter::operator=(const PushConverter& other)
{
    if (this != &other) {
        src_delete(state_);
        auto error = 0;
        state_ = src_clone(other.state_, &error);
        if (error != 0) {
            throw std::runtime_error(src_strerror(error));
        }
        type_ = other.type_;
        channels_ = other.channels_;
        factor_ = other.factor_;
        reserved_input_ = other.reserved_input_;
        last_input_ = other.last_input_;
        input_frames_consumed_ = other.input_frames_consumed_;
        output_frames_produced_ = other.output_frames_produced_;
    }
    return *this;
}

inline PushConverter::PushConverter(PushConverter&& other) noexcept
    : state_(other.state_)
    , type_(other.type_)
    , channels_(other.channels_)
    , factor_(other.factor_)
    , reserved_input_(std::move(other.reserved_input_))
    , last_input_(std::move(other.last_input_))
    , input_frames_consumed_(other.input_frames_consumed_)
    , output_frames_produced_(other.output_frames_produced_)
{
    other.state_ = nullptr;
}

inline PushConverter& PushConverter::operator=(PushConverter&& other) noexcept
{
    if (this != &other) {
        src_delete(state_);
        state_ = other.state_;
        type_ = other.type_;
        channels_ = other.channels_;
        factor_ = other.factor_;
        reserved_input_ = std::move(other.reserved_input_);
        last_input_ = std::move(other.last_input_);
        input_frames_consumed_ = other.input_frames_consumed_;
        output_frames_produced_ = other.output_frames_produced_;
        other.state_ = nullptr;
    }
    return *this;
}

#if SRCPP_USE_CPP23
inline auto PushConverter::convert_expected(std::span<const float> input,
    std::span<float> output) -> std::expected<std::span<float>, std::string>
{
    auto [result, error] = convert(input, output);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto PushConverter::convert_expected(std::span<const float> input)
    -> std::expected<std::vector<float>, std::string>
{
    auto [result, error] = convert(input);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto PushConverter::flush_expected()
    -> std::expected<std::vector<float>, std::string>
{
    return convert_expected({});
}
#endif // SRCPP_USE_CPP23

inline auto PushConverter::convert(
    std::span<const float> input, std::span<float> output)
    -> std::pair<std::optional<std::span<float>>, std::string>
{
    reserved_input_.insert(reserved_input_.end(), input.begin(), input.end());
    auto [result, error]
        = convertWithFixFor208(reserved_input_, output, input.empty());
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    auto& [input_data, output_data] = result.value();
    std::copy(input_data.begin(), input_data.end(), reserved_input_.begin());
    reserved_input_.resize(input_data.size());

    return { output_data, {} };
}

inline auto PushConverter::convert(std::span<const float> input)
    -> std::pair<std::optional<std::vector<float>>, std::string>
{
    auto expected_frames_produced
        = static_cast<size_t>(std::ceil(input_frames_consumed_ * factor_));
    auto amount = [&]() -> size_t {
        if (!input.empty()) {
            return std::ceil((input.size() / channels_) * factor_);
        }
        if (expected_frames_produced >= output_frames_produced_) {
            return expected_frames_produced - output_frames_produced_;
        }
        return 0;
    }() + 1;
    std::vector<float> output(amount * channels_);
    auto [result, error] = convert(input, output);
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    output.resize(result->size());
    return { output, {} };
}

inline auto PushConverter::flush()
    -> std::pair<std::optional<std::vector<float>>, std::string>
{
    return convert({});
}

inline auto PushConverter::convert(
    std::span<const float> input, std::span<float> output, bool end)
    -> std::pair<
        std::optional<std::pair<std::span<const float>, std::span<float>>>,
        std::string>
{
    auto* input_ptr = input.empty() ? &dummy_ : input.data();
    auto* output_ptr = output.data();
    auto input_frames = input.size() / channels_;
    auto output_frames = output.size() / channels_;
    auto src_data = SRC_DATA {
        input_ptr,
        output_ptr,
        static_cast<long>(input_frames),
        static_cast<long>(output_frames),
        0,
        0,
        end,
        factor_,
    };
    if (auto result = src_process(state_, &src_data); result != 0) {
        return { std::nullopt, src_strerror(result) };
    }
    input_frames_consumed_ += src_data.input_frames_used;
    output_frames_produced_ += src_data.output_frames_gen;

    return { std::pair { input.subspan(src_data.input_frames_used * channels_),
                 output.subspan(0, src_data.output_frames_gen * channels_) },
        {} };
}

inline auto PushConverter::convertWithFixFor208(
    std::span<const float> input, std::span<float> output, bool end)
    -> std::pair<
        std::optional<std::pair<std::span<const float>, std::span<float>>>,
        std::string>
{
    // https://github.com/libsndfile/libsamplerate/issues/208
    // When there is 1 frame of data, the linear SRC assumes it can read
    // the previous values and reads off the begin of the array.
    // Temporary fix until that is resolved.
    if (type_ != SRCpp::Type::Linear) {
        return convert(input, output, end);
    }

    auto [result, error] = [&] {
        if (input.size() == static_cast<size_t>(channels_)) {
            last_input_.insert(last_input_.end(), input.begin(), input.end());
            return convert(
                { last_input_.data() + channels_, input.size() }, output, end);
        }
        return convert(input, output, end);
    }();
    if (!result.has_value()) {
        return { std::nullopt, error };
    }

    auto& [input_unused, output_created] = result.value();
    auto input_data_used = input.size() - input_unused.size();

    // if we are linear, save the last input for next time
    if (input_data_used) {
        last_input_.assign(input.data() + input_data_used - channels_,
            input.data() + input_data_used);
    } else {
        last_input_.clear();
    }

    return { std::pair { input.subspan(input_data_used), output_created }, {} };
}

inline PullConverter::PullConverter(
    callback_t callback, SRCpp::Type type, int channels, double factor)
    : callback_(callback)
    , channels_ { channels }
    , factor_ { factor }
{
    auto error = 0;
    state_ = src_callback_new(
        [](void* cb_data, float** data) -> long {
            auto* self = static_cast<PullConverter*>(cb_data);
            return self->handle_calback(data);
        },
        static_cast<int>(type), channels, &error, this);
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }
}

inline PullConverter::~PullConverter() { src_delete(state_); }

inline PullConverter::PullConverter(PullConverter&& other) noexcept
    : callback_(std::move(other.callback_))
    , state_(other.state_)
    , channels_(other.channels_)
    , factor_(other.factor_)
    , dummy_(other.dummy_)
{
    other.state_ = nullptr;
}

inline PullConverter& PullConverter::operator=(PullConverter&& other) noexcept
{
    if (this != &other) {
        std::swap(callback_, other.callback_);
        std::swap(state_, other.state_);
        std::swap(channels_, other.channels_);
        std::swap(factor_, other.factor_);
        std::swap(dummy_, other.dummy_);
    }
    return *this;
}

#if SRCPP_USE_CPP23
inline auto PullConverter::convert_expected(std::span<float> output)
    -> std::expected<std::span<float>, std::string>
{
    auto [result, error] = convert(output);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}
#endif // SRCPP_USE_CPP23

inline auto PullConverter::convert(std::span<float> output)
    -> std::pair<std::optional<std::span<float>>, std::string>
{
    auto size = src_callback_read(
        state_, factor_, output.size() / channels_, output.data());
    if (size < 0) {
        return { std::nullopt, src_strerror(src_error(state_)) };
    }
    return { output.subspan(0, size * channels_), {} };
}

inline auto PullConverter::handle_calback(float** data) -> long
{
    if (data == nullptr) {
        return 0;
    }
    auto newData = callback_();
    // SRC is pendantic that input and output buffers don't overlap, even if
    // the input size is 0, such as an end iterator.  If a client has input
    // and output buffers that are adjacent, this would cause an error.  So
    // in the cases of a size of zero, we provide a safe dummy pointer.
    if (newData.empty()) {
        *data = &dummy_;
        return 0;
    }
    *data = newData.data();
    return newData.size() / channels_;
}
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
