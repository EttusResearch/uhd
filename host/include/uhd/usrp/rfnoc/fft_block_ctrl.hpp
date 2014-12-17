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

    static const size_t DEFAULT_FFT_SIZE            = 256;
    static const magnitude_t DEFAULT_MAGNITUDE_OUT  = COMPLEX;
    static const boost::uint32_t SR_FFT_RESET       = 131;  // Note: AXI config bus uses 129 & 130
    static const boost::uint32_t SR_MAGNITUDE_OUT   = 132;
    static const boost::uint32_t RB_FFT_RESET       = 0;
    static const boost::uint32_t RB_MAGNITUDE_OUT   = 1;

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

    //! Configure the FFT size.
    //
    // This will not only configure the FFT core, but also sets
    // the packet size in the in- and output signatures.
    //
    // Note that not all FFT sizes are valid for this core. If
    // an invalid FFT size is selected, it will return false.
    virtual void set_fft_size(size_t fft_size) = 0;

    //! Returns the currently selected FFT size.
    //
    // You can use this after calling set_fft_size() to see what
    // the actual, current value is.
    virtual size_t get_fft_size() const = 0;

    //! Enable magnitude output
    //
    // FFT output can be sc16 or magnitude (still sc16 with mag in real,
    // imag set to 0). This sets the magnitude out register. It is advisable to
    // reset the FFT core via set_reset() when changing this value.
    virtual void set_magnitude_out(magnitude_t magnitude_out) = 0;

    //! Returns the whether magnitude output is enabled or not
    //
    // This queries the magnitude out readback register (instead of a class variable) as the
    // FFT RFNoC block can be configured to not include the magnitude calculation logic.
    virtual magnitude_t get_magnitude_out() = 0;
}; /* class fft_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_FFT_BLOCK_CTRL_HPP */
