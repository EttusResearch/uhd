//
// Copyright 2013 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/math/special_functions/round.hpp>
#include <vector>

using namespace uhd::convert;

typedef boost::uint32_t (*towire32_type)(boost::uint32_t);

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
template <typename type, towire32_type towire>
void convert_star_4_to_sc12_item32_3
(
    const std::complex<type> &in0,
    const std::complex<type> &in1,
    const std::complex<type> &in2,
    const std::complex<type> &in3,
    const int enable,
    item32_sc12_3x &output,
    const double scalar
)
{
    const item32_t i0 = boost::int32_t(type(in0.real()*scalar)) & 0xfff;
    const item32_t q0 = boost::int32_t(type(in0.imag()*scalar)) & 0xfff;

    const item32_t i1 = boost::int32_t(type(in1.real()*scalar)) & 0xfff;
    const item32_t q1 = boost::int32_t(type(in1.imag()*scalar)) & 0xfff;

    const item32_t i2 = boost::int32_t(type(in2.real()*scalar)) & 0xfff;
    const item32_t q2 = boost::int32_t(type(in2.imag()*scalar)) & 0xfff;

    const item32_t i3 = boost::int32_t(type(in3.real()*scalar)) & 0xfff;
    const item32_t q3 = boost::int32_t(type(in3.imag()*scalar)) & 0xfff;

    const item32_t line0 = (i0 << 20) | (q0 << 8) | (i1 >> 4);
    const item32_t line1 = (i1 << 28) | (q1 << 16) | (i2 << 4) | (q2 >> 8);
    const item32_t line2 = (q2 << 24) | (i3 << 12) | (q3);

    if (enable & CONVERT12_LINE0)
        output.line0 = towire(line0);
    if (enable & CONVERT12_LINE1)
        output.line1 = towire(line1);
    if (enable & CONVERT12_LINE2)
        output.line2 = towire(line2);
}

template <typename type, towire32_type towire>
struct convert_star_1_to_sc12_item32_1 : public converter
{
    convert_star_1_to_sc12_item32_1(void):_scalar(0.0)
    {
        //NOP
    }

    void set_scalar(const double scalar)
    {
        _scalar = scalar;
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        const std::complex<type> *input = reinterpret_cast<const std::complex<type> *>(inputs[0]);

        /*
         * Effectively outputs will point to a managed_buffer instance. These buffers are 32 bit aligned.
         * For a detailed description see comments in 'convert_unpack_sc12.cpp'.
         */
        const size_t head_samps = size_t(outputs[0]) & 0x3;
        int enable;
        size_t rewind = 0;
        switch(head_samps)
        {
            case 0: break;
            case 1: rewind = 9; break;
            case 2: rewind = 6; break;
            case 3: rewind = 3; break;
        }
        item32_sc12_3x *output = reinterpret_cast<item32_sc12_3x *>(size_t(outputs[0]) - rewind);

        //helper variables
        size_t i = 0, o = 0;

        //handle the head case
        switch (head_samps)
        {
        case 0:
            break; //no head
        case 1:
            enable = CONVERT12_LINE2;
            convert_star_4_to_sc12_item32_3<type, towire>(0, 0, 0, input[0], enable, output[o++], _scalar);
            break;
        case 2:
            enable = CONVERT12_LINE2 | CONVERT12_LINE1;
            convert_star_4_to_sc12_item32_3<type, towire>(0, 0, input[0], input[1], enable, output[o++], _scalar);
            break;
        case 3:
            enable = CONVERT12_LINE2 | CONVERT12_LINE1 | CONVERT12_LINE0;
            convert_star_4_to_sc12_item32_3<type, towire>(0, input[0], input[1], input[2], enable, output[o++], _scalar);
            break;
        }
        i += head_samps;

        //convert the body
        while (i+3 < nsamps)
        {
            convert_star_4_to_sc12_item32_3<type, towire>(input[i+0], input[i+1], input[i+2], input[i+3], CONVERT12_LINE_ALL, output[o], _scalar);
            o++; i += 4;
        }

        //handle the tail case
        const size_t tail_samps = nsamps - i;
        switch (tail_samps)
        {
        case 0:
            break; //no tail
        case 1:
            enable = CONVERT12_LINE0;
            convert_star_4_to_sc12_item32_3<type, towire>(input[i+0], 0, 0, 0, enable, output[o], _scalar);
            break;
        case 2:
            enable = CONVERT12_LINE0 | CONVERT12_LINE1;
            convert_star_4_to_sc12_item32_3<type, towire>(input[i+0], input[i+1], 0, 0, enable, output[o], _scalar);
            break;
        case 3:
            enable = CONVERT12_LINE0 | CONVERT12_LINE1 | CONVERT12_LINE2;
            convert_star_4_to_sc12_item32_3<type, towire>(input[i+0], input[i+1], input[i+2], 0, enable, output[o], _scalar);
            break;
        }
    }

    double _scalar;
};

static converter::sptr make_convert_fc32_1_to_sc12_item32_le_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_1<float, uhd::wtohx>());
}

static converter::sptr make_convert_fc32_1_to_sc12_item32_be_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_1<float, uhd::ntohx>());
}

UHD_STATIC_BLOCK(register_convert_pack_sc12)
{
    //uhd::convert::register_bytes_per_item("sc12", 3/*bytes*/); //registered in unpack

    uhd::convert::id_type id;
    id.num_inputs = 1;
    id.num_outputs = 1;
    id.input_format = "fc32";

    id.output_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_fc32_1_to_sc12_item32_le_1, PRIORITY_GENERAL);

    id.output_format = "sc12_item32_be";
    uhd::convert::register_converter(id, &make_convert_fc32_1_to_sc12_item32_be_1, PRIORITY_GENERAL);
}
