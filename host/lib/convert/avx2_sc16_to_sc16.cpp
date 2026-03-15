//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER(sc16, 1, sc16_item32_le, 1, PRIORITY_SIMD_AVX2)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        __m256i m0;

        /* load from input */
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));

        /* swap 16-bit pairs */
        m0 = _mm256_shufflelo_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));
        m0 = _mm256_shufflehi_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));

        /* store to output */
        _mm256_storeu_si256((__m256i*)(output + i), m0);
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htowx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16, 1, sc16_item32_be, 1, PRIORITY_SIMD_AVX2)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        __m256i m0, m1, m2;

        /* load from input */
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));

        /* byteswap 16 bit words */
        m1 = _mm256_srli_epi16(m0, 8);
        m2 = _mm256_slli_epi16(m0, 8);
        m0 = _mm256_or_si256(m1, m2);

        /* store to output */
        _mm256_storeu_si256((__m256i*)(output + i), m0);
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htonx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_le, 1, sc16, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    sc16_t* output        = reinterpret_cast<sc16_t*>(outputs[0]);

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        __m256i m0;

        /* load from input */
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));

        /* swap 16-bit pairs */
        m0 = _mm256_shufflelo_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));
        m0 = _mm256_shufflehi_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));

        /* store to output */
        _mm256_storeu_si256((__m256i*)(output + i), m0);
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htowx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_be, 1, sc16, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    sc16_t* output        = reinterpret_cast<sc16_t*>(outputs[0]);

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        __m256i m0, m1, m2;

        /* load from input */
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));

        /* byteswap 16 bit words */
        m1 = _mm256_srli_epi16(m0, 8);
        m2 = _mm256_slli_epi16(m0, 8);
        m0 = _mm256_or_si256(m1, m2);

        /* store to output */
        _mm256_storeu_si256((__m256i*)(output + i), m0);
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htonx>(input + i, output + i, nsamps - i, 1.0);
}
