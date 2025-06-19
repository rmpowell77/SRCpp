# SRCpp
Modern C++ wrapper for libsamplerate

## Features

SRCpp provides a modern C++ interface for libsamplerate.  It requires minimum of C++20, and offers three distinct ways to perform sample rate conversion.

The library avoids using exceptions and instead leans towards using "error-like" results, specifically `std::pair` of an optional result and error string, or `std::expected` version if available.  This makes it the caller and only the caller's responsibility to deal with the error as they are encountered.

### 1. Convert
The `Convert` method allows you to process an entire audio buffer in one go. This is ideal for scenarios where you have all the audio data available upfront and need to perform a straightforward conversion.

```cpp
namespace SRCpp {

enum struct Type : int {
    Sinc_BestQuality = SRC_SINC_BEST_QUALITY,
    Sinc_MediumQuality = SRC_SINC_MEDIUM_QUALITY,
    Sinc_Fastest = SRC_SINC_FASTEST,
    ZeroOrderHold = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR
};

auto Convert_expected(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::expected<std::span<float>, std::string>;
auto Convert_expected(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::expected<std::vector<float>, std::string>;

auto Convert(std::span<const float> input, std::span<float> output,
    SRCpp::Type type, int channels, double factor)
    -> std::pair<optional<std::span<float>>, std::string>;
auto Convert(std::span<const float> input, SRCpp::Type type, int channels,
    double factor) -> std::pair<std::optional<std::vector<float>>, std::string>;

} // namespace SRCpp
```

Here is an example using SRCpp::Convert.

```cpp
#include "SRCpp.h"
#include <vector>
#include <print>

int main() {
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };
    auto ratio = 1.5;
    auto channels = 1;

    // Perform sample rate conversion with a ratio of 1.5
    auto [output, error] = SRCpp::Convert(
        input, SRCpp::Type::Sinc_MediumQuality, channels, ratio);

    // Output the converted data
    if (output) {
        std::println("Converted audio data: {}", *output);
    } else {
        std::println("Error: {}", error);
    }
}
```

### 2. PushConverter
The `PushConvert` enables you to feed audio data incrementally. This is useful for streaming applications where audio data arrives in chunks, and you want to process it as it becomes available.  The SRC wrapper may hold on to samples so it is necessary to "flush" the data.

There are two flavors: one where memory for output is supplied in the form of a `std::span`, and one where a `std::vector` is created and returned to hold on to the result.  It is incumbent on the caller to ensure they supply an appropriately sized output `std::span`.

```cpp
namespace SRCpp {

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

} // namespace SRCpp
```

Here is an example using SRCpp::PushConverter.

```cpp
// Example: Using SRCpp::Push
#include "SRCpp.h"
#include <vector>
#include <print>

int main() {
{
    // Input audio data (e.g., a sine wave)
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };
    auto ratio = 1.5;
    auto channels = 1;

    auto converter = SRCpp::PushConverter(
        SRCpp::Type::Sinc_MediumQuality, channels, ratio);
    auto output = converter.convert_expected(input).and_then(
        [&converter](
            auto data) -> std::expected<std::vector<float>, std::string> {
            auto flush = converter.flush();
            if (flush.has_value()) {
                data.insert(data.end(), flush->begin(), flush->end());
            }
            return data;
        });

    if (output.has_value()) {
        std::println("Push Converted audio data: {}", *output);
    } else {
        std::println("Error: {}", output.error());
    }
    return 0;
}
```

### 3. Pull
The `Pull` method allows you to request converted audio data on demand. This is particularly suited for cases where the consumer of the audio data dictates the pace of processing.  Returning an empty `span` indicates that the all the data has been consumed.


```cpp
namespace SRCpp {

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

} // namespace SRCpp
```

Here is an example using SRCpp::PullConverter.

```cpp
#include "SRCpp.h"
#include <vector>
#include <print>

int main() {
    // Input audio data (e.g., a sine wave)
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };

    auto ratio = 1.5;
    auto channels = 1;

    auto frames_left = input.size();
    auto callback =
        [&frames_left, input_span = std::span { input }]() -> std::span<float> {
        if (frames_left == 0) {
            return {};
        }
        frames_left -= input_span.size();
        return input_span;
    };
    auto puller = SRCpp::PullConverter(
        callback, SRCpp::Type::Sinc_MediumQuality, channels, ratio);

    auto buffer = std::vector<float>(input.size() * ratio);

    auto output = puller.convert(buffer);
    if (output.has_value()) {
        std::println("Pull Converted audio data: {}", *output);
    } else {
        std::println("Error: {}", output.error());
    }
    return 0;
}
```

## Installation

To use SRCpp, clone the repository and include it in your project. Ensure that you have libsamplerate installed on your system.

```bash
git clone https://github.com/rmpowell77/SRCpp.git
cd SRCpp
```

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests to improve SRCpp.

## Acknowledgments

This project is built on top of [libsamplerate](http://www.mega-nerd.com/SRC/). Special thanks to its developers for providing such a robust library.
