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

#ifndef INCLUDED_LIBUHD_RFNOC_FFT_BLOCK_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_FFT_BLOCK_CTRL_HPP

#include <uhd/usrp/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/sink_block_ctrl_base.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for the standard FFT RFNoC block.
 *
 * The standard FFT has the following features:
 * - One input- and output-port
 * - Configurable FFT size, limited to powers of two
 * - Supports data type sc16 (16-Bit fix-point complex samples)
 *
 * Args:
 * - spp: This will set the actual FFT size, as well as the vector
 *   and packet lengths.
 *
 * This block requires packets to be the same size as the FFT length.
 * It will perform one FFT operation per incoming packet, treating it
 * as a vector of samples.
 */
class UHD_API fft_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(fft_block_ctrl)

    typedef enum magnitude_t
    {
        COMPLEX = 0,
        MAGNITUDE = 1,
        MAGNITUDE_SQUARED = 2
    } magnitude_t;

    //! Reset FFT
    //
    // Toggles FFT reset bit
    virtual void reset_fft() = 0;

    //! Set FFT reset register
    //
    // Sets FFT reset register, which resets the FFT core, FFT shift,
    // magnitude calculation modules.
    virtual void set_fft_reset(bool enable) = 0;

    //! Returns current state of FFT reset register
    //
    // This queries the FFT reset readback register
    virtual bool get_fft_reset() = 0;

    //! Returns the whether magnitude output is enabled or not
    //
    // This queries the magnitude out readback register (instead of a class variable) as the
    // FFT RFNoC block can be configured to not include the magnitude calculation logic.
    virtual magnitude_t get_magnitude_out() = 0;
}; /* class fft_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_FFT_BLOCK_CTRL_HPP */
