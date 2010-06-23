//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_TRANSPORT_CONVERT_TYPES_IMPL_HPP
#define INCLUDED_LIBUHD_TRANSPORT_CONVERT_TYPES_IMPL_HPP

#include <uhd/config.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/cstdint.hpp>
#include <cstring>
#include <complex>

#ifdef HAVE_EMMINTRIN_H
    #define USE_EMMINTRIN_H //use sse2 intrinsics
#endif

/***********************************************************************
 * Typedefs
 **********************************************************************/
typedef std::complex<float>          fc32_t;
typedef std::complex<boost::int16_t> sc16_t;
typedef boost::uint32_t              item32_t;

/***********************************************************************
 * Convert complex short buffer to items32
 **********************************************************************/
static UHD_INLINE void sc16_to_item32_nswap(
    const sc16_t *input, item32_t *output, size_t nsamps
){
    std::memcpy(output, input, nsamps*sizeof(item32_t));
}

static UHD_INLINE void sc16_to_item32_bswap(
    const sc16_t *input, item32_t *output, size_t nsamps
){
    const item32_t *item32_input = (const item32_t *)input;
    for (size_t i = 0; i < nsamps; i++){
        output[i] = uhd::byteswap(item32_input[i]);
    }
}

/***********************************************************************
 * Convert items32 buffer to complex short
 **********************************************************************/
static UHD_INLINE void item32_to_sc16_nswap(
    const item32_t *input, sc16_t *output, size_t nsamps
){
    std::memcpy(output, input, nsamps*sizeof(item32_t));
}

static UHD_INLINE void item32_to_sc16_bswap(
    const item32_t *input, sc16_t *output, size_t nsamps
){
    item32_t *item32_output = (item32_t *)output;
    for (size_t i = 0; i < nsamps; i++){
        item32_output[i] = uhd::byteswap(input[i]);
    }
}

/***********************************************************************
 * Convert complex float buffer to items32
 **********************************************************************/
static const float shorts_per_float = float(32767);

static UHD_INLINE item32_t fc32_to_item32(fc32_t num){
    boost::uint16_t real = boost::int16_t(num.real()*shorts_per_float);
    boost::uint16_t imag = boost::int16_t(num.imag()*shorts_per_float);
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

static UHD_INLINE void fc32_to_item32_nswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        output[i] = fc32_to_item32(input[i]);
    }
}

#if defined(USE_EMMINTRIN_H)
#include <emmintrin.h>

static UHD_INLINE void fc32_to_item32_bswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    __m128 scalar = _mm_set_ps1(shorts_per_float);

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < nsamps/4; i+=4){
        //load from input
        __m128 tmplo = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+0));
        __m128 tmphi = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+2));

        //convert and scale
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));

        //pack + byteswap -> byteswap 32 bit words
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

#else
static UHD_INLINE void fc32_to_item32_bswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        output[i] = uhd::byteswap(fc32_to_item32(input[i]));
    }
}

#endif

/***********************************************************************
 * Convert items32 buffer to complex float
 **********************************************************************/
static const float floats_per_short = float(1.0/shorts_per_float);

static UHD_INLINE fc32_t item32_to_fc32(item32_t item){
    return fc32_t(
        float(boost::int16_t(item >> 16)*floats_per_short),
        float(boost::int16_t(item >> 0)*floats_per_short)
    );
}

static UHD_INLINE void item32_to_fc32_nswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        output[i] = item32_to_fc32(input[i]);
    }
}

#if defined(USE_EMMINTRIN_H)
#include <emmintrin.h>

static UHD_INLINE void item32_to_fc32_bswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    __m128 scalar = _mm_set_ps1(floats_per_short/(1 << 16));

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < nsamps/4; i+=4){
        //load from input
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i));

        //byteswap + unpack -> byteswap 32 bit words
        tmpi = _mm_or_si128(_mm_srli_epi16(tmpi, 8), _mm_slli_epi16(tmpi, 8));
        __m128i tmpilo = _mm_unpacklo_epi16(tmpi, tmpi);
        __m128i tmpihi = _mm_unpackhi_epi16(tmpi, tmpi);

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

#else
static UHD_INLINE void item32_to_fc32_bswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        output[i] = item32_to_fc32(uhd::byteswap(input[i]));
    }
}

#endif

#endif /* INCLUDED_LIBUHD_TRANSPORT_CONVERT_TYPES_IMPL_HPP */
