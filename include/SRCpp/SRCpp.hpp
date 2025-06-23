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
#include <memory>
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

/*
# SRCpp.hpp

**C++ wrapper for libsamplerate sample rate conversion**

This header provides the `SRCpp` namespace, which contains high-level C++
interfaces for performing sample rate conversion using the libsamplerate
library. It offers both push and pull conversion models, as well as utility
functions for one-shot conversions.

---

## Namespace

### `SRCpp`

All enums, functions, and types exist in the `SRCpp` namespace.

```cpp
namespace SRCpp {
}
```

---

## Enumerations

### `enum struct Type`

```cpp
enum struct Type : int {
    Sinc_BestQuality = SRC_SINC_BEST_QUALITY,
    Sinc_MediumQuality = SRC_SINC_MEDIUM_QUALITY,
    Sinc_Fastest = SRC_SINC_FASTEST,
    ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR
};
```

Enumerates the available sample rate conversion algorithms:

- `Sinc_BestQuality`: Best quality sinc interpolation (`SRC_SINC_BEST_QUALITY`)
- `Sinc_MediumQuality`: Medium quality sinc interpolation
(`SRC_SINC_MEDIUM_QUALITY`)
- `Sinc_Fastest`: Fastest sinc interpolation (`SRC_SINC_FASTEST`)
- `ZeroOrderHold`: Zero-order hold interpolation (`SRC_ZERO_ORDER_HOLD`)
- `Linear`: Linear interpolation (`SRC_LINEAR`)

---

## Functions

### `Convert`

Converts input audio data to a different sample rate.

```cpp
auto Convert(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<optional<std::span<float>>, std::string>;
```

- **Parameters:**
    - `input`: Input audio samples (interleaved, per channel)
    - `output`: Output buffer for converted samples
    - `type`: Conversion algorithm
    - `channels`: Number of audio channels
    - `factor`: Sample rate conversion factor
- **Returns:** Pair of optional span of output samples written if no error, and
error string if error occurred.

### `Convert` (with allocation)

Converts input audio data to a different sample rate, allocating the output
buffer.

```cpp
auto Convert(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<float>>, std::string>;
```

- **Parameters:**
    - `input`: Input audio samples (interleaved, per channel)
    - `type`: Conversion algorithm
    - `channels`: Number of audio channels
    - `factor`: Sample rate conversion factor
- **Returns:** Pair of optional vector of output samples if no error, and error
string if error occurred.


### `Convert_expected` (C++23 only)

As `Convert`, except returning `std::expected`.

```cpp
auto Convert_expected(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>;
auto Convert_expected(std::span<const float> input, SRCpp::Type type, int
channels, double factor) -> std::expected<std::vector<float>, std::string>;
```

---

## Classes

### `PushConverter`

Stateful sample rate converter for push-based processing.

```cpp
class PushConverter {
public:
    PushConverter(SRCpp::Type type, int channels, double factor);

    auto convert(std::span<const float> input, std::span<float> output)
        -> std::expected<std::span<float>, std::string>;
    auto convert(std::span<const float> input)
        -> std::expected<std::vector<float>, std::string>;
    // flush will push any remaining data through
    auto flush() -> std::expected<std::vector<float>, std::string>;

    auto convert(std::span<const float> input, std::span<float> output)
        -> std::pair<std::optional<std::span<float>>, std::string>;
    auto convert(std::span<const float> input)
        -> std::pair<std::optional<std::vector<float>>, std::string>;
    // flush will push any remaining data through
    auto flush() -> std::pair<std::optional<std::vector<float>>, std::string>;
};
```

Provides an interface for incremental (push) sample rate conversion, allowing
input to be provided in chunks and output to be retrieved as available.

- **Constructor:** `PushConverter(Type type, int channels, double factor)`
    - Constructs a new push converter with the specified algorithm, channel
count, and conversion factor.

- **Methods:**
    - `convert(input, output)`: Converts a chunk of input samples, writing to
the provided output buffer. Returns pair of optional span of output samples
written and error string.
    - `convert(input)`: Converts a chunk of input samples, allocating the output
buffer. Returns pair of optional vector of output samples and error string.
    - `flush()`: Flushes any remaining samples from the converter.
        Returns pair of optional vector of output samples and error string.
    - `convert_expected`, `flush_expected`: (C++23 only) Variants returning
`std::expected`.

- **Notes:** Copy and move constructors/assignment are supported. Copying clones
the internal state.

---

### `PullConverter`

Stateful sample rate converter for pull-based processing.

```cpp
class PullConverter {
public:
    using callback_t = std::function<std::span<float>()>;
    PullConverter(
        callback_t callback, SRCpp::Type type, int channels, double factor);

    auto convert(std::span<float> output)
        -> std::expected<std::span<float>, std::string>;
    auto convert(std::span<float> output)
        -> std::pair<std::optional<std::span<float>>, std::string>;
};
```

Provides an interface for sample rate conversion where output is requested and
input is supplied via a callback.

- **Typedef:** `callback_t`
    `std::function<std::span<float>()>`
    Callback type for supplying input samples.

- **Constructor:** `PullConverter(callback_t callback, Type type, int channels,
double factor)` Constructs a new pull converter with the specified callback,
algorithm, channel count, and conversion factor.

- **Methods:**
    - `convert(output)`: Requests output samples, filling the provided buffer.
        Returns pair of optional span of output samples written and error
string.
    - `convert_expected`: (C++23 only) Variant returning `std::expected`.

- **Notes:** Copying is disabled; only move operations are supported.

---

## Details

- All functions and classes are exception-safe. Errors are reported via return
values or exceptions (for construction failures).
- Input and output buffers are assumed to be interleaved per channel.
- The sample rate conversion factor is defined as `output_sample_rate /
input_sample_rate`.
- The implementation includes a workaround for libsamplerate issue #208 for
linear interpolation.

---

## See Also

- [libsamplerate documentation](http://www.mega-nerd.com/SRC/)
- `std::span`, `std::vector`, `std::expected` (C++23)
*/
namespace SRCpp {

enum struct Type : uint8_t {
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
    auto operator=(const PushConverter& other) -> PushConverter&;
    PushConverter(PushConverter&& other) noexcept;
    auto operator=(PushConverter&& other) noexcept -> PushConverter&;

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

    PullConverter(PullConverter&& other) noexcept;
    auto operator=(PullConverter&& other) noexcept -> PullConverter&;

#if SRCPP_USE_CPP23
    auto convert_expected(std::span<float> output)
        -> std::expected<std::span<float>, std::string>;
#endif // SRCPP_USE_CPP23

    auto convert(std::span<float> output)
        -> std::pair<std::optional<std::span<float>>, std::string>;

private:
    struct CallbackHandle {
        callback_t callback_;
        float dummy_ {};
        int channels_ { 0 };
        auto handle_callback(float** data) -> long;
    };
    std::unique_ptr<CallbackHandle> callback_;
    SRC_STATE* state_ { nullptr };
    double factor_ { 1.0 };
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

inline auto PushConverter::operator=(const PushConverter& other)
    -> PushConverter&
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

inline auto PushConverter::operator=(PushConverter&& other) noexcept
    -> PushConverter&
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
    if (end) {
        if (auto result = src_reset(state_); result != 0) {
            return { std::nullopt, src_strerror(result) };
        }
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
    : callback_ { std::make_unique<CallbackHandle>(callback, 0, channels) }
    , factor_ { factor }
{
    auto error = 0;
    state_ = src_callback_new(
        [](void* cb_data, float** data) -> long {
            auto* self = static_cast<CallbackHandle*>(cb_data);
            return self->handle_callback(data);
        },
        static_cast<int>(type), channels, &error, callback_.get());
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }
}

inline PullConverter::~PullConverter() { src_delete(state_); }

inline PullConverter::PullConverter(PullConverter&& other) noexcept
    : callback_(std::move(other.callback_))
    , state_(other.state_)
    , factor_(other.factor_)
{
    other.state_ = nullptr;
}

inline auto PullConverter::operator=(PullConverter&& other) noexcept
    -> PullConverter&
{
    if (this != &other) {
        std::swap(callback_, other.callback_);
        std::swap(state_, other.state_);
        std::swap(factor_, other.factor_);
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
        state_, factor_, output.size() / callback_->channels_, output.data());
    if (size < 0) {
        return { std::nullopt, src_strerror(src_error(state_)) };
    }
    return { output.subspan(0, size * callback_->channels_), {} };
}

inline auto PullConverter::CallbackHandle::handle_callback(float** data) -> long
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
