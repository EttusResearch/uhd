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

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>
#include <boost/math/special_functions/round.hpp>
#include <vector>

using namespace uhd::convert;

static const size_t sc16_table_len = size_t(1 << 16);

typedef boost::uint16_t (*tohost16_type)(boost::uint16_t);

/***********************************************************************
 * Implementation for sc16 lookup table
 *  - Lookup the real and imaginary parts individually
 **********************************************************************/
template <typename type, tohost16_type tohost, size_t re_shift, size_t im_shift>
class convert_sc16_item32_1_to_fcxx_1 : public converter{
public:
    convert_sc16_item32_1_to_fcxx_1(void): _table(sc16_table_len){}

    void set_scalar(const double scalar){
        for (size_t i = 0; i < sc16_table_len; i++){
            const boost::uint16_t val = tohost(boost::uint16_t(i & 0xffff));
            _table[i] = type(boost::int16_t(val)*scalar);
        }
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps){
        const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        for (size_t i = 0; i < nsamps; i++){
            const item32_t item = input[i];
            output[i] = std::complex<type>(
                _table[boost::uint16_t(item >> re_shift)],
                _table[boost::uint16_t(item >> im_shift)]
            );
        }
    }

private:
    std::vector<type> _table;
};

/***********************************************************************
 * Implementation for sc8 lookup table
 *  - Lookup the real and imaginary parts together
 **********************************************************************/
template <typename type, tohost16_type tohost, size_t lo_shift, size_t hi_shift>
class convert_sc8_item32_1_to_fcxx_1 : public converter{
public:
    convert_sc8_item32_1_to_fcxx_1(void): _table(sc16_table_len){}

    void set_scalar(const double scalar){

        //special case, this is converts sc8 to sc16,
        //use the scale-factor but no normalization
        if (sizeof(type) == sizeof(s16_t)){
            for (size_t i = 0; i < sc16_table_len; i++){
                const boost::uint16_t val = tohost(boost::uint16_t(i & 0xffff));
                const type real = type(boost::math::iround(boost::int8_t(val >> 8)*scalar*32767));
                const type imag = type(boost::math::iround(boost::int8_t(val >> 0)*scalar*32767));
                _table[i] = std::complex<type>(real, imag);
            }
            return;
        }

        for (size_t i = 0; i < sc16_table_len; i++){
            const boost::uint16_t val = tohost(boost::uint16_t(i & 0xffff));
            const type real = type(boost::int8_t(val >> 8)*scalar);
            const type imag = type(boost::int8_t(val >> 0)*scalar);
            _table[i] = std::complex<type>(real, imag);
        }
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps){
        const item32_t *input = reinterpret_cast<const item32_t *>(size_t(inputs[0]) & ~0x3);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        size_t num_samps = nsamps;

        if ((size_t(inputs[0]) & 0x3) != 0){
            const item32_t item0 = *input++;
            *output++ = _table[boost::uint16_t(item0 >> hi_shift)];
            num_samps--;
        }

        const size_t num_pairs = num_samps/2;
        for (size_t i = 0, j = 0; i < num_pairs; i++, j+=2){
            const item32_t item_i = (input[i]);
            output[j] = _table[boost::uint16_t(item_i >> lo_shift)];
            output[j + 1] = _table[boost::uint16_t(item_i >> hi_shift)];
        }

        if (num_samps != num_pairs*2){
            const item32_t item_n = input[num_pairs];
            output[num_samps-1] = _table[boost::uint16_t(item_n >> lo_shift)];
        }
    }

private:
    std::vector<std::complex<type> > _table;
};

/***********************************************************************
 * Factory functions and registration
 **********************************************************************/

#ifdef BOOST_BIG_ENDIAN
#  define SHIFT_PAIR0 16, 0
#  define SHIFT_PAIR1 0, 16
#else
#  define SHIFT_PAIR0 0, 16
#  define SHIFT_PAIR1 16, 0
#endif

static converter::sptr make_convert_sc16_item32_be_1_to_fc32_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<float, uhd::ntohx, SHIFT_PAIR0>());
}

static converter::sptr make_convert_sc16_item32_be_1_to_fc64_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<double, uhd::ntohx, SHIFT_PAIR0>());
}

static converter::sptr make_convert_sc16_item32_le_1_to_fc32_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<float, uhd::wtohx, SHIFT_PAIR1>());
}

static converter::sptr make_convert_sc16_item32_le_1_to_fc64_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<double, uhd::wtohx, SHIFT_PAIR1>());
}

static converter::sptr make_convert_sc8_item32_be_1_to_fc32_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<float, uhd::ntohx, SHIFT_PAIR1>());
}

static converter::sptr make_convert_sc8_item32_be_1_to_fc64_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<double, uhd::ntohx, SHIFT_PAIR1>());
}

static converter::sptr make_convert_sc8_item32_le_1_to_fc32_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<float, uhd::wtohx, SHIFT_PAIR0>());
}

static converter::sptr make_convert_sc8_item32_le_1_to_fc64_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<double, uhd::wtohx, SHIFT_PAIR0>());
}

static converter::sptr make_convert_sc8_item32_be_1_to_sc16_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<s16_t, uhd::ntohx, SHIFT_PAIR1>());
}

static converter::sptr make_convert_sc8_item32_le_1_to_sc16_1(void){
    return converter::sptr(new convert_sc8_item32_1_to_fcxx_1<s16_t, uhd::wtohx, SHIFT_PAIR0>());
}

UHD_STATIC_BLOCK(register_convert_sc16_item32_1_to_fcxx_1){
    uhd::convert::id_type id;
    id.num_inputs = 1;
    id.num_outputs = 1;

    id.output_format = "fc32";
    id.input_format = "sc16_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc16_item32_be_1_to_fc32_1, PRIORITY_TABLE);

    id.output_format = "fc64";
    id.input_format = "sc16_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc16_item32_be_1_to_fc64_1, PRIORITY_TABLE);

    id.output_format = "fc32";
    id.input_format = "sc16_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc16_item32_le_1_to_fc32_1, PRIORITY_TABLE);

    id.output_format = "fc64";
    id.input_format = "sc16_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc16_item32_le_1_to_fc64_1, PRIORITY_TABLE);

    id.output_format = "fc32";
    id.input_format = "sc8_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_be_1_to_fc32_1, PRIORITY_TABLE);

    id.output_format = "fc64";
    id.input_format = "sc8_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_be_1_to_fc64_1, PRIORITY_TABLE);

    id.output_format = "fc32";
    id.input_format = "sc8_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_le_1_to_fc32_1, PRIORITY_TABLE);

    id.output_format = "fc64";
    id.input_format = "sc8_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_le_1_to_fc64_1, PRIORITY_TABLE);

    id.output_format = "sc16";
    id.input_format = "sc8_item32_be";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_be_1_to_sc16_1, PRIORITY_TABLE);

    id.output_format = "sc16";
    id.input_format = "sc8_item32_le";
    uhd::convert::register_converter(id, &make_convert_sc8_item32_le_1_to_sc16_1, PRIORITY_TABLE);
}
