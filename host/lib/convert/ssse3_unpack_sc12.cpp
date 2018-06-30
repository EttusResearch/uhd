//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_unpack_sc12.hpp"
#include <emmintrin.h>
#include <tmmintrin.h>

using namespace uhd::convert;

/*
 * Shuffle Orderings - Single 128-bit SSE register
 *
 *     12-bit packed I/Q byteswapped
 *      -----------------------
 *     |   I0   |   Q0   |  I1 | 0
 *     |-----------------------|
 *     | I1 |  Q1  |  I2  | Q2 |             Input
 *     |-----------------------|
 *     | Q2  |   I3   |   Q3   | 2
 *      -----------------------
 *     31                     0
 *
 *
 *   12-bit interleaved packed I/Q
 *  ---------------------------------------
 * |I0|Q0|I1|Q1|I2|Q2|I3|Q3|               | Byteswap Removed
 *  ---------------------------------------
 * | 127                32 | 31  Empty   0 |
 *
 *
 *           Packed   Unpacked
 *  Sample    Index    Index   Offset
 * =====================================
 *    I0      15,14     0,1      0
 *    Q0      14,13     8,9      4
 *    I1      12,11     2,3      0
 *    Q1      11,10    10,11     4           12-bit Indices
 *    I2       9,8      4,5      0
 *    Q2       8,7     12,13     4
 *    I3       6,5      6,7      0
 *    Q3       5,4     14,15     4
 *
 *
 *   12-bit deinterleaved unpacked I/Q
 *  ---------------------------------------
 * | Q3 | Q2 | Q1 | Q0 | I3 | I2 | I1 | I0 | Shuffle-1
 *  ---------------------------------------
 * |  4-bit >> offset  | High bit aligned  |
 *
 *
 *   16-bit interleaved I/Q
 *  ---------------------------------------
 * | Q3 | I3 | Q2 | I2 | Q1 | I1 | Q0 | I0 | Output (Shuffle-2)
 *  ---------------------------------------
 * | 127                                 0 |
 *
 */
#define SC12_SHIFT_MASK      0x0fff0fff, 0x0fff0fff, 0xfff0fff0, 0xfff0fff0
#define SC12_PACK_SHUFFLE1   5,4,8,7,11,10,14,13,6,5,9,8,12,11,15,14
#define SC12_PACK_SHUFFLE2   15,14,7,6,13,12,5,4,11,10,3,2,9,8,1,0

template <typename type, tohost32_type tohost>
inline void convert_sc12_item32_3_to_star_4
(
    const item32_sc12_3x &input,
    std::complex<type> *out,
    double scalar,
    typename std::enable_if<std::is_same<type, float>::value>::type* = NULL
)
{
    __m128i m0, m1, m2, m3, m4;
    m0 = _mm_set_epi32(SC12_SHIFT_MASK);
    m1 = _mm_set_epi8(SC12_PACK_SHUFFLE1);
    m2 = _mm_loadu_si128((__m128i*) &input);
    m2 = _mm_shuffle_epi32(m2, _MM_SHUFFLE(0, 1, 2, 3));
    m3 = _mm_shuffle_epi8(m2, m1);
    m3 = _mm_and_si128(m3, m0);

    m4 = _mm_setzero_si128();
    m1 = _mm_unpacklo_epi16(m4, m3);
    m2 = _mm_unpackhi_epi16(m4, m3);
    m2 = _mm_slli_epi32(m2, 4);
    m3 = _mm_unpacklo_epi32(m1, m2);
    m4 = _mm_unpackhi_epi32(m1, m2);

    __m128 m5, m6, m7;
    m5 = _mm_set_ps1(scalar/(1 << 16));
    m6 = _mm_cvtepi32_ps(m3);
    m7 = _mm_cvtepi32_ps(m4);
    m6 = _mm_mul_ps(m6, m5);
    m7 = _mm_mul_ps(m7, m5);

    _mm_storeu_ps(reinterpret_cast<float*>(&out[0]), m6);
    _mm_storeu_ps(reinterpret_cast<float*>(&out[2]), m7);
}

template <typename type, tohost32_type tohost>
inline void convert_sc12_item32_3_to_star_4
(
    const item32_sc12_3x &input,
    std::complex<type> *out,
    double,
    typename std::enable_if<std::is_same<type, short>::value>::type* = NULL
)
{
    __m128i m0, m1, m2, m3;
    m0 = _mm_set_epi32(SC12_SHIFT_MASK);
    m1 = _mm_set_epi8(SC12_PACK_SHUFFLE1);
    m2 = _mm_set_epi8(SC12_PACK_SHUFFLE2);

    m3 = _mm_loadu_si128((__m128i*) &input);
    m3 = _mm_shuffle_epi32(m3, _MM_SHUFFLE(0, 1, 2, 3));
    m3 = _mm_shuffle_epi8(m3, m1);
    m3 = _mm_and_si128(m3, m0);

    m0 = _mm_slli_epi16(m3, 4);
    m1 = _mm_shuffle_epi32(m3, _MM_SHUFFLE(1, 0, 0, 0));
    m0 = _mm_unpackhi_epi64(m1, m0);
    m1 = _mm_shuffle_epi8(m0, m2);

    _mm_storeu_si128((__m128i*) out, m1);
}

template <typename type, tohost32_type tohost>
struct convert_sc12_item32_1_to_star_2 : public converter
{
    convert_sc12_item32_1_to_star_2(void):_scalar(0.0)
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
        const size_t head_samps = size_t(inputs[0]) & 0x3;
        size_t rewind = 0;
        switch(head_samps)
        {
            case 0: break;
            case 1: rewind = 9; break;
            case 2: rewind = 6; break;
            case 3: rewind = 3; break;
        }

        const item32_sc12_3x *input = reinterpret_cast<const item32_sc12_3x *>(size_t(inputs[0]) - rewind);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);
        std::complex<type> dummy;
        size_t i = 0, o = 0;
        switch (head_samps)
        {
        case 0: break; //no head
        case 1: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy, dummy, dummy, output[0], _scalar); break;
        case 2: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy, dummy, output[0], output[1], _scalar); break;
        case 3: convert_sc12_item32_3_to_star_4<type, tohost>(input[i++], dummy, output[0], output[1], output[2], _scalar); break;
        }
        o += head_samps;

        //convert the body
        while (o+3 < nsamps)
        {
           convert_sc12_item32_3_to_star_4<type, tohost>(input[i], &output[o], _scalar);
            i += 1; o += 4;
        }

        const size_t tail_samps = nsamps - o;
        switch (tail_samps)
        {
        case 0: break; //no tail
        case 1: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], dummy, dummy, dummy, _scalar); break;
        case 2: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], output[o+1], dummy, dummy, _scalar); break;
        case 3: convert_sc12_item32_3_to_star_4<type, tohost>(input[i], output[o+0], output[o+1], output[o+2], dummy, _scalar); break;
        }
    }

    double _scalar;
};

static converter::sptr make_convert_sc12_item32_le_1_to_fc32_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_2<float, uhd::wtohx>());
}

static converter::sptr make_convert_sc12_item32_le_1_to_sc16_1(void)
{
    return converter::sptr(new convert_sc12_item32_1_to_star_2<short, uhd::wtohx>());
}

UHD_STATIC_BLOCK(register_sse_unpack_sc12)
{
    uhd::convert::id_type id;
    id.num_inputs = 1;
    id.num_outputs = 1;
    id.output_format = "fc32";
    id.input_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_le_1_to_fc32_1, PRIORITY_SIMD);

    id.output_format = "sc16";
    id.input_format = "sc12_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc12_item32_le_1_to_sc16_1, PRIORITY_SIMD);
}
