//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature_getter_iface.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace uhd { namespace rfnoc {

/*! Parent class for radio block controllers
 */
class radio_control : public noc_block_base,
                      public virtual ::uhd::features::discoverable_feature_getter_iface
{
public:
    static const std::string ALL_LOS;
    static const std::string ALL_GAINS;
    static constexpr size_t ALL_CHANS = size_t(~0);

    RFNOC_DECLARE_BLOCK(radio_control)

    /**************************************************************************
     * Rate-Related API Calls
     *************************************************************************/
    //! Set the sample rate
    //
    // This function will coerce the rate and return the actual, current value.
    virtual double set_rate(const double rate) = 0;

    //! Get the sample rate
    virtual double get_rate() const = 0;

    //! Return a list of valid rates
    virtual uhd::meta_range_t get_rate_range() const = 0;

    /**************************************************************************
     * RF-Related API Calls
     *************************************************************************/
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

    /*! Select RX antenna \p for channel \p chan.
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
     * \param freq Requested frequency
     * \param chan Channel number.
     * \return The actual frequency.
     */
    virtual double set_rx_frequency(const double freq, const size_t chan) = 0;

    /*! Set the TX tune args, if supported by the hardware.
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

    /*! Return true if this channel has a reference power API enabled
     *
     * Many devices either don't have a built-in reference power API, or they
     * require calibration data for it to work. This means that it is not clear,
     * even when the device type is known, if a device supports setting a power
     * reference level. Use this method to query the availability of
     * set_tx_power_reference() and get_tx_power_reference(), which will throw
     * a uhd::not_implemented_error or uhd::runtime_error if they cannot be used.
     *
     * See \ref page_power for more information, or query the specific device's
     * manual page to see if a power API is available, and how to enable it.
     *
     * \param chan The channel for which this feature is queried
     *
     * \returns true if this channel has a TX power API available
     */
    virtual bool has_tx_power_reference(const size_t chan) = 0;

    /*! Set the reference TX power level for a given channel
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param power_dbm The reference power level in dBm
     * \param chan The channel for which this setting applies
     *
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual void set_tx_power_reference(
        const double power_dbm, const size_t chan) = 0;

    /*! Return the actual reference TX power level.
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param chan The channel for which this setting is queried
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual double get_tx_power_reference(const size_t chan) = 0;

    /*! Return the keys by which the power calibration data is referenced for this
     * channel.
     *
     * The first entry is the key, the second the serial. These are the same
     * arguments that can be used for uhd::usrp::cal::database::read_cal_data()
     * and friends. See also \ref cal_db_serial.
     *
     * Note that the key can change at runtime, e.g., when the antenna port is
     * switched.
     *
     * The difference between this and has_tx_power_reference() is that the
     * latter requires both device support as well as calibration data, whereas
     * this function will never throw, and will always return a non-empty vector
     * if device support is there, even if the device does not have calbration
     * data loaded.
     *
     * \returns an empty vector if no power calibration is supported, or a
     *          vector of length 2 with key and serial if it does.
     */
    virtual std::vector<std::string> get_tx_power_ref_keys(const size_t chan = 0) = 0;

    /*! Return the available TX power range given the current configuration
     *
     * This will return the range of available power levels given the current
     * frequency, gain profile, antenna, and whatever other settings may affect
     * the available power ranges. Note that the available power range may
     * change frequently, so don't assume an immutable range.
     *
     * \param chan The channel index
     */
    virtual meta_range_t get_tx_power_range(const size_t chan) = 0;

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

    /*! Return true if this channel has a reference power API enabled
     *
     * Many devices either don't have a built-in reference power API, or they
     * require calibration data for it to work. This means that it is not clear,
     * even when the device type is known, if a device supports setting a power
     * reference level. Use this method to query the availability of
     * set_rx_power_reference() and get_rx_power_reference(), which will throw
     * a uhd::not_implemented_error or uhd::runtime_error if they cannot be used.
     *
     * See \ref page_power for more information, or query the specific device's
     * manual page to see if a power API is available, and how to enable it.
     *
     * \param chan The channel for which this feature is queried
     *
     * \returns true if this channel has an RX power API available
     */
    virtual bool has_rx_power_reference(const size_t chan) = 0;

    /*! Set the reference RX power level for a given channel
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param power_dbm The reference power level in dBm
     * \param chan The channel for which this setting applies
     *
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual void set_rx_power_reference(
        const double power_dbm, const size_t chan) = 0;

    /*! Return the actual reference RX power level.
     *
     * Note: This functionality is not supported for most devices, and will
     * cause a uhd::not_implemented_error exception to be thrown on devices that
     * do not have this functionality.
     *
     * For more information on how to use this API, see \ref page_power.
     *
     * \param chan The channel for which this setting is queried
     * \throws uhd::not_implemented_error if this functionality does not exist
     *         for this device
     */
    virtual double get_rx_power_reference(const size_t chan) = 0;

    /*! Return the keys by which the power calibration data is referenced for this
     * channel.
     *
     * The first entry is the key, the second the serial. These are the same
     * arguments that can be used for uhd::usrp::cal::database::read_cal_data()
     * and friends. See also \ref cal_db_serial.
     *
     * Note that the key can change at runtime, e.g., when the antenna port is
     * switched.
     *
     * The difference between this and has_rx_power_reference() is that the
     * latter requires both device support as well as calibration data, whereas
     * this function will never throw, and will always return a non-empty vector
     * if device support is there, even if the device does not have calbration
     * data loaded.
     *
     * \returns an empty vector if no power calibration is supported, or a
     *          vector of length 2 with key and serial if it does.
     */
    virtual std::vector<std::string> get_rx_power_ref_keys(const size_t chan = 0) = 0;

    /*! Return the available RX power range given the current configuration
     *
     * This will return the range of available power levels given the current
     * frequency, gain profile, antenna, and whatever other settings may affect
     * the available power ranges. Note that the available power range may
     * change frequently, so don't assume an immutable range.
     *
     * \param chan The channel index
     */
    virtual meta_range_t get_rx_power_range(const size_t chan) = 0;

    /*! Return a list of TX gain profiles for this radio
     */
    virtual std::vector<std::string> get_tx_gain_profile_names(
        const size_t chan) const = 0;

    /*! Return a list of TX gain profiles for this radio
     */
    virtual std::vector<std::string> get_rx_gain_profile_names(
        const size_t chan) const = 0;

    /*! Set the TX gain profile
     */
    virtual void set_tx_gain_profile(const std::string& profile, const size_t chan) = 0;

    /*! Set the RX gain profile
     */
    virtual void set_rx_gain_profile(const std::string& profile, const size_t chan) = 0;

    /*! Return the TX gain profile
     */
    virtual std::string get_tx_gain_profile(const size_t chan) const = 0;

    /*! Set the RX gain profile
     */
    virtual std::string get_rx_gain_profile(const size_t chan) const = 0;

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
    virtual const std::string get_rx_lo_source(
        const std::string& name, const size_t chan) = 0;

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
    virtual bool get_rx_lo_export_enabled(
        const std::string& name, const size_t chan) const = 0;

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
        const std::string& name, const size_t chan) = 0;

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
    virtual const std::string get_tx_lo_source(
        const std::string& name, const size_t chan) = 0;

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

    /**************************************************************************
     * Calibration-Related API Calls
     *************************************************************************/
    /*! Set a constant TX DC offset value
     *
     * The value is complex to control both I and Q.
     *
     * \param offset the dc offset (1.0 is full-scale)
     * \param chan the channel index
     */
    virtual void set_tx_dc_offset(const std::complex<double>& offset, size_t chan) = 0;

    /*! Get the valid range for TX DC offset values.
     *
     * \param chan the channel index
     */
    virtual meta_range_t get_tx_dc_offset_range(size_t chan) const = 0;

    /*! Set the TX frontend IQ imbalance correction.
     *
     * Use this to adjust the magnitude and phase of I and Q.
     *
     * \param correction the complex correction
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_iq_balance(
        const std::complex<double>& correction, size_t chan) = 0;

    /*! Enable/disable the automatic RX DC offset correction.
     * The automatic correction subtracts out the long-run average.
     *
     * When disabled, the averaging option operation is halted.
     * Once halted, the average value will be held constant
     * until the user re-enables the automatic correction
     * or overrides the value by manually setting the offset.
     *
     * \param enb true to enable automatic DC offset correction
     * \param chan the channel index
     */
    virtual void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) = 0;

    /*! Set a constant RX DC offset value.
     *
     * The value is complex to control both I and Q.
     * Only set this when automatic correction is disabled.
     * \param offset the dc offset (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_dc_offset(const std::complex<double>& offset, size_t chan) = 0;

    /*! Get the valid range for RX DC offset values.
     *
     * \param chan the channel index
     */
    virtual meta_range_t get_rx_dc_offset_range(size_t chan) const = 0;

    /*! Enable/disable the automatic IQ imbalance correction.
     *
     * \param enb true to enable automatic IQ balance correction
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_iq_balance(const bool enb, size_t chan) = 0;

    /*! Enable/disable the automatic IQ imbalance correction.
     *
     * \param correction the complex correction (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_iq_balance(
        const std::complex<double>& correction, size_t chan) = 0;

    /**************************************************************************
     * GPIO Controls
     *************************************************************************/
    /*! Returns the list of GPIO banks that are associated with this radio.
     *
     * \returns list of GPIO bank names
     */
    virtual std::vector<std::string> get_gpio_banks() const = 0;

    /*!
     * Set a GPIO attribute on a particular GPIO bank.
     * Possible attribute names:
     *  - CTRL - 1 for ATR mode 0 for GPIO mode
     *  - DDR - 1 for output 0 for input
     *  - OUT - GPIO output level (not ATR mode)
     *  - ATR_0X - ATR idle state
     *  - ATR_RX - ATR receive only state
     *  - ATR_TX - ATR transmit only state
     *  - ATR_XX - ATR full duplex state
     * \param bank the name of a GPIO bank (e.g., FP0)
     * \param attr the name of a GPIO attribute (e.g., CTRL)
     * \param value the new value for this GPIO bank
     */
    virtual void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) = 0;

    /*!
     * Get a GPIO attribute on a particular GPIO bank.
     * Possible attribute names:
     *  - CTRL - 1 for ATR mode 0 for GPIO mode
     *  - DDR - 1 for output 0 for input
     *  - OUT - GPIO output level (not ATR mode)
     *  - ATR_0X - ATR idle state
     *  - ATR_RX - ATR receive only state
     *  - ATR_TX - ATR transmit only state
     *  - ATR_XX - ATR full duplex state
     *  - READBACK - readback input GPIOs
     * \param bank the name of a GPIO bank
     * \param attr the name of a GPIO attribute
     * \return the value set for this attribute
     */
    virtual uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) = 0;

    /**************************************************************************
     * Sensor API
     *************************************************************************/
    /*! Return a list of RX sensors
     *
     * The names returned in this list can be used as an input to get_rx_sensor()
     */
    virtual std::vector<std::string> get_rx_sensor_names(size_t chan) const = 0;

    /*! Return the sensor value for sensor \p name
     *
     * \param name Sensor name (e.g. "lo_locked")
     * \param chan Channel index
     * \throws uhd::key_error if the sensor does not exist
     */
    virtual uhd::sensor_value_t get_rx_sensor(const std::string& name, size_t chan) = 0;

    /*! Return a list of TX sensors
     *
     * The names returned in this list can be used as an input to get_tx_sensor()
     */
    virtual std::vector<std::string> get_tx_sensor_names(size_t chan) const = 0;

    /*! Return the sensor value for sensor \p name
     *
     * \param name Sensor name (e.g. "lo_locked")
     * \param chan Channel index
     * \throws uhd::key_error if the sensor does not exist
     */
    virtual uhd::sensor_value_t get_tx_sensor(const std::string& name, size_t chan) = 0;

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

    /*! Enable or disable the setting of timestamps on Rx.
     */
    virtual void enable_rx_timestamps(const bool enable, const size_t chan) = 0;

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/
    //! Returns this radio's slot name (typically "A" or "B")
    virtual std::string get_slot_name() const = 0;

    //! Return the channel that corresponds to a frontend's name
    //
    // Example: "0" -> 0 (for UBX), or "A" -> 0 (for E310)
    virtual size_t get_chan_from_dboard_fe(
        const std::string& fe, const uhd::direction_t direction) const = 0;

    //! Return the frontend name for a channel index
    //
    // Example: 0 -> "0" (for UBX), or 0 -> "A" (for E310)
    virtual std::string get_dboard_fe_from_chan(
        const size_t chan, const uhd::direction_t direction) const = 0;

    //! Return the name of the frontend, as given by the dboard driver
    virtual std::string get_fe_name(
        const size_t chan, const uhd::direction_t direction) const = 0;

    /**************************************************************************
     * EEPROM API Calls
     *************************************************************************/
    //! Update the daughterboard EEPROM
    //
    // Note: EEPROMs have finite numbers of write cycles, so don't overuse this
    // method!
    virtual void set_db_eeprom(const uhd::eeprom_map_t& db_eeprom) = 0;

    //! Return the content of the daughterboard EEPROM
    virtual uhd::eeprom_map_t get_db_eeprom() = 0;
};

}} // namespace uhd::rfnoc
