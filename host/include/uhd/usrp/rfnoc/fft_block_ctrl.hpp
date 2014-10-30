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

#include <uhd/usrp/rfnoc/rx_block_ctrl_base.hpp>
#include <uhd/usrp/rfnoc/tx_block_ctrl_base.hpp>

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
class UHD_API fft_block_ctrl : public rx_block_ctrl_base, public tx_block_ctrl_base
{
public:
    UHD_RFNOC_BLOCK_OBJECT(fft_block_ctrl)

    static const size_t DEFAULT_FFT_SIZE = 256;

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

    //! In this block, the input signature may not be changed.
    //
    // Will return false if \p stream_sig does not match the current stream sig.
    virtual bool set_input_signature(const stream_sig_t &stream_sig, size_t port=0) = 0;

    //! In this block, the output signature may not be changed.
    //
    // Will return false if \p stream_sig does not match the current stream sig.
    virtual bool set_output_signature(const stream_sig_t &stream_sig, size_t port=0) = 0;

protected:
    //! Checks the args \p fftsize and \p spp are OK.
    //
    // If fftsize is given, it will actually change the FFT size (i.e. call
    // set_fft_size()). If spp is given, it must match the FFT size, or we
    // throw.
    virtual void _set_args() = 0;

    //! Check stream args match FFT size
    virtual void _init_rx(uhd::stream_args_t &args) = 0;

    //! Check stream args match FFT size
    virtual void _init_tx(uhd::stream_args_t &args) = 0;
}; /* class fft_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_FFT_BLOCK_CTRL_HPP */
// vim: sw=4 et:
