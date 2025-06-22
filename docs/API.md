# SRCpp.hpp

**C++ wrapper for libsamplerate sample rate conversion**

This header provides the `SRCpp` namespace, which contains high-level C++ interfaces for performing sample rate conversion using the libsamplerate library. It offers both push and pull conversion models, as well as utility functions for one-shot conversions.

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
- `Sinc_MediumQuality`: Medium quality sinc interpolation (`SRC_SINC_MEDIUM_QUALITY`)
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
- **Returns:** Pair of optional span of output samples written if no error, and error string if error occurred.

### `Convert` (with allocation)

Converts input audio data to a different sample rate, allocating the output buffer.

```cpp
auto Convert(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<float>>, std::string>;
```

- **Parameters:**
    - `input`: Input audio samples (interleaved, per channel)
    - `type`: Conversion algorithm
    - `channels`: Number of audio channels
    - `factor`: Sample rate conversion factor
- **Returns:** Pair of optional vector of output samples if no error, and error string if error occurred.


### `Convert_expected` (C++23 only)

As `Convert`, except returning `std::expected`.

```cpp
auto Convert_expected(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>;
auto Convert_expected(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::expected<std::vector<float>, std::string>;
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

Provides an interface for incremental (push) sample rate conversion, allowing input to be provided in chunks and output to be retrieved as available.

- **Constructor:** `PushConverter(Type type, int channels, double factor)`
    - Constructs a new push converter with the specified algorithm, channel count, and conversion factor.

- **Methods:**
    - `convert(input, output)`: Converts a chunk of input samples, writing to the provided output buffer.  
        Returns pair of optional span of output samples written and error string.
    - `convert(input)`: Converts a chunk of input samples, allocating the output buffer.  
        Returns pair of optional vector of output samples and error string.
    - `flush()`: Flushes any remaining samples from the converter.  
        Returns pair of optional vector of output samples and error string.
    - `convert_expected`, `flush_expected`: (C++23 only) Variants returning `std::expected`.

- **Notes:** Copy and move constructors/assignment are supported. Copying clones the internal state.

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

Provides an interface for sample rate conversion where output is requested and input is supplied via a callback.

- **Typedef:** `callback_t`  
    `std::function<std::span<float>()>`  
    Callback type for supplying input samples.

- **Constructor:** `PullConverter(callback_t callback, Type type, int channels, double factor)`  
    Constructs a new pull converter with the specified callback, algorithm, channel count, and conversion factor.

- **Methods:**
    - `convert(output)`: Requests output samples, filling the provided buffer.  
        Returns pair of optional span of output samples written and error string.
    - `convert_expected`: (C++23 only) Variant returning `std::expected`.

- **Notes:** Copying is disabled; only move operations are supported.

---

## Details

- All functions and classes are exception-safe. Errors are reported via return values or exceptions (for construction failures).
- Input and output buffers are assumed to be interleaved per channel.
- The sample rate conversion factor is defined as `output_sample_rate / input_sample_rate`.
- The implementation includes a workaround for libsamplerate issue #208 for linear interpolation.

---

## See Also

- [libsamplerate documentation](http://www.mega-nerd.com/SRC/)
- `std::span`, `std::vector`, `std::expected` (C++23)
