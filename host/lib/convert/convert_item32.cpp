//
// Copyright 2012 Ettus Research LLC
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

#define __DECLARE_ITEM32_CONVERTER(cpu_type, wire_type, xe, htoxx, xxtoh) \
    DECLARE_CONVERTER(cpu_type, 1, wire_type ## _item32_ ## xe, 1, PRIORITY_GENERAL){ \
        const cpu_type ## _t *input = reinterpret_cast<const cpu_type ## _t *>(inputs[0]); \
        item32_t *output = reinterpret_cast<item32_t *>(outputs[0]); \
        xx_to_item32_ ## wire_type<htoxx>(input, output, nsamps, scale_factor); \
    } \
    DECLARE_CONVERTER(wire_type ## _item32_ ## xe, 1, cpu_type, 1, PRIORITY_GENERAL){ \
        const item32_t *input = reinterpret_cast<const item32_t *>(inputs[0]); \
        cpu_type ## _t *output = reinterpret_cast<cpu_type ## _t *>(outputs[0]); \
        item32_ ## wire_type ## _to_xx<xxtoh>(input, output, nsamps, scale_factor); \
    }

#define _DECLARE_ITEM32_CONVERTER(cpu_type, wire_type) \
    __DECLARE_ITEM32_CONVERTER(cpu_type, wire_type, be, uhd::htonx, uhd::ntohx) \
    __DECLARE_ITEM32_CONVERTER(cpu_type, wire_type, le, uhd::htowx, uhd::wtohx)

#define DECLARE_ITEM32_CONVERTER(cpu_type) \
    _DECLARE_ITEM32_CONVERTER(cpu_type, sc8) \
    _DECLARE_ITEM32_CONVERTER(cpu_type, sc16)

DECLARE_ITEM32_CONVERTER(sc16)
DECLARE_ITEM32_CONVERTER(fc32)
DECLARE_ITEM32_CONVERTER(fc64)
_DECLARE_ITEM32_CONVERTER(sc8, sc8)
