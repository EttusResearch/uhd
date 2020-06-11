//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <memory>

namespace uhd { namespace rfnoc {

//----------------------------------------------
// Types
//----------------------------------------------

//! Device ID Type
using device_id_t = uint16_t;
//! Stream Endpoint Instance Number Type
using sep_inst_t = uint16_t;
//! Stream Endpoint Physical Address Type
using sep_addr_t = std::pair<device_id_t, sep_inst_t>;
//! Stream Endpoint Physical Address Type (first = source, second = destination)
using sep_addr_pair_t = std::pair<sep_addr_t, sep_addr_t>;
//! Stream Endpoint pair Type (first = source, second = destination)
using sep_id_pair_t = std::pair<sep_id_t, sep_id_t>;
//! Stream Endpoint Virtual Channel Type
using sep_vc_t = uint8_t;

//! NULL/unassigned device ID
static constexpr device_id_t NULL_DEVICE_ID = 0;
//! NULL/unassigned device address
static constexpr sep_addr_t NULL_DEVICE_ADDR{NULL_DEVICE_ID, 0};
//! NULL/unassigned stream endpoint ID
static constexpr sep_id_t NULL_EPID = 0;


//! Flow control buffer configuration parameters
struct stream_buff_params_t
{
    uint64_t bytes;
    uint32_t packets;
};

//! The data type of the buffer used to capture/generate data
enum sw_buff_t { BUFF_U64 = 0, BUFF_U32 = 1, BUFF_U16 = 2, BUFF_U8 = 3 };
//! Conversion from number of bits to sw_buff
constexpr sw_buff_t bits_to_sw_buff(size_t bits)
{
    if (bits <= 8) {
        return BUFF_U8;
    } else if (bits <= 16) {
        return BUFF_U16;
    } else if (bits <= 32) {
        return BUFF_U32;
    } else {
        return BUFF_U64;
    }
}

//----------------------------------------------
// Constants
//----------------------------------------------

constexpr uint64_t MAX_FC_CAPACITY_BYTES = (uint64_t(1) << 40) - 1;
constexpr uint32_t MAX_FC_CAPACITY_PKTS  = (uint32_t(1) << 24) - 1;
constexpr uint64_t MAX_FC_FREQ_BYTES     = (uint64_t(1) << 40) - 1;
constexpr uint32_t MAX_FC_FREQ_PKTS      = (uint32_t(1) << 24) - 1;
constexpr uint64_t MAX_FC_HEADROOM_BYTES = (uint64_t(1) << 16) - 1;
constexpr uint32_t MAX_FC_HEADROOM_PKTS  = (uint32_t(1) << 8) - 1;

// RFNoC devices need a minimum of two frame buffers to be available from the
// link--one for the data transport and one for the control transport to
// simultaneously handle MGMT and STRC/STRS initialization packets.
constexpr size_t MIN_NUM_FRAMES = 2;

}} // namespace uhd::rfnoc
