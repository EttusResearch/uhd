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

DECLARE_CONVERTER(sc16_item32_le, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor)/(1 << 16));
    const __m128i zeroi = _mm_setzero_si128();

    // this macro converts values faster by using SSE intrinsics to convert 4 values at a time
    #define convert_item32_1_to_fc32_1_nswap_guts(_al_)                 \
    for (; i+3 < nsamps; i+=4){                                         \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* unpack + swap 16-bit pairs */                                \
        tmpi = _mm_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));      \
        tmpi = _mm_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));      \
        __m128i tmpilo = _mm_unpacklo_epi16(zeroi, tmpi); /* value in upper 16 bits */ \
        __m128i tmpihi = _mm_unpackhi_epi16(zeroi, tmpi);               \
                                                                        \
        /* convert and scale */                                         \
        __m128 tmplo = _mm_mul_ps(_mm_cvtepi32_ps(tmpilo), scalar);     \
        __m128 tmphi = _mm_mul_ps(_mm_cvtepi32_ps(tmpihi), scalar);     \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+i+0), tmplo); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+i+2), tmphi); \
    }                                                                   \

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        convert_item32_1_to_fc32_1_nswap_guts(_)
        break;
    case 0x8:
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        item32_sc16_to_xx<uhd::htowx>(input, output, 1, scale_factor);
        i++;
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        convert_item32_1_to_fc32_1_nswap_guts(_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load and store
        convert_item32_1_to_fc32_1_nswap_guts(u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_be, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor)/(1 << 16));
    const __m128i zeroi = _mm_setzero_si128();

    // this macro converts values faster by using SSE intrinsics to convert 4 values at a time
    #define convert_item32_1_to_fc32_1_bswap_guts(_al_)                 \
    for (; i+3 < nsamps; i+=4){                                         \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* byteswap + unpack -> byteswap 16 bit words */                \
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8)); \
        __m128i tmpilo = _mm_unpacklo_epi16(zeroi, tmpi); /* value in upper 16 bits */ \
        __m128i tmpihi = _mm_unpackhi_epi16(zeroi, tmpi);               \
                                                                        \
        /* convert and scale */                                         \
        __m128 tmplo = _mm_mul_ps(_mm_cvtepi32_ps(tmpilo), scalar);     \
        __m128 tmphi = _mm_mul_ps(_mm_cvtepi32_ps(tmpihi), scalar);     \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+i+0), tmplo); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+i+2), tmphi); \
    }                                                                   \

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        convert_item32_1_to_fc32_1_bswap_guts(_)
        break;
    case 0x8:
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        item32_sc16_to_xx<uhd::htonx>(input, output, 1, scale_factor);
        i++;
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        convert_item32_1_to_fc32_1_bswap_guts(_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load and store
        convert_item32_1_to_fc32_1_bswap_guts(u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htonx>(input+i, output+i, nsamps-i, scale_factor);
}
