# SRCpp
Modern C++ wrapper for libsamplerate

## Features

SRCpp provides a modern C++ interface for libsamplerate.  It requires C++23, and offers three distinct ways to perform sample rate conversion.

### 1. Convert
The `Convert` method allows you to process an entire audio buffer in one go. This is ideal for scenarios where you have all the audio data available upfront and need to perform a straightforward conversion.

```cpp
// Example: Using SRCpp::Convert
#include "SRCpp.h"
#include <vector>
#include <print>

int main() {
    auto input
        = std::vector { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f, -0.5f, -1.0f, -0.5f };
    auto ratio = 1.5;
    auto channels = 1;

    // Perform sample rate conversion with a ratio of 1.5
    auto output = SRCpp::Convert(
        input, SRCpp::Type::Sinc_MediumQuality, channels, ratio);

    // Output the converted data
    if (output) {
        std::println("Converted audio data: {}", *output);
    } else {
        std::println("Error: {}", output.error());
    }
}
```

### 2. PushConverter
The `PushConvert` enables you to feed audio data incrementally. This is useful for streaming applications where audio data arrives in chunks, and you want to process it as it becomes available.  The SRC wrapper may hold on to samples so it is necessary to "flush" the data.

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
    auto output = converter.convert(input).and_then(
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
// Example: Using SRCpp::Pull
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
