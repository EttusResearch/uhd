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

#ifndef INCLUDED_LIBUHD_RFNOC_PACKET_RESIZER_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_PACKET_RESIZER_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/tx_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the Packet Resizer RFNoC block.
 *
 * The Packet Resizer RFNoC block augments the packet length of a stream. It can break large
 * packets into smaller packets or combine small packets into a single larger packet.
 */
class UHD_API packet_resizer_block_ctrl : public rx_block_ctrl_base, public tx_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(packet_resizer_block_ctrl)

    static const boost::uint16_t DEFAULT_PKT_SIZE  = 32;
    // Settings bus address for packet size
    static const boost::uint32_t SR_PKT_SIZE       = 129;

    //! Set the output packet size
    virtual void set_packet_size(const boost::uint16_t pkt_size) = 0;

    //! Get the output packet size
    virtual boost::uint16_t get_packet_size() = 0;

}; /* class packet_resizer_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_PACKET_RESIZER_BLOCK_CTRL_HPP */
