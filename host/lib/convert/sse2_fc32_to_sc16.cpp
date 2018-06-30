//
// Copyright 2011-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <emmintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER(fc32, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));

    // this macro converts values faster by using SSE intrinsics to convert 4 values at a time
    #define convert_fc32_1_to_item32_1_nswap_guts(_al_)                 \
    for (; i+3 < nsamps; i+=4){                                         \
        /* load from input */                                           \
        __m128 tmplo = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+0)); \
        __m128 tmphi = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+2)); \
                                                                        \
        /* convert and scale */                                         \
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));    \
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));    \
                                                                        \
        /* pack + swap 16-bit pairs */                                  \
        __m128i tmpi = _mm_packs_epi32(tmpilo, tmpihi);                 \
        tmpi = _mm_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));      \
        tmpi = _mm_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));      \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+i), tmpi);  \
    }                                                                   \

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        convert_fc32_1_to_item32_1_nswap_guts(_)
        break;
    case 0x8:
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        xx_to_item32_sc16<uhd::htowx>(input, output, 1, scale_factor);
        i++;
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        convert_fc32_1_to_item32_1_nswap_guts(_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load
        convert_fc32_1_to_item32_1_nswap_guts(u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(fc32, 1, sc16_item32_be, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));

    // this macro converts values faster by using SSE intrinsics to convert 4 values at a time
    #define convert_fc32_1_to_item32_1_bswap_guts(_al_)                 \
    for (; i+3 < nsamps; i+=4){                                         \
        /* load from input */                                           \
        __m128 tmplo = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+0)); \
        __m128 tmphi = _mm_load ## _al_ ## ps(reinterpret_cast<const float *>(input+i+2)); \
                                                                        \
        /* convert and scale */                                         \
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));    \
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));    \
                                                                        \
        /* pack + byteswap -> byteswap 16 bit words */                  \
        __m128i tmpi = _mm_packs_epi32(tmpilo, tmpihi);                 \
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8)); \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+i), tmpi);  \
    }                                                                   \

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        convert_fc32_1_to_item32_1_bswap_guts(_)
        break;
    case 0x8:
        // the first value is 8-byte aligned - process it and prepare the bulk of the data for fast conversion
        xx_to_item32_sc16<uhd::htonx>(input, output, 1, scale_factor);
        i++;
        // do faster processing of the remaining samples now that we are 16-byte aligned
        convert_fc32_1_to_item32_1_bswap_guts(_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load
        convert_fc32_1_to_item32_1_bswap_guts(u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htonx>(input+i, output+i, nsamps-i, scale_factor);
}
