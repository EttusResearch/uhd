//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <immintrin.h>

using namespace uhd::convert;

#define CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_ialign_, _oalign_)   \
    for (; i + 7 < nsamps; i += 8) {                              \
        __m256i m0;                                               \
                                                                  \
        /* load from input */                                     \
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));     \
                                                                  \
        /* swap 16-bit pairs */                                   \
        m0 = _mm256_shufflelo_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1)); \
        m0 = _mm256_shufflehi_epi16(m0, _MM_SHUFFLE(2, 3, 0, 1)); \
                                                                  \
        /* store to output */                                     \
        _mm256_storeu_si256((__m256i*)(output + i), m0);          \
    }

#define CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_ialign_, _oalign_) \
    for (; i + 7 < nsamps; i += 8) {                            \
        __m256i m0, m1, m2;                                     \
                                                                \
        /* load from input */                                   \
        m0 = _mm256_loadu_si256((const __m256i*)(input + i));   \
                                                                \
        /* byteswap 16 bit words */                             \
        m1 = _mm256_srli_epi16(m0, 8);                          \
        m2 = _mm256_slli_epi16(m0, 8);                          \
        m0 = _mm256_or_si256(m1, m2);                           \
                                                                \
        /* store to output */                                   \
        _mm256_storeu_si256((__m256i*)(output + i), m0);        \
    }

DECLARE_CONVERTER(sc16, 1, sc16_item32_le, 1, PRIORITY_SIMD)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf) {
        case 0x0:
            // the data is 16-byte aligned, so do the fast processing of the bulk of the
            // samples
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_, u_)
            break;
        case 0x8:
            if (nsamps < 2)
                break;
            // the first sample is 8-byte aligned - process it to align the remainder of
            // the samples to 16-bytes
            xx_to_item32_sc16<uhd::htowx>(input, output, 2, 1.0);
            i += 2;
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(_, u_)
            // do faster processing of the bulk of the samples now that we are 16-byte
            // aligned
            break;
        default:
            // we are not 8 or 16-byte aligned, so do fast processing with the unaligned
            // load
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_, u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htowx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16, 1, sc16_item32_be, 1, PRIORITY_SIMD)
{
    const sc16_t* input = reinterpret_cast<const sc16_t*>(inputs[0]);
    item32_t* output    = reinterpret_cast<item32_t*>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(input) & 0xf) {
        case 0x0:
            // the data is 16-byte aligned, so do the fast processing of the bulk of the
            // samples
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_, u_)
            break;
        case 0x8:
            if (nsamps < 2)
                break;
            // the first value is 8-byte aligned - process it and prepare the bulk of the
            // data for fast conversion
            xx_to_item32_sc16<uhd::htonx>(input, output, 2, 1.0);
            i += 2;
            // do faster processing of the remaining samples now that we are 16-byte
            // aligned
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(_, u_)
            break;
        default:
            // we are not 8 or 16-byte aligned, so do fast processing with the unaligned
            // load
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_, u_)
    }

    // convert any remaining samples
    xx_to_item32_sc16<uhd::htonx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_le, 1, sc16, 1, PRIORITY_SIMD)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    sc16_t* output        = reinterpret_cast<sc16_t*>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf) {
        case 0x0:
            // the data is 16-byte aligned, so do the fast processing of the bulk of the
            // samples
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_, _)
            break;
        case 0x8:
            if (nsamps < 2)
                break;
            // the first sample is 8-byte aligned - process it to align the remainder of
            // the samples to 16-bytes
            item32_sc16_to_xx<uhd::htowx>(input, output, 2, 1.0);
            i += 2;
            // do faster processing of the bulk of the samples now that we are 16-byte
            // aligned
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_, _)
            break;
        default:
            // we are not 8 or 16-byte aligned, so do fast processing with the unaligned
            // load and store
            CONVERT_SC16_1_TO_SC16_1_NSWAP_GUTS(u_, u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htowx>(input + i, output + i, nsamps - i, 1.0);
}

DECLARE_CONVERTER(sc16_item32_be, 1, sc16, 1, PRIORITY_SIMD)
{
    const item32_t* input = reinterpret_cast<const item32_t*>(inputs[0]);
    sc16_t* output        = reinterpret_cast<sc16_t*>(outputs[0]);

    size_t i = 0;

    // need to dispatch according to alignment for fastest conversion
    switch (size_t(output) & 0xf) {
        case 0x0:
            // the data is 16-byte aligned, so do the fast processing of the bulk of the
            // samples
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_, _)
            break;
        case 0x8:
            if (nsamps < 2)
                break;
            // the first sample is 8-byte aligned - process it to align the remainder of
            // the samples to 16-bytes
            item32_sc16_to_xx<uhd::htonx>(input, output, 2, 1.0);
            i += 2;
            // do faster processing of the bulk of the samples now that we are 16-byte
            // aligned
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_, _)
            break;
        default:
            // we are not 8 or 16-byte aligned, so do fast processing with the unaligned
            // load and store
            CONVERT_SC16_1_TO_SC16_1_BSWAP_GUTS(u_, u_)
    }

    // convert any remaining samples
    item32_sc16_to_xx<uhd::htonx>(input + i, output + i, nsamps - i, 1.0);
}
