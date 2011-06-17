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

#ifndef INCLUDED_CTRL_PACKET_HPP
#define INCLUDED_CTRL_PACKET_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <uhd/types/serial.hpp>

typedef std::vector<boost::uint16_t> ctrl_data_t;

/*!
 * Control packet operation type
 */
enum ctrl_pkt_op_t { 
    CTRL_PKT_OP_WRITE = 1,
    CTRL_PKT_OP_READ = 2,
    CTRL_PKT_OP_READBACK = 3
};

/*!
 * Control packet transaction length
 */
const size_t CTRL_PACKET_LENGTH = 32;
const size_t CTRL_PACKET_HEADER_LENGTH = 8;
const size_t CTRL_PACKET_DATA_LENGTH = 24; //=length-header

/*!
 * Control packet header magic value
 */
const boost::uint8_t CTRL_PACKET_HEADER_MAGIC = 0xAA;

/*! 
 * Callback triggers for readback operation
 */
//FIXME: these are not real numbers, callbacks aren't implemented yet
const boost::uint16_t CTRL_PACKET_CALLBACK_SPI = 0x0001;
const boost::uint16_t CTRL_PACKET_CALLBACK_I2C = 0x0002;
//and so on

/*!
 * Metadata structure to describe a control packet
 */
struct UHD_API ctrl_pkt_meta_t {
    ctrl_pkt_op_t op;
    boost::uint8_t callbacks;
    boost::uint8_t seq;
    boost::uint16_t len;
    boost::uint32_t addr;
};

/*! 
 * Full control packet structure
 */
struct UHD_API ctrl_pkt_t {
    ctrl_pkt_meta_t pkt_meta;
    ctrl_data_t data;
};

#endif /* INCLUDED_CTRL_PACKET_HPP */
