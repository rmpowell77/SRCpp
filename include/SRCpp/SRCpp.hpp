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

## Concepts

### `SupportedSampleType`

```cpp
// Concept to restrict types to short, int, or float
template <typename T>
concept SupportedSampleType = std::is_same_v<T, short> || std::is_same_v<T, int>
    || std::is_same_v<T, float>;
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

### `enum struct Type`

```cpp
enum struct Format : uint8_t { Short, Int, Float };
```

Enumerates the available unsafe conversions of short, int, and floats.

---

## Functions

### `Convert`

Converts input audio data to a different sample rate.

```cpp
template <SupportedSampleType To, SupportedSampleType From>
auto Convert(std::span<const From> input, std::span<To> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::span<To>>, std::string>;
```

- **Template Parameters:**
    - `To`: The Type to convert to.  Usually deduced implicitly.
    - `From`: The Type to convert from.  Usually deduced implicitly.
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
template <SupportedSampleType To, SupportedSampleType From>
auto Convert(std::span<const From> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<To>>, std::string>;
```

- **Template Parameters:**
    - `To`: The Type to convert to. Must be supplied.
    - `From`: The Type to convert from.  Usually deduced implicitly.
- **Parameters:**
    - `input`: Input audio samples (interleaved, per channel)
    - `type`: Conversion algorithm
    - `channels`: Number of audio channels
    - `factor`: Sample rate conversion factor
- **Returns:** Pair of optional vector of output samples if no error, and error
string if error occurred.

---

## Classes

### `PushConverter`

Stateful sample rate converter for push-based processing.

```cpp
class PushConverter {
public:
    PushConverter(SRCpp::Type type, int channels, double factor);

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert(std::span<const From> input, std::span<To> output)
        -> std::pair<std::optional<std::span<To>>, std::string>;

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert(std::span<const From> input)
        -> std::pair<std::optional<std::vector<To>>, std::string>;

    template <SupportedSampleType To>
    auto flush() -> std::pair<std::optional<std::vector<To>>, std::string>;
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
written and error string.  `To` and `From` are often deduced automatically.
    - `convert(input)`: Converts a chunk of input samples, allocating the output
buffer. Returns pair of optional vector of output samples and error string. `To`
must be supplied.
    - `flush()`: Flushes any remaining samples from the converter.
        Returns pair of optional vector of output samples and error string. `To`
must be supplied.

- **Notes:** Copy and move constructors/assignment are supported. Copying clones
the internal state.

---

### `PullConverter`

Stateful sample rate converter for pull-based processing.  A Callable that
returns

```cpp
class PullConverter {
public:
    template <typename Callback>
    PullConverter(
        Callback&& callback, SRCpp::Type type, int channels, double factor);

    template <SupportedSampleType From>
    PullConverter(std::span<From> (*func)(void*), void* context, SRCpp::Type
type, int channels, double factor);

    template <SupportedSampleType To>
    auto convert(std::span<To> output)
        -> std::pair<std::optional<std::span<To>>, std::string>;
};
```

Provides an interface for sample rate conversion where output is requested and
input is supplied via a callback.

- **Constructor:** `PullConverter(Callback&& callback, Type type, int channels,
double factor)` Constructs a new pull converter with the specified callback,
algorithm, channel count, and conversion factor. `Callback` must be a callable
that returns `std::span<SupportedSampleType>`.

- **Methods:**
    - `convert(output)`: Requests output samples, filling the provided buffer.
        Returns pair of optional span of output samples written and error
string.

- **Notes:** Copying is disabled; only move operations are supported.

---

## Unsafe

Each form of conversion supports an "unsafe" type.  This is where the caller
supplies untyped memory location and a `Format` enum of the appropriate "from"
and "to" type.  This is to allow usage of conversion at run time where the
underlying type is not known.  The APIs are intentionally labeled as unsafe to
give caution to the caller that these be used sparingly and with extra scrutiny.

---

## Details

- All functions and classes are exception-safe. Errors are reported via return
values or exceptions (for construction failures).
- All functions that return `std::pair<std::optional<T>, std::string>` have an
equivalent `std::expected<T, std::string>` to allow C++23 style error handling.
- Input and output buffers are assumed to be interleaved per channel.
- The library attempts to handle deduction of containers to `std::span` and the
appropriate type when possible.
- The sample rate conversion factor is defined as `output_sample_rate /
input_sample_rate`.
- The implementation includes a workaround for libsamplerate issue #208 for
linear interpolation.
- Lifetime of the Callback Input and output buffers are assumed to be
interleaved per channel.

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

enum struct Format : uint8_t { Short, Int, Float };

// Concept to restrict types to short, int, or float
template <typename T>
concept SupportedSampleType = std::is_same_v<T, short> || std::is_same_v<T, int>
    || std::is_same_v<T, float>;

template <SupportedSampleType T> constexpr auto SampleTypeToFormat() -> Format
{
    if constexpr (std::is_same_v<T, short>) {
        return Format::Short;
    } else if constexpr (std::is_same_v<T, int>) {
        return Format::Int;
    } else {
        return Format::Float;
    }
}

constexpr auto SizeOfFormat(Format format) -> size_t
{
    switch (format) {
    case Format::Short:
        return sizeof(short);
    case Format::Int:
        return sizeof(int);
    case Format::Float:
        return sizeof(float);
    }
    return sizeof(short);
}

#if SRCPP_USE_CPP23
template <SupportedSampleType To, SupportedSampleType From>
auto Convert_expected(std::span<const From> input, std::span<To> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<To>, std::string>;
template <SupportedSampleType To, SupportedSampleType From = float>
auto Convert_expected(std::span<const From> input, SRCpp::Type type,
    int channels, double factor) -> std::expected<std::vector<To>, std::string>;

auto Convert_unsafe_expected(Format from, const void* input, size_t input_size,
    Format to, void* output, size_t output_size, SRCpp::Type type, int channels,
    double factor) -> std::expected<size_t, std::string>;
auto Convert_unsafe_expected(Format from, const void* input, size_t input_size,
    Format to, SRCpp::Type type, int channels, double factor)
    -> std::expected<std::vector<std::byte>, std::string>;
#endif // SRCPP_USE_CPP23

template <SupportedSampleType To, SupportedSampleType From>
auto Convert(std::span<const From> input, std::span<To> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::span<To>>, std::string>;
template <SupportedSampleType To, SupportedSampleType From>
auto Convert(std::span<const From> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<To>>, std::string>;

auto Convert_unsafe(Format from, const void* input, size_t input_size,
    Format to, void* output, size_t output_size, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<size_t>, std::string>;
auto Convert_unsafe(Format from, const void* input, size_t input_size,
    Format to, SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::vector<std::byte>>, std::string>;

class PushConverter {
public:
    PushConverter(SRCpp::Type type, int channels, double factor);
    ~PushConverter();
    PushConverter(const PushConverter& other);
    auto operator=(const PushConverter& other) -> PushConverter&;
    PushConverter(PushConverter&& other) noexcept;
    auto operator=(PushConverter&& other) noexcept -> PushConverter&;

#if SRCPP_USE_CPP23
    template <SupportedSampleType To, SupportedSampleType From>
    auto convert_expected(std::span<const From> input, std::span<To> output)
        -> std::expected<std::span<To>, std::string>;

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert_expected(std::span<const From> input)
        -> std::expected<std::vector<To>, std::string>;

    template <SupportedSampleType To>
    auto flush_expected() -> std::expected<std::vector<To>, std::string>;

    auto convert_unsafe_expected(Format from, const void* input,
        size_t input_size, Format to, void* output, size_t output_size)
        -> std::expected<size_t, std::string>;

    auto convert_unsafe_expected(
        Format from, const void* input, size_t input_size, Format to)
        -> std::expected<std::vector<std::byte>, std::string>;
#endif // SRCPP_USE_CPP23

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert(std::span<const From> input, std::span<To> output)
        -> std::pair<std::optional<std::span<To>>, std::string>;

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert(std::span<const From> input)
        -> std::pair<std::optional<std::vector<To>>, std::string>;

    template <SupportedSampleType To>
    auto flush() -> std::pair<std::optional<std::vector<To>>, std::string>;

    auto convert_unsafe(Format from, const void* input, size_t input_size,
        Format to, void* output, size_t output_size)
        -> std::pair<std::optional<size_t>, std::string>;

    auto convert_unsafe(
        Format from, const void* input, size_t input_size, Format to)
        -> std::pair<std::optional<std::vector<std::byte>>, std::string>;

#if SRCPP_USE_CPP23
    template <typename ToContainer, typename FromContainer,
        SupportedSampleType To = typename ToContainer::value_type,
        SupportedSampleType From = typename FromContainer::value_type>
    auto convert_expected(FromContainer const& input, ToContainer& output)
    {
        return convert_expected<To, From>(
            std::span<const From> { input }, std::span<To> { output });
    }

    template <SupportedSampleType To, typename FromContainer,
        SupportedSampleType From = typename FromContainer::value_type>
    auto convert_expected(FromContainer const& input)
    {
        return convert_expected<To, From>(std::span<const From> { input });
    }
#endif // SRCPP_USE_CPP23

    template <typename ToContainer, typename FromContainer,
        SupportedSampleType To = typename ToContainer::value_type,
        SupportedSampleType From = typename FromContainer::value_type>
    auto convert(FromContainer const& input, ToContainer& output)
    {
        return convert(
            std::span<const From> { input }, std::span<To> { output });
    }

    template <SupportedSampleType To, typename FromContainer,
        SupportedSampleType From = typename FromContainer::value_type>
    auto convert(FromContainer const& input)
    {
        return convert<To, From>(std::span<const From> { input });
    }

private:
    SRC_STATE* state_ { nullptr };
    SRCpp::Type type_ { SRC_SINC_BEST_QUALITY };
    int channels_ { 0 };
    double factor_ { 1.0 };
    const float dummy_ {};
    std::vector<float> reserved_input_;
    std::vector<float> last_input_;
    std::vector<float> scratch_output_;
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
    auto framesToReserve(size_t frames) const -> size_t;
};

class PullConverter {
public:
    // using callback_t = std::function<std::span<From>()>;
    template <typename Callback>
    PullConverter(
        Callback&& callback, SRCpp::Type type, int channels, double factor);
    ~PullConverter();

    template <SupportedSampleType From>
    PullConverter(std::span<From> (*func)(void*), void* context,
        SRCpp::Type type, int channels, double factor)
        : PullConverter(
            [func, context]() { return func(context); }, type, channels, factor)
    {
        static_assert(SupportedSampleType<From>,
            "Function must return std::span<From> where From is short, int, or "
            "float");
    }

    PullConverter(PullConverter&& other) noexcept;
    auto operator=(PullConverter&& other) noexcept -> PullConverter&;

#if SRCPP_USE_CPP23
    template <SupportedSampleType To>
    auto convert_expected(std::span<To> output)
        -> std::expected<std::span<To>, std::string>;

    auto convert_unsafe_expected(Format to, void* output, size_t output_size)
        -> std::expected<size_t, std::string>;
#endif // SRCPP_USE_CPP23

    template <SupportedSampleType To>
    auto convert(std::span<To> output)
        -> std::pair<std::optional<std::span<To>>, std::string>;

    auto convert_unsafe(Format to, void* output, size_t output_size)
        -> std::pair<std::optional<size_t>, std::string>;

#if SRCPP_USE_CPP23
    template <typename ToContainer,
        SupportedSampleType To = typename ToContainer::value_type>
    auto convert_expected(ToContainer& output)
    {
        return convert_expected<To>(std::span<To> { output });
    }
#endif // SRCPP_USE_CPP23

    template <typename ToContainer,
        SupportedSampleType To = typename ToContainer::value_type>
    auto convert(ToContainer& output)
    {
        return convert(std::span<To> { output });
    }

private:
    struct CallbackHandle {
        virtual ~CallbackHandle() = default;
        virtual auto handle_callback(float** data) -> long = 0;
    };
    template <typename Callback> struct CallbackHandleImpl : CallbackHandle {
        CallbackHandleImpl(Callback&& callback, int channels, SRCpp::Type type)
            : callback_(std::forward<Callback>(callback))
            , channels_(channels)
            , type_(type)
            , last_input_(channels_)
        {
            static_assert(
                std::is_invocable_r_v<std::span<typename std::invoke_result_t<
                                          Callback>::value_type>,
                    Callback>,
                "Callback must be callable with no arguments and return "
                "std::span of SupportedSampleType");
            static_assert(
                SupportedSampleType<
                    typename std::invoke_result_t<Callback>::value_type>,
                "Callback must return std::span<T> where T is short, int, or "
                "float");
        }
        auto handle_callback(float** data) -> long;

    private:
        Callback callback_;
        float dummy_ {};
        int channels_ { 0 };
        SRCpp::Type type_;
        std::vector<float> scratch_input_;
        std::vector<float> last_input_;
    };
    std::unique_ptr<CallbackHandle> callback_;
    std::vector<float> scratch_output_;
    SRC_STATE* state_ { nullptr };
    double factor_ { 1.0 };
    int channels_ { 0 };
};

// Implementation details
#if SRCPP_USE_CPP23
template <SupportedSampleType To, SupportedSampleType From>
inline auto Convert_expected(std::span<const From> input, std::span<To> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<To>, std::string>
{
    auto [result, error] = Convert(input, output, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

template <SupportedSampleType To, SupportedSampleType From>
inline auto Convert_expected(std::span<const From> input, SRCpp::Type type,
    int channels, double factor) -> std::expected<std::vector<To>, std::string>
{
    auto [result, error] = Convert<To, From>(input, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto Convert_unsafe_expected(Format from, const void* input,
    size_t input_size, Format to, void* output, size_t output_size,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<size_t, std::string>
{
    auto [result, error] = Convert_unsafe(from, input, input_size, to, output,
        output_size, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto Convert_unsafe_expected(Format from, const void* input,
    size_t input_size, Format to, SRCpp::Type type, int channels, double factor)
    -> std::expected<std::vector<std::byte>, std::string>
{
    auto [result, error]
        = Convert_unsafe(from, input, input_size, to, type, channels, factor);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

#endif // SRCPP_USE_CPP23

template <SupportedSampleType To, SupportedSampleType From>
inline auto Convert(std::span<const From> input, std::span<To> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::span<To>>, std::string>
{
    // convert input format to Float
    if constexpr (std::is_same_v<From, short>) {
        std::vector<float> converted_input(input.size());
        src_short_to_float_array(
            input.data(), converted_input.data(), input.size());
        return Convert(converted_input, output, type, channels, factor);
    } else if constexpr (std::is_same_v<From, int>) {
        std::vector<float> converted_input(input.size());
        src_int_to_float_array(
            input.data(), converted_input.data(), input.size());
        return Convert(converted_input, output, type, channels, factor);
    } else {
        std::vector<float> data;
        float* outputData = nullptr;
        auto outputSize = output.size();
        if constexpr (!std::is_same_v<To, float>) {
            data.resize(outputSize);
            outputData = data.data();
        } else {
            outputData = output.data();
        }
        auto src_data = SRC_DATA {
            input.data(),
            outputData,
            static_cast<long>(input.size() / channels),
            static_cast<long>(outputSize / channels),
            0,
            0,
            1,
            factor,
        };
        if (auto result
            = src_simple(&src_data, static_cast<int>(type), channels);
            result != 0) {
            return { std::nullopt, src_strerror(result) };
        }

        // convert from float to output format
        if constexpr (std::is_same_v<To, short>) {
            src_float_to_short_array(outputData, output.data(),
                src_data.output_frames_gen * channels);
        }
        if constexpr (std::is_same_v<To, int>) {
            src_float_to_int_array(outputData, output.data(),
                src_data.output_frames_gen * channels);
        }
        return { std::span { output.data(),
                     static_cast<size_t>(
                         src_data.output_frames_gen * channels) },
            {} };
    }
}

template <SupportedSampleType To, SupportedSampleType From>
inline auto Convert(std::span<const From> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<To>>, std::string>
{
    std::vector<To> output(
        (std::ceil((input.size() / channels) * factor) + 1) * channels);
    auto [result, error]
        = Convert<To, From>(input, output, type, channels, factor);
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    output.resize(result->size());
    return { output, {} };
}

namespace details {
    template <SupportedSampleType To, SupportedSampleType From>
    inline auto Convert_unsafe_helper(std::span<const From> input_span,
        void* output, size_t output_size, SRCpp::Type type, int channels,
        double factor) -> std::pair<std::optional<size_t>, std::string>
    {
        auto output_span
            = std::span<To>(static_cast<To*>(output), output_size / sizeof(To));
        auto [result, error] = Convert<To, From>(
            input_span, output_span, type, channels, factor);
        if (!result.has_value()) {
            return { std::nullopt, error };
        }
        return { result->size_bytes(), {} };
    }

    template <SupportedSampleType From>
    inline auto Convert_unsafe_helper(const void* input, size_t input_size,
        Format to, void* output, size_t output_size, SRCpp::Type type,
        int channels, double factor)
        -> std::pair<std::optional<size_t>, std::string>
    {
        auto input_span = std::span<const From>(
            static_cast<const From*>(input), input_size / sizeof(From));
        switch (to) {
        case Format::Short:
            return Convert_unsafe_helper<short>(
                input_span, output, output_size, type, channels, factor);
        case Format::Int:
            return Convert_unsafe_helper<int>(
                input_span, output, output_size, type, channels, factor);
        case Format::Float:
            return Convert_unsafe_helper<float>(
                input_span, output, output_size, type, channels, factor);
        }
        return { std::nullopt, "Invalid format combination" };
    }

    template <SupportedSampleType To>
    auto Convert_unsafe_helper(Format from, const void* input,
        size_t input_size, Format to, size_t output_elements, SRCpp::Type type,
        int channels, double factor)
        -> std::pair<std::optional<std::vector<std::byte>>, std::string>
    {
        std::vector<std::byte> output(output_elements * sizeof(To));
        auto [result, error] = Convert_unsafe(from, input, input_size, to,
            output.data(), output.size(), type, channels, factor);
        if (!result.has_value()) {
            return { std::nullopt, error };
        }
        output.resize(*result);
        return { std::move(output), {} };
    }

    template <SupportedSampleType To, SupportedSampleType From>
    auto convert_unsafe_helper(PushConverter& push,
        std::span<const From> input_span, void* output, size_t output_size)
        -> std::pair<std::optional<size_t>, std::string>
    {
        auto output_span
            = std::span<To>(static_cast<To*>(output), output_size / sizeof(To));
        auto [result, error] = push.convert<To, From>(input_span, output_span);
        if (!result.has_value()) {
            return { std::nullopt, error };
        }
        return { result->size_bytes(), {} };
    }

    template <SupportedSampleType From>
    inline auto convert_unsafe_helper(PushConverter& push, const void* input,
        size_t input_size, Format to, void* output, size_t output_size)
        -> std::pair<std::optional<size_t>, std::string>
    {
        auto input_span = std::span<const From>(
            static_cast<const From*>(input), input_size / sizeof(From));
        switch (to) {
        case Format::Short:
            return details::convert_unsafe_helper<short>(
                push, input_span, output, output_size);
        case Format::Int:
            return details::convert_unsafe_helper<int>(
                push, input_span, output, output_size);
        case Format::Float:
            return details::convert_unsafe_helper<float>(
                push, input_span, output, output_size);
        }
        return { std::nullopt, "Invalid format combination" };
    }

    template <SupportedSampleType To>
    auto convert_unsafe_helper(PushConverter& push, Format from,
        const void* input, size_t input_size, Format to, size_t out_samples)
        -> std::pair<std::optional<std::vector<std::byte>>, std::string>
    {
        std::vector<std::byte> output(out_samples * sizeof(To));
        auto [result, error] = push.convert_unsafe(
            from, input, input_size, to, output.data(), output.size());
        if (!result.has_value()) {
            return { std::nullopt, error };
        }
        output.resize(*result);
        return { std::move(output), {} };
    }

    template <SupportedSampleType To>
    auto convert_unsafe_helper(PullConverter& pull, void* output,
        size_t output_size) -> std::pair<std::optional<size_t>, std::string>
    {
        auto [size, error] = pull.convert(std::span<To> {
            static_cast<To*>(output), output_size / sizeof(To) });
        if (!size) {
            return { std::nullopt, error };
        }
        return { size->size_bytes(), {} };
    }
}

inline auto Convert_unsafe(Format from, const void* input, size_t input_size,
    Format to, void* output, size_t output_size, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<size_t>, std::string>
{
    switch (from) {
    case Format::Short:
        return details::Convert_unsafe_helper<short>(
            input, input_size, to, output, output_size, type, channels, factor);
    case Format::Int:
        return details::Convert_unsafe_helper<int>(
            input, input_size, to, output, output_size, type, channels, factor);
    case Format::Float:
        return details::Convert_unsafe_helper<float>(
            input, input_size, to, output, output_size, type, channels, factor);
    }
    return { std::nullopt, "Invalid format combination" };
}

inline auto Convert_unsafe(Format from, const void* input, size_t input_size,
    Format to, SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::vector<std::byte>>, std::string>
{
    size_t output_elements = static_cast<size_t>(std::ceil(
                                 (input_size / SizeOfFormat(from)) * factor))
        + 1;
    switch (to) {
    case Format::Short:
        return details::Convert_unsafe_helper<short>(from, input, input_size,
            to, output_elements, type, channels, factor);
    case Format::Int:
        return details::Convert_unsafe_helper<int>(from, input, input_size, to,
            output_elements, type, channels, factor);
    case Format::Float:
        return details::Convert_unsafe_helper<float>(from, input, input_size,
            to, output_elements, type, channels, factor);
    }
    return { std::nullopt, "Invalid format combination" };
}

inline PushConverter::PushConverter(
    SRCpp::Type type, int channels, double factor)
    : type_ { type }
    , channels_ { channels }
    , factor_ { factor }
    , last_input_(channels)
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
    , scratch_output_(other.scratch_output_)
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
        scratch_output_ = other.scratch_output_;
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
    , scratch_output_(std::move(other.scratch_output_))
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
        scratch_output_ = std::move(other.scratch_output_);
        input_frames_consumed_ = other.input_frames_consumed_;
        output_frames_produced_ = other.output_frames_produced_;
        other.state_ = nullptr;
    }
    return *this;
}

#if SRCPP_USE_CPP23
template <SupportedSampleType To, SupportedSampleType From>
inline auto PushConverter::convert_expected(std::span<const From> input,
    std::span<To> output) -> std::expected<std::span<To>, std::string>
{
    auto [result, error] = convert<To, From>(input, output);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

template <SupportedSampleType To, SupportedSampleType From>
inline auto PushConverter::convert_expected(std::span<const From> input)
    -> std::expected<std::vector<To>, std::string>
{
    auto [result, error] = convert<To, From>(input);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

template <SupportedSampleType To>
inline auto PushConverter::flush_expected()
    -> std::expected<std::vector<To>, std::string>
{
    return convert_expected<To, float>(std::vector<float> {});
}

inline auto PushConverter::convert_unsafe_expected(Format from,
    const void* input, size_t input_size, Format to, void* output,
    size_t output_size) -> std::expected<size_t, std::string>
{
    auto [result, error]
        = convert_unsafe(from, input, input_size, to, output, output_size);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto PushConverter::convert_unsafe_expected(
    Format from, const void* input, size_t input_size, Format to)
    -> std::expected<std::vector<std::byte>, std::string>
{
    auto [result, error] = convert_unsafe(from, input, input_size, to);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}
#endif // SRCPP_USE_CPP23

template <SupportedSampleType To, SupportedSampleType From>
inline auto PushConverter::convert(
    std::span<const From> input, std::span<To> output)
    -> std::pair<std::optional<std::span<To>>, std::string>
{
    // convert from input format to float
    auto offsetToPlace = reserved_input_.size();
    reserved_input_.resize(reserved_input_.size() + input.size());
    auto* whereToPlaceData = reserved_input_.data() + offsetToPlace;
    if constexpr (std::is_same_v<From, short>) {
        src_short_to_float_array(input.data(), whereToPlaceData, input.size());
    } else if constexpr (std::is_same_v<From, int>) {
        src_int_to_float_array(input.data(), whereToPlaceData, input.size());
    } else {
        std::copy(input.begin(), input.end(), whereToPlaceData);
    }
    // where to put things?
    auto output_span = [&]() -> std::span<float> {
        if constexpr (std::is_same_v<To, float>) {
            return output;
        } else {
            scratch_output_.resize(output.size());
            return scratch_output_;
        }
    }();
    auto [result, error]
        = convertWithFixFor208(reserved_input_, output_span, input.empty());
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    auto& [input_data, output_data] = result.value();
    std::copy(input_data.begin(), input_data.end(), reserved_input_.begin());
    reserved_input_.resize(input_data.size());

    // convert from float to output format
    if constexpr (std::is_same_v<To, short>) {
        src_float_to_short_array(
            output_data.data(), output.data(), output_data.size());
        return { output.first(output_data.size()), {} };
    } else if constexpr (std::is_same_v<To, int>) {
        src_float_to_int_array(
            output_data.data(), output.data(), output_data.size());
        return { output.first(output_data.size()), {} };
    } else {
        return { output_data, {} };
    }
}

template <SupportedSampleType To, SupportedSampleType From>
inline auto PushConverter::convert(std::span<const From> input)
    -> std::pair<std::optional<std::vector<To>>, std::string>
{
    auto amount = framesToReserve(input.size());
    std::vector<To> output(amount * channels_);
    auto [result, error] = convert(input, output);
    if (!result.has_value()) {
        return { std::nullopt, error };
    }
    output.resize(result->size());
    return { output, {} };
}

template <SupportedSampleType To>
inline auto PushConverter::flush()
    -> std::pair<std::optional<std::vector<To>>, std::string>
{
    return convert<To, float>(std::vector<float> {});
}

inline auto PushConverter::convert_unsafe(Format from, const void* input,
    size_t input_size, Format to, void* output, size_t output_size)
    -> std::pair<std::optional<size_t>, std::string>
{
    switch (from) {
    case Format::Short:
        return details::convert_unsafe_helper<short>(
            *this, input, input_size, to, output, output_size);
    case Format::Int:
        return details::convert_unsafe_helper<int>(
            *this, input, input_size, to, output, output_size);
    case Format::Float:
        return details::convert_unsafe_helper<float>(
            *this, input, input_size, to, output, output_size);
    }
    return { std::nullopt, "Invalid format combination" };
}

inline auto PushConverter::convert_unsafe(
    Format from, const void* input, size_t input_size, Format to)
    -> std::pair<std::optional<std::vector<std::byte>>, std::string>
{
    size_t output_samples
        = framesToReserve(input_size / SizeOfFormat(from)) * channels_;
    switch (to) {
    case Format::Short:
        return details::convert_unsafe_helper<short>(
            *this, from, input, input_size, to, output_samples);
    case Format::Int:
        return details::convert_unsafe_helper<int>(
            *this, from, input, input_size, to, output_samples);
    case Format::Float:
        return details::convert_unsafe_helper<float>(
            *this, from, input, input_size, to, output_samples);
    }
    return { std::nullopt, "Invalid format combination" };
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

inline auto PushConverter::framesToReserve(size_t frames) const -> size_t
{
    auto expected_frames_produced = static_cast<size_t>(
        std::ceil(static_cast<double>(input_frames_consumed_) * factor_));
    return [&]() -> size_t {
        if (frames) {
            return static_cast<size_t>(
                std::ceil((frames / channels_) * factor_));
        }
        if (expected_frames_produced >= output_frames_produced_) {
            return expected_frames_produced - output_frames_produced_;
        }
        return 0;
    }() + 1;
}

template <typename Callback>
inline PullConverter::PullConverter(
    Callback&& callback, SRCpp::Type type, int channels, double factor)
    : callback_ { std::make_unique<CallbackHandleImpl<Callback>>(
        std::forward<Callback>(callback), channels, type) }
    , factor_ { factor }
    , channels_ { channels }
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
    , scratch_output_(std::move(other.scratch_output_))
    , state_(other.state_)
    , factor_(other.factor_)
    , channels_(other.channels_)
{
    other.state_ = nullptr;
}

inline auto PullConverter::operator=(PullConverter&& other) noexcept
    -> PullConverter&
{
    if (this != &other) {
        using std::swap;
        swap(callback_, other.callback_);
        swap(scratch_output_, other.scratch_output_);
        swap(state_, other.state_);
        swap(factor_, other.factor_);
        swap(channels_, other.channels_);
    }
    return *this;
}

#if SRCPP_USE_CPP23
template <SupportedSampleType To>
inline auto PullConverter::convert_expected(std::span<To> output)
    -> std::expected<std::span<To>, std::string>
{
    auto [result, error] = convert(output);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}

inline auto PullConverter::convert_unsafe_expected(Format to, void* output,
    size_t output_size) -> std::expected<size_t, std::string>
{
    auto [result, error] = convert_unsafe(to, output, output_size);
    if (result.has_value()) {
        return *result;
    }
    return std::unexpected(error);
}
#endif // SRCPP_USE_CPP23

template <SupportedSampleType To>
inline auto PullConverter::convert(std::span<To> output)
    -> std::pair<std::optional<std::span<To>>, std::string>
{
    // where to put things?
    auto output_data = [&]() -> std::span<float> {
        if constexpr (std::is_same_v<To, float>) {
            return output;
        } else {
            scratch_output_.resize(output.size());
            return scratch_output_;
        }
    }();
    auto size = src_callback_read(
        state_, factor_, output_data.size() / channels_, output_data.data());
    if (size < 0) {
        return { std::nullopt, src_strerror(src_error(state_)) };
    }
    auto samples = size * channels_;
    // convert from float to output format
    if constexpr (std::is_same_v<To, short>) {
        src_float_to_short_array(output_data.data(), output.data(), samples);
    } else if constexpr (std::is_same_v<To, int>) {
        src_float_to_int_array(output_data.data(), output.data(), samples);
    }
    return { output.first(samples), {} };
}

inline auto PullConverter::convert_unsafe(Format to, void* output,
    size_t output_size) -> std::pair<std::optional<size_t>, std::string>
{
    switch (to) {
    case Format::Short:
        return details::convert_unsafe_helper<short>(
            *this, output, output_size);
    case Format::Int:
        return details::convert_unsafe_helper<int>(*this, output, output_size);
    case Format::Float:
        return details::convert_unsafe_helper<float>(
            *this, output, output_size);
    }
    return { std::nullopt, "Invalid format combination" };
}

template <typename Callback>
inline auto PullConverter::CallbackHandleImpl<Callback>::handle_callback(
    float** data) -> long
{
    using From = std::remove_cvref_t<
        typename std::invoke_result_t<Callback>::value_type>;
    static_assert(SupportedSampleType<From>,
        "Callback must return std::span<T> where T is short, int, or "
        "float");
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
    // convert from input format to float
    auto* inputData = [&]() {
        if constexpr (std::is_same_v<From, short>) {
            scratch_input_.resize(newData.size());
            src_short_to_float_array(
                newData.data(), scratch_input_.data(), newData.size());
            return scratch_input_.data();
        } else if constexpr (std::is_same_v<From, int>) {
            scratch_input_.resize(newData.size());
            src_int_to_float_array(
                newData.data(), scratch_input_.data(), newData.size());
            return scratch_input_.data();
        } else {
            return newData.data();
        }
    }();
    // https://github.com/libsndfile/libsamplerate/issues/208
    // When there is 1 frame of data, the linear SRC assumes it can read
    // the previous values and reads off the begin of the array.
    // Temporary fix until that is resolved.
    auto* fixedInputData = [&]() {
        if (type_ != SRCpp::Type::Linear) {
            return inputData;
        }
        if (newData.size() == static_cast<size_t>(channels_)) {
            last_input_.erase(
                last_input_.begin(), last_input_.end() - channels_);
            last_input_.insert(
                last_input_.end(), inputData, inputData + channels_);
            return last_input_.data() + channels_;
        }
        last_input_.assign(
            inputData + newData.size() - channels_, inputData + newData.size());
        return inputData;
    }();
    *data = fixedInputData;
    return newData.size() / channels_;
}

// deduction helpers
#if SRCPP_USE_CPP23
template <typename ToContainer, typename FromContainer,
    SupportedSampleType To = typename ToContainer::value_type,
    SupportedSampleType From = typename FromContainer::value_type>
auto Convert_expected(FromContainer const& input, ToContainer& output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<To>, std::string>
{
    return Convert_expected<To, From>(std::span<const From> { input },
        std::span<To> { output }, type, channels, factor);
}

template <SupportedSampleType To, typename FromContainer,
    SupportedSampleType From = typename FromContainer::value_type>
auto Convert_expected(
    FromContainer const& input, SRCpp::Type type, int channels, double factor)
    -> std::pair<std::optional<std::vector<To>>, std::string>
{
    return Convert_expected<To, From>(
        std::span<const From>(input), type, channels, factor);
}
#endif // SRCPP_USE_CPP23

template <typename FromContainer, typename ToContainer,
    SupportedSampleType From = typename FromContainer::value_type,
    SupportedSampleType To = typename ToContainer::value_type>
auto Convert(FromContainer const& input, ToContainer& output, SRCpp::Type type,
    int channels, double factor)
    -> std::pair<std::optional<std::span<To>>, std::string>
{
    return Convert<To, From>(std::span<const From> { input },
        std::span<To> { output }, type, channels, factor);
}

template <SupportedSampleType To, typename FromContainer,
    SupportedSampleType From = typename FromContainer::value_type>
auto Convert(FromContainer const& input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<To>>, std::string>
{
    return Convert<To, From>(
        std::span<const From> { input }, type, channels, factor);
}

namespace details {
    static_assert(SupportedSampleType<int>);
    static_assert(!SupportedSampleType<double>);
    static_assert(SampleTypeToFormat<short>() == Format::Short);
    static_assert(SampleTypeToFormat<int>() == Format::Int);
    static_assert(SampleTypeToFormat<float>() == Format::Float);
}
} // namespace SRCpp

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
