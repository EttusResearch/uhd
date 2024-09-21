//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER_AVX2(fc64, 1, sc16_item32_le, 1, PRIORITY_SIMD_AVX2)
{
    const fc64_t* input = reinterpret_cast<const fc64_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor);

#define convert_fc64_1_to_item32_1_nswap_guts(_al_)                                \
    for (; i + 7 < nsamps; i += 8) {                                               \
        /* load from input */                                                      \
        __m256d tmp0 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 0)); \
        __m256d tmp1 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 2)); \
        __m256d tmp2 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 4)); \
        __m256d tmp3 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 6)); \
                                                                                   \
        /* convert and scale */                                                    \
        __m128i tmpi0 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp0, scalar));          \
        __m128i tmpi1 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp1, scalar));          \
        __m128i tmpi2 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp2, scalar));          \
        __m128i tmpi3 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp3, scalar));          \
                                                                                   \
        /* Unpack and interleave the results */                                    \
        __m256i tmpilo = _mm256_set_m128i(tmpi1, tmpi0);                           \
        __m256i tmpihi = _mm256_set_m128i(tmpi3, tmpi2);                           \
                                                                                   \
        /* Pack and swap 16-bit pairs */                                           \
        __m256i shuffled_lo = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x20);     \
        __m256i shuffled_hi = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x31);     \
                                                                                   \
        /* pack + swap 16-bit pairs */                                             \
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);               \
                                                                                   \
        /* pack + swap 16-bit pairs */                                             \
        tmpi = _mm256_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));              \
        tmpi = _mm256_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));              \
                                                                                   \
        /* store to output */                                                      \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);         \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc64_1_to_item32_1_nswap_guts(_)
    } else {
        convert_fc64_1_to_item32_1_nswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc16<uhd::htowx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(fc64, 1, sc16_item32_be, 1, PRIORITY_SIMD_AVX2)
{
    const fc64_t* input = reinterpret_cast<const fc64_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor);

#define convert_fc64_1_to_item32_1_bswap_guts(_al_)                                     \
    for (; i + 7 < nsamps; i += 8) {                                                    \
        /* load from input */                                                           \
        __m256d tmp0 =                                                                  \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 0));      \
        __m256d tmp1 =                                                                  \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 2));      \
        __m256d tmp2 =                                                                  \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 4));      \
        __m256d tmp3 =                                                                  \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 6));      \
                                                                                        \
        /* convert and scale */                                                         \
        __m128i tmpi0 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp0, scalar));               \
        __m128i tmpi1 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp1, scalar));               \
        __m128i tmpi2 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp2, scalar));               \
        __m128i tmpi3 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp3, scalar));               \
                                                                                        \
                                                                                        \
        /* Unpack and interleave the results */                                         \
        __m256i tmpilo = _mm256_set_m128i(tmpi1, tmpi0);                                \
        __m256i tmpihi = _mm256_set_m128i(tmpi3, tmpi2);                                \
                                                                                        \
        /* Pack and swap 16-bit pairs */                                                \
        __m256i shuffled_lo = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x20);          \
        __m256i shuffled_hi = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x31);          \
                                                                                        \
        /* pack + swap 16-bit pairs */                                                  \
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);                    \
        tmpi = _mm256_or_si256(_mm256_srli_epi16(tmpi, 8), _mm256_slli_epi16(tmpi, 8)); \
                                                                                        \
        /* store to output */                                                           \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);              \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc64_1_to_item32_1_bswap_guts(_)
    } else {
        convert_fc64_1_to_item32_1_bswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc16<uhd::htonx>(input + i, output + i, nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(fc64, 1, sc16_chdr, 1, PRIORITY_SIMD_AVX2)
{
    const fc64_t* input = reinterpret_cast<const fc64_t*>(inputs[0]);
    sc16_t* output      = reinterpret_cast<sc16_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor);

#define convert_fc64_1_to_chdr_1_guts(_al_)                                        \
    for (; i + 7 < nsamps; i += 8) {                                               \
        /* load from input */                                                      \
        __m256d tmp0 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 0)); \
        __m256d tmp1 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 2)); \
        __m256d tmp2 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 4)); \
        __m256d tmp3 =                                                             \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 6)); \
                                                                                   \
        /* convert and scale */                                                    \
        __m128i tmpi0 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp0, scalar));          \
        __m128i tmpi1 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp1, scalar));          \
        __m128i tmpi2 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp2, scalar));          \
        __m128i tmpi3 = _mm256_cvttpd_epi32(_mm256_mul_pd(tmp3, scalar));          \
                                                                                   \
        /* Unpack and interleave the results */                                    \
        __m256i tmpilo = _mm256_set_m128i(tmpi1, tmpi0);                           \
        __m256i tmpihi = _mm256_set_m128i(tmpi3, tmpi2);                           \
                                                                                   \
        /* Pack and swap 16-bit pairs */                                           \
        __m256i shuffled_lo = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x20);     \
        __m256i shuffled_hi = _mm256_permute2x128_si256(tmpilo, tmpihi, 0x31);     \
                                                                                   \
        /* pack + swap 16-bit pairs */                                             \
        __m256i tmpi = _mm256_packs_epi32(shuffled_lo, shuffled_hi);               \
                                                                                   \
        /* store to output */                                                      \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), tmpi);         \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc64_1_to_chdr_1_guts(_)
    } else {
        convert_fc64_1_to_chdr_1_guts(u_)
    }

    // convert remainder
    xx_to_chdr_sc16(input + i, output + i, nsamps - i, scale_factor);
}
