# SRCpp
Modern C++ wrapper for libsamplerate

## Features

SRCpp provides a modern C++ interface for libsamplerate.  It requires minimum of C++20, and offers three distinct ways to perform sample rate conversion, Covert ("one shot") conversion, Push Conversion, and Pull Conversion.

The library avoids using exceptions and instead leans towards using "error-like" results, specifically `std::pair` of an optional result and error string, or `std::expected` version if available.  This makes it the caller and only the caller's responsibility to deal with the error as they are encountered.  There are inevitable errors such as out of memory conditons where exceptions would be thrown, but those are, well, exceptional.

SRCpp operates on arrays of audio samples, and assumes the audio data is always interleaved. In this context, a sample refers to an individual PCM value for a single channel, while a frame denotes a set of samples—one per channel—that are time-aligned.

SRCpp attempts to be flexibly on the types of inputs and outputs, supporting the most common audio types - Int16 (shorts), Int32 (int), and Floating point (floats).  It attempts to use implicit type deduction but in situations where the type cannot be implicitly inferred, we require types to be used to avoid confusion -- we want you to fall into pits of success.  Most APIs can deduce the `Input`/`From` type, but the `Output`/`To` often is *not* able to be deduce, some APIs require explicit type parameters to be supplied.

Please consult the [API](docs/API.md) for specific API details.

### 1. Convert
The `Convert` method allows you to process an entire audio buffer in one go. This is ideal for scenarios where you have all the audio data available upfront and need to perform a straightforward conversion.

Here is an example using `SRCpp::Convert`:

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
    auto [output, error] = SRCpp::Convert<float>(
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

Here is an example using `SRCpp::PushConverter`.

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
    auto output = converter.convert_expected<float>(input).and_then(
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

Care should be taken with callback lambda supplied.  Returning a `std::span` of size 0 indicates an end of stream condition, which will cause the SRC to be flushed and, once exhausted, will no longer produce output.  Once the lambda returns size 0, it is expect that it will continue to do so on every subsequent call or else the result is undefined.


Here is an example using `SRCpp::PullConverter`:

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
