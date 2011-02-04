//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_CONVERT_COMMON_HPP
#define INCLUDED_LIBUHD_CONVERT_COMMON_HPP

#include <uhd/convert.hpp>
#include <uhd/utils/static.hpp>
#include <boost/cstdint.hpp>
#include <complex>

#define DECLARE_CONVERTER(fcn, prio) \
    static void fcn( \
        uhd::convert::input_type &inputs, \
        uhd::convert::output_type &outputs, \
        size_t nsamps \
    ); \
    UHD_STATIC_BLOCK(register_##fcn##_##prio){ \
        uhd::convert::register_converter(#fcn, fcn, prio); \
    } \
    static void fcn( \
        uhd::convert::input_type &inputs, \
        uhd::convert::output_type &outputs, \
        size_t nsamps \
    )

/***********************************************************************
 * Typedefs
 **********************************************************************/
typedef std::complex<double>         fc64_t;
typedef std::complex<float>          fc32_t;
typedef std::complex<boost::int16_t> sc16_t;
typedef std::complex<boost::int8_t>  sc8_t;
typedef boost::uint32_t              item32_t;

/***********************************************************************
 * Convert complex short buffer to items32
 **********************************************************************/
static UHD_INLINE item32_t sc16_to_item32(sc16_t num){
    boost::uint16_t real = num.real();
    boost::uint16_t imag = num.imag();
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

/***********************************************************************
 * Convert items32 buffer to complex short
 **********************************************************************/
static UHD_INLINE sc16_t item32_to_sc16(item32_t item){
    return sc16_t(
        boost::int16_t(item >> 16),
        boost::int16_t(item >> 0)
    );
}

/***********************************************************************
 * Convert complex float buffer to items32 (no swap)
 **********************************************************************/
static const float shorts_per_float = float(32767);

static UHD_INLINE item32_t fc32_to_item32(fc32_t num){
    boost::uint16_t real = boost::int16_t(num.real()*shorts_per_float);
    boost::uint16_t imag = boost::int16_t(num.imag()*shorts_per_float);
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

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

/***********************************************************************
 * Convert complex double buffer to items32 (no swap)
 **********************************************************************/
static const double shorts_per_double = double(32767);

static UHD_INLINE item32_t fc64_to_item32(fc64_t num){
    boost::uint16_t real = boost::int16_t(num.real()*shorts_per_double);
    boost::uint16_t imag = boost::int16_t(num.imag()*shorts_per_double);
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

/***********************************************************************
 * Convert items32 buffer to complex double
 **********************************************************************/
static const double doubles_per_short = double(1.0/shorts_per_double);

static UHD_INLINE fc64_t item32_to_fc64(item32_t item){
    return fc64_t(
        float(boost::int16_t(item >> 16)*doubles_per_short),
        float(boost::int16_t(item >> 0)*doubles_per_short)
    );
}

#endif /* INCLUDED_LIBUHD_CONVERT_COMMON_HPP */
