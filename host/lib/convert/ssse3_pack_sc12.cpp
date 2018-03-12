//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <tmmintrin.h>
#include "convert_pack_sc12.hpp"

/*
 * Shuffle Orderings - Single 128-bit SSE register
 *
 *   16-bit interleaved I/Q
 *  ---------------------------------------
 * | Q3 | I3 | Q2 | I2 | Q1 | I1 | Q0 | I0 | Input
 *  ---------------------------------------
 * | 127                                 0 |
 *
 *
 *   12-bit deinterleaved unpacked I/Q
 *  ---------------------------------------
 * | I3 | I2 | I1 | I0 | Q3 | Q2 | Q1 | Q0 | Shuffle-1
 *  ---------------------------------------
 * | High bit aligned  |  4-bit >> offset  |
 *
 *
 *   12-bit interleaved packed I/Q
 *  ---------------------------------------
 * |I0|Q0|I1|Q1|I2|Q2|I3|Q3|               | Shuffle-2 | Shuffle-3
 *  ---------------------------------------
 * | 127                32 | 31  Empty   0 |
 *
 *
 *     12-bit packed I/Q byteswapped
 *      -----------------------
 *     |   I0   |   Q0   |  I1 | 0
 *     |-----------------------|
 *     | I1 |  Q1  |  I2  | Q2 |             Output
 *     |-----------------------|
 *     | Q2  |   I3   |   Q3   |
 *     |-----------------------|
 *     |        Unused         | 3
 *      -----------------------
 *     31                     0
 */
#define SC12_SHIFT_MASK      0xfff0fff0, 0xfff0fff0, 0x0fff0fff, 0x0fff0fff
#define SC12_PACK_SHUFFLE1   13,12,9,8,5,4,1,0,15,14,11,10,7,6,3,2
#define SC12_PACK_SHUFFLE2   9,8,0,11,10,2,13,12,4,15,14,6,0,0,0,0
#define SC12_PACK_SHUFFLE3   8,1,8,8,3,8,8,5,8,8,7,8,8,8,8,8

template <typename type>
inline void convert_star_4_to_sc12_item32_3
(
    const std::complex<type> *in,
    item32_sc12_3x &output,
    const double scalar,
    typename std::enable_if<std::is_same<type, float>::value>::type* = NULL
)
{
    __m128 m0, m1, m2;
    m0 = _mm_set1_ps(scalar);
    m1 = _mm_loadu_ps((const float *) &in[0]);
    m2 = _mm_loadu_ps((const float *) &in[2]);
    m1 = _mm_mul_ps(m1, m0);
    m2 = _mm_mul_ps(m2, m0);
    m0 = _mm_shuffle_ps(m1, m2, _MM_SHUFFLE(2, 0, 2, 0));
    m1 = _mm_shuffle_ps(m1, m2, _MM_SHUFFLE(3, 1, 3, 1));

    __m128i m3, m4, m5, m6, m7;
    m3 = _mm_set_epi32(SC12_SHIFT_MASK);
    m4 = _mm_set_epi8(SC12_PACK_SHUFFLE2);
    m5 = _mm_set_epi8(SC12_PACK_SHUFFLE3);

    m6 = _mm_cvtps_epi32(m0);
    m7 = _mm_cvtps_epi32(m1);
    m6 = _mm_slli_epi32(m6, 4);
    m6 = _mm_packs_epi32(m7, m6);
    m6 = _mm_and_si128(m6, m3);
    m7 = _mm_move_epi64(m6);

    m6 = _mm_shuffle_epi8(m6, m4);
    m7 = _mm_shuffle_epi8(m7, m5);
    m6 = _mm_or_si128(m6, m7);

    m6 = _mm_shuffle_epi32(m6, _MM_SHUFFLE(0, 1, 2, 3));
    _mm_storeu_si128((__m128i*) &output, m6);
}

template <typename type>
static void convert_star_4_to_sc12_item32_3
(
    const std::complex<type> *in,
    item32_sc12_3x &output,
    const double,
    typename std::enable_if<std::is_same<type, short>::value>::type* = NULL
)
{
    __m128i m0, m1, m2, m3, m4, m5;
    m0 = _mm_set_epi32(SC12_SHIFT_MASK);
    m1 = _mm_set_epi8(SC12_PACK_SHUFFLE1);
    m2 = _mm_set_epi8(SC12_PACK_SHUFFLE2);
    m3 = _mm_set_epi8(SC12_PACK_SHUFFLE3);

    m4 = _mm_loadu_si128((__m128i*) in);
    m4 = _mm_shuffle_epi8(m4, m1);
    m5 = _mm_srli_epi16(m4, 4);
    m4 = _mm_shuffle_epi32(m4, _MM_SHUFFLE(0, 0, 3, 2));
    m4 = _mm_unpacklo_epi64(m5, m4);

    m4 = _mm_and_si128(m4, m0);
    m5 = _mm_move_epi64(m4);
    m4 = _mm_shuffle_epi8(m4, m2);
    m5 = _mm_shuffle_epi8(m5, m3);
    m3 = _mm_or_si128(m4, m5);

    m3 = _mm_shuffle_epi32(m3, _MM_SHUFFLE(0, 1, 2, 3));
    _mm_storeu_si128((__m128i*) &output, m3);
}

template <typename type, towire32_type towire>
struct convert_star_1_to_sc12_item32_2 : public converter
{
    convert_star_1_to_sc12_item32_2(void):_scalar(0.0)
    {
    }

    void set_scalar(const double scalar)
    {
        _scalar = scalar;
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        const std::complex<type> *input = reinterpret_cast<const std::complex<type> *>(inputs[0]);

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

        // SSE packed write output is 16 bytes which overwrites the 12-bit
        // packed struct by 4 bytes. There is no concern if there are
        // subsequent samples to be converted (writes will simply happen
        // twice). So set the conversion loop to force a tail case on the
        // final 4 or fewer samples.
        while (i+4 < nsamps)
        {
            convert_star_4_to_sc12_item32_3<type>(&input[i], output[o], _scalar);
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
        case 4:
            enable = CONVERT12_LINE_ALL;
            convert_star_4_to_sc12_item32_3<type, towire>(input[i+0], input[i+1], input[i+2], input[i+3], enable, output[o], _scalar);
            break;
        }
    }

    double _scalar;
};

static converter::sptr make_convert_fc32_1_to_sc12_item32_le_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_2<float, uhd::wtohx>());
}

static converter::sptr make_convert_sc16_1_to_sc12_item32_le_1(void)
{
    return converter::sptr(new convert_star_1_to_sc12_item32_2<short, uhd::wtohx>());
}

UHD_STATIC_BLOCK(register_sse_pack_sc12)
{
    uhd::convert::id_type id;
    id.num_inputs = 1;
    id.num_outputs = 1;

    id.input_format = "fc32";
    id.output_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_fc32_1_to_sc12_item32_le_1, PRIORITY_SIMD);

    id.input_format = "sc16";
    id.output_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc16_1_to_sc12_item32_le_1, PRIORITY_SIMD);
}
