//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <type_traits>
#include <uhd/utils/byteswap.hpp>
#include "convert_common.hpp"

using namespace uhd::convert;

typedef uint32_t (*tohost32_type)(uint32_t);

/* C language specification requires this to be packed
 * (i.e., line0, line1, line2 will be in adjacent memory locations).
 * If this was not true, we'd need compiler flags here to specify
 * alignment/packing.
 */
struct item32_sc12_3x
{
    item32_t line0;
    item32_t line1;
    item32_t line2;
};

/*
 * convert_sc12_item32_3_to_star_4 takes in 3 lines with 32 bit each
 * and converts them 4 samples of type 'std::complex<type>'.
 * The structure of the 3 lines is as follows:
 *  _ _ _ _ _ _ _ _
 * |_ _ _1_ _ _|_ _|
 * |_2_ _ _|_ _ _3_|
 * |_ _|_ _ _4_ _ _|
 *
 * The numbers mark the position of one complex sample.
 */
template <typename type, tohost32_type tohost>
void convert_sc12_item32_3_to_star_4
(
    const item32_sc12_3x &input,
    std::complex<type> &out0,
    std::complex<type> &out1,
    std::complex<type> &out2,
    std::complex<type> &out3,
    const double scalar,
    typename std::enable_if<std::is_floating_point<type>::value>::type* = NULL
)
{
    //step 0: extract the lines from the input buffer
    const item32_t line0 = tohost(input.line0);
    const item32_t line1 = tohost(input.line1);
    const item32_t line2 = tohost(input.line2);
    const uint64_t line01 = (uint64_t(line0) << 32) | line1;
    const uint64_t line12 = (uint64_t(line1) << 32) | line2;

    //step 1: shift out and mask off the individual numbers
    const type i0 = type(int16_t((line0 >> 16) & 0xfff0)*scalar);
    const type q0 = type(int16_t((line0 >> 4) & 0xfff0)*scalar);

    const type i1 = type(int16_t((line01 >> 24) & 0xfff0)*scalar);
    const type q1 = type(int16_t((line1 >> 12) & 0xfff0)*scalar);

    const type i2 = type(int16_t((line1 >> 0) & 0xfff0)*scalar);
    const type q2 = type(int16_t((line12 >> 20) & 0xfff0)*scalar);

    const type i3 = type(int16_t((line2 >> 8) & 0xfff0)*scalar);
    const type q3 = type(int16_t((line2 << 4) & 0xfff0)*scalar);

    //step 2: load the outputs
    out0 = std::complex<type>(i0, q0);
    out1 = std::complex<type>(i1, q1);
    out2 = std::complex<type>(i2, q2);
    out3 = std::complex<type>(i3, q3);
}

template <typename type, tohost32_type tohost>
void convert_sc12_item32_3_to_star_4
(
    const item32_sc12_3x &input,
    std::complex<type> &out0,
    std::complex<type> &out1,
    std::complex<type> &out2,
    std::complex<type> &out3,
    const double,
    typename std::enable_if<std::is_integral<type>::value>::type* = NULL
)
{
    //step 0: extract the lines from the input buffer
    const item32_t line0 = tohost(input.line0);
    const item32_t line1 = tohost(input.line1);
    const item32_t line2 = tohost(input.line2);
    const uint64_t line01 = (uint64_t(line0) << 32) | line1;
    const uint64_t line12 = (uint64_t(line1) << 32) | line2;

    //step 1: extract and load the outputs
    out0 = std::complex<type>(line0  >> 16 & 0xfff0, line0  >>  4 & 0xfff0);
    out1 = std::complex<type>(line01 >> 24 & 0xfff0, line1  >> 12 & 0xfff0);
    out2 = std::complex<type>(line1  >>  0 & 0xfff0, line12 >> 20 & 0xfff0);
    out3 = std::complex<type>(line2  >>  8 & 0xfff0, line2  <<  4 & 0xfff0);
}
