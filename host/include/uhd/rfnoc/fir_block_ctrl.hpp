//
// Copyright 2014-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP
#define INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP

#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>

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
class UHD_RFNOC_API fir_block_ctrl : public source_block_ctrl_base, public sink_block_ctrl_base
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
}; /* class fir_block_ctrl*/

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_fir_block_ctrl_HPP */
// vim: sw=4 et:
