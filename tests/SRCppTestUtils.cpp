#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <iostream>

// this effectively is what SRCpp::Convert does
auto CreateOneShotReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>
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

    output.resize(src_data.output_frames_gen * channels);
    return output;
}

// this effectively is what SRCpp::Push does.
auto CreatePushReference(const std::vector<float>& input, size_t channels,
    double factor, SRCpp::Type type) -> std::vector<float>
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

struct Callback {
    std::function<std::span<float>()> function;
    size_t channels;
};

// this effectively is what SRCpp::Pull does.
auto CreatePullReference(std::vector<float> input, size_t channels,
    double factor, SRCpp::Type type, size_t in_size, size_t out_size)
    -> std::vector<float>
{
    auto error = 0;

    auto input_span = std::span<float>(input);
    Callback callback {
        [&]() -> std::span<float> {
            auto input_samples = [&]() {
                return std::min(in_size * channels, input_span.size());
            }();
            auto result = input_span.subspan(0, input_samples);
            input_span = input_span.subspan(input_samples);
            return result;
        },
        channels,
    };
    auto* state = src_callback_new(
        [](void* cb_data, float** data) -> long {
            auto* callback = static_cast<Callback*>(cb_data);
            auto inData = callback->function();
            static float dummy;
            *data = inData.data();
            if (inData.size() == 0) {
                *data = &dummy;
                return 0;
            }
            return inData.size() / callback->channels;
        },
        static_cast<int>(type), channels, &error, &callback);
    if (error != 0) {
        throw std::runtime_error(src_strerror(error));
    }

    std::vector<float> output(std::max<size_t>(
        input.size() * factor * channels * 4, out_size * channels * 4));

    auto framesProduced = 0UL;
    auto result0count = 0;
    while (result0count < 2) {
        auto framesToPull = out_size;
        auto pullBuffer = std::span { output.data() + framesProduced * channels,
            framesToPull * channels };
        auto result = src_callback_read(
            state, factor, pullBuffer.size() / channels, pullBuffer.data());
        if (result < 0) {
            throw std::runtime_error(src_strerror(src_error(state)));
        }
        if (result == 0) {
            ++result0count;
        } else {
            result0count = 0;
        }
        framesProduced += result;
    }
    output.resize(framesProduced * channels);
    src_delete(state);
    return output;
}
