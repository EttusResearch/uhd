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

#ifndef INCLUDED_LIBUHD_CONVERT_COMMON_HPP
#define INCLUDED_LIBUHD_CONVERT_COMMON_HPP

#include <uhd/convert.hpp>
#include <uhd/utils/static.hpp>
#include <boost/cstdint.hpp>
#include <complex>

#define _DECLARE_CONVERTER(name, in_form, num_in, out_form, num_out, prio) \
    struct name : public uhd::convert::converter{ \
        static sptr make(void){return sptr(new name());} \
        double scale_factor; \
        void set_scalar(const double s){scale_factor = s;} \
        void operator()(const input_type&, const output_type&, const size_t); \
    }; \
    UHD_STATIC_BLOCK(__register_##name##_##prio){ \
        uhd::convert::id_type id; \
        id.input_format = #in_form; \
        id.num_inputs = num_in; \
        id.output_format = #out_form; \
        id.num_outputs = num_out; \
        uhd::convert::register_converter(id, &name::make, prio); \
    } \
    void name::operator()( \
        const input_type &inputs, const output_type &outputs, const size_t nsamps \
    )

#define DECLARE_CONVERTER(in_form, num_in, out_form, num_out, prio) \
    _DECLARE_CONVERTER(__convert_##in_form##_##num_in##_##out_form##_##num_out##_##prio, in_form, num_in, out_form, num_out, prio)

/***********************************************************************
 * Setup priorities
 **********************************************************************/
static const int PRIORITY_GENERAL = 0;
static const int PRIORITY_EMPTY = -1;

#ifdef __ARM_NEON__
static const int PRIORITY_LIBORC = 3;
static const int PRIORITY_SIMD = 1; //neon conversions could be implemented better, orc wins
static const int PRIORITY_TABLE = 2; //tables require large cache, so they are slower on arm
#else
static const int PRIORITY_LIBORC = 1;
static const int PRIORITY_SIMD = 3;
static const int PRIORITY_TABLE = 2;
#endif

/***********************************************************************
 * Typedefs
 **********************************************************************/
typedef std::complex<double>         fc64_t;
typedef std::complex<float>          fc32_t;
typedef std::complex<boost::int32_t> sc32_t;
typedef std::complex<boost::int16_t> sc16_t;
typedef std::complex<boost::int8_t>  sc8_t;
typedef double                       f64_t;
typedef float                        f32_t;
typedef boost::int32_t               s32_t;
typedef boost::int16_t               s16_t;
typedef boost::int8_t                s8_t;

typedef boost::uint32_t              item32_t;

/***********************************************************************
 * Convert complex short buffer to items32 sc16
 **********************************************************************/
static UHD_INLINE item32_t sc16_to_item32_sc16(sc16_t num, double){
    boost::uint16_t real = num.real();
    boost::uint16_t imag = num.imag();
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

/***********************************************************************
 * Convert items32 sc16 buffer to complex short
 **********************************************************************/
static UHD_INLINE sc16_t item32_sc16_to_sc16(item32_t item, double){
    return sc16_t(
        boost::int16_t(item >> 16),
        boost::int16_t(item >> 0)
    );
}

/***********************************************************************
 * Convert complex float buffer to items32 sc16
 **********************************************************************/
static UHD_INLINE item32_t fc32_to_item32_sc16(fc32_t num, double scale_factor){
    boost::uint16_t real = boost::int16_t(num.real()*float(scale_factor));
    boost::uint16_t imag = boost::int16_t(num.imag()*float(scale_factor));
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

/***********************************************************************
 * Convert items32 sc16 buffer to complex float
 **********************************************************************/
static UHD_INLINE fc32_t item32_sc16_to_fc32(item32_t item, double scale_factor){
    return fc32_t(
        float(boost::int16_t(item >> 16)*float(scale_factor)),
        float(boost::int16_t(item >> 0)*float(scale_factor))
    );
}

/***********************************************************************
 * Convert complex double buffer to items32 sc16
 **********************************************************************/
static UHD_INLINE item32_t fc64_to_item32_sc16(fc64_t num, double scale_factor){
    boost::uint16_t real = boost::int16_t(num.real()*scale_factor);
    boost::uint16_t imag = boost::int16_t(num.imag()*scale_factor);
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

/***********************************************************************
 * Convert items32 sc16 buffer to complex double
 **********************************************************************/
static UHD_INLINE fc64_t item32_sc16_to_fc64(item32_t item, double scale_factor){
    return fc64_t(
        float(boost::int16_t(item >> 16)*scale_factor),
        float(boost::int16_t(item >> 0)*scale_factor)
    );
}

/***********************************************************************
 * Convert items32 sc8 buffer to complex char
 **********************************************************************/
static UHD_INLINE void item32_sc8_to_sc8(item32_t item, sc8_t &out0, sc8_t &out1, double){
    out0 = sc8_t(
        boost::int8_t(item >> 8),
        boost::int8_t(item >> 0)
    );
    out1 = sc8_t(
        boost::int8_t(item >> 24),
        boost::int8_t(item >> 16)
    );
}

/***********************************************************************
 * Convert items32 sc8 buffer to complex short
 **********************************************************************/
static UHD_INLINE void item32_sc8_to_sc16(item32_t item, sc16_t &out0, sc16_t &out1, double){
    out0 = sc16_t(
        boost::int8_t(item >> 8),
        boost::int8_t(item >> 0)
    );
    out1 = sc16_t(
        boost::int8_t(item >> 24),
        boost::int8_t(item >> 16)
    );
}

/***********************************************************************
 * Convert items32 sc8 buffer to complex float
 **********************************************************************/
static UHD_INLINE void item32_sc8_to_fc32(item32_t item, fc32_t &out0, fc32_t &out1, double scale_factor){
    out0 = fc32_t(
        float(boost::int8_t(item >> 8)*float(scale_factor)),
        float(boost::int8_t(item >> 0)*float(scale_factor))
    );
    out1 = fc32_t(
        float(boost::int8_t(item >> 24)*float(scale_factor)),
        float(boost::int8_t(item >> 16)*float(scale_factor))
    );
}

/***********************************************************************
 * Convert items32 sc8 buffer to complex double
 **********************************************************************/
static UHD_INLINE void item32_sc8_to_fc64(item32_t item, fc64_t &out0, fc64_t &out1, double scale_factor){
    out0 = fc64_t(
        float(boost::int8_t(item >> 8)*scale_factor),
        float(boost::int8_t(item >> 0)*scale_factor)
    );
    out1 = fc64_t(
        float(boost::int8_t(item >> 24)*scale_factor),
        float(boost::int8_t(item >> 16)*scale_factor)
    );
}

/***********************************************************************
 * Convert complex char to items32 sc8 buffer
 **********************************************************************/
static UHD_INLINE item32_t sc8_to_item32_sc8(sc8_t in0, sc8_t in1, double){
    boost::uint8_t real0 = boost::int8_t(in0.real());
    boost::uint8_t imag0 = boost::int8_t(in0.imag());
    boost::uint8_t real1 = boost::int8_t(in1.real());
    boost::uint8_t imag1 = boost::int8_t(in1.imag());
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

/***********************************************************************
 * Convert complex short to items32 sc8 buffer
 **********************************************************************/
static UHD_INLINE item32_t sc16_to_item32_sc8(sc16_t in0, sc16_t in1, double){
    boost::uint8_t real0 = boost::int8_t(in0.real());
    boost::uint8_t imag0 = boost::int8_t(in0.imag());
    boost::uint8_t real1 = boost::int8_t(in1.real());
    boost::uint8_t imag1 = boost::int8_t(in1.imag());
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

/***********************************************************************
 * Convert complex float to items32 sc8 buffer
 **********************************************************************/
static UHD_INLINE item32_t fc32_to_item32_sc8(fc32_t in0, fc32_t in1, double scale_factor){
    boost::uint8_t real0 = boost::int8_t(in0.real()*float(scale_factor));
    boost::uint8_t imag0 = boost::int8_t(in0.imag()*float(scale_factor));
    boost::uint8_t real1 = boost::int8_t(in1.real()*float(scale_factor));
    boost::uint8_t imag1 = boost::int8_t(in1.imag()*float(scale_factor));
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

/***********************************************************************
 * Convert complex double to items32 sc8 buffer
 **********************************************************************/
static UHD_INLINE item32_t fc64_to_item32_sc8(fc64_t in0, fc64_t in1, double scale_factor){
    boost::uint8_t real0 = boost::int8_t(in0.real()*(scale_factor));
    boost::uint8_t imag0 = boost::int8_t(in0.imag()*(scale_factor));
    boost::uint8_t real1 = boost::int8_t(in1.real()*(scale_factor));
    boost::uint8_t imag1 = boost::int8_t(in1.imag()*(scale_factor));
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

#endif /* INCLUDED_LIBUHD_CONVERT_COMMON_HPP */
