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

/*! FIR Filter Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The FIR Filter Block is a finite impulse response filter block for RFNoC.
 *
 * The RFNoC FIR block supports one input and output port of sc16 data
 * (16-bit fixed-point complex samples) and a configurable (but fixed)
 * number of taps.
 */
class UHD_API fir_filter_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fir_filter_block_control)

    // Block registers
    static const uint32_t REG_FIR_BLOCK_SIZE;
    static const uint32_t REG_FIR_MAX_NUM_COEFFS_ADDR;
    static const uint32_t REG_FIR_LOAD_COEFF_ADDR;
    static const uint32_t REG_FIR_LOAD_COEFF_LAST_ADDR;

    /*! Get the maximum number of filter coefficients supported by this block
     *
     * Get the maximum number of filter coefficients supported by this
     * block.
     *
     * \returns The maximum number of filter coefficients supported by this block
     */
    virtual size_t get_max_num_coefficients(const size_t chan = 0) const = 0;

    /*! Set the filter coefficients
     *
     * Set the filter coefficients for this FIR block. The number of
     * coefficients must be equal to or less than the maximum number of
     * coefficients supported by the block. If the vector of coefficients
     * passed to this function is smaller than the maximum number of
     * coefficients supported by the block, it will automatically be padded
     * with zeroes. If the vector of coefficients passed to this function is
     * larger than the maximum number of coefficients supported by the block,
     * a `uhd::value_error` is thrown.
     *
     * \param coeffs A vector of integer coefficients for the FIR filter
     * \param chan Channel index
     */
    virtual void set_coefficients(const std::vector<int16_t>& coeffs, const size_t chan = 0) = 0;

    /*! Get the filter coefficients
     *
     * Return a vector with the current filter coefficients.
     *
     * \returns The vector of current filter coefficients
     */
    virtual std::vector<int16_t> get_coefficients(const size_t chan = 0) const = 0;
};

}} // namespace uhd::rfnoc
