//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER(sc16_item32_le, 1, fc32, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    fc32_t* output        = reinterpret_cast<fc32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load from input */
        __m256i tmpi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));

        /* swap 16-bit pairs: [imag, real] -> [real, imag] */
        tmpi = _mm256_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
        tmpi = _mm256_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));

        /* split into 128-bit halves and sign-extend int16 to int32 */
        __m256i int32_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(tmpi));
        __m256i int32_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(tmpi, 1));

        /* convert to float and scale */
        __m256 tmplo = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_lo), scalar);
        __m256 tmphi = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_hi), scalar);

        /* store to output */
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 0), tmplo);
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 4), tmphi);
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htowx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_be, 1, fc32, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    fc32_t* output        = reinterpret_cast<fc32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load from input */
        __m256i tmpi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));

        /* byteswap within each 16-bit word */
        tmpi = _mm256_or_si256(_mm256_srli_epi16(tmpi, 8), _mm256_slli_epi16(tmpi, 8));

        /* split into 128-bit halves and sign-extend int16 to int32 */
        __m256i int32_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(tmpi));
        __m256i int32_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(tmpi, 1));

        /* convert to float and scale */
        __m256 tmplo = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_lo), scalar);
        __m256 tmphi = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_hi), scalar);

        /* store to output */
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 0), tmplo);
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 4), tmphi);
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htonx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER(sc16_chdr, 1, fc32, 1, PRIORITY_SIMD_AVX2)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    fc32_t* output      = reinterpret_cast<fc32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load 8 complex samples as 2x 128-bit halves */
        __m128i in_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
        __m128i in_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i + 4));

        /* sign-extend int16 to int32 */
        __m256i int32_lo = _mm256_cvtepi16_epi32(in_lo);
        __m256i int32_hi = _mm256_cvtepi16_epi32(in_hi);

        /* convert to float and scale */
        __m256 float_lo = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_lo), scalar);
        __m256 float_hi = _mm256_mul_ps(_mm256_cvtepi32_ps(int32_hi), scalar);

        /* store to output */
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 0), float_lo);
        _mm256_storeu_ps(reinterpret_cast<float*>(output + i + 4), float_hi);
    }

    // convert any remaining samples
    for (; i < nsamps; i++) {
        output[i] = fc32_t(float(input[i].real()) * float(scale_factor),
            float(input[i].imag()) * float(scale_factor));
    }
}
