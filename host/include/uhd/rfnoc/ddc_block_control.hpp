//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/optional.hpp>

namespace uhd { namespace rfnoc {

/*! DDC Block Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The DDC Block is a multi-channel digital downconverter (DDC) with built-in
 * frequency shift. The number of channels as well as the maximum decimation is
 * configurable in the FPGA, the block controller will read out registers to
 * identify the capabilities of this block.
 *
 * This block has two user properties per channel:
 * - `freq`: The frequency shift at the input. Note: A convenience method
 *   set_freq() is provided to set this property. It also takes care of the
 *   command time, which set_property() does not, and thus should be preferred.
 * - `decim`: The decimation value
 */
class UHD_API ddc_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(ddc_block_control)

    static const uint16_t MAJOR_COMPAT;
    static const uint16_t MINOR_COMPAT;
    // Readback addresses
    static const uint32_t RB_COMPAT_NUM;
    static const uint32_t RB_NUM_HB;
    static const uint32_t RB_CIC_MAX_DECIM;
    // Write addresses
    static const uint32_t SR_N_ADDR;
    static const uint32_t SR_M_ADDR;
    static const uint32_t SR_CONFIG_ADDR;
    static const uint32_t SR_FREQ_ADDR;
    static const uint32_t SR_SCALE_IQ_ADDR;
    static const uint32_t SR_DECIM_ADDR;
    static const uint32_t SR_MUX_ADDR;
    static const uint32_t SR_COEFFS_ADDR;
    static const uint32_t SR_TIME_INCR_ADDR;

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
        const boost::optional<uhd::time_spec_t> time = boost::none) = 0;

    /*! Return the current DDS frequency
     *
     * \returns The current frequency of the DDS
     */
    virtual double get_freq(const size_t chan) const = 0;

    /*! Return the range of frequencies that \p chan can be set to.
     *
     * The frequency shifter is the first component in the DDC, and thus can
     * shift frequencies (digitally) between -get_input_rate()/2
     * and +get_input_rate()/2.
     *
     * The returned values are in Hz (not normalized frequencies) and are valid
     * inputs for set_freq().
     *
     * \return The range of frequencies that the DDC can shift the input by
     */
    virtual uhd::freq_range_t get_frequency_range(const size_t chan) const = 0;

    /*! Return the sampling rate at this block's input
     *
     * \param chan The channel for which the rate is being queried
     * \returns the sampling rate at this block's input
     */
    virtual double get_input_rate(const size_t chan) const = 0;

    /*! Manually set the sampling rate at this block's input
     *
     * \param rate The requested rate
     * \param chan The channel for which the rate is being set
     */
    virtual void set_input_rate(const double rate, const size_t chan) = 0;

    /*! Return the sampling rate at this block's output
     *
     * This is equivalent to calling get_input_rate() divided by the decimation.
     *
     * \param chan The channel for which the rate is being queried
     * \returns the sampling rate at this block's input
     */
    virtual double get_output_rate(const size_t chan) const = 0;

    /*! Return a range of valid output rates, based on the current input rate
     *
     * Note the return value is only valid as long as the input rate does not
     * change.
     */
    virtual uhd::meta_range_t get_output_rates(const size_t chan) const = 0;

    /*! Attempt to set the output rate of this block
     *
     * This will set the decimation such that the input rate is untouched, and
     * that the input rate divided by the new decimation is as close as possible
     * to the requested \p rate.
     *
     * \param rate The requested rate
     * \param chan The channel for which the rate is being queried
     * \returns the coerced sampling rate at this block's output
     */
    virtual double set_output_rate(const double rate, const size_t chan) = 0;

    /**************************************************************************
     * Streaming-Related API Calls
     *************************************************************************/
    /*! Issue stream command: Instruct the RX part of the radio to send samples
     *
     * \param stream_cmd The actual stream command to execute
     * \param port The port for which the stream command is meant
     */
    virtual void issue_stream_cmd(
        const uhd::stream_cmd_t& stream_cmd, const size_t port) = 0;
};

}} // namespace uhd::rfnoc
