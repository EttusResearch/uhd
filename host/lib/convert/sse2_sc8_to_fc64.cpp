//
// Copyright 2012 Ettus Research LLC
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

static const __m128i zeroi = _mm_setzero_si128();

UHD_INLINE void unpack_sc32_8x(
    const __m128i &in,
    __m128d &out0, __m128d &out1,
    __m128d &out2, __m128d &out3,
    __m128d &out4, __m128d &out5,
    __m128d &out6, __m128d &out7,
    const __m128d &scalar
){
    const int shuf = _MM_SHUFFLE(1, 0, 3, 2);
    __m128i tmp;

    const __m128i tmplo = _mm_unpacklo_epi8(zeroi, in); /* value in upper 8 bits */
    tmp = _mm_unpacklo_epi16(zeroi, tmplo); /* value in upper 16 bits */
    out0 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_shuffle_epi32(tmp, shuf);
    out1 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_unpackhi_epi16(zeroi, tmplo);
    out2 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_shuffle_epi32(tmp, shuf);
    out3 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);

    const __m128i tmphi = _mm_unpackhi_epi8(zeroi, in);
    tmp = _mm_unpacklo_epi16(zeroi, tmphi);
    out4 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_shuffle_epi32(tmp, shuf);
    out5 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_unpackhi_epi16(zeroi, tmphi);
    out6 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
    tmp = _mm_shuffle_epi32(tmp, shuf);
    out7 = _mm_mul_pd(_mm_cvtepi32_pd(tmp), scalar);
}

DECLARE_CONVERTER(sc8_item32_be, 1, fc64, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(size_t(inputs[0]) & ~0x3);
    fc64_t *output = reinterpret_cast<fc64_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor/(1 << 24));

    size_t i = 0, j = 0;
    fc32_t dummy;
    size_t num_samps = nsamps;

    if ((size_t(inputs[0]) & 0x3) != 0){
        item32_sc8_to_xx<uhd::ntohx>(input++, output++, 1, scale_factor);
        num_samps--;
    }

    #define convert_sc8_item32_1_to_fc64_1_bswap_guts(_al_)             \
    for (; j+7 < num_samps; j+=8, i+=4){                                \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* unpack */                                                    \
        __m128d tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;         \
        unpack_sc32_8x(tmpi, tmp1, tmp0, tmp3, tmp2, tmp5, tmp4, tmp7, tmp6, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+0), tmp0); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+1), tmp1); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+2), tmp2); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+3), tmp3); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+4), tmp4); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+5), tmp5); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+6), tmp6); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+7), tmp7); \
    }

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_sc8_item32_1_to_fc64_1_bswap_guts(_)
    }
    else{
        convert_sc8_item32_1_to_fc64_1_bswap_guts(u_)
    }

    //convert remainder
    item32_sc8_to_xx<uhd::ntohx>(input+i, output+j, num_samps-j, scale_factor);
}

DECLARE_CONVERTER(sc8_item32_le, 1, fc64, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(size_t(inputs[0]) & ~0x3);
    fc64_t *output = reinterpret_cast<fc64_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor/(1 << 24));

    size_t i = 0, j = 0;
    fc32_t dummy;
    size_t num_samps = nsamps;

    if ((size_t(inputs[0]) & 0x3) != 0){
        item32_sc8_to_xx<uhd::wtohx>(input++, output++, 1, scale_factor);
        num_samps--;
    }

    #define convert_sc8_item32_1_to_fc64_1_nswap_guts(_al_)             \
    for (; j+7 < num_samps; j+=8, i+=4){                                \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* unpack */                                                    \
        __m128d tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;         \
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8)); /*byteswap*/\
        unpack_sc32_8x(tmpi, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+0), tmp0); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+1), tmp1); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+2), tmp2); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+3), tmp3); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+4), tmp4); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+5), tmp5); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+6), tmp6); \
        _mm_store ## _al_ ## pd(reinterpret_cast<double *>(output+j+7), tmp7); \
    }

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_sc8_item32_1_to_fc64_1_nswap_guts(_)
    }
    else{
        convert_sc8_item32_1_to_fc64_1_nswap_guts(u_)
    }

    //convert remainder
    item32_sc8_to_xx<uhd::wtohx>(input+i, output+j, num_samps-j, scale_factor);
}
