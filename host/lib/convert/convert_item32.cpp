//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

/* Create sc16<->sc16,sc8(otw) */
DECLARE_ITEM32_CONVERTER(sc16)
/* Create fc32<->sc16,sc8(otw) */
DECLARE_ITEM32_CONVERTER(fc32)
/* Create fc64<->sc16,sc8(otw) */
DECLARE_ITEM32_CONVERTER(fc64)
_DECLARE_ITEM32_CONVERTER(sc8, sc8)
