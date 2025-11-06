//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhd/types/ranges.hpp>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Interface for generic RF control functions
 *
 * This interface contains all methods related directly to RF control which
 * aren't contained in a different rf_control class. These methods are not
 * usually accessed via this interface, but are usually accessed via an
 * instance of radio_control, which implements this class.
 */
class core_iface
{
public:
    using sptr = std::shared_ptr<core_iface>;

    virtual ~core_iface() = default;

    /*! Return the selected TX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_tx_antenna(const size_t chan) const = 0;

    /*! Return a list of valid TX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::vector<std::string> get_tx_antennas(const size_t chan) const = 0;

    /*! Select TX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_tx_antenna(const std::string& ant, const size_t chan) = 0;

    /*! Return the selected RX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_rx_antenna(const size_t chan) const = 0;

    /*! Return a list of valid RX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::vector<std::string> get_rx_antennas(const size_t chan) const = 0;

    /*! Select RX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_rx_antenna(const std::string& ant, const size_t chan) = 0;

    /*! Return the current transmit LO frequency on channel \p chan.
     *
     * \return The current LO frequency.
     */
    virtual double get_tx_frequency(const size_t chan) = 0;

    /*! Tune the TX frequency for channel \p chan.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * If there is a single LO in this radio, and we're doing direct conversion,
     * then this is the LO frequency.
     *
     * Note that unlike the uhd::usrp::multi_usrp::set_tx_freq() API, this does
     * not attempt to tune any attached digital frequency shifter, unless it is
     * part of the radio. That is why this API only returns a double value (the
     * actual frequency) instead of a uhd::tune_result_t. If a combined tuning
     * of digital frequency correction and LO tuning is desired (the same way
     * that uhd::usrp::multi_usrp does by default), then the caller has to
     * either also call uhd::rfnoc::dc_block_control::set_freq() with the
     * residual frequency, or tune through the graph.
     *
     * \param freq Frequency in Hz
     * \param chan Channel to tune
     *
     * \return The actual frequency.
     */
    virtual double set_tx_frequency(const double freq, size_t chan) = 0;

    /*! Set the TX tune args, if supported by the hardware.
     */
    virtual void set_tx_tune_args(const uhd::device_addr_t& args, const size_t chan) = 0;

    /*! Return the range of frequencies that \p chan can be tuned to.
     *
     * \return The range of frequencies that we can tune the TX chan to
     */
    virtual uhd::freq_range_t get_tx_frequency_range(const size_t chan) const = 0;

    /*! Return the current receive LO frequency on channel \p chan.
     *
     * \return The current LO frequency.
     */
    virtual double get_rx_frequency(const size_t chan) = 0;

    /*! Tune the RX frequency for channel \p chan.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * If there is a single LO in this radio, and we're doing direct conversion,
     * then this is the LO frequency.
     *
     * Note that unlike the uhd::usrp::multi_usrp::set_rx_freq() API, this does
     * not attempt to tune any attached digital frequency shifter, unless it is
     * part of the radio. That is why this API only returns a double value (the
     * actual frequency) instead of a uhd::tune_result_t. If a combined tuning
     * of digital frequency correction and LO tuning is desired (the same way
     * that uhd::usrp::multi_usrp does by default), then the caller has to
     * either also call uhd::rfnoc::ddc_block_control::set_freq() with the
     * residual frequency, or tune through the graph.
     *
     * \param freq Requested frequency
     * \param chan Channel number.
     * \return The actual frequency.
     */
    virtual double set_rx_frequency(const double freq, const size_t chan) = 0;

    /*! Set the RX tune args, if supported by the hardware.
     */
    virtual void set_rx_tune_args(const uhd::device_addr_t& args, const size_t chan) = 0;

    /*! Return the range of frequencies that \p chan can be tuned to.
     *
     * \return The range of frequencies that we can tune the RX chan to
     */
    virtual uhd::freq_range_t get_rx_frequency_range(const size_t chan) const = 0;

    /*! Return a list of valid TX gain names
     */
    virtual std::vector<std::string> get_tx_gain_names(const size_t chan) const = 0;

    /*! Return a range of valid TX gains
     */
    virtual uhd::gain_range_t get_tx_gain_range(const size_t chan) const = 0;

    /*! Return a range of valid TX gains
     */
    virtual uhd::gain_range_t get_tx_gain_range(
        const std::string& name, const size_t chan) const = 0;

    /*! Return the overall transmit gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_tx_gain(const size_t chan) = 0;

    /*! Return the transmit gain \p name on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_tx_gain(const std::string& name, const size_t chan) = 0;

    /*! Set the transmit gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * This method will set the overall gain. To set a specific gain, use
     * set_tx_gain(const double, const std::string&, const size_t).
     *
     * \return The actual gain value
     */
    virtual double set_tx_gain(const double gain, const size_t chan) = 0;

    /*! Set the transmit gain \p name on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_tx_gain(
        const double gain, const std::string& name, const size_t chan) = 0;

    /*! Return a list of valid RX gain names
     */
    virtual std::vector<std::string> get_rx_gain_names(const size_t chan) const = 0;

    /*! Return a range of valid RX gains
     */
    virtual uhd::gain_range_t get_rx_gain_range(const size_t chan) const = 0;

    /*! Return a range of valid RX gains
     */
    virtual uhd::gain_range_t get_rx_gain_range(
        const std::string& name, const size_t chan) const = 0;

    /*! Return the overall receive gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_rx_gain(const size_t chan) = 0;

    /*! Return the receive gain \p name on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_rx_gain(const std::string& name, const size_t chan) = 0;

    /*! Set the overall receive gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_rx_gain(const double gain, const size_t chan) = 0;

    /*! Set the receive gain \p on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_rx_gain(
        const double gain, const std::string& name, const size_t chan) = 0;

    /*! Enable RX AGC on this radio
     *
     * \throws uhd::not_implemented_error if this radio doesn't support RX AGC
     */
    virtual void set_rx_agc(const bool enable, const size_t chan) = 0;

    /*! Return a range of valid TX bandwidths
     */
    virtual meta_range_t get_tx_bandwidth_range(size_t chan) const = 0;

    /*! Return the analog filter bandwidth channel \p chan
     *
     * \return The actual bandwidth value
     */
    virtual double get_tx_bandwidth(const size_t chan) = 0;

    /*! Set the analog filter bandwidth channel \p chan
     *
     * This function will attempt to set the analog bandwidth.
     *
     * \return The actual bandwidth value
     */
    virtual double set_tx_bandwidth(const double bandwidth, const size_t chan) = 0;

    /*! Return a range of valid RX bandwidths
     */
    virtual meta_range_t get_rx_bandwidth_range(size_t chan) const = 0;

    /*! Return the analog filter bandwidth channel \p chan
     *
     * \return The actual bandwidth value
     */
    virtual double get_rx_bandwidth(const size_t chan) = 0;

    /*! Set the analog filter bandwidth channel \p chan
     *
     * This function will attempt to set the analog bandwidth.
     *
     * \return The actual bandwidth value
     */
    virtual double set_rx_bandwidth(const double bandwidth, const size_t chan) = 0;

    /**************************************************************************
     * LO Controls
     *************************************************************************/
    /*! Get a list of possible LO stage names
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names
     */
    virtual std::vector<std::string> get_rx_lo_names(const size_t chan) const = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources
     * will return "internal".
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t chan) const = 0;

    /*!
     * Get the LO frequency range of the RX LO.
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const = 0;

    /*!
     * Set the LO source for a channel.
     * For usrps that support selectable LOs, this function
     * allows switching between them.
     * Typical options for source: internal, external.
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) = 0;

    /*!
     * Get the currently set LO source.
     * Channels without controllable LO sources will return
     * "internal"
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual std::string get_rx_lo_source(const std::string& name, const size_t chan) = 0;

    /*!
     * Set whether the LO used by the usrp device is exported
     * For usrps that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     */
    virtual void set_rx_lo_export_enabled(
        bool enabled, const std::string& name, const size_t chan) = 0;

    /*!
     * Returns true if the currently selected LO is being exported.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_rx_lo_export_enabled(const std::string& name, const size_t chan) = 0;

    /*!
     * Set the RX LO frequency (Advanced).
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     */
    virtual double set_rx_lo_freq(
        double freq, const std::string& name, const size_t chan) = 0;

    /*!
     * Get the current RX LO frequency (Advanced).
     * If the channel does not have independently configurable LOs
     * the current rf frequency will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_rx_lo_freq(const std::string& name, const size_t chan) = 0;

    /*! Get a list of possible LO stage names
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names
     */
    virtual std::vector<std::string> get_tx_lo_names(const size_t chan) const = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources
     * will return "internal".
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_tx_lo_sources(
        const std::string& name, const size_t chan) const = 0;

    /*!
     * Get the LO frequency range of the tx LO.
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan) = 0;

    /*!
     * Set the LO source for a channel.
     * For usrps that support selectable LOs, this function
     * allows switching between them.
     * Typical options for source: internal, external.
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) = 0;

    /*!
     * Get the currently set LO source.
     * Channels without controllable LO sources will return
     * "internal"
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual std::string get_tx_lo_source(const std::string& name, const size_t chan) = 0;

    /*!
     * Set whether the LO used by the usrp device is exported
     * For usrps that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     */
    virtual void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) = 0;

    /*!
     * Returns true if the currently selected LO is being exported.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_tx_lo_export_enabled(const std::string& name, const size_t chan) = 0;

    /*!  Set the tx LO frequency (Advanced).
     *
     * See also multi_usrp::set_tx_lo_freq().
     *
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     */
    virtual double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) = 0;

    /*!  Get the current TX LO frequency (Advanced).
     *
     * See also multi_usrp::get_tx_lo_freq()
     *
     * If the channel does not have independently configurable LOs
     * the current RF frequency will be returned.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_tx_lo_freq(const std::string& name, const size_t chan) = 0;
};

}}} // namespace uhd::rfnoc::rf_control
