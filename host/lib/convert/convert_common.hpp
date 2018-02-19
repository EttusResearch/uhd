//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_CONVERT_COMMON_HPP
#define INCLUDED_LIBUHD_CONVERT_COMMON_HPP

#include <uhd/convert.hpp>
#include <uhd/utils/static.hpp>
#include <stdint.h>
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

/*! Convenience macro to declare a single-function converter
 *
 * Most converters consist of a single for loop, and can make use of
 * this macro for declaration and registering.
 *
 * Following this macro should be a function block in curly braces
 * which runs the conversion. Available parameters in this function block
 * are:
 * - `inputs`: Vector of pointers to the input data. Size of the vector == `num_in`
 * - `outputs`: Vector of pointers to where the output data goes. Size of the vector == `num_out`
 * - `nsamps`: Number of items per input buffer to convert
 * - `scale_factor`: Scaling factor for float conversions
 */
#define DECLARE_CONVERTER(in_form, num_in, out_form, num_out, prio) \
    _DECLARE_CONVERTER(__convert_##in_form##_##num_in##_##out_form##_##num_out##_##prio, in_form, num_in, out_form, num_out, prio)

/***********************************************************************
 * Setup priorities
 **********************************************************************/
static const int PRIORITY_GENERAL = 0;
static const int PRIORITY_EMPTY = -1;

#ifdef __ARM_NEON__
static const int PRIORITY_SIMD = 2;
static const int PRIORITY_TABLE = 1; //tables require large cache, so they are slower on arm
#else
// We used to have ORC, too, so SIMD is 3
static const int PRIORITY_SIMD = 3;
static const int PRIORITY_TABLE = 1;
#endif

/***********************************************************************
 * Typedefs
 **********************************************************************/
typedef std::complex<double>         fc64_t;
typedef std::complex<float>          fc32_t;
typedef std::complex<int32_t> sc32_t;
typedef std::complex<int16_t> sc16_t;
typedef std::complex<int8_t>  sc8_t;
typedef double                       f64_t;
typedef float                        f32_t;
typedef int32_t               s32_t;
typedef int16_t               s16_t;
typedef int8_t                s8_t;
typedef uint8_t               u8_t;

typedef uint32_t              item32_t;

typedef item32_t (*xtox_t)(item32_t);

/***********************************************************************
 * Convert xx to items32 sc16 buffer
 **********************************************************************/
template <typename T> UHD_INLINE item32_t xx_to_item32_sc16_x1(
    const std::complex<T> &num, const double scale_factor
){
    uint16_t real = int16_t(num.real()*float(scale_factor));
    uint16_t imag = int16_t(num.imag()*float(scale_factor));
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

template <> UHD_INLINE item32_t xx_to_item32_sc16_x1(
    const sc16_t &num, const double
){
    uint16_t real = int16_t(num.real());
    uint16_t imag = int16_t(num.imag());
    return (item32_t(real) << 16) | (item32_t(imag) << 0);
}

template <xtox_t to_wire, typename T>
UHD_INLINE void xx_to_item32_sc16(
    const std::complex<T> *input,
    item32_t *output,
    const size_t nsamps,
    const double scale_factor
){
    for (size_t i = 0; i < nsamps; i++){
        const item32_t item = xx_to_item32_sc16_x1(input[i], scale_factor);
        output[i] = to_wire(item);
    }
}

/***********************************************************************
 * Convert items32 sc16 buffer to xx
 **********************************************************************/
template <typename T> UHD_INLINE std::complex<T> item32_sc16_x1_to_xx(
    const item32_t item, const double scale_factor
){
    return std::complex<T>(
        T(int16_t(item >> 16)*float(scale_factor)),
        T(int16_t(item >> 0)*float(scale_factor))
    );
}

template <> UHD_INLINE sc16_t item32_sc16_x1_to_xx(
    const item32_t item, const double
){
    return sc16_t(
        int16_t(item >> 16), int16_t(item >> 0)
    );
}

template <xtox_t to_host, typename T>
UHD_INLINE void item32_sc16_to_xx(
    const item32_t *input,
    std::complex<T> *output,
    const size_t nsamps,
    const double scale_factor
){
    for (size_t i = 0; i < nsamps; i++){
        const item32_t item_i = to_host(input[i]);
        output[i] = item32_sc16_x1_to_xx<T>(item_i, scale_factor);
    }
}

/***********************************************************************
 * Convert xx to items32 sc8 buffer
 **********************************************************************/
template <typename T> UHD_INLINE item32_t xx_to_item32_sc8_x1(
    const std::complex<T> &in0, const std::complex<T> &in1, const double scale_factor
){
    uint8_t real1 = int8_t(in0.real()*float(scale_factor));
    uint8_t imag1 = int8_t(in0.imag()*float(scale_factor));
    uint8_t real0 = int8_t(in1.real()*float(scale_factor));
    uint8_t imag0 = int8_t(in1.imag()*float(scale_factor));
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

template <> UHD_INLINE item32_t xx_to_item32_sc8_x1(
    const sc16_t &in0, const sc16_t &in1, const double
){
    uint8_t real1 = int8_t(in0.real());
    uint8_t imag1 = int8_t(in0.imag());
    uint8_t real0 = int8_t(in1.real());
    uint8_t imag0 = int8_t(in1.imag());
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

template <> UHD_INLINE item32_t xx_to_item32_sc8_x1(
    const sc8_t &in0, const sc8_t &in1, const double
){
    uint8_t real1 = int8_t(in0.real());
    uint8_t imag1 = int8_t(in0.imag());
    uint8_t real0 = int8_t(in1.real());
    uint8_t imag0 = int8_t(in1.imag());
    return
        (item32_t(real0) << 8) | (item32_t(imag0) << 0) |
        (item32_t(real1) << 24) | (item32_t(imag1) << 16)
    ;
}

template <xtox_t to_wire, typename T>
UHD_INLINE void xx_to_item32_sc8(
    const std::complex<T> *input,
    item32_t *output,
    const size_t nsamps,
    const double scale_factor
){
    const size_t num_pairs = nsamps/2;
    for (size_t i = 0, j = 0; i < num_pairs; i++, j+=2){
        const item32_t item = xx_to_item32_sc8_x1(input[j], input[j+1], scale_factor);
        output[i] = to_wire(item);
    }

    if (nsamps != num_pairs*2){
        const item32_t item = xx_to_item32_sc8_x1(input[nsamps-1], std::complex<T>(0), scale_factor);
        output[num_pairs] = to_wire(item);
    }
}

/***********************************************************************
 * Convert items32 sc8 buffer to xx
 **********************************************************************/
template <typename T> UHD_INLINE void item32_sc8_x1_to_xx(
    const item32_t item, std::complex<T> &out0, std::complex<T> &out1, const double scale_factor
){
    out1 = std::complex<T>(
        T(int8_t(item >> 8)*float(scale_factor)),
        T(int8_t(item >> 0)*float(scale_factor))
    );
    out0 = std::complex<T>(
        T(int8_t(item >> 24)*float(scale_factor)),
        T(int8_t(item >> 16)*float(scale_factor))
    );
}

template <> UHD_INLINE void item32_sc8_x1_to_xx(
    const item32_t item, sc16_t &out0, sc16_t &out1, const double
){
    out1 = sc16_t(
        int16_t(int8_t(item >> 8)),
        int16_t(int8_t(item >> 0))
    );
    out0 = sc16_t(
        int16_t(int8_t(item >> 24)),
        int16_t(int8_t(item >> 16))
    );
}

template <> UHD_INLINE void item32_sc8_x1_to_xx(
    const item32_t item, sc8_t &out0, sc8_t &out1, const double
){
    out1 = sc8_t(
        int8_t(int8_t(item >> 8)),
        int8_t(int8_t(item >> 0))
    );
    out0 = sc8_t(
        int8_t(int8_t(item >> 24)),
        int8_t(int8_t(item >> 16))
    );
}

template <xtox_t to_host, typename T>
UHD_INLINE void item32_sc8_to_xx(
    const item32_t *input,
    std::complex<T> *output,
    const size_t nsamps,
    const double scale_factor
){
    input = reinterpret_cast<const item32_t *>(size_t(input) & ~0x3);
    std::complex<T> dummy;
    size_t num_samps = nsamps;

    if ((size_t(input) & 0x3) != 0){
        const item32_t item0 = to_host(*input++);
        item32_sc8_x1_to_xx(item0, dummy, *output++, scale_factor);
        num_samps--;
    }

    const size_t num_pairs = num_samps/2;
    for (size_t i = 0, j = 0; i < num_pairs; i++, j+=2){
        const item32_t item_i = to_host(input[i]);
        item32_sc8_x1_to_xx(item_i, output[j], output[j+1], scale_factor);
    }

    if (num_samps != num_pairs*2){
        const item32_t item_n = to_host(input[num_pairs]);
        item32_sc8_x1_to_xx(item_n, output[num_samps-1], dummy, scale_factor);
    }
}

#endif /* INCLUDED_LIBUHD_CONVERT_COMMON_HPP */
