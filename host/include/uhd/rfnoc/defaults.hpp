//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <cstdint>
#include <string>

namespace uhd { namespace rfnoc {

static const std::string CLOCK_KEY_GRAPH("__graph__");
static const std::string CLOCK_KEY_AUTO("__auto__");

static const std::string PROP_KEY_DECIM("decim");
static const std::string PROP_KEY_INTERP("interp");
static const std::string PROP_KEY_SAMP_RATE("samp_rate");
static const std::string PROP_KEY_SCALING("scaling");
static const std::string PROP_KEY_TYPE("type");
static const std::string PROP_KEY_FREQ("freq");
static const std::string PROP_KEY_TICK_RATE("tick_rate");
static const std::string PROP_KEY_SPP("spp");
static const std::string PROP_KEY_MTU("mtu");
static const std::string PROP_KEY_ATOMIC_ITEM_SIZE("atomic_item_size");

static const std::string NODE_ID_SEP("SEP");

using io_type_t                     = std::string;
static const io_type_t IO_TYPE_S16  = "s16";
static const io_type_t IO_TYPE_SC16 = "sc16";
static const io_type_t IO_TYPE_U8   = "u8";

static const std::string ACTION_KEY_STREAM_CMD("stream_cmd");
static const std::string ACTION_KEY_RX_EVENT("rx_event");
static const std::string ACTION_KEY_RX_RESTART_REQ("restart_request");
static const std::string ACTION_KEY_TX_EVENT("tx_event");

//! If the block name can't be automatically detected, this name is used
static const std::string DEFAULT_BLOCK_NAME = "Block";
//! This NOC-ID is used to look up the default block
static const uint32_t DEFAULT_NOC_ID  = 0xFFFFFFFF;
static const double DEFAULT_TICK_RATE = 1.0;

/*! The NoC ID is the unique identifier of the block type. All blocks of the
 * same type have the same NoC ID.
 */
using noc_id_t = uint32_t;

/*** Device Identifiers ******************************************************/
//! Device Type
using device_type_t = uint16_t;
// first nibble for device family (E = E, N = 1, X = A), remaining three nibbles
// for device number
//! placeholder for unspecified device
static const device_type_t ANY_DEVICE = 0xFFFF;
//! E300 device family
static const device_type_t E300 = 0xE300;
//! E310 device
static const device_type_t E310 = 0xE310;
//! E320
static const device_type_t E320 = 0xE320;
//! N300 device family (N300, N310)
static const device_type_t N300 = 0x1300;
//! N320 device
static const device_type_t N320 = 0x1320;
//! X300 device family (X300, X310)
static const device_type_t X300 = 0xA300;
//! X400 device family
static const device_type_t X400 = 0xA400;

// block identifiers
static const noc_id_t ADDSUB_BLOCK         = 0xADD00000;
static const noc_id_t DUC_BLOCK            = 0xD0C00000;
static const noc_id_t DDC_BLOCK            = 0xDDC00000;
static const noc_id_t FFT_BLOCK            = 0xFF700000;
static const noc_id_t FIR_FILTER_BLOCK     = 0xF1120000;
static const noc_id_t FIR_FILTER_BLOCK_V2  = 0xF1120002;
static const noc_id_t FOSPHOR_BLOCK        = 0x666F0000;
static const noc_id_t LICCHECK_BLOCK       = 0x11C0CECC;
static const noc_id_t LOGPWR_BLOCK         = 0x4C500000;
static const noc_id_t KEEP_ONE_IN_N_BLOCK  = 0x02460000;
static const noc_id_t MOVING_AVERAGE_BLOCK = 0xAAD20000;
static const noc_id_t RADIO_BLOCK          = 0x12AD1000;
static const noc_id_t REPLAY_BLOCK         = 0x4E91A000;
static const noc_id_t SIGGEN_BLOCK         = 0x51663110;
static const noc_id_t SPLIT_STREAM_BLOCK   = 0x57570000;
static const noc_id_t SWITCHBOARD_BLOCK    = 0xBE110000;
static const noc_id_t VECTOR_IIR_BLOCK     = 0x11120000;
static const noc_id_t WINDOW_BLOCK         = 0xD0530000;

}} // namespace uhd::rfnoc
