//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_RFNOC_COMMON_HPP
#define INCLUDED_RFNOC_RFNOC_COMMON_HPP

#include <memory>

namespace uhd { namespace rfnoc {

//----------------------------------------------
// Types
//----------------------------------------------

//! Type that indicates the CHDR Width in bits
enum chdr_w_t { CHDR_W_64 = 0, CHDR_W_128 = 1, CHDR_W_256 = 2, CHDR_W_512 = 3 };
//! Conversion from chdr_w_t to a number of bits
constexpr size_t chdr_w_to_bits(chdr_w_t chdr_w)
{
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
}

//! Device ID Type
using device_id_t = uint16_t;
//! Stream Endpoint Instance Number Type
using sep_inst_t = uint16_t;
//! Stream Endpoint Physical Address Type
using sep_addr_t = std::pair<device_id_t, sep_inst_t>;
//! Stream Endpoint ID Type
using sep_id_t = uint16_t;

//! Type of data carried by the stream endpoint
enum class sep_type_t { CTRL, DATA };


//----------------------------------------------
// Constants
//----------------------------------------------

constexpr uint16_t RFNOC_PROTO_VER = 0x0100;


}} // namespace uhd::rfnoc

#endif /* INCLUDED_RFNOC_RFNOC_COMMON_HPP */
