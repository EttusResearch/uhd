//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP
#define INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP

#include <uhd/types/dict.hpp>
#include <boost/assign/list_of.hpp>
#include <stdint.h>
#include <string>

namespace uhd {
    namespace rfnoc {

// All these configure the XML reader
//! Where the RFNoC block/component definition files lie, relative to UHD_PKG_DIR
static const std::string XML_DEFAULT_PATH = "share/uhd/rfnoc";
//! The name of the environment variable storing the bath to the block definition files
static const std::string XML_PATH_ENV = "UHD_RFNOC_DIR";

//! If the block name can't be automatically detected, this name is used
static const std::string DEFAULT_BLOCK_NAME = "Block";
static const uint64_t DEFAULT_NOC_ID = 0xFFFFFFFFFFFFFFFF;
static const size_t NOC_SHELL_COMPAT_MAJOR = 2;
static const size_t NOC_SHELL_COMPAT_MINOR = 0;

static const size_t MAX_PACKET_SIZE = 8000; // bytes
static const size_t DEFAULT_PACKET_SIZE = 1456; // bytes

// One line in FPGA is 64 Bits
static const size_t BYTES_PER_LINE = 8;

//! For flow control within a single crossbar
static const size_t DEFAULT_FC_XBAR_PKTS_PER_ACK = 2;
//! For flow control when data is flowing from device to host (rx)
static const size_t DEFAULT_FC_RX_RESPONSE_FREQ = 64; // ACKs per flow control window
//! For flow control when data is flowing from host to device (tx)
static const size_t DEFAULT_FC_TX_RESPONSE_FREQ = 8; // ACKs per flow control window
//! On the receive side, how full do we want the buffers?
//  Why not 100% full? Because we need to have some headroom to account for the inaccuracy
//  when computing the window size. We compute the flow control window based on the frame
//  size but the buffer can have overhead due to things like UDP headers, page alignment,
//  housekeeping info, etc. This number has to be transport agnostic so 20% of headroom is safe.
static const double DEFAULT_FC_RX_SW_BUFF_FULL_FACTOR = 0.80;

// Common settings registers.
static const uint32_t SR_FLOW_CTRL_CYCS_PER_ACK          = 0;
static const uint32_t SR_FLOW_CTRL_PKTS_PER_ACK          = 1;
static const uint32_t SR_FLOW_CTRL_WINDOW_SIZE           = 2;
static const uint32_t SR_FLOW_CTRL_WINDOW_EN             = 3;
static const uint32_t SR_ERROR_POLICY                    = 4;
static const uint32_t SR_BLOCK_SID                       = 5; // TODO rename to SRC_SID
static const uint32_t SR_NEXT_DST_SID                    = 6;
static const uint32_t SR_RESP_IN_DST_SID                 = 7;
static const uint32_t SR_RESP_OUT_DST_SID                = 8;

static const uint32_t SR_READBACK_ADDR                   = 124;
static const uint32_t SR_READBACK                        = 127;

static const uint32_t SR_CLEAR_RX_FC                     = 125;
static const uint32_t SR_CLEAR_TX_FC                     = 126;

//! Settings register readback
enum settingsbus_reg_t {
    SR_READBACK_REG_ID         = 0,
    SR_READBACK_REG_GLOBAL_PARAMS       = 1,
    SR_READBACK_REG_FIFOSIZE   = 2, // fifo size
    SR_READBACK_REG_MTU       = 3,
    SR_READBACK_REG_BLOCKPORT_SIDS = 4,
    SR_READBACK_REG_USER       = 5,
    SR_READBACK_COMPAT         = 6
};

// AXI stream configuration bus (output master bus of axi wrapper) registers
static const uint32_t AXI_WRAPPER_BASE      = 128;
static const uint32_t AXIS_CONFIG_BUS       = AXI_WRAPPER_BASE+1; // tdata with tvalid asserted
static const uint32_t AXIS_CONFIG_BUS_TLAST = AXI_WRAPPER_BASE+2; // tdata with tvalid & tlast asserted

static const size_t CMD_FIFO_SIZE = 128; // Lines == multiples of 8 bytes

// Named settings registers
static const uhd::dict<std::string, uint32_t> DEFAULT_NAMED_SR = boost::assign::map_list_of
        ("AXIS_CONFIG_BUS", AXIS_CONFIG_BUS)
        ("AXIS_CONFIG_BUS_TLAST", AXIS_CONFIG_BUS_TLAST)
;

// Block ports
static const size_t ANY_PORT = size_t(~0);
static const size_t MAX_NUM_PORTS = 16;

// Regular expressions
static const std::string VALID_BLOCKNAME_REGEX = "[A-Za-z][A-Za-z0-9]*";
static const std::string VALID_BLOCKID_REGEX = "(?:(\\d+)(?:/))?([A-Za-z][A-Za-z0-9]*)(?:(?:_)(\\d\\d?))?";

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP */
// vim: sw=4 et:
