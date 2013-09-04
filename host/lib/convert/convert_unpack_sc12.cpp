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

typedef boost::uint32_t (*tohost32_type)(boost::uint32_t);

struct item32_sc12_3x
{
    item32_t line0;
    item32_t line1;
    item32_t line2;
};

template <typename type, tohost32_type tohost>
void convert_sc12_item32_3_to_star_4
(
    const item32_sc12_3x &input,
    std::complex<type> &out0,
    std::complex<type> &out1,
    std::complex<type> &out2,
    std::complex<type> &out3,
    const double scalar
)
{
    //step 0: extract the lines from the input buffer
    const item32_t line0 = tohost(input.line0);
    const item32_t line1 = tohost(input.line1);
    const item32_t line2 = tohost(input.line2);
    const boost::uint64_t line01 = (boost::uint64_t(line0) << 32) | line1;
    const boost::uint64_t line12 = (boost::uint64_t(line1) << 32) | line2;

    //step 1: shift out and mask off the individual numbers
    const type i0 = type(boost::int16_t(line0 >> 16)*scalar);
    const type q0 = type(boost::int16_t(line0 >> 4)*scalar);

    const type i1 = type(boost::int16_t(line01 >> 24)*scalar);
    const type q1 = type(boost::int16_t(line1 >> 12)*scalar);

    const type i2 = type(boost::int16_t(line1 >> 0)*scalar);
    const type q2 = type(boost::int16_t(line12 >> 20)*scalar);

    const type i3 = type(boost::int16_t(line2 >> 8)*scalar);
    const type q3 = type(boost::int16_t(line2 << 4)*scalar);

    //step 2: load the outputs
    out0 = std::complex<type>(i0, q0);
    out1 = std::complex<type>(i1, q1);
    out2 = std::complex<type>(i2, q2);
    out3 = std::complex<type>(i3, q3);
}

template <typename type, tohost32_type tohost>
struct convert_sc12_item32_1_to_star_1 : public converter
{
    convert_sc12_item32_1_to_star_1(void)
    {
        //NOP
    }

    void set_scalar(const double scalar)
    {
        const int unpack_growth = 16;
        _scalar = scalar/unpack_growth;
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        const item32_sc12_3x *input = reinterpret_cast<const item32_sc12_3x *>(size_t(inputs[0]) & ~0x3);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        //helper variables
        std::complex<type> dummy0, dummy1, dummy2;
        size_t i = 0, o = 0;

        //handle the head case
        const size_t head_samps = size_t(inputs[0]) & 0x3;
        switch (head_samps)
        {
        case 0: break; //no head
        case 1: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy0, dummy1, dummy2, output[0], _scalar); break;
        case 2: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy0, dummy1, output[0], output[1], _scalar); break;
        case 3: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy0, output[0], output[1], output[2], _scalar); break;
        }
        o += head_samps;

        //convert the body
        while (o+3 < nsamps)
        {
            convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], output[o+1], output[o+2], output[o+3], _scalar);
            i++; o += 4;
        }

        //handle the tail case
        const size_t tail_samps = nsamps - o;
        switch (tail_samps)
        {
        case 0: break; //no tail
        case 1: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], dummy0, dummy1, dummy2, _scalar); break;
        case 2: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], output[o+1], dummy1, dummy2, _scalar); break;
        case 3: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], output[o+1], output[o+2], dummy2, _scalar); break;
        }
    }

    double _scalar;
};

static converter::sptr make_convert_sc12_item32_le_1_to_fc32_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_1<float, uhd::wtohx>());
}

static converter::sptr make_convert_sc12_item32_be_1_to_fc32_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_1<float, uhd::ntohx>());
}

UHD_STATIC_BLOCK(register_convert_unpack_sc12)
{
    uhd::convert::register_bytes_per_item("sc12", 3/*bytes*/);

    uhd::convert::id_type id;
    id.num_inputs = 1;
    id.num_outputs = 1;
    id.output_format = "fc32";

    id.input_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_le_1_to_fc32_1, PRIORITY_GENERAL);

    id.input_format = "sc12_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_be_1_to_fc32_1, PRIORITY_GENERAL);
}
