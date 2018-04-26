//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <complex>
#include <vector>
#include <cstdlib>
#include <iostream>

using namespace uhd;

//typedefs for complex types
typedef std::complex<int16_t> sc16_t;
typedef std::complex<float> fc32_t;
typedef std::complex<double> fc64_t;

#define MY_CHECK_CLOSE(a, b, f) { \
    BOOST_CHECK_MESSAGE(std::abs((a)-(b)) < f, "\n\t" << #a << " (" << (a) << ") error " << #b << " (" << (b) << ")"); \
}

/***********************************************************************
 * Loopback runner:
 *    convert input buffer into intermediate buffer
 *    convert intermediate buffer into output buffer
 **********************************************************************/
template <typename Range> static void loopback(
    size_t nsamps,
    convert::id_type &in_id,
    convert::id_type &out_id,
    const Range &input,
    Range &output,
    const int prio_in = -1,
    const int prio_out = -1
){
    //make this buffer large enough for all test types
    std::vector<uint64_t> interm(nsamps);

    std::vector<const void *> input0(1, &input[0]), input1(1, &interm[0]);
    std::vector<void *> output0(1, &interm[0]), output1(1, &output[0]);

    //convert to intermediate type
    convert::converter::sptr c0 = convert::get_converter(in_id, prio_in)();
    c0->set_scalar(32767.);
    c0->conv(input0, output0, nsamps);

    //convert back to host type
    convert::converter::sptr c1 = convert::get_converter(out_id, prio_out)();
    c1->set_scalar(1/32767.);
    c1->conv(input1, output1, nsamps);
}

/***********************************************************************
 * Test short conversion
 **********************************************************************/
static void test_convert_types_sc16(
    size_t nsamps, convert::id_type &id, const int extra_div = 1, int mask = 0xffff
){
    //fill the input samples
    std::vector<sc16_t> input(nsamps), output(nsamps);
    for(sc16_t &in:  input) in = sc16_t(
        short((float((std::rand())/(double(RAND_MAX)/2)) - 1)*32767/extra_div) & mask,
        short((float((std::rand())/(double(RAND_MAX)/2)) - 1)*32767/extra_div) & mask
    );

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    BOOST_CHECK_EQUAL_COLLECTIONS(input.begin(), input.end(), output.begin(), output.end());
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_sc16){
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_sc16){
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id);
    }
}

/***********************************************************************
 * Test float conversion
 **********************************************************************/
template <typename data_type>
static void test_convert_types_for_floats(
    size_t nsamps, convert::id_type &id, const double extra_scale = 1.0
){
    typedef typename data_type::value_type value_type;

    //fill the input samples
    std::vector<data_type> input(nsamps), output(nsamps);
    for(data_type &in:  input) in = data_type(
        ((std::rand()/(value_type(RAND_MAX)/2)) - 1)*float(extra_scale),
        ((std::rand()/(value_type(RAND_MAX)/2)) - 1)*float(extra_scale)
    );

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);

    //make a list of all prio: best/generic combos
    typedef std::pair<int, int> int_pair_t;
    const std::vector<int_pair_t> prios{
        int_pair_t(0, 0),
        int_pair_t(-1, 0),
        int_pair_t(0, -1),
        int_pair_t(-1, -1)
    };

    //loopback foreach prio combo (generic vs best)
    for (const auto &prio : prios) {
        loopback(nsamps, in_id, out_id, input, output, prio.first, prio.second);
        for (size_t i = 0; i < nsamps; i++){
            MY_CHECK_CLOSE(input[i].real(), output[i].real(), value_type(1./(1 << 14)));
            MY_CHECK_CLOSE(input[i].imag(), output[i].imag(), value_type(1./(1 << 14)));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc64){
    convert::id_type id;
    id.input_format = "fc64";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_be";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc64_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc64){
    convert::id_type id;
    id.input_format = "fc64";
    id.num_inputs = 1;
    id.output_format = "sc16_item32_le";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc64_t>(nsamps, id);
    }
}

/***********************************************************************
 * Test float to/from sc12 conversion loopback
 **********************************************************************/

BOOST_AUTO_TEST_CASE(test_convert_types_le_sc12_with_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc12_item32_le";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1./16);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_sc12_with_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "sc12_item32_be";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1./16);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_le_sc16_and_sc12){
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "sc12_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id, 1, 0xfff0);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_sc16_and_sc12){
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs = 1;
    id.num_outputs = 1;

    id.output_format = "sc12_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id, 1, 0xfff0);
    }
}

/***********************************************************************
 * Test float to/from fc32 conversion loopback
 **********************************************************************/

BOOST_AUTO_TEST_CASE(test_convert_types_le_fc32_with_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "fc32_item32_le";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_be_fc32_with_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.output_format = "fc32_item32_be";
    id.num_outputs = 1;

    //try various lengths to test edge cases
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id);
    }
}

/***********************************************************************
 * Test float to short conversion loopback
 **********************************************************************/
BOOST_AUTO_TEST_CASE(test_convert_types_fc32_to_sc16){
    convert::id_type in_id;
    in_id.input_format = "fc32";
    in_id.num_inputs = 1;
    in_id.output_format = "sc16_item32_le";
    in_id.num_outputs = 1;

    convert::id_type out_id;
    out_id.input_format = "sc16_item32_le";
    out_id.num_inputs = 1;
    out_id.output_format = "sc16";
    out_id.num_outputs = 1;

    const size_t nsamps = 13;
    std::vector<fc32_t> input(nsamps);
    for(fc32_t &in:  input) in = fc32_t(
        (std::rand()/(RAND_MAX/2.0)) - 1,
        (std::rand()/(RAND_MAX/2.0)) - 1
    );
    std::vector<uint32_t> interm(nsamps);
    std::vector<sc16_t> output(nsamps);

    std::vector<const void *> input0(1, &input[0]), input1(1, &interm[0]);
    std::vector<void *> output0(1, &interm[0]), output1(1, &output[0]);

    //convert float to intermediate
    convert::converter::sptr c0 = convert::get_converter(in_id)();
    c0->set_scalar(32767.);
    c0->conv(input0, output0, nsamps);

    //convert intermediate to short
    convert::converter::sptr c1 = convert::get_converter(out_id)();
    c1->set_scalar(1/32767.);
    c1->conv(input1, output1, nsamps);

    //test that the inputs and outputs match
    for (size_t i = 0; i < nsamps; i++){
        MY_CHECK_CLOSE(input[i].real(), output[i].real()/float(32767), float(0.01));
        MY_CHECK_CLOSE(input[i].imag(), output[i].imag()/float(32767), float(0.01));
    }
}

/***********************************************************************
 * Test short to float conversion loopback
 **********************************************************************/
BOOST_AUTO_TEST_CASE(test_convert_types_sc16_to_fc32){
    convert::id_type in_id;
    in_id.input_format = "sc16";
    in_id.num_inputs = 1;
    in_id.output_format = "sc16_item32_le";
    in_id.num_outputs = 1;

    convert::id_type out_id;
    out_id.input_format = "sc16_item32_le";
    out_id.num_inputs = 1;
    out_id.output_format = "fc32";
    out_id.num_outputs = 1;

    const size_t nsamps = 13;
    std::vector<sc16_t> input(nsamps);
    for(sc16_t &in:  input) in = sc16_t(
        std::rand()-(RAND_MAX/2),
        std::rand()-(RAND_MAX/2)
    );
    std::vector<uint32_t> interm(nsamps);
    std::vector<fc32_t> output(nsamps);

    std::vector<const void *> input0(1, &input[0]), input1(1, &interm[0]);
    std::vector<void *> output0(1, &interm[0]), output1(1, &output[0]);

    //convert short to intermediate
    convert::converter::sptr c0 = convert::get_converter(in_id)();
    c0->set_scalar(32767.);
    c0->conv(input0, output0, nsamps);

    //convert intermediate to float
    convert::converter::sptr c1 = convert::get_converter(out_id)();
    c1->set_scalar(1/32767.);
    c1->conv(input1, output1, nsamps);

    //test that the inputs and outputs match
    for (size_t i = 0; i < nsamps; i++){
        MY_CHECK_CLOSE(input[i].real()/float(32767), output[i].real(), float(0.01));
        MY_CHECK_CLOSE(input[i].imag()/float(32767), output[i].imag(), float(0.01));
    }
}

/***********************************************************************
 * Test sc8 conversions
 **********************************************************************/
BOOST_AUTO_TEST_CASE(test_convert_types_fc64_and_sc8){
    convert::id_type id;
    id.input_format = "fc64";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc64_t>(nsamps, id, 1./256);
    }

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc64_t>(nsamps, id, 1./256);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_fc32_and_sc8){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1./256);
    }

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_for_floats<fc32_t>(nsamps, id, 1./256);
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_sc16_and_sc8){
    convert::id_type id;
    id.input_format = "sc16";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id, 256);
    }

    //try various lengths to test edge cases
    id.output_format = "sc8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_sc16(nsamps, id, 256);
    }
}

/***********************************************************************
 * Test u8 conversion
 **********************************************************************/
static void test_convert_types_u8(
    size_t nsamps, convert::id_type &id
){
    //fill the input samples
    std::vector<uint8_t> input(nsamps), output(nsamps);
    for(uint8_t &in:  input) in = uint8_t(std::rand() & 0xFF);
    //uint32_t d = 48;
    //for(uint8_t &in:  input) in = d++;

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    BOOST_CHECK_EQUAL_COLLECTIONS(input.begin(), input.end(), output.begin(), output.end());
}

BOOST_AUTO_TEST_CASE(test_convert_types_u8_and_u8){
    convert::id_type id;
    id.input_format = "u8";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "u8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_u8(nsamps, id);
    }

    //try various lengths to test edge cases
    id.output_format = "u8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_u8(nsamps, id);
    }
}

/***********************************************************************
 * Test s8 conversion
 **********************************************************************/
static void test_convert_types_s8(
    size_t nsamps, convert::id_type &id
){
    //fill the input samples
    std::vector<int8_t> input(nsamps), output(nsamps);
    for(int8_t &in:  input) in = int8_t(std::rand() & 0xFF);

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    BOOST_CHECK_EQUAL_COLLECTIONS(input.begin(), input.end(), output.begin(), output.end());
}

BOOST_AUTO_TEST_CASE(test_convert_types_s8_and_s8){
    convert::id_type id;
    id.input_format = "s8";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "s8_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_s8(nsamps, id);
    }

    //try various lengths to test edge cases
    id.output_format = "s8_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_s8(nsamps, id);
    }
}

/***********************************************************************
 * Test s16 conversion
 **********************************************************************/
static void test_convert_types_s16(
    size_t nsamps, convert::id_type &id
){
    //fill the input samples
    std::vector<int16_t> input(nsamps), output(nsamps);
    for(int16_t &in:  input) in = int16_t(std::rand() & 0xFFFF);

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    BOOST_CHECK_EQUAL_COLLECTIONS(input.begin(), input.end(), output.begin(), output.end());
}

BOOST_AUTO_TEST_CASE(test_convert_types_s16_and_s16){
    convert::id_type id;
    id.input_format = "s16";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "s16_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_s16(nsamps, id);
    }

    //try various lengths to test edge cases
    id.output_format = "s16_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_s16(nsamps, id);
    }
}

/***********************************************************************
 * Test fc32 -> fc32 conversion
 **********************************************************************/
static void test_convert_types_fc32(
    size_t nsamps, convert::id_type &id
){
    //fill the input samples
    std::vector< std::complex<float> > input(nsamps), output(nsamps);
    for(fc32_t &in:  input) in = fc32_t(
        (std::rand()/float(RAND_MAX/2)) - 1,
        (std::rand()/float(RAND_MAX/2)) - 1
    );

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    for (size_t i = 0; i < nsamps; i++){
        MY_CHECK_CLOSE(input[i].real(), output[i].real(), float(1./(1 << 16)));
        MY_CHECK_CLOSE(input[i].imag(), output[i].imag(), float(1./(1 << 16)));
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_fc32_and_fc32){
    convert::id_type id;
    id.input_format = "fc32";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "fc32_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_fc32(nsamps, id);
    }

    //try various lengths to test edge cases
    id.output_format = "fc32_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_fc32(nsamps, id);
    }
}

/***********************************************************************
 * Test f32 -> f32 conversion
 **********************************************************************/
static void test_convert_types_f32(
    size_t nsamps, convert::id_type &id
){
    //fill the input samples
    std::vector<float> input(nsamps), output(nsamps);
    for(float &in:  input) in = float((float(std::rand())/float(RAND_MAX/2)) - 1);

    //run the loopback and test
    convert::id_type in_id = id;
    convert::id_type out_id = id;
    std::swap(out_id.input_format, out_id.output_format);
    std::swap(out_id.num_inputs, out_id.num_outputs);
    loopback(nsamps, in_id, out_id, input, output);
    for (size_t i = 0; i < nsamps; i++){
        MY_CHECK_CLOSE(input[i], output[i], float(1./(1 << 16)));
    }
}

BOOST_AUTO_TEST_CASE(test_convert_types_f32_and_f32){
    convert::id_type id;
    id.input_format = "f32";
    id.num_inputs = 1;
    id.num_outputs = 1;

    //try various lengths to test edge cases
    id.output_format = "f32_item32_le";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_f32(nsamps, id);
    }

    //try various lengths to test edge cases
    id.output_format = "f32_item32_be";
    for (size_t nsamps = 1; nsamps < 16; nsamps++){
        test_convert_types_f32(nsamps, id);
    }
}
