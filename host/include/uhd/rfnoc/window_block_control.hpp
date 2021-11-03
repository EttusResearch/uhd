//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/ranges.hpp>

namespace uhd { namespace rfnoc {

/*! Window Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Window Block is a windowing block for RFNoC that is intended to be
 * used with the FFT block. The block can be configured with coefficients,
 * by which the samples in each input packet are multiplied before begin
 * output. The first sample of the first packet is multiplied by the first
 * first coefficient, the second sample is multiplied by the second
 * coefficient, and so on.
 *
 * The RFNoC window block supports a configurable number of pairs of input
 * and output ports of sc16 data (16-bit fixed-point complex samples) and
 * a configurable window length and coefficients for each.
 */
class UHD_API window_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(window_block_control)

    // Block registers
    static const uint32_t REG_WINDOW_BLOCK_SIZE;

    static const uint32_t REG_WINDOW_LEN_OFFSET;
    static const uint32_t REG_WINDOW_MAX_LEN_OFFSET;
    static const uint32_t REG_WINDOW_LOAD_COEFF_OFFSET;
    static const uint32_t REG_WINDOW_LOAD_COEFF_LAST_OFFSET;

    /*! Get the maximum number of window coefficients supported by this block
     *
     * Get the maximum number of window coefficients supported by this
     * block.
     *
     * \param chan The channel to retrieve the maximum number of coefficients from
     * \returns The maximum number of window coefficients supported by this block
     */
    virtual size_t get_max_num_coefficients(const size_t chan) const = 0;

    /*! Set the window coefficients
     *
     * Set the window coefficients for a given channel. The number of
     * coefficients must be equal to or less than the maximum number of
     * coefficients supported by the given channel of the block.
     *
     * \param coeffs A vector of integer coefficients for the window
     * \param chan The channel to apply the coefficients to
     */
    virtual void set_coefficients(
        const std::vector<int16_t>& coeffs, const size_t chan) = 0;

    /*! Get the window coefficients
     *
     * Return a vector with the current window coefficients for a given channel.
     *
     * \param chan The channel to retrieve the current window coefficients from
     * \returns The vector of current window coefficients
     */
    virtual std::vector<int16_t> get_coefficients(const size_t chan) const = 0;
};

}} // namespace uhd::rfnoc
