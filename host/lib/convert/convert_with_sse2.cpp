//
// Copyright 2011-2011 Ettus Research LLC
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

DECLARE_CONVERTER(convert_fc32_1_to_item32_1_nswap, PRIORITY_CUSTOM){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    __m128 scalar = _mm_set_ps1(shorts_per_float);

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128 tmplo = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+0));
        __m128 tmphi = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+2));

        //convert and scale
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));

        //pack + swap 16-bit pairs
        __m128i tmpi = _mm_packs_epi32(tmpilo, tmpihi);
        tmpi = _mm_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
        tmpi = _mm_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));

        //store to output
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+i), tmpi);
    }

    //convert remainder
    for (; i < nsamps; i++){
        output[i] = fc32_to_item32(input[i]);
    }
}

DECLARE_CONVERTER(convert_fc32_1_to_item32_1_bswap, PRIORITY_CUSTOM){
    const fc32_t *input = reinterpret_cast<const fc32_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    __m128 scalar = _mm_set_ps1(shorts_per_float);

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128 tmplo = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+0));
        __m128 tmphi = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+2));

        //convert and scale
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));

        //pack + byteswap -> byteswap 16 bit words
        __m128i tmpi = _mm_packs_epi32(tmpilo, tmpihi);
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8));

        //store to output
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+i), tmpi);
    }

    //convert remainder
    for (; i < nsamps; i++){
        output[i] = uhd::byteswap(fc32_to_item32(input[i]));
    }
}

DECLARE_CONVERTER(convert_item32_1_to_fc32_1_nswap, PRIORITY_CUSTOM){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    __m128 scalar = _mm_set_ps1(floats_per_short/(1 << 16));
    __m128i zeroi = _mm_setzero_si128();

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i));

        //unpack + swap 16-bit pairs
        tmpi = _mm_shufflelo_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
        tmpi = _mm_shufflehi_epi16(tmpi, _MM_SHUFFLE(2, 3, 0, 1));
        __m128i tmpilo = _mm_unpacklo_epi16(zeroi, tmpi); //value in upper 16 bits
        __m128i tmpihi = _mm_unpackhi_epi16(zeroi, tmpi);

        //convert and scale
        __m128 tmplo = _mm_mul_ps(_mm_cvtepi32_ps(tmpilo), scalar);
        __m128 tmphi = _mm_mul_ps(_mm_cvtepi32_ps(tmpihi), scalar);

        //store to output
        _mm_storeu_ps(reinterpret_cast<float *>(output+i+0), tmplo);
        _mm_storeu_ps(reinterpret_cast<float *>(output+i+2), tmphi);
    }

    //convert remainder
    for (; i < nsamps; i++){
        output[i] = item32_to_fc32(input[i]);
    }
}

DECLARE_CONVERTER(convert_item32_1_to_fc32_1_bswap, PRIORITY_CUSTOM){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    fc32_t *output = reinterpret_cast<fc32_t *>(outputs[0]);

    __m128 scalar = _mm_set_ps1(floats_per_short/(1 << 16));
    __m128i zeroi = _mm_setzero_si128();

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i));

        //byteswap + unpack -> byteswap 16 bit words
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8));
        __m128i tmpilo = _mm_unpacklo_epi16(zeroi, tmpi); //value in upper 16 bits
        __m128i tmpihi = _mm_unpackhi_epi16(zeroi, tmpi);

        //convert and scale
        __m128 tmplo = _mm_mul_ps(_mm_cvtepi32_ps(tmpilo), scalar);
        __m128 tmphi = _mm_mul_ps(_mm_cvtepi32_ps(tmpihi), scalar);

        //store to output
        _mm_storeu_ps(reinterpret_cast<float *>(output+i+0), tmplo);
        _mm_storeu_ps(reinterpret_cast<float *>(output+i+2), tmphi);
    }

    //convert remainder
    for (; i < nsamps; i++){
        output[i] = item32_to_fc32(uhd::byteswap(input[i]));
    }
}
