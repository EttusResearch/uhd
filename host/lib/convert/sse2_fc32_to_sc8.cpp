//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <emmintrin.h>

using namespace uhd::convert;

template <const int shuf>
UHD_INLINE __m128i pack_sc32_4x(
    const __m128 &in0, const __m128 &in1,
    const __m128 &in2, const __m128 &in3,
    const __m128 &scalar
){
    __m128i tmpi0 = _mm_cvtps_epi32(_mm_mul_ps(in0, scalar));
    tmpi0 = _mm_shuffle_epi32(tmpi0, shuf);
    __m128i tmpi1 = _mm_cvtps_epi32(_mm_mul_ps(in1, scalar));
    tmpi1 = _mm_shuffle_epi32(tmpi1, shuf);
    const __m128i lo = _mm_packs_epi32(tmpi0, tmpi1);

    __m128i tmpi2 = _mm_cvtps_epi32(_mm_mul_ps(in2, scalar));
    tmpi2 = _mm_shuffle_epi32(tmpi2, shuf);
    __m128i tmpi3 = _mm_cvtps_epi32(_mm_mul_ps(in3, scalar));
    tmpi3 = _mm_shuffle_epi32(tmpi3, shuf);
    const __m128i hi = _mm_packs_epi32(tmpi2, tmpi3);

    return _mm_packs_epi16(lo, hi);
}

DECLARE_CONVERTER(fc32, 1, sc8_item32_be, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));
    const int shuf = _MM_SHUFFLE(3, 2, 1, 0);

    #define convert_fc32_1_to_sc8_item32_1_bswap_guts(_al_)             \
    for (size_t j = 0; i+7 < nsamps; i+=8, j+=4){                       \
        /* load from input */                                           \
        __m128 tmp0 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+0)); \
        __m128 tmp1 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+2)); \
        __m128 tmp2 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+4)); \
        __m128 tmp3 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+6)); \
                                                                        \
        /* convert */                                                   \
        const __m128i tmpi = pack_sc32_4x<shuf>(tmp0, tmp1, tmp2, tmp3, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+j), tmpi);  \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(input) & 0xf) == 0){
        convert_fc32_1_to_sc8_item32_1_bswap_guts(_)
    }
    else{
        convert_fc32_1_to_sc8_item32_1_bswap_guts(u_)
    }

    //convert remainder
    xx_to_item32_sc8<uhd::htonx>(input+i, output+(i/2), nsamps-i, scale_factor);
}

DECLARE_CONVERTER(fc32, 1, sc8_item32_le, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));
    const int shuf = _MM_SHUFFLE(0, 1, 2, 3);

    #define convert_fc32_1_to_sc8_item32_1_nswap_guts(_al_)             \
    for (size_t j = 0; i+7 < nsamps; i+=8, j+=4){                       \
        /* load from input */                                           \
        __m128 tmp0 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+0)); \
        __m128 tmp1 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+2)); \
        __m128 tmp2 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+4)); \
        __m128 tmp3 = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+6)); \
                                                                        \
        /* convert */                                                   \
        const __m128i tmpi = pack_sc32_4x<shuf>(tmp0, tmp1, tmp2, tmp3, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+j), tmpi);  \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(input) & 0xf) == 0){
        convert_fc32_1_to_sc8_item32_1_nswap_guts(_)
    }
    else{
        convert_fc32_1_to_sc8_item32_1_nswap_guts(u_)
    }

    //convert remainder
    xx_to_item32_sc8<uhd::htowx>(input+i, output+(i/2), nsamps-i, scale_factor);
}
