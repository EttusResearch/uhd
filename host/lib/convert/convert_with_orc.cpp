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
#include <emmintrin.h>

using namespace uhd::convert;

extern "C" {
extern void _convert_fc32_1_to_item32_1_nswap_orc(void *, const void *, float, int);
extern void _convert_fc32_1_to_item32_1_bswap_orc(void *, const void *, float, int);
extern void _convert_item32_1_to_fc32_1_nswap_orc(void *, const void *, float, int);
extern void _convert_item32_1_to_fc32_1_bswap_orc(void *, const void *, float, int);
extern void _convert_sc16_1_to_item32_1_nswap_orc(void *, const void *, float, int);
extern void _convert_item32_1_to_sc16_1_nswap_orc(void *, const void *, float, int);
}

DECLARE_CONVERTER(convert_fc32_1_to_item32_1_nswap, PRIORITY_LIBORC){
    _convert_fc32_1_to_item32_1_nswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}

DECLARE_CONVERTER(convert_fc32_1_to_item32_1_bswap, PRIORITY_LIBORC){
    _convert_fc32_1_to_item32_1_bswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}

DECLARE_CONVERTER(convert_item32_1_to_fc32_1_nswap, PRIORITY_LIBORC){
    _convert_item32_1_to_fc32_1_nswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}

DECLARE_CONVERTER(convert_item32_1_to_fc32_1_bswap, PRIORITY_LIBORC){
    _convert_item32_1_to_fc32_1_bswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}

DECLARE_CONVERTER(convert_sc16_1_to_item32_1_nswap, PRIORITY_LIBORC){
    _convert_sc16_1_to_item32_1_nswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}

DECLARE_CONVERTER(convert_item32_1_to_sc16_1_nswap, PRIORITY_LIBORC){
    _convert_item32_1_to_sc16_1_nswap_orc(outputs[0], inputs[0], scale_factor, nsamps);
}
