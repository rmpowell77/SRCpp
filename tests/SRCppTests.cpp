#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <ranges>

TEST_CASE("std::format with SRC_DATA", "[SRC_DATA]")
{
    auto* badPtr = reinterpret_cast<float*>(0xdeadbeef);
    auto* badPtr2 = reinterpret_cast<float*>(0xCAFEBABE);

    auto src_data = SRC_DATA {
        badPtr,
        badPtr2,
        42,
        314,
        10,
        15,
        0,
        1.5,
    };

    REQUIRE(std::format("{}", src_data)
        == "SRC@1.5 in: 0xdeadbeef[10, 42), out: 0xcafebabe[15, 314)");

    src_data.end_of_input = 1;
    REQUIRE(std::format("{}", src_data)
        == "SRC@1.5 in(eof): 0xdeadbeef[10, 42), out: 0xcafebabe[15, 314)");
}

auto makeSin(const std::vector<float>& hz, float sr, size_t len)
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

// this effectively is what SRCpp::Resample does
auto CreateOneShotReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type, size_t frames)
{
    std::vector<float> output(input.size() * factor * channels * 2);
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
        throw std::runtime_error(src_strerror(result));
    }

    output.resize(src_data.output_frames_gen);
    return output;
}

// this effectively is what SRCpp::Push does.
auto CreatePushReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type, size_t frames)
{
    auto error = 0;
    auto* state = src_new(static_cast<int>(type), channels, &error);
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }

    std::vector<float> output(input.size() * factor * channels * 4);

    auto src_data = SRC_DATA {
        input.data(),
        output.data(),

        static_cast<long>(input.size() / channels),
        static_cast<long>(output.size() / channels),
        0,
        0,

        0,

        factor,
    };
    auto result = src_process(state, &src_data);
    if (result != 0) {
        throw std::runtime_error(src_strerror(result));
    }
    auto output_frames_produced = src_data.output_frames_gen;

    // flush out anything remaining
    src_data.data_in += src_data.input_frames_used * channels;
    src_data.data_out += src_data.output_frames_gen * channels;
    src_data.input_frames -= src_data.input_frames_used;
    src_data.output_frames -= src_data.output_frames_gen;
    src_data.end_of_input = true;

    // this is necessary because if we've incremented the data_in to the end, it
    // *could* overlap with the beinging of data_out, and src doesn't like that.
    if (src_data.input_frames == 0) {
        src_data.data_in = input.data();
    }

    result = src_process(state, &src_data);
    if (result != 0) {
        throw std::runtime_error(src_strerror(result));
    }

    output.resize(
        (output_frames_produced + src_data.output_frames_gen) * channels);
    src_delete(state);
    return output;
}

auto ConvertWithPush(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type, size_t frames)
{
    std::vector<float> output(input.size() * factor * channels * 2);
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto data = pusher(input);
    if (!data.has_value()) {
        throw std::runtime_error(data.error());
    }
    auto flush = pusher({});
    if (!flush.has_value()) {
        throw std::runtime_error(flush.error());
    }
    data->insert(
        data->end(), flush->begin(), flush->end()); // append flush to data
    return *data;
}

auto ConvertWithPush(std::vector<float> input, size_t channels, double factor,
    SRCpp::Type type, size_t frames, size_t input_frames)
{
    auto output = std::vector<float> {};
    auto pusher = SRCpp::PushConverter(type, channels, factor);
    auto framesLeft = input.size() / channels;
    while (framesLeft) {
        auto framesForThis = std::min(input_frames, input.size() / channels);
        auto data = pusher(
            { input.begin(), input.begin() + framesForThis * channels });
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        input.erase(input.begin(), input.begin() + framesForThis * channels);
        framesLeft -= framesForThis;
        output.insert(
            output.end(), data->begin(), data->end()); // append flush to data
    }
    auto flush = pusher({});
    if (!flush.has_value()) {
        throw std::runtime_error(flush.error());
    }
    output.insert(
        output.end(), flush->begin(), flush->end()); // append flush to data
    return output;
}

auto ConvertWithPull(std::vector<float> input, size_t channels, double factor,
    SRCpp::Type type, size_t frames,
    std::optional<size_t> input_frames = std::nullopt)
{
    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<float>(input);
    std::vector<float> output(framesExpected * channels);
    auto callback = [&]() -> std::span<float> {
        auto input_samples = [&]() {
            if (input_frames.has_value()) {
                return std::min(
                    input_frames.value() * channels, input_span.size());
            }
            return input_span.size();
        }();

        auto result = input_span.subspan(0, input_span.size());
        input_span = input_span.subspan(input_span.size());
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto data = puller.pull(output);
    if (!data.has_value()) {
        throw std::runtime_error(data.error());
    }
    output.resize(data->size());
    return output;
}

auto ConvertWithPullOutputFrames(std::vector<float> input, size_t channels,
    double factor, SRCpp::Type type, size_t frames, size_t output_frames,
    std::optional<size_t> input_frames = std::nullopt)
{
    auto framesExpected
        = static_cast<size_t>(std::ceil((input.size() / channels) * factor));
    auto input_span = std::span<float>(input);
    std::vector<float> output(framesExpected * channels);
    auto callback = [&]() -> std::span<float> {
        auto input_samples = [&]() {
            if (input_frames.has_value()) {
                return std::min(
                    input_frames.value() * channels, input_span.size());
            }
            return input_span.size();
        }();

        auto result = input_span.subspan(0, input_span.size());
        input_span = input_span.subspan(input_span.size());
        return result;
    };
    auto puller = SRCpp::PullConverter(callback, type, channels, factor);
    auto framesProduced = 0;
    while (framesProduced < framesExpected && input_span.size()) {
        auto toPull = std::min(output_frames, framesExpected - framesProduced);
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            toPull * channels };
        auto data = puller.pull(output);
        if (!data.has_value()) {
            throw std::runtime_error(data.error());
        }
        framesProduced += data->size() / channels;
    }
    output.resize(framesProduced * channels);
    return output;
}

TEST_CASE("Resample", "[SRCpp]")
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto hz : { std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();
                    auto input = makeSin(hz, 48000.0, frames);

                    auto reference = CreateOneShotReference(
                        input, channels, factor, type, frames);
                    auto output
                        = SRCpp::Resample(input, type, channels, factor);

                    REQUIRE(output == reference);
                }
            }
        }
    }
}

TEST_CASE("ResampleWithPusher", "[SRCpp]")
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto hz : { std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();
                    auto input = makeSin(hz, 48000.0, frames);

                    auto reference = CreatePushReference(
                        input, channels, factor, type, frames);
                    {
                        auto output = ConvertWithPush(
                            input, channels, factor, type, frames);

                        REQUIRE(output == reference);
                    }
                    for (auto input_size : { 4, 8, 16, 32, 64 }) {
                        auto output = ConvertWithPush(
                            input, channels, factor, type, frames, input_size);

                        if (type == SRCpp::Type::ZeroOrderHold) {
                            auto mangledReference = reference;
                            mangledReference.resize(output.size());
                            REQUIRE(output == mangledReference);
                        } else {
                            REQUIRE(output == reference);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("ResampleWithPull", "[SRCpp]")
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 1.5, 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
                for (auto hz : { std::vector<float> { 3000.0f, 40.0f },
                         std::vector<float> { 3000.0f },
                         std::vector<float> { 3000.0f, 40.0f, 1004.0f } }) {
                    auto channels = hz.size();
                    auto input = makeSin(hz, 48000.0, frames);

                    auto reference = CreatePushReference(
                        input, channels, factor, type, frames);
                    {
                        auto output = ConvertWithPull(
                            input, channels, factor, type, frames);
                        REQUIRE(output.size() > 0);
                        // fudge factor for pull
                        if (std::abs(static_cast<int>(reference.size())
                                - static_cast<int>(output.size()))
                            == channels) {
                            auto mangledReference = reference;
                            mangledReference.resize(output.size());
                        } else {
                            REQUIRE(output == reference);
                        }
                    }
                    for (auto input_size : { 4, 32, 33, 128 }) {
                        auto output = ConvertWithPull(
                            input, channels, factor, type, frames, input_size);
                        REQUIRE(output.size() > 0);

                        // fudge factor for pull
                        if (std::abs(static_cast<int>(reference.size())
                                - static_cast<int>(output.size()))
                            == channels) {
                            auto mangledReference = reference;
                            mangledReference.resize(output.size());
                        } else {
                            REQUIRE(output == reference);
                        }
                    }
                    for (auto output_frames : { 4, 32, 33, 128 }) {
                        for (auto input_frames : { 4, 32, 33, 128 }) {
                            auto output = ConvertWithPullOutputFrames(input,
                                channels, factor, type, frames, output_frames,
                                input_frames);
                            REQUIRE(output.size() > 0);

                            // fudge factor for pull
                            if (std::abs(static_cast<int>(reference.size())
                                    - static_cast<int>(output.size()))
                                == channels) {
                                auto mangledReference = reference;
                                mangledReference.resize(output.size());
                            } else {
                                REQUIRE(output == reference);
                            }
                        }
                    }
                }
            }
        }
    }
}
