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

UHD_INLINE __m128i pack_sc8_item32_4x(
    const __m128i &in0, const __m128i &in1,
    const __m128i &in2, const __m128i &in3
){
    const __m128i lo = _mm_packs_epi32(in0, in1);
    const __m128i hi = _mm_packs_epi32(in2, in3);
    return _mm_packs_epi16(lo, hi);
}

UHD_INLINE __m128i pack_sc32_4x_be(
    const __m128d &lo, const __m128d &hi,
    const __m128d &scalar
){
    const __m128i tmpi_lo = _mm_cvttpd_epi32(_mm_mul_pd(hi, scalar));
    const __m128i tmpi_hi = _mm_cvttpd_epi32(_mm_mul_pd(lo, scalar));
    return _mm_unpacklo_epi64(tmpi_lo, tmpi_hi);
}

UHD_INLINE __m128i pack_sc32_4x_le(
    const __m128d &lo, const __m128d &hi,
    const __m128d &scalar
){
    const __m128i tmpi_lo = _mm_cvttpd_epi32(_mm_mul_pd(lo, scalar));
    const __m128i tmpi_hi = _mm_cvttpd_epi32(_mm_mul_pd(hi, scalar));
    const __m128i tmpi = _mm_unpacklo_epi64(tmpi_lo, tmpi_hi);
    return _mm_shuffle_epi32(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
}

DECLARE_CONVERTER(fc64, 1, sc8_item32_be, 1, PRIORITY_SIMD){
    const fc64_t *input = reinterpret_cast<const fc64_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor);

    #define convert_fc64_1_to_sc8_item32_1_bswap_guts(_al_)             \
    for (size_t j = 0; i+7 < nsamps; i+=8, j+=4){                       \
        /* load from input */                                           \
        __m128d tmp0 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+0)); \
        __m128d tmp1 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+1)); \
        __m128d tmp2 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+2)); \
        __m128d tmp3 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+3)); \
        __m128d tmp4 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+4)); \
        __m128d tmp5 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+5)); \
        __m128d tmp6 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+6)); \
        __m128d tmp7 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+7)); \
                                                                        \
        /* interleave */                                                \
        const __m128i tmpi = pack_sc8_item32_4x(                        \
            pack_sc32_4x_be(tmp0, tmp1, scalar),                        \
            pack_sc32_4x_be(tmp2, tmp3, scalar),                        \
            pack_sc32_4x_be(tmp4, tmp5, scalar),                        \
            pack_sc32_4x_be(tmp6, tmp7, scalar)                         \
        );                                                              \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+j), tmpi);  \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(input) & 0xf) == 0){
        convert_fc64_1_to_sc8_item32_1_bswap_guts(_)
    }
    else{
        convert_fc64_1_to_sc8_item32_1_bswap_guts(u_)
    }

    //convert remainder
    const size_t num_pairs = nsamps/2;
    for (size_t j = i/2; j < num_pairs; j++, i+=2){
        const item32_t item = fc64_to_item32_sc8(input[i], input[i+1], scale_factor);
        output[j] = uhd::byteswap(item);
    }

    if (nsamps != num_pairs*2){
        const item32_t item = fc64_to_item32_sc8(input[nsamps-1], 0, scale_factor);
        output[num_pairs] = uhd::byteswap(item);
    }
}

DECLARE_CONVERTER(fc64, 1, sc8_item32_le, 1, PRIORITY_SIMD){
    const fc64_t *input = reinterpret_cast<const fc64_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    const __m128d scalar = _mm_set1_pd(scale_factor);

    #define convert_fc64_1_to_sc8_item32_1_nswap_guts(_al_)             \
    for (size_t j = 0; i+7 < nsamps; i+=8, j+=4){                       \
        /* load from input */                                           \
        __m128d tmp0 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+0)); \
        __m128d tmp1 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+1)); \
        __m128d tmp2 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+2)); \
        __m128d tmp3 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+3)); \
        __m128d tmp4 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+4)); \
        __m128d tmp5 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+5)); \
        __m128d tmp6 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+6)); \
        __m128d tmp7 = _mm_load ## _al_ ## pd(reinterpret_cast<const double *>(input+i+7)); \
                                                                        \
        /* interleave */                                                \
        const __m128i tmpi = pack_sc8_item32_4x(                        \
            pack_sc32_4x_le(tmp0, tmp1, scalar),                        \
            pack_sc32_4x_le(tmp2, tmp3, scalar),                        \
            pack_sc32_4x_le(tmp4, tmp5, scalar),                        \
            pack_sc32_4x_le(tmp6, tmp7, scalar)                         \
        );                                                              \
                                                                        \
        /* store to output */                                           \
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+j), tmpi);  \
    }                                                                   \

    size_t i = 0;

    //dispatch according to alignment
    if ((size_t(input) & 0xf) == 0){
        convert_fc64_1_to_sc8_item32_1_nswap_guts(_)
    }
    else{
        convert_fc64_1_to_sc8_item32_1_nswap_guts(u_)
    }

    //convert remainder
    const size_t num_pairs = nsamps/2;
    for (size_t j = i/2; j < num_pairs; j++, i+=2){
        const item32_t item = fc64_to_item32_sc8(input[i], input[i+1], scale_factor);
        output[j] = (item);
    }

    if (nsamps != num_pairs*2){
        const item32_t item = fc64_to_item32_sc8(input[nsamps-1], 0, scale_factor);
        output[num_pairs] = (item);
    }
}
