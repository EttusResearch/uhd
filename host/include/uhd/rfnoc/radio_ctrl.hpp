//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP
#define INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP

#include <uhd/types/ranges.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/rfnoc/sink_block_ctrl_base.hpp>
#include <uhd/rfnoc/rate_node_ctrl.hpp>
#include <uhd/rfnoc/tick_node_ctrl.hpp>
#include <uhd/rfnoc/scalar_node_ctrl.hpp>
#include <uhd/rfnoc/terminator_node_ctrl.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Block controller for all RFNoC-based radio blocks
 */
class UHD_RFNOC_API radio_ctrl :
    public source_block_ctrl_base,
    public sink_block_ctrl_base,
    public rate_node_ctrl,
    public tick_node_ctrl,
    public terminator_node_ctrl
{
public:
    UHD_RFNOC_BLOCK_OBJECT(radio_ctrl)

    virtual ~radio_ctrl(){}


    //! A wildcard channel index
    static const size_t ALL_CHANS = size_t(~0);

    //! A wildcard gain element name
    static const std::string ALL_GAINS;

    //! A wildcard local oscillator element name
    static const std::string ALL_LOS;

    /************************************************************************
     * API calls
     ***********************************************************************/
    /*! Return the tick rate on all channels (rx and tx).
     *
     * \return The tick rate.
     */
    virtual double get_rate() const = 0;

    /*! Set the tick/sample rate on all channels (rx and tx).
     *
     * Will coerce to the nearest possible rate and return the actual value.
     */
    virtual double set_rate(double rate) = 0;

    /*! Return the selected TX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_tx_antenna(const size_t chan) /* const */ = 0;

    /*! Select RX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_tx_antenna(const std::string &ant, const size_t chan) = 0;

    /*! Return the selected RX antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_rx_antenna(const size_t chan) /* const */ = 0;

    /*! Select RX antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_rx_antenna(const std::string &ant, const size_t chan) = 0;

    /*! Return the current transmit LO frequency on channel \p chan.
     *
     * Note that the AD9361 only has one LO for all TX channels, and the
     * \p chan parameter is thus only for API compatibility.
     *
     * \return The current LO frequency.
     */
    virtual double get_tx_frequency(const size_t chan) /* const */ = 0;

    /*! Tune the TX LO for channel \p chan.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * \param freq Frequency in Hz
     * \param chan Channel to tune
     *
     * \return The actual LO frequency.
     */
    virtual double set_tx_frequency(const double freq, size_t chan) = 0;

    /*! Return the current receive LO frequency on channel \p chan.
     *
     * \return The current LO frequency.
     */
    virtual double get_rx_frequency(const size_t chan) /* const */ = 0;

    /*! Tune the RX LO for channel \p chan.
     *
     * This function will attempt to tune as close as possible, and return a
     * coerced value of the actual tuning result.
     *
     * \param freq Requested LO frequency
     * \param chan Channel number.
     * \return The actual LO frequency.
     */
    virtual double set_rx_frequency(const double freq, const size_t chan) = 0;

    /*! Return the transmit gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_tx_gain(const size_t chan) = 0;

    /*! Set the transmit gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_tx_gain(const double gain, const size_t chan) = 0;

    /*! Return the transmit gain on channel \p chan
     *
     * \return The actual gain value
     */
    virtual double get_rx_gain(const size_t chan) = 0;

    /*! Set the transmit gain on channel \p chan
     *
     * This function will attempt to set the gain as close as possible,
     * and return a coerced value of the actual gain value.
     *
     * \return The actual gain value
     */
    virtual double set_rx_gain(const double gain, const size_t chan) = 0;

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
    virtual double set_tx_bandwidth(
        const double bandwidth,
        const size_t chan
    ) = 0;

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

    /*! Sets the time in the radio's timekeeper to the given value.
     *
     * Note that there is a non-deterministic delay between calling this
     * function and the valung written to the register. For setting the
     * time in alignment with a certain reference time, use
     * set_time_next_pps().
     */
    virtual void set_time_now(const time_spec_t &time_spec) = 0;

    /*! Set the time registers at the next pps tick.
     *
     * The values will not be latched in until the pulse occurs.
     * It is recommended that the user sleep(1) after calling to ensure
     * that the time registers will be in a known state prior to use.
     *
     * Note: Because this call sets the time on the "next" pps,
     * the seconds in the time spec should be current seconds + 1.
     *
     * \param time_spec the time to latch into the timekeeper
     */
    virtual void set_time_next_pps(const time_spec_t &time_spec) = 0;

    /*! Get the current time in the timekeeper registers.
     *
     * Note that there is a non-deterministic delay between the time the
     * register is read and the time the function value is returned.
     * To get the time with respect to a tick edge, use get_time_last_pps().
     *
     * \return A timespec representing current radio time
     */
    virtual time_spec_t get_time_now() = 0;

    /*! Get the time when the last PPS pulse occurred.
     *
     * \return A timespec representing the last PPS
     */
    virtual time_spec_t get_time_last_pps() = 0;

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
     * \param mask the bit mask to effect which pins are changed
     */
    virtual void set_gpio_attr(
            const std::string &bank,
            const std::string &attr,
            const uint32_t value,
            const uint32_t mask
    ) = 0;

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
    virtual uint32_t get_gpio_attr(const std::string &bank, const std::string &attr) = 0;

    /**************************************************************************
     * LO Controls
     *************************************************************************/
    /*! Get a list of possible LO stage names
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names
     */
    virtual std::vector<std::string> get_rx_lo_names(const size_t chan) = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources
     * will return "internal".
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_rx_lo_sources(const std::string &name, const size_t chan) = 0;

    /*!
     * Get the LO frequency range of the RX LO.
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_rx_lo_freq_range(const std::string &name, const size_t chan) = 0;

    /*!
     * Set the LO source for a channel.
     * For usrps that support selectable LOs, this function
     * allows switching between them.
     * Typical options for source: internal, external.
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_lo_source(const std::string &src, const std::string &name, const size_t chan) = 0;

    /*!
     * Get the currently set LO source.
     * Channels without controllable LO sources will return
     * "internal"
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual const std::string get_rx_lo_source(const std::string &name, const size_t chan) = 0;

    /*!
     * Set whether the LO used by the usrp device is exported
     * For usrps that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     */
    virtual void set_rx_lo_export_enabled(bool enabled, const std::string &name, const size_t chan) = 0;

    /*!
     * Returns true if the currently selected LO is being exported.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_rx_lo_export_enabled(const std::string &name, const size_t chan) = 0;

    /*!
     * Set the RX LO frequency (Advanced).
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     */
    virtual double set_rx_lo_freq(double freq, const std::string &name, const size_t chan) = 0;

    /*!
     * Get the current RX LO frequency (Advanced).
     * If the channel does not have independently configurable LOs
     * the current rf frequency will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_rx_lo_freq(const std::string &name, const size_t chan) = 0;

    /*! Get a list of possible LO stage names
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names
     */
    virtual std::vector<std::string> get_tx_lo_names(const size_t chan) = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources
     * will return "internal".
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_tx_lo_sources(
            const std::string &name,
            const size_t chan
    ) = 0;

    /*!
     * Get the LO frequency range of the tx LO.
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_tx_lo_freq_range(
            const std::string &name,
            const size_t chan
    ) = 0;

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
            const std::string &src,
            const std::string &name,
            const size_t chan
    ) = 0;

    /*!
     * Get the currently set LO source.
     * Channels without controllable LO sources will return
     * "internal"
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual const std::string get_tx_lo_source(
            const std::string &name,
            const size_t chan
    ) = 0;

    /*!
     * Set whether the LO used by the usrp device is exported
     * For usrps that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     */
    virtual void set_tx_lo_export_enabled(
            const bool enabled,
            const std::string &name,
            const size_t chan
    ) = 0;

    /*!
     * Returns true if the currently selected LO is being exported.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_tx_lo_export_enabled(
            const std::string &name,
            const size_t chan
    ) = 0;

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
            const double freq,
            const std::string &name,
            const size_t chan
    ) = 0;

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
    virtual double get_tx_lo_freq(
            const std::string &name,
            const size_t chan
    ) = 0;

    /**************************************************************************
     * Time and clock control
     *************************************************************************/

    /*! 
     * Set the time source for this radio.
     *
     * May affect other radio blocks.
     *
     * \param source A string representing the time source
     * \throws uhd::value_error if the value can't be applied
     */
    virtual void set_time_source(const std::string &source) = 0;

    /*!
     * Get the currently set time source.
     *
     * \return the string representing the time source
     */
    virtual std::string get_time_source() = 0;

    /*!
     * Get a list of possible time sources.
     *
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_time_sources() = 0;

    /*!
     * Set the clock source for the usrp device (for reference clock).
     *
     * Typical options for source: internal, external.
     *
     * \param source a string representing the clock source
     */
    virtual void set_clock_source(const std::string &source) = 0;

    /*!
     * Get the currently set clock source.
     *
     * \return the string representing the clock source
     */
    virtual std::string get_clock_source() = 0;

    /*!
     * Get a list of possible clock sources.
     *
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_clock_sources() = 0;

    /*! Given a frontend name, return the channel mapping.
     *
     * E.g.: For a TwinRX board, there's two frontends, '0' and '1', which
     * map to channels 0 and 1 respectively. A BasicRX boards has alphabetical
     * frontends (A, B) which map to channels differently.
     */
    virtual size_t get_chan_from_dboard_fe(const std::string &fe, const uhd::direction_t dir) = 0;

    /*! The inverse function to get_chan_from_dboard_fe()
     */
    virtual std::string get_dboard_fe_from_chan(const size_t chan, const uhd::direction_t dir) = 0;

}; /* class radio_ctrl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RADIO_CTRL_HPP */
