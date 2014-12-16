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

#ifndef INCLUDED_LIBUHD_RFNOC_KEEP_ONE_IN_N_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_KEEP_ONE_IN_N_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the Keep One in N RFNoC block.
 *
 * For every N packets received, this block will output a single packet.
 * - One input / output block port
 * - N up to 65535
 */
class UHD_API keep_one_in_n_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(keep_one_in_n_block_ctrl)

    static const size_t DEFAULT_N               = 256;
    static const boost::uint32_t SR_N           = 129;

    //! Configure N
    //
    // This function sets N, which configures the block to
    // drop all but one packet in N consecutive packets.
    // Packet counter in block is 16-bits.
    virtual void set_n(boost::uint16_t n) = 0;

    //! Returns the currently selected FFT size.
    //
    // You can use this after calling set_n() to see what
    // the set value.
    virtual boost::uint16_t get_n() const = 0;
}; /* class keep_one_in_n_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_KEEP_ONE_IN_N_BLOCK_CTRL_HPP */
