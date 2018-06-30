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

typedef uint32_t (*towire32_type)(uint32_t);

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

enum item32_sc12_3x_enable {
    CONVERT12_LINE0 = 0x01,
    CONVERT12_LINE1 = 0x02,
    CONVERT12_LINE2 = 0x04,
    CONVERT12_LINE_ALL = 0x07,
};

/*
 * Packed 12-bit converter with selective line enable
 *
 * The converter operates on 4 complex inputs and selectively writes to one to
 * three 32-bit lines. Line selection allows for partial writes of less than
 * 4 complex samples, or a full 3 x 32-bit struct. Writes are always full 32-bit
 * lines, so in the case of partial writes, the number of bytes written will
 * exceed the the number of bytes filled by actual samples.
 *
 *  _ _ _ _ _ _ _ _
 * |_ _ _1_ _ _|_ _| 0
 * |_2_ _ _|_ _ _3_|
 * |_ _|_ _ _4_ _ _| 2
 * 31              0
 */
template <towire32_type towire>
void pack(item32_sc12_3x &output, int enable, const int32_t iq[8])
{
    if (enable & CONVERT12_LINE0)
        output.line0 = towire(iq[0] << 20 | iq[1] <<  8 | iq[2] >> 4);
    if (enable & CONVERT12_LINE1)
        output.line1 = towire(iq[2] << 28 | iq[3] << 16 | iq[4] << 4 | iq[5] >> 8);
    if (enable & CONVERT12_LINE2)
        output.line2 = towire(iq[5] << 24 | iq[6] << 12 | iq[7] << 0);
}

template <typename type, towire32_type towire>
void convert_star_4_to_sc12_item32_3
(
    const std::complex<type> &in0,
    const std::complex<type> &in1,
    const std::complex<type> &in2,
    const std::complex<type> &in3,
    const int enable,
    item32_sc12_3x &output,
    const double scalar,
    typename std::enable_if<std::is_floating_point<type>::value>::type* = NULL
)
{
    int32_t iq[8] {
        int32_t(in0.real()*scalar) & 0xfff,
        int32_t(in0.imag()*scalar) & 0xfff,
        int32_t(in1.real()*scalar) & 0xfff,
        int32_t(in1.imag()*scalar) & 0xfff,

        int32_t(in2.real()*scalar) & 0xfff,
        int32_t(in2.imag()*scalar) & 0xfff,
        int32_t(in3.real()*scalar) & 0xfff,
        int32_t(in3.imag()*scalar) & 0xfff,
    };
    pack<towire>(output, enable, iq);
}

template <typename type, towire32_type towire>
void convert_star_4_to_sc12_item32_3
(
    const std::complex<type> &in0,
    const std::complex<type> &in1,
    const std::complex<type> &in2,
    const std::complex<type> &in3,
    const int enable,
    item32_sc12_3x &output,
    const double,
    typename std::enable_if<std::is_same<type, short>::value>::type* = NULL
)
{
    int32_t iq[8] {
        int32_t(in0.real() >> 4) & 0xfff,
        int32_t(in0.imag() >> 4) & 0xfff,
        int32_t(in1.real() >> 4) & 0xfff,
        int32_t(in1.imag() >> 4) & 0xfff,

        int32_t(in2.real() >> 4) & 0xfff,
        int32_t(in2.imag() >> 4) & 0xfff,
        int32_t(in3.real() >> 4) & 0xfff,
        int32_t(in3.imag() >> 4) & 0xfff,
    };
    pack<towire>(output, enable, iq);
}
