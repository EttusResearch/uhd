//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP
#define INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP

#include <string>
#include <boost/cstdint.hpp>

namespace uhd {
    namespace rfnoc {

// All these configure the XML reader
//! Where the RFNoC block/component definition files lie, relative to UHD_PKG_DIR
static const std::string XML_DEFAULT_PATH = "share/uhd/rfnoc";
static const std::string XML_PATH_ENV = "UHD_RFNOC_XML_PATH";

//! If the block name can't be automatically detected, this name is used
static const std::string DEFAULT_BLOCK_NAME = "Block";

static const size_t MAX_PACKET_SIZE = 8000; // bytes
static const size_t DEFAULT_PACKET_SIZE = 1456; // bytes

//! For flow control within a single crossbar
static const size_t DEFAULT_FC_XBAR_PKTS_PER_ACK = 2;
//! For flow control when data is flowing from device to host (rx)
static const size_t DEFAULT_FC_RX_RESPONSE_FREQ = 32; // ACKs per flow control window
//! For flow control when data is flowing from host to device (tx)
static const size_t DEFAULT_FC_TX_RESPONSE_FREQ = 8; // ACKs per flow control window
//! On the receive side, how full do we want the buffers?
static const double DEFAULT_FC_RX_SW_BUFF_FULL_FACTOR     = 0.90; // Buffer should ideally be 90% full.

// Common settings registers.
static const boost::uint32_t SR_FLOW_CTRL_CYCS_PER_ACK_BASE     = 0;
static const boost::uint32_t SR_FLOW_CTRL_PKTS_PER_ACK_BASE     = 16;
static const boost::uint32_t SR_FLOW_CTRL_WINDOW_SIZE_BASE      = 32;
static const boost::uint32_t SR_FLOW_CTRL_WINDOW_EN_BASE        = 48;
static const boost::uint32_t SR_FLOW_CTRL_CLR_SEQ               = 126;
static const boost::uint32_t SR_READBACK                        = 127;
static const boost::uint32_t SR_NEXT_DST_BASE                   = 128;
static const boost::uint32_t SR_READBACK_ADDR                   = 255;

//! Settings register readback
enum settingsbus_reg_t {
    SR_READBACK_REG_ID         = 0,
    SR_READBACK_REG_BUFFALLOC0 = 1,
    SR_READBACK_REG_BUFFALLOC1 = 2,
    SR_READBACK_REG_USER       = 3,
};

// AXI stream configuration bus (output master bus of axi wrapper) registers
static const boost::uint32_t AXI_WRAPPER_BASE      = SR_NEXT_DST_BASE;
static const boost::uint32_t AXIS_CONFIG_BUS       = AXI_WRAPPER_BASE+1; // tdata with tvalid asserted
static const boost::uint32_t AXIS_CONFIG_BUS_TLAST = AXI_WRAPPER_BASE+2; // tdata with tvalid & tlast asserted

// Block ports
static const size_t ANY_PORT = size_t(~0);

// Regular expressions
static const std::string VALID_BLOCKNAME_REGEX = "[A-Za-z][A-Za-z0-9]*";
static const std::string VALID_BLOCKID_REGEX = "(?:(\\d+)(?:/))?([A-Za-z][A-Za-z0-9]*)(?:(?:_)(\\d\\d?))?";

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CONSTANTS_HPP */
// vim: sw=4 et:
