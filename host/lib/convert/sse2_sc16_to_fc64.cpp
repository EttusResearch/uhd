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

DECLARE_CONVERTER(sc16_item32_le, 1, fc64, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc64_t *output = reinterpret_cast<fc64_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor/(1 << 16));
    const __m128i zeroi = _mm_setzero_si128();

    #define convert_item32_1_to_fc64_1_nswap_guts(_al_)                 \
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
        __m128d tmp0 = _mm_mul_pd(_mm_cvtepi32_pd(tmpilo), scalar);     \
        tmpilo = _mm_unpackhi_epi64(tmpilo, zeroi);                     \
        __m128d tmp1 = _mm_mul_pd(_mm_cvtepi32_pd(tmpilo), scalar);     \
        __m128d tmp2 = _mm_mul_pd(_mm_cvtepi32_pd(tmpihi), scalar);     \
        tmpihi = _mm_unpackhi_epi64(tmpihi, zeroi);                     \
        __m128d tmp3 = _mm_mul_pd(_mm_cvtepi32_pd(tmpihi), scalar);     \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+0), tmp0); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+1), tmp1); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+2), tmp2); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+3), tmp3); \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_item32_1_to_fc64_1_nswap_guts(_)
    }
    else{
        convert_item32_1_to_fc64_1_nswap_guts(u_)
    }

    //convert remainder
    item32_sc16_to_xx<uhd::htowx>(input+i, output+i, nsamps-i, scale_factor);
}

DECLARE_CONVERTER(sc16_item32_be, 1, fc64, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc64_t *output = reinterpret_cast<fc64_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor/(1 << 16));
    const __m128i zeroi = _mm_setzero_si128();

    #define convert_item32_1_to_fc64_1_bswap_guts(_al_)                 \
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
        __m128d tmp0 = _mm_mul_pd(_mm_cvtepi32_pd(tmpilo), scalar);     \
        tmpilo = _mm_unpackhi_epi64(tmpilo, zeroi);                     \
        __m128d tmp1 = _mm_mul_pd(_mm_cvtepi32_pd(tmpilo), scalar);     \
        __m128d tmp2 = _mm_mul_pd(_mm_cvtepi32_pd(tmpihi), scalar);     \
        tmpihi = _mm_unpackhi_epi64(tmpihi, zeroi);                     \
        __m128d tmp3 = _mm_mul_pd(_mm_cvtepi32_pd(tmpihi), scalar);     \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+0), tmp0); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+1), tmp1); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+2), tmp2); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+i+3), tmp3); \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_item32_1_to_fc64_1_bswap_guts(_)
    }
    else{
        convert_item32_1_to_fc64_1_bswap_guts(u_)
    }

    //convert remainder
    item32_sc16_to_xx<uhd::htonx>(input+i, output+i, nsamps-i, scale_factor);
}
