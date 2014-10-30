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

#ifndef INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP
#define INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP

#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/tx_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the standard FIR RFNoC block.
 *
 * The standard FIR has the following features:
 * - One input- and output-port
 * - Configurable taps, but fixed number of taps
 * - Supports data type sc16 (16-Bit fix-point complex samples)
 *
 * This block requires packets to be the same size as the FFT length.
 * It will perform one FFT operation per incoming packet, treating it
 * as a vector of samples.
 */
class UHD_API fir_block_ctrl : public rx_block_ctrl_base, public tx_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(fir_block_ctrl)

    //! Configure the filter taps.
    //
    // The length of \p taps must correspond the number of taps
    // in this block. If it's shorter, zeros will be padded.
    // If it's longer, throws a uhd::value_error.
    virtual void set_taps(const std::vector<int> &taps) = 0;

    //! Returns the number of filter taps in this block.
    virtual size_t get_n_taps() const = 0;

protected:
    virtual void _init_rx(uhd::stream_args_t &args) = 0;

    virtual void _init_tx(uhd::stream_args_t &args) = 0;
}; /* class fir_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP */
// vim: sw=4 et:
