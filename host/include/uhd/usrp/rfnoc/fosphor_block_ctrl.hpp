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

#ifndef INCLUDED_LIBUHD_RFNOC_FOSPHOR_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_FOSPHOR_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the fopshor IIR RFNoC block.
 *
 * Accelerate a fosphor style display on the FPGA
 * - One input / output block port
 * - Supports input data type sc16 (16-Bit fix-point complex samples)
 * - Output is u8 packetized display data that's meant to be fed to the matching
 *   host side display block
 *
 * The input packet size is taken as the FFT size
 */
class UHD_API fosphor_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(fosphor_block_ctrl)

    static const boost::uint32_t RANDOM_LSB = 1 << 0;
    static const boost::uint32_t RANDOM_ADD = 1 << 1;

    virtual void clear() = 0;

    virtual void set_decim(int decim) = 0;
    virtual int get_decim() const = 0;

    virtual void set_offset(int offset) = 0;
    virtual int get_offset() const = 0;

    virtual void set_scale(int scale) = 0;
    virtual int get_scale() const = 0;

    virtual void set_trise(int trise) = 0;
    virtual int get_trise() const = 0;

    virtual void set_tdecay(int tdecay) = 0;
    virtual int get_tdecay() const = 0;

    virtual void set_alpha(int alpha) = 0;
    virtual int get_alpha() const = 0;

    virtual void set_epsilon(int epsilon) = 0;
    virtual int get_epsilon() const = 0;

    virtual void set_random(int random) = 0;
    virtual int get_random() const = 0;
}; /* class fosphor_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_FOSPHOR_BLOCK_CTRL_HPP */
