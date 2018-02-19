//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <boost/math/special_functions/round.hpp>
#include <vector>

using namespace uhd::convert;

typedef uint32_t (*to32_type)(uint32_t);

template <typename type, to32_type tohost>
struct convert_fc32_item32_1_to_star_1 : public converter
{
    convert_fc32_item32_1_to_star_1(void):_scalar(0.0)
    {
        //NOP
    }

    void set_scalar(const double scalar)
    {
        _scalar = scalar;
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        size_t i = 0;
        for (size_t o = 0; o < nsamps; o++)
        {
            const item32_t i32 = tohost(input[i++]);
            const item32_t q32 = tohost(input[i++]);
            const float *i_f32p = reinterpret_cast<const float *>(&i32);
            const float *q_f32p = reinterpret_cast<const float *>(&q32);
            output[o] = std::complex<type>(type((*i_f32p)*_scalar), type((*q_f32p)*_scalar));
        }
    }

    double _scalar;
};

template <typename type, to32_type towire>
struct convert_star_1_to_fc32_item32_1 : public converter
{
    convert_star_1_to_fc32_item32_1(void):_scalar(0.0)
    {
        //NOP
    }

    void set_scalar(const double scalar)
    {
        _scalar = scalar;
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps)
    {
        const std::complex<type> *input = reinterpret_cast<const std::complex<type> *>(inputs[0]);
        item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

        size_t o = 0;
        for (size_t i = 0; i < nsamps; i++)
        {
            const float i_f32 = type(input[i].real()*_scalar);
            const float q_f32 = type(input[i].imag()*_scalar);
            const item32_t *i32p = reinterpret_cast<const item32_t *>(&i_f32);
            const item32_t *q32p = reinterpret_cast<const item32_t *>(&q_f32);
            output[o++] = towire(*i32p);
            output[o++] = towire(*q32p);
        }
    }

    double _scalar;
};

#define __make_registrations(itype, otype, fcn, type, conv) \
static converter::sptr make_convert_ ## itype ## _1_ ## otype ## _1(void) \
{ \
    return converter::sptr(new fcn<type, conv>()); \
} \
UHD_STATIC_BLOCK(register_convert_ ## itype ## _1_ ## otype ## _1) \
{ \
    uhd::convert::id_type id; \
    id.num_inputs = 1; id.num_outputs = 1;  \
    id.input_format = #itype; id.output_format = #otype; \
    uhd::convert::register_converter(id, &make_convert_ ## itype ## _1_ ## otype ## _1, PRIORITY_GENERAL); \
}

__make_registrations(fc32_item32_le, fc32, convert_fc32_item32_1_to_star_1, float, uhd::wtohx)
__make_registrations(fc32_item32_be, fc32, convert_fc32_item32_1_to_star_1, float, uhd::ntohx)
__make_registrations(fc32_item32_le, fc64, convert_fc32_item32_1_to_star_1, double, uhd::wtohx)
__make_registrations(fc32_item32_be, fc64, convert_fc32_item32_1_to_star_1, double, uhd::ntohx)

__make_registrations(fc32, fc32_item32_le, convert_star_1_to_fc32_item32_1, float, uhd::wtohx)
__make_registrations(fc32, fc32_item32_be, convert_star_1_to_fc32_item32_1, float, uhd::ntohx)
__make_registrations(fc64, fc32_item32_le, convert_star_1_to_fc32_item32_1, double, uhd::wtohx)
__make_registrations(fc64, fc32_item32_be, convert_star_1_to_fc32_item32_1, double, uhd::ntohx)
