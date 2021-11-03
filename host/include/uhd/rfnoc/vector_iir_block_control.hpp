//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

/*! Vector IIR Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Vector IIR Block is an RFNoC block that implements an infinite
 * impulse filter with a variable length delay line. The transfer
 * function is defined as follows:
 *
 *                    beta
 *    H(z) = ------------------------
 *            1 - alpha * z ^ -delay
 *
 * where
 *    - beta is the feedforward tap
 *    - alpha is the feedback tap
 *    - delay (a.k.a. vector length) is the feedback tap delay
 */
class UHD_API vector_iir_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(vector_iir_block_control)

    static const uint32_t REG_BLOCK_SIZE;

    static const uint32_t REG_DELAY_OFFSET;
    static const uint32_t REG_ALPHA_OFFSET;
    static const uint32_t REG_BETA_OFFSET;

    /*! Set the feedback tap value
     *
     * Sets the feedback tap value for channel \p chan of the IIR filter.
     *
     * \param alpha The feedback tap value for the filter
     * \param chan The channel to apply the feedback tap value to
     */
    virtual void set_alpha(const double alpha, const size_t chan) = 0;

    /*! Return the feedback tap value
     *
     * Returns the feedback tap value for channel \p chan of the IIR filter.
     *
     * \param chan The channel to retrieve the feedback tap value from
     * \returns The feedback tap value for the filter
     */
    virtual double get_alpha(const size_t chan) const = 0;

    /*! Set the feedforward tap value
     *
     * Sets the feedforward tap value for channel \p chan of the IIR filter.
     *
     * \param beta The feedforward tap value for the filter
     * \param chan The channel to apply the feedforward tap value to
     */
    virtual void set_beta(const double beta, const size_t chan) = 0;

    /*! Return the feedforward tap value
     *
     * Returns the feedforward tap value for channel \p chan of the IIR filter.
     *
     * \param chan The channel to retrieve the feedforward tap value from
     * \returns The feedforward tap value for the filter
     */
    virtual double get_beta(const size_t chan) const = 0;

    /*! Set the feedback tap delay
     *
     * Sets the feedback tap delay in samples for channel \p chan of the IIR
     * filter. The delay value for the filter must be less than or equal to
     * the maximum delay length supported by the filter.
     *
     * \param delay The feedback tap delay of the filter in samples
     * \param chan The channel to apply the feedback tap delay to
     */
    virtual void set_delay(const uint16_t delay, const size_t chan) = 0;

    /*! Return the feedback tap delay
     *
     * Returns the feedback tap delay value in samples for channel \p chan
     * of the IIR filter.
     *
     * \param chan The channel to retrieve the feedback tap delay value from
     * \returns The feedback tap delay of the filter in samples
     */
    virtual uint16_t get_delay(const size_t chan) const = 0;

    /*! Return the maximum filter delay
     *
     * Returns the maximum allowable filter delay value, in samples, for
     * channel \p chan.
     *
     * \param chan The channel to retrieve the maximum delay from
     * \returns The maximum filter delay
     */
    virtual uint16_t get_max_delay(const size_t chan) const = 0;
};

}} // namespace uhd::rfnoc
