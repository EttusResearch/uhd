//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_unpack_sc12.hpp"

using namespace uhd::convert;

template <typename type, tohost32_type tohost>
struct convert_sc12_item32_1_to_star_1 : public converter
{
    convert_sc12_item32_1_to_star_1(void):_scalar(0.0)
    {
        //NOP
    }

    void set_scalar(const double scalar)
    {
        const int unpack_growth = 16;
        _scalar = scalar/unpack_growth;
    }

    /*
     * This converter takes in 24 bits complex samples, 12 bits I and 12 bits Q, and converts them to type 'std::complex<type>'.
     * 'type' is usually 'float'.
     * For the converter to work correctly the used managed_buffer which holds all samples of one packet has to be 32 bits aligned.
     * We assume 32 bits to be one line. This said the converter must be aware where it is supposed to start within 3 lines.
     *
     */
    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        /*
         * Looking at the line structure above we can identify 4 cases.
         * Each corresponds to the start of a different sample within a 3 line block.
         * head_samps derives the number of samples left within one block.
         * Then the number of bytes the converter has to rewind are calculated.
         */
        const size_t head_samps = size_t(inputs[0]) & 0x3;
        size_t rewind = 0;
        switch(head_samps)
        {
            case 0: break;
            case 1: rewind = 9; break;
            case 2: rewind = 6; break;
            case 3: rewind = 3; break;
        }

        /*
         * The pointer *input now points to the head of a 3 line block.
         */
        const item32_sc12_3x *input = reinterpret_cast<const item32_sc12_3x *>(size_t(inputs[0]) - rewind);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        //helper variables
        std::complex<type> dummy0, dummy1, dummy2;
        size_t i = 0, o = 0;

        /*
         * handle the head case
         * head_samps holds the number of samples left in a block.
         * The 3 line converter is called for the whole block and already processed samples are dumped.
         * We don't run into the risk of a SIGSEGV because input will always point to valid memory within a managed_buffer.
         * Furthermore the bytes in a buffer remain unchanged after they have been copied into it.
         */
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

        /*
         * handle the tail case
         * The converter can be called with any number of samples to be converted.
         * This can end up in only a part of a block to be converted in one call.
         * We never have to worry about SIGSEGVs here as long as we end in the middle of a managed_buffer.
         * If we are at the end of managed_buffer there are 2 precautions to prevent SIGSEGVs.
         * Firstly only a read operation is performed.
         * Secondly managed_buffers allocate a fixed size memory which is always larger than the actually used size.
         * e.g. The current sample maximum is 2000 samples in a packet over USB.
         * With sc12 samples a packet consists of 6000kb but managed_buffers allocate 16kb each.
         * Thus we don't run into problems here either.
         */
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

static converter::sptr make_convert_sc12_item32_le_1_to_sc16_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_1<short, uhd::wtohx>());
}

static converter::sptr make_convert_sc12_item32_be_1_to_sc16_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_1<short, uhd::ntohx>());
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

    id.output_format = "sc16";
    id.input_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_le_1_to_sc16_1, PRIORITY_GENERAL);
    id.input_format = "sc12_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_be_1_to_sc16_1, PRIORITY_GENERAL);
}
