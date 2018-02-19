//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <emmintrin.h>

using namespace uhd::convert;

//
// SSE 16-bit pair swap
//
// Valid alignment macro arguments are 'u_' and '_' for unaligned and aligned
// access respectively. Macro operates on 4 complex 16-bit integers at a time.
//
//       -----------------
//      | A | B | C | D |   Input
//      -----------------
//        0   1   2   3     Address
//      -----------------
//      | C | D | A | B |   Output
//      -----------------
//
#define CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_ialign_,_oalign_)          \
    for (; i+3 < nsamps; i+=4) {                                        \
        __m128i m0;                                                     \
                                                                        \
        /* load from input */                                           \
        m0 = _mm_load ## _ialign_ ## si128((const __m128i *) (input+i));\
                                                                        \
        /* swap 16-bit pairs */                                         \
        m0 = _mm_shufflelo_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));          \
        m0 = _mm_shufflehi_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1));          \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _oalign_ ## si128((__m128i *) (output+i), m0);     \
    }                                                                   \

//
// SSE byte swap
//
// Valid alignment macro arguments are 'u_' and '_' for unaligned and aligned
// access respectively. Macro operates on 4 complex 16-bit integers at a time.
//
//       -----------------
//      | A | B | C | D |   Input
//      -----------------
//        0   1   2   3     Address
//      -----------------
//      | B | A | D | C |   Output
//      -----------------
//
#define CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_ialign_,_oalign_)          \
    for (; i+3 < nsamps; i+=4) {                                        \
        __m128i m0, m1, m2;                                             \
                                                                        \
        /* load from input */                                           \
        m0 = _mm_load ## _ialign_ ## si128((const __m128i *) (input+i));\
                                                                        \
        /* byteswap 16 bit words */                                     \
        m1 = _mm_srli_epi16(m0, 8);                                     \
        m2 = _mm_slli_epi16(m0, 8);                                     \
        m0 = _mm_or_si128(m1, m2);                                      \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _oalign_ ## si128((__m128i *) (output+i), m0);     \
    }                                                                   \

DECLARE_CONVERTER(sc16, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const sc16_t *input = reinterpret_cast<const sc16_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_,u_)
        break;
    case 0x8:
        if (nsamps < 2)
            break;
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        xx_to_item32_sc16<uhd::htowx>(input, output, 2, 1.0);
        i += 2;
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_,u_)
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_,u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, 1.0);
}

DECLARE_CONVERTER(sc16, 1, sc16_item32_be, 1, PRIORITY_SIMD){
    const sc16_t *input = reinterpret_cast<const sc16_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_,u_)
        break;
    case 0x8:
        if (nsamps < 2)
            break;
        // the first value is 8-byte aligned - process it and prepare the bulk of the data for fast conversion
        xx_to_item32_sc16<uhd::htonx>(input, output, 2, 1.0);
        i += 2;
        // do faster processing of the remaining samples now that we are 16-byte aligned
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_,u_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_,u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htonx>(input+i, output+i, nsamps-i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_le, 1, sc16, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    sc16_t *output = reinterpret_cast<sc16_t *>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_,_)
        break;
    case 0x8:
        if (nsamps < 2)
            break;
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        item32_sc16_to_xx<uhd::htowx>(input, output, 2, 1.0);
        i += 2;
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_,_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load and store
        CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_,u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htowx>(input+i, output+i, nsamps-i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_be, 1, sc16, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    sc16_t *output = reinterpret_cast<sc16_t *>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf){
    case 0x0:
        // the data is 16-byte aligned, so do the fast processing of the bulk of the samples
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_,_)
        break;
    case 0x8:
        if (nsamps < 2)
            break;
        // the first sample is 8-byte aligned - process it to align the remainder of the samples to 16-bytes
        item32_sc16_to_xx<uhd::htonx>(input, output, 2, 1.0);
        i += 2;
        // do faster processing of the bulk of the samples now that we are 16-byte aligned
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_,_)
        break;
    default:
        // we are not 8 or 16-byte aligned, so do fast processing with the unaligned load and store
        CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_,u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htonx>(input+i, output+i, nsamps-i, 1.0);
}
