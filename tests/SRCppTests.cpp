#include <SRCpp/SRCpp.hpp>
#include <catch2/catch_test_macros.hpp>
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

    for (size_t i : std::views::iota(0UL, len)) {
        for (size_t ch = 0; ch < channels; ++ch) {
            data.at(i * channels + ch) = std::sin(hz[ch] * i * 2 * M_PI / sr);
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
    auto output_frames_produced = src_data.output_frames_gen;

    // flush out anything remaining
    src_data.data_in += src_data.input_frames_used * channels;
    src_data.data_out += src_data.output_frames_gen * channels;
    src_data.input_frames -= src_data.input_frames_used;
    src_data.output_frames -= src_data.output_frames_gen;
    src_data.end_of_input = true;

    result = src_process(state, &src_data);

    output.resize(
        (output_frames_produced + src_data.output_frames_gen) * channels);
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

TEST_CASE("ResampleWithPusher One Shot", "[SRCpp]")
{
    for (auto frames : { 16, 256, 257, 500 }) {
        for (auto type : { SRCpp::Type::Sinc_BestQuality,
                 SRCpp::Type::Sinc_MediumQuality, SRCpp::Type::Sinc_Fastest,
                 SRCpp::Type::ZeroOrderHold, SRCpp::Type::Linear }) {
            for (auto factor : { 1.5, 0.1, 0.5, 0.9, 1.0, 1.5, 2.0, 4.5 }) {
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
