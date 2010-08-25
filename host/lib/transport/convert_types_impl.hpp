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

#if defined(USE_EMMINTRIN_H)
    #include <emmintrin.h>
#endif

//! shortcut for a byteswap16 with casting
#define BSWAP16_C(num) uhd::byteswap(boost::uint16_t(num))

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
    for (size_t i = 0; i < nsamps; i++){
        boost::uint16_t real = BSWAP16_C(input[i].real());
        boost::uint16_t imag = BSWAP16_C(input[i].imag());
        output[i] = (item32_t(real) << 0) | (item32_t(imag) << 16);
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
    for (size_t i = 0; i < nsamps; i++){
        boost::int16_t real = BSWAP16_C(input[i] >> 0);
        boost::int16_t imag = BSWAP16_C(input[i] >> 16);
        output[i] = sc16_t(real, imag);
    }
}

/***********************************************************************
 * Convert complex float buffer to items32 (no swap)
 **********************************************************************/
static const float shorts_per_float = float(32767);

#define FC32_TO_SC16_C(num) boost::int16_t(num*shorts_per_float)

////////////////////////////////////
// none-swap
////////////////////////////////////
#if defined(USE_EMMINTRIN_H)
static UHD_INLINE void fc32_to_item32_nswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    __m128 scalar = _mm_set_ps1(shorts_per_float);

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128 tmplo = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+0));
        __m128 tmphi = _mm_loadu_ps(reinterpret_cast<const float *>(input+i+2));

        //convert and scale
        __m128i tmpilo = _mm_cvtps_epi32(_mm_mul_ps(tmplo, scalar));
        __m128i tmpihi = _mm_cvtps_epi32(_mm_mul_ps(tmphi, scalar));

        //pack
        __m128i tmpi = _mm_packs_epi32(tmpilo, tmpihi);

        //store to output
        _mm_storeu_si128(reinterpret_cast<__m128i *>(output+i), tmpi);
    }

    //convert remainder
    for (; i < nsamps; i++){
        boost::uint16_t real = FC32_TO_SC16_C(input[i].real());
        boost::uint16_t imag = FC32_TO_SC16_C(input[i].imag());
        output[i] = (item32_t(real) << 0) | (item32_t(imag) << 16);
    }
}

#else
static UHD_INLINE void fc32_to_item32_nswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        boost::uint16_t real = FC32_TO_SC16_C(input[i].real());
        boost::uint16_t imag = FC32_TO_SC16_C(input[i].imag());
        output[i] = (item32_t(real) << 0) | (item32_t(imag) << 16);
    }
}

#endif

////////////////////////////////////
// byte-swap
////////////////////////////////////
#if defined(USE_EMMINTRIN_H)
static UHD_INLINE void fc32_to_item32_bswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
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
        boost::uint16_t real = BSWAP16_C(FC32_TO_SC16_C(input[i].real()));
        boost::uint16_t imag = BSWAP16_C(FC32_TO_SC16_C(input[i].imag()));
        output[i] = (item32_t(real) << 0) | (item32_t(imag) << 16);
    }
}

#else
static UHD_INLINE void fc32_to_item32_bswap(
    const fc32_t *input, item32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        boost::uint16_t real = BSWAP16_C(FC32_TO_SC16_C(input[i].real()));
        boost::uint16_t imag = BSWAP16_C(FC32_TO_SC16_C(input[i].imag()));
        output[i] = (item32_t(real) << 0) | (item32_t(imag) << 16);
    }
}

#endif

/***********************************************************************
 * Convert items32 buffer to complex float
 **********************************************************************/
static const float floats_per_short = float(1.0/shorts_per_float);

#define I16_TO_FC32_C(num) (boost::int16_t(num)*floats_per_short)

////////////////////////////////////
// none-swap
////////////////////////////////////
#if defined(USE_EMMINTRIN_H)
static UHD_INLINE void item32_to_fc32_nswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    __m128 scalar = _mm_set_ps1(floats_per_short/(1 << 16));
    __m128i zeroi = _mm_setzero_si128();

    //convert blocks of samples with intrinsics
    size_t i = 0; for (; i < (nsamps & ~0x3); i+=4){
        //load from input
        __m128i tmpi = _mm_loadu_si128(reinterpret_cast<const __m128i *>(input+i));

        //unpack
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
        float real = I16_TO_FC32_C(input[i] >> 0);
        float imag = I16_TO_FC32_C(input[i] >> 16);
        output[i] = fc32_t(real, imag);
    }
}

#else
static UHD_INLINE void item32_to_fc32_nswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        float real = I16_TO_FC32_C(input[i] >> 0);
        float imag = I16_TO_FC32_C(input[i] >> 16);
        output[i] = fc32_t(real, imag);
    }
}
#endif

////////////////////////////////////
// byte-swap
////////////////////////////////////
#if defined(USE_EMMINTRIN_H)
static UHD_INLINE void item32_to_fc32_bswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
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
        float real = I16_TO_FC32_C(BSWAP16_C(input[i] >> 0));
        float imag = I16_TO_FC32_C(BSWAP16_C(input[i] >> 16));
        output[i] = fc32_t(real, imag);
    }
}

#else
static UHD_INLINE void item32_to_fc32_bswap(
    const item32_t *input, fc32_t *output, size_t nsamps
){
    for (size_t i = 0; i < nsamps; i++){
        float real = I16_TO_FC32_C(BSWAP16_C(input[i] >> 0));
        float imag = I16_TO_FC32_C(BSWAP16_C(input[i] >> 16));
        output[i] = fc32_t(real, imag);
    }
}

#endif

#endif /* INCLUDED_LIBUHD_TRANSPORT_CONVERT_TYPES_IMPL_HPP */
