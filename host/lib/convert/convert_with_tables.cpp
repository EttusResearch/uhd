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
#include <vector>

using namespace uhd::convert;

static const size_t sc16_table_len = size_t(1 << 16);

typedef boost::uint16_t (*tohost16_type)(boost::uint16_t);

/***********************************************************************
 * Implementation for sc16 lookup table
 **********************************************************************/
template <typename type, tohost16_type tohost, size_t real_shift, size_t imag_shift>
class convert_sc16_item32_1_to_fcxx_1 : public converter{
public:
    convert_sc16_item32_1_to_fcxx_1(void): _table(sc16_table_len){}

    void set_scalar(const double scalar){
        for (size_t i = 0; i < sc16_table_len; i++){
            const boost::int16_t val = tohost(boost::uint16_t(i & 0xffff));
            _table[i] = type(val*scalar);
        }
    }

    void operator()(const input_type &inputs, const output_type &outputs, const size_t nsamps){
        const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
        std::complex<type> *output = reinterpret_cast<std::complex<type> *>(outputs[0]);

        for (size_t i = 0; i < nsamps; i++){
            const item32_t item = input[i];
            output[i] = std::complex<type>(
                _table[boost::uint16_t(item >> real_shift)],
                _table[boost::uint16_t(item >> imag_shift)]
            );
        }
    }

private:
    std::vector<type> _table;
};

/***********************************************************************
 * Factory functions and registration
 **********************************************************************/

#ifdef BOOST_BIG_ENDIAN
#  define ITEM32_BE_TO_R16 16
#  define ITEM32_BE_TO_I16 0
#  define ITEM32_LE_TO_R16 0
#  define ITEM32_LE_TO_I16 16
#else
#  define ITEM32_BE_TO_R16 0
#  define ITEM32_BE_TO_I16 16
#  define ITEM32_LE_TO_R16 16
#  define ITEM32_LE_TO_I16 0
#endif

static converter::sptr make_convert_sc16_item32_be_1_to_fc32_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<float, uhd::ntohx, ITEM32_BE_TO_R16, ITEM32_BE_TO_I16>());
}

static converter::sptr make_convert_sc16_item32_be_1_to_fc64_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<double, uhd::ntohx, ITEM32_BE_TO_R16, ITEM32_BE_TO_I16>());
}

static converter::sptr make_convert_sc16_item32_le_1_to_fc32_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<float, uhd::wtohx, ITEM32_LE_TO_R16, ITEM32_LE_TO_I16>());
}

static converter::sptr make_convert_sc16_item32_le_1_to_fc64_1(void){
    return converter::sptr(new convert_sc16_item32_1_to_fcxx_1<double, uhd::wtohx, ITEM32_LE_TO_R16, ITEM32_LE_TO_I16>());
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
}
