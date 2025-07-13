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
supplies untyped memory location and a `Format` enum of the appropriate "from" and
"to" type.  This is to allow usage of conversion at run time where the
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
