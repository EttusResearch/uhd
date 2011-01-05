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

#include "convert_common.hpp"
#include <uhd/utils/byteswap.hpp>

using namespace uhd::convert;

/***********************************************************************
 * Convert complex short buffer to items32
 **********************************************************************/
DECLARE_CONVERTER(convert_sc16_1_to_item32_1_nswap, PRIORITY_GENERAL){
    const sc16_t *input = reinterpret_cast<const sc16_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    for (size_t i = 0; i < nsamps; i++){
        output[i] = sc16_to_item32(input[i]);
    }
}

DECLARE_CONVERTER(convert_sc16_1_to_item32_1_bswap, PRIORITY_GENERAL){
    const sc16_t *input = reinterpret_cast<const sc16_t *>(inputs[0]);
    item32_t *output = reinterpret_cast<item32_t *>(outputs[0]);

    for (size_t i = 0; i < nsamps; i++){
        output[i] = uhd::byteswap(sc16_to_item32(input[i]));
    }
}

/***********************************************************************
 * Convert items32 buffer to complex short
 **********************************************************************/
DECLARE_CONVERTER(convert_item32_1_to_sc16_1_nswap, PRIORITY_GENERAL){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    sc16_t *output = reinterpret_cast<sc16_t *>(outputs[0]);

    for (size_t i = 0; i < nsamps; i++){
        output[i] = item32_to_sc16(input[i]);
    }
}

DECLARE_CONVERTER(convert_item32_1_to_sc16_1_bswap, PRIORITY_GENERAL){
    const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]);
    sc16_t *output = reinterpret_cast<sc16_t *>(outputs[0]);

    for (size_t i = 0; i < nsamps; i++){
        output[i] = item32_to_sc16(uhd::byteswap(input[i]));
    }
}
