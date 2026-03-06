//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

UHD_INLINE __m256i pack_sc8_item32_4x(
    const __m256i& in0, const __m256i& in1, const __m256i& in2, const __m256i& in3)
{
    const __m256i shuffled_in0_lo = _mm256_permute2x128_si256(in0, in1, 0x20);
    const __m256i shuffled_in0_hi = _mm256_permute2x128_si256(in0, in1, 0x31);
    const __m256i shuffled_in1_lo = _mm256_permute2x128_si256(in2, in3, 0x20);
    const __m256i shuffled_in1_hi = _mm256_permute2x128_si256(in2, in3, 0x31);

    const __m256i lo = _mm256_packs_epi32(shuffled_in0_lo, shuffled_in0_hi);
    const __m256i hi = _mm256_packs_epi32(shuffled_in1_lo, shuffled_in1_hi);
    return _mm256_packs_epi16(lo, hi);
}

UHD_INLINE __m256i pack_sc32_4x(
    const __m256d& lo, const __m256d& hi, const __m256d& scalar)
{
    const __m128i tmpi_lo = _mm256_cvttpd_epi32(_mm256_mul_pd(hi, scalar));
    const __m128i tmpi_hi = _mm256_cvttpd_epi32(_mm256_mul_pd(lo, scalar));

    return _mm256_set_m128i(tmpi_hi, tmpi_lo);
}

DECLARE_CONVERTER_AVX2(fc64, 1, sc8_item32_be, 1, PRIORITY_SIMD_AVX2)
{
    const fc64_t* input = reinterpret_cast<const fc64_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor);

#define convert_fc64_1_to_sc8_item32_1_bswap_guts(_al_)                             \
    for (size_t j = 0; i + 15 < nsamps; i += 16, j += 8) {                          \
        /* load from input */                                                       \
        __m256d tmp0 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 0));  \
        __m256d tmp1 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 2));  \
        __m256d tmp2 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 4));  \
        __m256d tmp3 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 6));  \
        __m256d tmp4 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 8));  \
        __m256d tmp5 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 10)); \
        __m256d tmp6 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 12)); \
        __m256d tmp7 =                                                              \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 14)); \
                                                                                    \
        /* interleave */                                                            \
        const __m256i tmpi = pack_sc8_item32_4x(pack_sc32_4x(tmp1, tmp0, scalar),   \
            pack_sc32_4x(tmp3, tmp2, scalar),                                       \
            pack_sc32_4x(tmp5, tmp4, scalar),                                       \
            pack_sc32_4x(tmp7, tmp6, scalar));                                      \
                                                                                    \
        /* store to output */                                                       \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + j), tmpi);          \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc64_1_to_sc8_item32_1_bswap_guts(_)
    } else {
        convert_fc64_1_to_sc8_item32_1_bswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc8<uhd::htonx>(input + i, output + (i / 2), nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(fc64, 1, sc8_item32_le, 1, PRIORITY_SIMD_AVX2)
{
    const fc64_t* input = reinterpret_cast<const fc64_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256d scalar = _mm256_set1_pd(scale_factor);

#define convert_fc64_1_to_sc8_item32_1_nswap_guts(_al_)                                   \
    for (size_t j = 0; i + 15 < nsamps; i += 16, j += 8) {                                \
        /* load from input */                                                             \
        __m256d tmp0 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 0));        \
        __m256d tmp1 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 2));        \
        __m256d tmp2 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 4));        \
        __m256d tmp3 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 6));        \
        __m256d tmp4 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 8));        \
        __m256d tmp5 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 10));       \
        __m256d tmp6 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 12));       \
        __m256d tmp7 =                                                                    \
            _mm256_load##_al_##pd(reinterpret_cast<const double*>(input + i + 14));       \
                                                                                          \
        /* interleave */                                                                  \
        __m256i tmpi = pack_sc8_item32_4x(pack_sc32_4x(tmp0, tmp1, scalar),               \
            pack_sc32_4x(tmp2, tmp3, scalar),                                             \
            pack_sc32_4x(tmp4, tmp5, scalar),                                             \
            pack_sc32_4x(tmp6, tmp7, scalar));                                            \
        tmpi         = _mm256_or_si256(                                                   \
            _mm256_srli_epi16(tmpi, 8), _mm256_slli_epi16(tmpi, 8)); /*byteswap*/ \
                                                                                          \
        /* store to output */                                                             \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + j), tmpi);                \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc64_1_to_sc8_item32_1_nswap_guts(_)
    } else {
        convert_fc64_1_to_sc8_item32_1_nswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc8<uhd::htowx>(input + i, output + (i / 2), nsamps - i, scale_factor);
}
