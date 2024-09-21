//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

template <const int shuf>
UHD_INLINE __m256i pack_sc32_4x(const __m256& in0,
    const __m256& in1,
    const __m256& in2,
    const __m256& in3,
    const __m256& scalar)
{
    __m256i tmpi0 = _mm256_cvtps_epi32(_mm256_mul_ps(in0, scalar));
    tmpi0         = _mm256_shuffle_epi32(tmpi0, shuf);
    __m256i tmpi1 = _mm256_cvtps_epi32(_mm256_mul_ps(in1, scalar));
    tmpi1         = _mm256_shuffle_epi32(tmpi1, shuf);

    __m256i shuf_lo_lo = _mm256_permute2x128_si256(tmpi0, tmpi1, 0x20);
    __m256i shuf_lo_hi = _mm256_permute2x128_si256(tmpi0, tmpi1, 0x31);
    const __m256i lo   = _mm256_packs_epi32(shuf_lo_lo, shuf_lo_hi);

    __m256i tmpi2 = _mm256_cvtps_epi32(_mm256_mul_ps(in2, scalar));
    tmpi2         = _mm256_shuffle_epi32(tmpi2, shuf);
    __m256i tmpi3 = _mm256_cvtps_epi32(_mm256_mul_ps(in3, scalar));
    tmpi3         = _mm256_shuffle_epi32(tmpi3, shuf);

    __m256i shuf_hi_lo = _mm256_permute2x128_si256(tmpi2, tmpi3, 0x20);
    __m256i shuf_hi_hi = _mm256_permute2x128_si256(tmpi2, tmpi3, 0x31);
    const __m256i hi   = _mm256_packs_epi32(shuf_hi_lo, shuf_hi_hi);

    __m256i shuf_lo = _mm256_permute2x128_si256(lo, hi, 0x20);
    __m256i shuf_hi = _mm256_permute2x128_si256(lo, hi, 0x31);

    return _mm256_packs_epi16(shuf_lo, shuf_hi);
}

DECLARE_CONVERTER_AVX2(fc32, 1, sc8_item32_be, 1, PRIORITY_SIMD_AVX2)
{
    const fc32_t* input = reinterpret_cast<const fc32_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));
    const int shuf      = _MM_SHUFFLE(3, 2, 1, 0);

#define convert_fc32_1_to_sc8_item32_1_bswap_guts(_al_)                            \
    for (size_t j = 0; i + 15 < nsamps; i += 16, j += 8) {                         \
        /* load from input */                                                      \
        __m256 tmp0 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 0));  \
        __m256 tmp1 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 4));  \
        __m256 tmp2 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 8));  \
        __m256 tmp3 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 12)); \
                                                                                   \
        /* convert */                                                              \
        const __m256i tmpi = pack_sc32_4x<shuf>(tmp0, tmp1, tmp2, tmp3, scalar);   \
                                                                                   \
        /* store to output */                                                      \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + j), tmpi);         \
    }


    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc32_1_to_sc8_item32_1_bswap_guts(_)
    } else {
        convert_fc32_1_to_sc8_item32_1_bswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc8<uhd::htonx>(input + i, output + (i / 2), nsamps - i, scale_factor);
}

DECLARE_CONVERTER_AVX2(fc32, 1, sc8_item32_le, 1, PRIORITY_SIMD_AVX2)
{
    const fc32_t* input = reinterpret_cast<const fc32_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    const __m256 scalar = _mm256_set1_ps(float(scale_factor));
    const int shuf      = _MM_SHUFFLE(0, 1, 2, 3);

#define convert_fc32_1_to_sc8_item32_1_nswap_guts(_al_)                            \
    for (size_t j = 0; i + 15 < nsamps; i += 16, j += 8) {                         \
        /* load from input */                                                      \
        __m256 tmp0 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 0));  \
        __m256 tmp1 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 4));  \
        __m256 tmp2 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 8));  \
        __m256 tmp3 =                                                              \
            _mm256_load##_al_##ps(reinterpret_cast<const float*>(input + i + 12)); \
                                                                                   \
        /* convert */                                                              \
        const __m256i tmpi = pack_sc32_4x<shuf>(tmp0, tmp1, tmp2, tmp3, scalar);   \
                                                                                   \
        /* store to output */                                                      \
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + j), tmpi);         \
    }

    size_t i = 0;

    // dispatch according to alignment
    if ((size_t(input) & 0x1f) == 0) {
        convert_fc32_1_to_sc8_item32_1_nswap_guts(_)
    } else {
        convert_fc32_1_to_sc8_item32_1_nswap_guts(u_)
    }

    // convert remainder
    xx_to_item32_sc8<uhd::htowx>(input + i, output + (i / 2), nsamps - i, scale_factor);
}
