//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER_AVX2(sc16_item32_le, 1, fc64, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    fc64_t* output        = reinterpret_cast<fc64_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor / (1 << 16));
    const __m256i zeroi  = _mm256_setzero_si256();

#define convert_item32_1_to_fc64_1_nswap_guts(_al_)                                     \
    for (; i + 7 < nsamps; i += 8) {                                                    \
        /* load from input */                                                           \
        __m256i tmpi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i)); \
                                                                                        \
        /* unpack + swap 16-bit pairs */                                                \
        tmpi           = _mm256_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));         \
        tmpi           = _mm256_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));         \
        __m256i tmpilo = _mm256_unpacklo_epi16(zeroi, tmpi);                            \
        __m256i tmpihi = _mm256_unpackhi_epi16(zeroi, tmpi);                            \
                                                                                        \
        __m128i tmpilo_lo = _mm256_castsi256_si128(tmpilo);                             \
        __m128i tmpilo_hi = _mm256_extracti128_si256(tmpilo, 1);                        \
        __m128i tmpihi_lo = _mm256_castsi256_si128(tmpihi);                             \
        __m128i tmpihi_hi = _mm256_extracti128_si256(tmpihi, 1);                        \
                                                                                        \
        /* convert and scale */                                                         \
        __m256d tmp0 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_lo), scalar);            \
        tmpilo       = _mm256_unpackhi_epi64(tmpilo, zeroi);                            \
        __m256d tmp1 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_lo), scalar);            \
        __m256d tmp2 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_hi), scalar);            \
        tmpihi       = _mm256_unpackhi_epi64(tmpihi, zeroi);                            \
        __m256d tmp3 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_hi), scalar);            \
                                                                                        \
        /* store to output */                                                           \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 0), tmp0);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 2), tmp1);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 4), tmp2);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 6), tmp3);              \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(output) & 0xf) == 0) {
        convert_item32_1_to_fc64_1_nswap_guts(_)
    } else {
        convert_item32_1_to_fc64_1_nswap_guts(u_)
    }

    // convert remainder
    item32_sc16_to_xx<uhd::htowx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(sc16_item32_be, 1, fc64, 1, PRIORITY_SIMD_AVX2)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    fc64_t* output        = reinterpret_cast<fc64_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor / (1 << 16));
    const __m256i zeroi  = _mm256_setzero_si256();

#define convert_item32_1_to_fc64_1_bswap_guts(_al_)                                     \
    for (; i + 7 < nsamps; i += 8) {                                                    \
        /* load from input */                                                           \
        __m256i tmpi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i)); \
                                                                                        \
        /* byteswap + unpack -> byteswap 16 bit words */                                \
        tmpi = _mm256_or_si256(_mm256_srli_epi16(tmpi, 8), _mm256_slli_epi16(tmpi, 8)); \
        __m256i tmpilo = _mm256_unpacklo_epi16(zeroi, tmpi);                            \
        __m256i tmpihi = _mm256_unpackhi_epi16(zeroi, tmpi);                            \
                                                                                        \
        __m128i tmpilo_lo = _mm256_castsi256_si128(tmpilo);                             \
        __m128i tmpilo_hi = _mm256_extracti128_si256(tmpilo, 1);                        \
        __m128i tmpihi_lo = _mm256_castsi256_si128(tmpihi);                             \
        __m128i tmpihi_hi = _mm256_extracti128_si256(tmpihi, 1);                        \
                                                                                        \
        /* convert and scale */                                                         \
        __m256d tmp0 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_lo), scalar);            \
        tmpilo       = _mm256_unpackhi_epi64(tmpilo, zeroi);                            \
        __m256d tmp1 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_lo), scalar);            \
        __m256d tmp2 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_hi), scalar);            \
        tmpihi       = _mm256_unpackhi_epi64(tmpihi, zeroi);                            \
        __m256d tmp3 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_hi), scalar);            \
                                                                                        \
        /* store to output */                                                           \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 0), tmp0);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 2), tmp1);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 4), tmp2);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 6), tmp3);              \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(output) & 0xf) == 0) {
        convert_item32_1_to_fc64_1_bswap_guts(_)
    } else {
        convert_item32_1_to_fc64_1_bswap_guts(u_)
    }

    // convert remainder
    item32_sc16_to_xx<uhd::htonx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(sc16_chdr, 1, fc64, 1, PRIORITY_SIMD_AVX2)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    fc64_t* output      = reinterpret_cast<fc64_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor / (1 << 16));
    const __m256i zeroi  = _mm256_setzero_si256();

#define convert_chdr_1_to_fc64_1_guts(_al_)                                             \
    for (; i + 7 < nsamps; i += 8) {                                                    \
        /* load from input */                                                           \
        __m256i tmpi = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i)); \
                                                                                        \
        /* unpack 16-bit pairs */                                                       \
        __m256i tmpilo = _mm256_unpacklo_epi16(zeroi, tmpi);                            \
        __m256i tmpihi = _mm256_unpackhi_epi16(zeroi, tmpi);                            \
                                                                                        \
        __m128i tmpilo_lo = _mm256_castsi256_si128(tmpilo);                             \
        __m128i tmpilo_hi = _mm256_extracti128_si256(tmpilo, 1);                        \
        __m128i tmpihi_lo = _mm256_castsi256_si128(tmpihi);                             \
        __m128i tmpihi_hi = _mm256_extracti128_si256(tmpihi, 1);                        \
                                                                                        \
        /* convert and scale */                                                         \
        __m256d tmp0 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_lo), scalar);            \
        tmpilo       = _mm256_unpackhi_epi64(tmpilo, zeroi);                            \
        __m256d tmp1 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_lo), scalar);            \
        __m256d tmp2 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpilo_hi), scalar);            \
        tmpihi       = _mm256_unpackhi_epi64(tmpihi, zeroi);                            \
        __m256d tmp3 = _mm256_mul_pd(_mm256_cvtepi32_pd(tmpihi_hi), scalar);            \
                                                                                        \
        /* store to output */                                                           \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 0), tmp0);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 2), tmp1);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 4), tmp2);              \
        _mm256_storeu_pd(reinterpret_cast<double*>(output + i + 6), tmp3);              \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(output) & 0xf) == 0) {
        convert_chdr_1_to_fc64_1_guts(_)
    } else {
        convert_chdr_1_to_fc64_1_guts(u_)
    }

    // convert remainder
    chdr_sc16_to_xx(input + i, output + i, nsamps - i, scale_factor);
}
