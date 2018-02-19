//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_pack_sc12.hpp"

using namespace uhd::convert;

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

static converter::sptr make_convert_sc16_1_to_sc12_item32_le_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_1<short, uhd::wtohx>());
}

static converter::sptr make_convert_sc16_1_to_sc12_item32_be_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_1<short, uhd::ntohx>());
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

    id.input_format = "sc16";
    id.output_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc16_1_to_sc12_item32_le_1, PRIORITY_GENERAL);
    id.output_format = "sc12_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc16_1_to_sc12_item32_be_1, PRIORITY_GENERAL);
}
