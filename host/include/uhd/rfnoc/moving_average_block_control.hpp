//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/optional.hpp>

namespace uhd { namespace rfnoc {

/*! Moving Average Block Control Class
 *
 * The Moving Average block is an RFNoC block that computes the running average of an
 * input data stream. The output is the sum of the last SUM_LEN samples divided by DIVISOR
 * which may or may not be equal to SUM_LEN. For example, if SUM_LEN is set to 10 and
 * DIVISOR is set to 10, the block will return a sample that is the average of the last 10
 * samples. If SUM_LEN is set to 10 and DIVISOR is set to 1, the block will return a
 * sample that is equal to the sum of the last 10 samples.
 *
 */
class UHD_API moving_average_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(moving_average_block_control)

    static const uint32_t REG_SUM_LEN_ADDR;
    static const uint32_t REG_DIVISOR_ADDR;

    /*! Set the Sum Length
     *
     * Changing the sum length will clear the history and reset the accumulated sum to 0.
     *
     * \param sum_len The number of samples to sum
     */
    virtual void set_sum_len(const uint8_t sum_len) = 0;

    /*! Return the current sum length
     *
     * \returns The number of samples to sum
     */
    virtual uint8_t get_sum_len() const = 0;

    /*! Set the divisor
     *
     * \param divisor The amount to divide the sum by
     */
    virtual void set_divisor(const uint32_t divisor) = 0;

    /*! Return the current divisor
     *
     * \returns The amount to divide the sum by
     */
    virtual uint32_t get_divisor() const = 0;
};

}} // namespace uhd::rfnoc
