#include "SRCppTestUtils.hpp"
#include <SRCpp/SRCpp.hpp>
#include <numbers>
#include <ranges>

// this effectively is what SRCpp::Resample does
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

    output.resize(src_data.output_frames_gen);
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
