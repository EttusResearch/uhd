//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER(fc32, 1, sc16_item32_le, 1, PRIORITY_SIMD_AVX2)
{
    const fc32_t* input = reinterpret_cast<const fc32_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load from input */
        __m256 tmplo = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 0));
        __m256 tmphi = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 4));

        /* convert and scale */
        __m256i tmpilo = _mm256_cvtps_epi32(_mm256_mul_ps(tmplo, scalar));
        __m256i tmpihi = _mm256_cvtps_epi32(_mm256_mul_ps(tmphi, scalar));

        __m256i shuffled_lo = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x20); /* lower 128-bit of tmpilo and tmpihi */
        __m256i shuffled_hi = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x31); /* upper 128-bit of tmpilo and tmpihi */

        /* now pack the shuffled data sequentially */
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);

        /* pack + swap 16-bit pairs */
        tmpi = _mm256_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
        tmpi = _mm256_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));

        /* store to output */
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htowx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER(fc32, 1, sc16_item32_be, 1, PRIORITY_SIMD_AVX2)
{
    const fc32_t* input = reinterpret_cast<const fc32_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load from input */
        __m256 tmplo = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 0));
        __m256 tmphi = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 4));

        /* convert and scale */
        __m256i tmpilo = _mm256_cvtps_epi32(_mm256_mul_ps(tmplo, scalar));
        __m256i tmpihi = _mm256_cvtps_epi32(_mm256_mul_ps(tmphi, scalar));

        __m256i shuffled_lo = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x20); /* lower 128-bit of tmpilo and tmpihi */
        __m256i shuffled_hi = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x31); /* upper 128-bit of tmpilo and tmpihi */

        /* Now pack the shuffled data sequentially */
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);

        tmpi = _mm256_or_si256(_mm256_srli_epi16(tmpi, 8), _mm256_slli_epi16(tmpi, 8));

        /* store to output */
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htonx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER(fc32, 1, sc16_chdr, 1, PRIORITY_SIMD_AVX2)
{
    const fc32_t* input = reinterpret_cast<const fc32_t*>(inputs[0]);
    sc16_t* output      = reinterpret_cast<sc16_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));

    size_t i = 0;

    for (; i + 7 < nsamps; i += 8) {
        /* load from input */
        __m256 tmplo = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 0));
        __m256 tmphi = _mm256_loadu_ps(reinterpret_cast<const float*>(input + i + 4));

        /* convert and scale */
        __m256i tmpilo = _mm256_cvtps_epi32(_mm256_mul_ps(tmplo, scalar));
        __m256i tmpihi = _mm256_cvtps_epi32(_mm256_mul_ps(tmphi, scalar));

        /* mm256_packs_epi32 is not sequential, it needs to be split into m128i */
        __m256i shuffled_lo = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x20); /* lower 128-bit of tmpilo and tmpihi */
        __m256i shuffled_hi = _mm256_permute2x128_si256(
            tmpilo, tmpihi, 0x31); /* upper 128-bit of tmpilo and tmpihi */

        /* Now pack the shuffled data sequentially */
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);

        /* store to output */
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);
    }

    // convert any remaining samples
    xx_to_chdr_sc16(input + i, output + i, nsamps - i, scale_factor);
}
