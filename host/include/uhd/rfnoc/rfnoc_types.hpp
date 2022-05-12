//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstddef>
#include <cstdint>

namespace uhd { namespace rfnoc {

//-----------------------------------------------------------------------------
// Types (note: internal types are defined in rfnoc_common.hpp)
//-----------------------------------------------------------------------------

//! Type that indicates the CHDR Width in bits
enum chdr_w_t { CHDR_W_64 = 0, CHDR_W_128 = 1, CHDR_W_256 = 2, CHDR_W_512 = 3 };
//! Conversion from chdr_w_t to a number of bits
constexpr size_t chdr_w_to_bits(chdr_w_t chdr_w)
{
    // clang-format off
    // This switch statement is what this function is doing, but it requires all
    // consumers of this file to use C++14, which we currently enforce. We
    // therefore keep this switch statement for reference, and for future
    // improvements of this code, but what we feed the compiler is the nested
    // ternary statement below.
    /*
    switch (chdr_w) {
        case CHDR_W_64:
            return 64;
        case CHDR_W_128:
            return 128;
        case CHDR_W_256:
            return 256;
        case CHDR_W_512:
            return 512;
        default:
            return 0;
    }
    */
    return
        chdr_w == CHDR_W_64  ?  64 :
        chdr_w == CHDR_W_128 ? 128 :
        chdr_w == CHDR_W_256 ? 256 :
        chdr_w == CHDR_W_512 ? 512 :
        /* default */            0 ;
    // clang-format on
}

//! Stream Endpoint ID Type
// Stream endpoints within a graph are unique. They are assigned dynamically
// during runtime when needed. Stream endpoints exist both in the host as well
// as in the devices. See also sep_addr_t. The value of any sep_id_t is
// meaningless, it provides no information on where the SEP is physically
// located. In comments and variables, it is often abbreviated as "EPID"
// ("endpoint ID").
using sep_id_t = uint16_t;

}} // namespace uhd::rfnoc
