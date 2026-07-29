//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/time_spec.hpp>
#include <boost/optional.hpp>
#include <optional>

namespace uhd { namespace rfnoc {

/*! DUC Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The DUC Block is a multi-channel digital upconverter (DUC) with built-in
 * frequency shift. The number of channels as well as the maximum interpolation
 * is configurable in the FPGA, the block controller will read out registers to
 * identify the capabilities of this block.
 *
 * This block has two user properties per channel:
 * - `freq`: The frequency shift at the input. Note: A convenience method
 *   set_freq() is provided to set this property. It also takes care of the
 *   command time, which set_property() does not, and thus should be preferred.
 * - `interp`: The interpolation value
 */
class UHD_API duc_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(duc_block_control)

    //! Version-specific register addresses
    struct reg_addrs_t
    {
        uint16_t major_compat;
        uint16_t minor_compat;
        uint32_t num_hb;
        uint32_t cic_max_interp;
        std::optional<uint32_t> n_addr;
        std::optional<uint32_t> m_addr;
        std::optional<uint32_t> config_addr;
        uint32_t interp_addr;
        uint32_t freq_addr;
        uint32_t scale_iq_addr;
        uint32_t time_incr_addr;
    };

    // Compat register address (same across all versions)
    static const uint32_t REG_COMPAT_NUM;
    // Register addresses for version 0.x
    static const reg_addrs_t REG_ADDRS_V0;
    // Register addresses for version 1.x
    static const reg_addrs_t REG_ADDRS_V1;

    /*! Set the DDS frequency
     *
     * This block will shift the signal at the input by this frequency before
     * decimation. The frequency is given in Hz, it is not a relative frequency
     * to the input sampling rate.
     *
     * Note: When the rate is modified, the frequency is kept constant. Because
     * the FPGA internally uses a relative phase increment, changing the input
     * sampling rate will trigger a property propagation to recalculate the
     * phase increment based off of this value.
     *
     * This function will coerce the frequency to a valid value, and return the
     * coerced value.
     *
     * \param freq The frequency shift in Hz
     * \param chan The channel to which this change shall be applied
     * \param time When to apply the new frequency
     * \returns The coerced, actual current frequency of the DDS
     */
    virtual double set_freq(const double freq,
        const size_t chan,
        const std::optional<uhd::time_spec_t> time = {}) = 0;

    double set_freq(const double freq, const size_t chan, const uhd::time_spec_t time)
    {
        return set_freq(freq, chan, std::make_optional(time));
    }

    [[deprecated("Prefer std::optional over boost::optional.")]] virtual double set_freq(
        const double freq,
        const size_t chan,
        const boost::optional<uhd::time_spec_t> time)
    {
        return set_freq(
            freq, chan, bool(time) ? std::make_optional(*time) : std::nullopt);
    }

    /*! Return the current DDS frequency
     *
     * \returns The current frequency of the DDS
     */
    virtual double get_freq(const size_t chan) const = 0;

    /*! Return the range of frequencies that \p chan can be set to.
     *
     * The frequency shifter is the last component in the DUC, and thus can
     * shift frequencies (digitally) between -get_output_rate()/2
     * and +get_output_rate()/2.
     *
     * The returned values are in Hz (not normalized frequencies) and are valid
     * inputs for set_freq().
     *
     * \return The range of frequencies that the DUC can shift the input by
     */
    virtual uhd::freq_range_t get_frequency_range(const size_t chan) const = 0;

    /*! Return the sampling rate at this block's input
     *
     * \param chan The channel for which the rate is being queried
     * \returns the sampling rate at this block's input
     */
    virtual double get_input_rate(const size_t chan) const = 0;

    /*! Return the sampling rate at this block's output
     *
     * This is equivalent to calling get_input_rate() multiplied by the interpolation
     *
     * \param chan The channel for which the rate is being queried
     * \returns the sampling rate at this block's input
     */
    virtual double get_output_rate(const size_t chan) const = 0;

    /*! Manually set the sampling rate at this block's output
     *
     * \param rate The requested rate
     * \param chan The channel for which the rate is being set
     */
    virtual void set_output_rate(const double rate, const size_t chan) = 0;

    /*! Return a range of valid input rates, based on the current output rate
     *
     * Note the return value is only valid as long as the output rate does not
     * change.
     */
    virtual uhd::meta_range_t get_input_rates(const size_t chan) const = 0;

    /*! Attempt to set the input rate of this block
     *
     * This will set the interpolation such that the output rate is untouched, and
     * that the output rate divided by the new interpolation is as close as
     * possible to the requested \p rate.
     *
     * \param rate The requested rate
     * \param chan The channel for which the rate is being queried
     * \returns the coerced sampling rate at this block's output
     */
    virtual double set_input_rate(const double rate, const size_t chan) = 0;
};

}} // namespace uhd::rfnoc
