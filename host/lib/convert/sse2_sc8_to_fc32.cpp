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

template <const int shuf>
UHD_INLINE void unpack_sc32_4x(
    const __m128i &in,
    __m128 &out0, __m128 &out1,
    __m128 &out2, __m128 &out3,
    const __m128 &scalar
){
    const __m128i tmplo = _mm_unpacklo_epi8(zeroi, in); /* value in upper 8 bits */
    __m128i tmp0 = _mm_shuffle_epi32(_mm_unpacklo_epi16(zeroi, tmplo), shuf); /* value in upper 16 bits */
    __m128i tmp1 = _mm_shuffle_epi32(_mm_unpackhi_epi16(zeroi, tmplo), shuf);
    out0 = _mm_mul_ps(_mm_cvtepi32_ps(tmp0), scalar);
    out1 = _mm_mul_ps(_mm_cvtepi32_ps(tmp1), scalar);

    const __m128i tmphi = _mm_unpackhi_epi8(zeroi, in);
    __m128i tmp2 = _mm_shuffle_epi32(_mm_unpacklo_epi16(zeroi, tmphi), shuf);
    __m128i tmp3 = _mm_shuffle_epi32(_mm_unpackhi_epi16(zeroi, tmphi), shuf);
    out2 = _mm_mul_ps(_mm_cvtepi32_ps(tmp2), scalar);
    out3 = _mm_mul_ps(_mm_cvtepi32_ps(tmp3), scalar);
}

DECLARE_CONVERTER(sc8_item32_be, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(size_t(inputs[0]) & ~0x3);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor)/(1 << 24));
    const int shuf = _MM_SHUFFLE(1, 0, 3, 2);

    size_t i = 0, j = 0;
    fc32_t dummy;
    size_t num_samps = nsamps;

    if ((size_t(inputs[0]) & 0x3) != 0){
        item32_sc8_to_xx<uhd::ntohx>(input++, output++, 1, scale_factor);
        num_samps--;
    }

    #define convert_sc8_item32_1_to_fc32_1_bswap_guts(_al_)             \
    for (; j+7 < num_samps; j+=8, i+=4){                                \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* unpack + swap 8-bit pairs */                                 \
        __m128 tmp0, tmp1, tmp2, tmp3;                                  \
        unpack_sc32_4x<shuf>(tmpi, tmp0, tmp1, tmp2, tmp3, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+0), tmp0); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+2), tmp1); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+4), tmp2); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+6), tmp3); \
    }

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_sc8_item32_1_to_fc32_1_bswap_guts(_)
    }
    else{
        convert_sc8_item32_1_to_fc32_1_bswap_guts(u_)
    }

    //convert remainder
    item32_sc8_to_xx<uhd::ntohx>(input+i, output+j, num_samps-j, scale_factor);
}

DECLARE_CONVERTER(sc8_item32_le, 1, fc32, 1, PRIORITY_SIMD){
    const item32_t *input = reinterpret_cast<const item32_t *>(size_t(inputs[0]) & ~0x3);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    const __m128 scalar = _mm_set_ps1(float(scale_factor)/(1 << 24));
    const int shuf = _MM_SHUFFLE(2, 3, 0, 1);

    size_t i = 0, j = 0;
    fc32_t dummy;
    size_t num_samps = nsamps;

    if ((size_t(inputs[0]) & 0x3) != 0){
        item32_sc8_to_xx<uhd::wtohx>(input++, output++, 1, scale_factor);
        num_samps--;
    }

    #define convert_sc8_item32_1_to_fc32_1_nswap_guts(_al_)             \
    for (; j+7 < num_samps; j+=8, i+=4){                                \
        /* load from input */                                           \
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i)); \
                                                                        \
        /* unpack + swap 8-bit pairs */                                 \
        __m128 tmp0, tmp1, tmp2, tmp3;                                  \
        unpack_sc32_4x<shuf>(tmpi, tmp0, tmp1, tmp2, tmp3, scalar); \
                                                                        \
        /* store to output */                                           \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+0), tmp0); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+2), tmp1); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+4), tmp2); \
        _mm_store ## _al_ ## ps(reinterpret_cast<float *>(output+j+6), tmp3); \
    }

    //dispatch according to alignment
    if ((size_t(output) & 0xf) == 0){
        convert_sc8_item32_1_to_fc32_1_nswap_guts(_)
    }
    else{
        convert_sc8_item32_1_to_fc32_1_nswap_guts(u_)
    }

    //convert remainder
    item32_sc8_to_xx<uhd::wtohx>(input+i, output+j, num_samps-j, scale_factor);
}
