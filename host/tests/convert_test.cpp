//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <stdint.h>
// NOTE: <list> MUST be included before <boost/test/data/test_case.hpp> to
// work around a bug in Boost 1.65.
// clang-format off
#include <list>
// clang-format on
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <array>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace uhd;

// typedefs for complex types
typedef std::complex<int16_t> sc16_t;
typedef std::complex<float> fc32_t;
typedef std::complex<double> fc64_t;

static constexpr size_t BENCHMARK_NSAMPS = 8 * 1024 * 1024;
static constexpr size_t BENCHMARK_NITERS = 4;

// Holds performance information about a conversion run
struct benchmark_result
{
    convert::id_type id;
    uhd::convert::priority_type prio;
    double elapsed_ns;
    size_t nsamps;
};

// List of priority types. This must be manually kept in sync with whatever is
// defined in convert_common.hpp
const std::array<uhd::convert::priority_type, 5> CONV_PRIO_TYPES{-1, 0, 1, 2, 3};

// Use this to create a converter with fixed prio in a test case. If prio does
// not exist, we simply exit the test case. That's normal.
#define GET_CONVERTER_SAFE(conv_name, id, prio)         \
    convert::converter::sptr conv_name;                 \
    try {                                               \
        conv_name = convert::get_converter(id, prio)(); \
    } catch (uhd::key_error&) {                         \
        return;                                         \
    }

// There appears to be a bug with Boost <1.68 where decorators on Boost data
// test cases are not honored. (While I could not find the bug itself, it is
// referred to in a comment at https://github.com/boostorg/test/issues/139 on
// Boost's GitHub bug tracker.) Unfortunately, that means that the benchmarks
// are run by default on those versions of Boost, causing this unit test to
// take much longer than usual and thus slowing down our entire CI pipeline.
// The SKIP_BENCHMARK_CHECK macro implements a run-time check on older versions
// of Boost to determine whether the benchmarks should be skipped or not. To
// 'unskip' the benchmarks on those versions, pass `--benchmark` to the
// invocation of `convert_test` (note that the `--benchmark` flag must be
// specified after `--` to ensure it is passed directly to the test and not
// interpreted by Boost.)
#if (BOOST_VERSION < 106800)
#    define SKIP_BENCHMARK_CHECK                                                         \
        if (!(boost::unit_test::framework::master_test_suite().argc >= 2                 \
                && std::string(boost::unit_test::framework::master_test_suite().argv[1]) \
                       == "--benchmark")) {                                              \
            return;                                                                      \
        }
#else
// For versions of Boost where this issue has been fixed, the benchmarks can
// be enabled by invoking convert_test with `--run-test=+benchmark*` to
// explicitly enable all the disabled benchmark tests.
#    define SKIP_BENCHMARK_CHECK
#endif

// Shorthand for defining a test case that tests all prios. Creates a variable
// 'conv_prio_type'
#define MULTI_CONVERTER_TEST_CASE(test_name) \
    BOOST_DATA_TEST_CASE(test_name, CONV_PRIO_TYPES, conv_prio_type)

#define MY_CHECK_CLOSE(a, b, f)                                                         \
    {                                                                                   \
        static bool error_encountered = false;                                          \
        if (!error_encountered && (std::abs((a) - (b)) >= f)) {                         \
            BOOST_ERROR(                                                                \
                "\n\t" << #a << " (" << (a) << ") error " << #b << " (" << (b) << ")"); \
            error_encountered = true;                                                   \
        }                                                                               \
    }

// Given a converter ID describing a conversion from input type to
// output type, return the 'reverse' converter ID from output type to
// input type
static convert::id_type reverse_converter(const convert::id_type& in)
{
    convert::id_type out = in;
    std::swap(out.input_format, out.output_format);
    std::swap(out.num_inputs, out.num_outputs);
    return out;
}

/***********************************************************************
 * Loopback runner:
 *    convert input buffer into intermediate buffer
 *    convert intermediate buffer into output buffer
 *    optionally collect benchmark data
 **********************************************************************/
template <typename Range>
static void loopback(size_t nsamps,
    convert::id_type& in_id,
    convert::id_type& out_id,
    const Range& input,
    Range& output,
    const int prio_in,
    const int prio_out,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // make this buffer large enough for all test types
    std::vector<uint64_t> interm(nsamps);

    std::vector<const void*> input0{&input[0]}, input1{&interm[0]};
    std::vector<void*> output0{&interm[0]}, output1{&output[0]};

    // convert to intermediate type
    convert::converter::sptr c0 = convert::get_converter(in_id, prio_in)();
    c0->set_scalar(32767.);
    if (benchmark_data) {
        const auto start_time = std::chrono::steady_clock::now();
        c0->conv(input0, output0, nsamps);
        const auto end_time = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::nano> elapsed_in2out =
            end_time - start_time;
        benchmark_data->push_back({in_id, prio_in, elapsed_in2out.count(), nsamps});
    } else {
        c0->conv(input0, output0, nsamps);
    }

    // convert back to host type
    convert::converter::sptr c1 = convert::get_converter(out_id, prio_out)();
    c1->set_scalar(1 / 32767.);
    if (benchmark_data) {
        const auto start_time = std::chrono::steady_clock::now();
        c1->conv(input1, output1, nsamps);
        const auto end_time = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::nano> elapsed_out2in =
            end_time - start_time;
        benchmark_data->push_back({out_id, prio_out, elapsed_out2in.count(), nsamps});
    } else {
        c1->conv(input1, output1, nsamps);
    }
}

// Use this to call the loopback runner from a test so that missing prio won't
// become an issue
#define CALL_LOOPBACK_SAFE(...) \
    try {                       \
        loopback(__VA_ARGS__);  \
    } catch (uhd::key_error&) { \
        return;                 \
    }

// Iterates over a collection of individual benchmark results, collecting
// the results from multiple runs with the same converter ID and priority
// and prints out the results
static void collate_benchmark_results(std::vector<benchmark_result> benchmarks)
{
    while (!benchmarks.empty()) {
        // Get the first entry from the per-iteration runs
        struct benchmark_result result = *(benchmarks.begin());
        // Remove that entry from the list, and look for other entries in
        // the list that have the same converter and priority
        auto b_iter = benchmarks.erase(benchmarks.begin());
        while (b_iter != benchmarks.end()) {
            if (b_iter->id == result.id && b_iter->prio == result.prio) {
                // If a match is found, accumulate the elapsed time and
                // number of samples
                result.elapsed_ns += b_iter->elapsed_ns;
                result.nsamps += b_iter->nsamps;
                // And then remove it from the list
                b_iter = benchmarks.erase(b_iter);
            } else {
                // Not a match; move on
                b_iter++;
            }
        }
        double ns_per_sample = result.elapsed_ns / result.nsamps;
        std::cout << "For converter " << result.id.to_string() << " prio " << result.prio
                  << ": " << ns_per_sample << " ns/sample" << std::endl;
    }
}

/***********************************************************************
 * Run converter test code under a benchmark
 * This overload takes a prio, for the MULTI_CONVERTER_TEST_CASES which
 * receive it as a parameter
 **********************************************************************/
template <typename ConverterFunction>
static void benchmark_converter(convert::id_type id,
    uhd::convert::priority_type prio,
    ConverterFunction&& converter_fn)
{
    std::vector<benchmark_result> benchmarks;
    for (size_t iter = 0; iter < BENCHMARK_NITERS; iter++) {
        std::vector<benchmark_result> benchmarks_iter;
        converter_fn(BENCHMARK_NSAMPS, id, prio, &benchmarks_iter);
        // Detect if the benchmark didn't run because the converter type
        // with the given priority wasn't found; if that's the case, bail
        // on the test case
        if (benchmarks_iter.empty()) {
            return;
        }
        // Save the results for this iteration
        std::copy(benchmarks_iter.begin(),
            benchmarks_iter.end(),
            std::back_inserter(benchmarks));
    }
    collate_benchmark_results(benchmarks);
}

/***********************************************************************
 * Run converter test code under a benchmark
 * This overload does not take a prio, for the converter functions which
 * iterate them automatically (the test_convert_types_for_floats variant)
 **********************************************************************/
template <typename ConverterFunction>
static void benchmark_converter(convert::id_type id, ConverterFunction&& converter_fn)
{
    std::vector<benchmark_result> benchmarks;
    for (size_t iter = 0; iter < BENCHMARK_NITERS; iter++) {
        std::vector<benchmark_result> benchmarks_iter;
        converter_fn(BENCHMARK_NSAMPS, id, &benchmarks_iter);
        // Detect if the benchmark didn't run because the converter type
        // with the given priority wasn't found; if that's the case, bail
        // on the test case
        if (benchmarks_iter.empty()) {
            return;
        }
        // Save the results for this iteration
        std::copy(benchmarks_iter.begin(),
            benchmarks_iter.end(),
            std::back_inserter(benchmarks));
    }
    collate_benchmark_results(benchmarks);
}

/***********************************************************************
 * Test short conversion
 **********************************************************************/
static void test_convert_types_sc16(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    const int extra_div                           = 1,
    int mask                                      = 0xffff,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // fill the input samples
    std::vector<sc16_t> input(nsamps), output(nsamps);
    for (sc16_t& in : input) {
        in = sc16_t(
            short((float((std::rand()) / (double(RAND_MAX) / 2)) - 1) * 32767 / extra_div)
                & mask,
            short((float((std::rand()) / (double(RAND_MAX) / 2)) - 1) * 32767 / extra_div)
                & mask);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio, benchmark_data);
    if (!benchmark_data) {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            input.begin(), input.end(), output.begin(), output.end());
    }
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_be_sc16)
{
    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_be_sc16)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 1, 0xffff, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_le_sc16)
{
    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_le_sc16)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 1, 0xffff, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_chdr_sc16)
{
    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_chdr_sc16)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "sc16";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 1, 0xffff, benchmarks);
        });
}

/***********************************************************************
 * Test float conversion
 **********************************************************************/
template <typename data_type>
static void test_convert_types_for_floats(size_t nsamps,
    convert::id_type& id,
    const double extra_scale                      = 1.0,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    typedef typename data_type::value_type value_type;

    // fill the input samples
    std::vector<data_type> input(nsamps), output(nsamps);
    for (data_type& in : input) {
        in = data_type(
            ((std::rand() / (value_type(RAND_MAX) / 2)) - 1) * float(extra_scale),
            ((std::rand() / (value_type(RAND_MAX) / 2)) - 1) * float(extra_scale));
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);

    // make a list of all prio: best/generic combos
    typedef std::pair<int, int> int_pair_t;
    const std::vector<int_pair_t> prios{
        int_pair_t(0, 0), int_pair_t(-1, 0), int_pair_t(0, -1), int_pair_t(-1, -1)};

    // loopback foreach prio combo (generic vs best)
    for (const auto& prio : prios) {
        CALL_LOOPBACK_SAFE(nsamps,
            in_id,
            out_id,
            input,
            output,
            prio.first,
            prio.second,
            benchmark_data);
        for (size_t i = 0; i < nsamps && (!benchmark_data); i++) {
            MY_CHECK_CLOSE(input[i].real(), output[i].real(), value_type(1. / (1 << 14)));
            MY_CHECK_CLOSE(input[i].imag(), output[i].imag(), value_type(1. / (1 << 14)));
        }
    }
}

template <typename data_type>
static void test_convert_types_for_floats_with_saturation(
    size_t nsamps, convert::id_type& id, const double extra_scale = 1.0)
{
    typedef typename data_type::value_type value_type;

    // fill the input samples
    std::vector<data_type> input(nsamps), output(nsamps), expected_output(nsamps);
    for (size_t sample_count = 0; sample_count < nsamps; sample_count++) {
        value_type real_data =
            ((std::rand() / (value_type(RAND_MAX) / 2)) - 1) * float(extra_scale);
        value_type imag_data =
            ((std::rand() / (value_type(RAND_MAX) / 2)) - 1) * float(extra_scale);
        value_type expected_real_data = real_data;
        value_type expected_imag_data = imag_data;
        if (sample_count & 1) {
            // To ensure that some samples are out of range and thus should be
            // saturated, every alternating sample's real and imaginary values
            // are pushed outside of the [-1..1] range.
            real_data = (real_data < 0.0) ? real_data - 1.0 : real_data + 1.0;
            imag_data = (imag_data < 0.0) ? imag_data - 1.0 : imag_data + 1.0;

            // The expected output values after loopback for these samples are
            // -1.0 or 1.0.
            expected_real_data = (real_data < 0.0) ? -1.0 : 1.0;
            expected_imag_data = (imag_data < 0.0) ? -1.0 : 1.0;
        }
        input[sample_count]           = data_type(real_data, imag_data);
        expected_output[sample_count] = data_type(expected_real_data, expected_imag_data);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);

    // make a list of all prio: best/generic combos
    typedef std::pair<int, int> int_pair_t;
    const std::vector<int_pair_t> prios{
        int_pair_t(0, 0), int_pair_t(-1, 0), int_pair_t(0, -1), int_pair_t(-1, -1)};

    // loopback foreach prio combo (generic vs best)
    for (const auto& prio : prios) {
        CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio.first, prio.second);
        for (size_t i = 0; i < nsamps; i++) {
            MY_CHECK_CLOSE(
                expected_output[i].real(), output[i].real(), value_type(1. / (1 << 14)));
            MY_CHECK_CLOSE(
                expected_output[i].imag(), output[i].imag(), value_type(1. / (1 << 14)));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_be_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_le_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_chdr_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_chdr_fc32_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_chdr_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc64)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc64_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc64_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc64_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_be_fc64)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc64_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc64)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc64_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc64_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc64_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_le_fc64)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc64_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_chdr_fc64)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc64_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_chdr_fc64_with_saturation)
{
    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats_with_saturation<fc64_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_chdr_fc64)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc64";
    id.num_inputs    = 1;
    id.output_format = "sc16_chdr";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc64_t>(nsamps, id, 1.0, benchmarks);
        });
}

/***********************************************************************
 * Test float to/from sc12 conversion loopback
 **********************************************************************/

BOOST_AUTO_TEST_CASE(test_convert_types_le_sc12_with_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc12_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 16);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_le_sc12_with_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc12_item32_le";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 16, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_sc12_with_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc12_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 16);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_be_sc12_with_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "sc12_item32_be";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 16, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_le_sc16_and_sc12)
{
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "sc12_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type, 1, 0xfff0);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_le_sc16_and_sc12)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "sc12_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 1, 0xfff0, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_be_sc16_and_sc12)
{
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "sc12_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type, 1, 0xfff0);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_be_sc16_and_sc12)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "sc12_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 1, 0xfff0, benchmarks);
        });
}

/***********************************************************************
 * Test float to/from fc32 conversion loopback
 **********************************************************************/

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32_with_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_item32_le";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_le_fc32_with_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_item32_le";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32_with_fc32)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_item32_be";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_be_fc32_with_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_item32_be";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_fc32_with_fc32_chdr)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_chdr";
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_fc32_with_fc32_chdr)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.num_inputs    = 1;
    id.output_format = "fc32_chdr";
    id.num_outputs   = 1;

    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1.0, benchmarks);
        });
}

/***********************************************************************
 * Test sc8 conversions
 **********************************************************************/
BOOST_AUTO_TEST_CASE(test_convert_types_fc64_and_sc8)
{
    convert::id_type id;
    id.input_format = "fc64";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc64_t>(nsamps, id, 1. / 256);
    }

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc64_t>(nsamps, id, 1. / 256);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_fc64_and_sc8)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "fc64";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "sc8_item32_le";
    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc64_t>(nsamps, id, 1. / 256, benchmarks);
        });

    id.output_format = "sc8_item32_be";
    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc64_t>(nsamps, id, 1. / 256, benchmarks);
        });
}

BOOST_AUTO_TEST_CASE(test_convert_types_fc32_and_sc8)
{
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 256);
    }

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 256);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
BOOST_AUTO_TEST_CASE(benchmark_convert_types_fc32_and_sc8)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "sc8_item32_le";
    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 256, benchmarks);
        });

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    benchmark_converter(id,
        [](size_t nsamps,
            convert::id_type id,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_for_floats<fc32_t>(nsamps, id, 1. / 256, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_sc16_and_sc8)
{
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type, 256);
    }

    // try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_sc16(nsamps, id, conv_prio_type, 256);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_sc16_and_sc8)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "sc8_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 256, 0xffff, benchmarks);
        });

    id.output_format = "sc8_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_sc16(nsamps, id, prio, 256, 0xffff, benchmarks);
        });
}

/***********************************************************************
 * Test u8 conversion
 **********************************************************************/
static void test_convert_types_u8(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // fill the input samples
    std::vector<uint8_t> input(nsamps), output(nsamps);
    for (uint8_t& in : input) {
        in = uint8_t(std::rand() & 0xFF);
    }
    // uint32_t d = 48;
    // for(uint8_t &in:  input) in = d++;

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio, benchmark_data);
    if (!benchmark_data) {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            input.begin(), input.end(), output.begin(), output.end());
    }
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_u8_and_u8)
{
    convert::id_type id;
    id.input_format = "u8";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "u8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_u8(nsamps, id, conv_prio_type);
    }

    // try various lengths to test edge cases
    id.output_format = "u8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_u8(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_u8_and_u8)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "u8";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "u8_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_u8(nsamps, id, prio, benchmarks);
        });

    // try various lengths to test edge cases
    id.output_format = "u8_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_u8(nsamps, id, prio, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_u8_and_u8_chdr)
{
    convert::id_type id;
    id.input_format  = "u8";
    id.output_format = "u8_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_u8(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_u8_and_u8_chdr)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "u8";
    id.output_format = "u8_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_u8(nsamps, id, prio, benchmarks);
        });
}

/***********************************************************************
 * Test s8 conversion
 **********************************************************************/
static void test_convert_types_s8(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // fill the input samples
    std::vector<int8_t> input(nsamps), output(nsamps);
    for (int8_t& in : input) {
        in = int8_t(std::rand() & 0xFF);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio, benchmark_data);
    if (!benchmark_data) {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            input.begin(), input.end(), output.begin(), output.end());
    }
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_s8_and_s8)
{
    convert::id_type id;
    id.input_format = "s8";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "s8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s8(nsamps, id, conv_prio_type);
    }

    // try various lengths to test edge cases
    id.output_format = "s8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s8(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_s8_and_s8)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "s8";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "s8_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s8(nsamps, id, prio, benchmarks);
        });

    // try various lengths to test edge cases
    id.output_format = "s8_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s8(nsamps, id, prio, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_s8_and_s8_chdr)
{
    convert::id_type id;
    id.input_format  = "s8";
    id.output_format = "s8_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s8(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_s8_and_s8_chdr)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "s8";
    id.output_format = "s8_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s8(nsamps, id, prio, benchmarks);
        });
}

/***********************************************************************
 * Test s16 conversion
 **********************************************************************/
static void test_convert_types_s16(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // fill the input samples
    std::vector<int16_t> input(nsamps), output(nsamps);
    for (int16_t& in : input) {
        in = int16_t(std::rand() & 0xFFFF);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio, benchmark_data);
    if (!benchmark_data) {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            input.begin(), input.end(), output.begin(), output.end());
    }
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_s16_and_s16)
{
    convert::id_type id;
    id.input_format = "s16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "s16_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s16(nsamps, id, conv_prio_type);
    }

    // try various lengths to test edge cases
    id.output_format = "s16_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s16(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_s16_and_s16)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "s16";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "s16_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s16(nsamps, id, prio, benchmarks);
        });

    // try various lengths to test edge cases
    id.output_format = "s16_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s16(nsamps, id, prio, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_s16_and_s16_chdr)
{
    convert::id_type id;
    id.input_format  = "s16";
    id.output_format = "s16_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_s16(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_s16_and_s16_chdr)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "s16";
    id.output_format = "s16_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_s16(nsamps, id, prio, benchmarks);
        });
}

/***********************************************************************
 * Test fc32 -> fc32 conversion
 **********************************************************************/
static void test_convert_types_fc32(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    std::vector<benchmark_result>* benchmark_data = nullptr)
{
    // fill the input samples
    std::vector<std::complex<float>> input(nsamps), output(nsamps);
    for (fc32_t& in : input) {
        in = fc32_t((std::rand() / float(RAND_MAX / 2)) - 1,
            (std::rand() / float(RAND_MAX / 2)) - 1);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio, benchmark_data);
    for (size_t i = 0; i < nsamps && (!benchmark_data); i++) {
        MY_CHECK_CLOSE(input[i].real(), output[i].real(), float(1. / (1 << 16)));
        MY_CHECK_CLOSE(input[i].imag(), output[i].imag(), float(1. / (1 << 16)));
    }
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_fc32_and_fc32)
{
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    // try various lengths to test edge cases
    id.output_format = "fc32_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_fc32(nsamps, id, conv_prio_type);
    }

    // try various lengths to test edge cases
    id.output_format = "fc32_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_fc32(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_fc32_and_fc32)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs   = 1;
    id.num_outputs  = 1;

    id.output_format = "fc32_item32_le";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_fc32(nsamps, id, prio, benchmarks);
        });

    id.output_format = "fc32_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_fc32(nsamps, id, prio, benchmarks);
        });
}

MULTI_CONVERTER_TEST_CASE(test_convert_types_fc32_and_fc32_chdr)
{
    convert::id_type id;
    id.input_format  = "fc32";
    id.output_format = "fc32_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    // try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++) {
        test_convert_types_fc32(nsamps, id, conv_prio_type);
    }
}

BOOST_TEST_DECORATOR(*boost::unit_test::disabled())
MULTI_CONVERTER_TEST_CASE(benchmark_convert_types_fc32_and_fc32_chdr)
{
    SKIP_BENCHMARK_CHECK;

    convert::id_type id;
    id.input_format  = "fc32";
    id.output_format = "fc32_chdr";
    id.num_inputs    = 1;
    id.num_outputs   = 1;

    id.output_format = "fc32_item32_be";
    benchmark_converter(id,
        conv_prio_type,
        [](size_t nsamps,
            convert::id_type id,
            uhd::convert::priority_type prio,
            std::vector<benchmark_result>* benchmarks) {
            test_convert_types_fc32(nsamps, id, prio, benchmarks);
        });
}
