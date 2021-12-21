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
#include <list>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <array>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace uhd;

// typedefs for complex types
typedef std::complex<int16_t> sc16_t;
typedef std::complex<float> fc32_t;
typedef std::complex<double> fc64_t;

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

// Shorthand for defining a test case that tests all prios. Creates a variable
// 'conv_prio_type'
#define MULTI_CONVERTER_TEST_CASE(test_name) \
    BOOST_DATA_TEST_CASE(test_name, CONV_PRIO_TYPES, conv_prio_type)

#define MY_CHECK_CLOSE(a, b, f)                                                     \
    {                                                                               \
        static bool error_encountered = false;                                      \
        if(!error_encountered && (std::abs((a) - (b)) >= f)) {                      \
        BOOST_ERROR(                                                                \
            "\n\t" << #a << " (" << (a) << ") error " << #b << " (" << (b) << ")"); \
            error_encountered = true;                                               \
        }                                                                           \
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
 **********************************************************************/
template <typename Range>
static void loopback(size_t nsamps,
    convert::id_type& in_id,
    convert::id_type& out_id,
    const Range& input,
    Range& output,
    const int prio_in,
    const int prio_out)
{
    // make this buffer large enough for all test types
    std::vector<uint64_t> interm(nsamps);

    std::vector<const void*> input0{&input[0]}, input1{&interm[0]};
    std::vector<void*> output0{&interm[0]}, output1{&output[0]};

    // convert to intermediate type
    convert::converter::sptr c0 = convert::get_converter(in_id, prio_in)();
    c0->set_scalar(32767.);
    c0->conv(input0, output0, nsamps);

    // convert back to host type
    convert::converter::sptr c1 = convert::get_converter(out_id, prio_out)();
    c1->set_scalar(1 / 32767.);
    c1->conv(input1, output1, nsamps);
}

// Use this to call the loopback runner from a test so that missing prio won't
// become an issue
#define CALL_LOOPBACK_SAFE(...) \
    try {                       \
        loopback(__VA_ARGS__);  \
    } catch (uhd::key_error&) { \
        return;                 \
    }

/***********************************************************************
 * Test short conversion
 **********************************************************************/
static void test_convert_types_sc16(size_t nsamps,
    convert::id_type& id,
    uhd::convert::priority_type prio,
    const int extra_div = 1,
    int mask            = 0xffff)
{
    // fill the input samples
    std::vector<sc16_t> input(nsamps), output(nsamps);
    for (sc16_t& in : input)
    {
        in = sc16_t(
            short((float((std::rand()) / (double(RAND_MAX) / 2)) - 1) * 32767 / extra_div)
                & mask,
            short((float((std::rand()) / (double(RAND_MAX) / 2)) - 1) * 32767 / extra_div)
                & mask);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        input.begin(), input.end(), output.begin(), output.end());
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

/***********************************************************************
 * Test float conversion
 **********************************************************************/
template <typename data_type>
static void test_convert_types_for_floats(size_t nsamps,
    convert::id_type& id,
    const double extra_scale = 1.0)
{
    typedef typename data_type::value_type value_type;

    // fill the input samples
    std::vector<data_type> input(nsamps), output(nsamps);
    for (data_type& in : input)
    {
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
        CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio.first, prio.second);
        for (size_t i = 0; i < nsamps; i++) {
            MY_CHECK_CLOSE(input[i].real(), output[i].real(), value_type(1. / (1 << 14)));
            MY_CHECK_CLOSE(input[i].imag(), output[i].imag(), value_type(1. / (1 << 14)));
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

/***********************************************************************
 * Test u8 conversion
 **********************************************************************/
static void test_convert_types_u8(
    size_t nsamps, convert::id_type& id, uhd::convert::priority_type prio)
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
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        input.begin(), input.end(), output.begin(), output.end());
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

/***********************************************************************
 * Test s8 conversion
 **********************************************************************/
static void test_convert_types_s8(
    size_t nsamps, convert::id_type& id, uhd::convert::priority_type prio)
{
    // fill the input samples
    std::vector<int8_t> input(nsamps), output(nsamps);
    for (int8_t& in : input) {
        in = int8_t(std::rand() & 0xFF);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        input.begin(), input.end(), output.begin(), output.end());
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

/***********************************************************************
 * Test s16 conversion
 **********************************************************************/
static void test_convert_types_s16(
    size_t nsamps, convert::id_type& id, uhd::convert::priority_type prio)
{
    // fill the input samples
    std::vector<int16_t> input(nsamps), output(nsamps);
    for (int16_t& in : input) {
        in = int16_t(std::rand() & 0xFFFF);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        input.begin(), input.end(), output.begin(), output.end());
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

/***********************************************************************
 * Test fc32 -> fc32 conversion
 **********************************************************************/
static void test_convert_types_fc32(
    size_t nsamps, convert::id_type& id, uhd::convert::priority_type prio)
{
    // fill the input samples
    std::vector<std::complex<float>> input(nsamps), output(nsamps);
    for (fc32_t& in : input)
    {
        in = fc32_t((std::rand() / float(RAND_MAX / 2)) - 1,
            (std::rand() / float(RAND_MAX / 2)) - 1);
    }

    // run the loopback and test
    convert::id_type in_id  = id;
    convert::id_type out_id = reverse_converter(id);
    CALL_LOOPBACK_SAFE(nsamps, in_id, out_id, input, output, prio, prio);
    for (size_t i = 0; i < nsamps; i++) {
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
