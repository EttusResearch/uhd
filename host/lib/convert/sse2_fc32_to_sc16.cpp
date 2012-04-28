//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <emmintrin.h>

using namespace uhd::convert;

DECLARE_CONVERTER(fc32, 1, sc16_item32_le, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));

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

    //dispatch according to alignment
    switch (size_t(input) & 0xf){
    case 0x8:
        xx_to_item32_sc16<uhd::htowx>(input, output, 1, scale_factor); i++;
    case 0x0:
        convert_fc32_1_to_item32_1_nswap_guts(_)
        break;
    default: convert_fc32_1_to_item32_1_nswap_guts(u_)
    }

    //convert remainder
    xx_to_item32_sc16<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(fc32, 1, sc16_item32_be, 1, PRIORITY_SIMD){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor));

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

    //dispatch according to alignment
    switch (size_t(input) & 0xf){
    case 0x8:
        xx_to_item32_sc16<uhd::htonx>(input, output, 1, scale_factor); i++;
    case 0x0:
        convert_fc32_1_to_item32_1_bswap_guts(_)
        break;
    default: convert_fc32_1_to_item32_1_bswap_guts(u_)
    }

    //convert remainder
    xx_to_item32_sc16<uhd::htonx>(input+i, output+i, nsamps-i, scale_factor);
}
