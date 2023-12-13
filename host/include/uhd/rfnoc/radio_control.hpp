//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature_getter_iface.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhd/rfnoc/rf_control/power_reference_iface.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/stream_cmd.hpp>

namespace uhd { namespace rfnoc {

/*! Parent class for radio block controllers
 *
 * \ingroup rfnoc_blocks
 *
 * \section rfnoc_radio_overrun_handling Overrun Handling
 *
 * When the radio block on the FPGA detects an overrun (i.e., a buffer filled up
 * and was not emptied fast enough), the radio block sends an asynchronous
 * message to the radio block controller to notify the software of an overrun.
 *
 * When streaming samples into a user application, the radio block is however
 * not the recipient of the streaming data. Rather, the recipient is a streamer
 * object.
 *
 * Because the topology of the RFNoC graph is not known at compile time, overruns
 * are handled with a special protocol. As an example, assume that multiple radios
 * are streaming to the host via a custom DSP block, and that Radio0 has detected
 * an overrun:
 *
 * ```
 * ┌─────────┐ O
 * │ Radio0  ├──┐
 * └─────────┘  │   ┌──────────────────┐    ┌─────────────┐
 *              ├───> Custom DSP Block ├────> RX Streamer │
 * ┌─────────┐  │   └──────────────────┘    └─────────────┘
 * │ Radio1  ├──┘
 * └─────────┘
 * ```
 *
 * In all cases, when the radio block controller is notified of an overrun by
 * the FPGA, it will log a 'O' character (using UHD_LOG_FASTPATH()) and post an
 * uhd::rfnoc::rx_event_action_info object downstream. The custom DSP block may
 * choose to act upon the overrun notification by registering an event handler
 * for such an action message, or it can ignore the message, in which case
 * RFNoC will forward the message downstream to the RX Streamer. If the user is
 * calling uhd::rx_streamer::recv(), then the overrun will be declared as part
 * of the uhd::rx_metadata_t object.
 *
 * Note that the FPGA will immediately stop streaming when overrun occurs, as it
 * has nowhere to put the received sample data. However, there is a special case
 * where UHD performs a more elaborate overrun handling: When the user requested
 * continuous streaming data (the stream command mode was
 * stream_cmd_t::STREAM_MODE_START_CONTINUOUS), the streamer will attempt to
 * restart the stream. This restarting of the stream is implemented as handshake
 * between the streamer and the radio blocks.
 *
 * In the example above, the following steps will be taken:
 * - At some point, the application starts requests a continuous stream from the
 *   radios. The radio block controllers will take a note of this continuous
 *   streaming state.
 * - Now the overrun occurs in Radio0. It will send an overrun notification
 *   downstream. We assume that the custom DSP block is not handling the message.
 *   The rx_event_action_info object with the overrun information is then
 *   delivered to the RX Streamer. This object also contains a flag whether or
 *   not the radio block controller was in continuous streaming mode.
 * - The RX Streamer will switch to overrun handling mode. This is to prevent it
 *   from overreacting from other overrun messages (e.g., Radio1 could be stalled
 *   by Radio0 and then also send an overrun message).
 * - The RX Streamer then sends a stop stream command to all upstream blocks.
 *   This is to ensure that other radios (in this example, Radio1) are no longer
 *   streaming.
 * - At some point, the RX streamer will run out of data to receive (because the
 *   upstream radios have either stopped due to an overrun, or because they were
 *   told to stop by the streamer). When this happens, the rx_streamer::recv()
 *   call will flag an overrun.
 * - If the overrun happened during continuous streaming, the RX streamer will
 *   now issue a restart request to the radio that first flagged an overrun (in
 *   this example, Radio0) by posting another uhd::rfnoc::action_info object.
 * - When Radio0 receives the restart request, it will send a new continuous
 *   stream command downstream to the RX streamer, with a timestamp that is
 *   sufficiently far enough in the future.
 * - The RX streamer sends the start-stream command back upstream to all radios.
 *   This ensures that all upstream radios start streaming at the same time.
 *   It also leaves overrun handling mode.
 * - Note that if either the radio block controller or the RX streamer receive
 *   a stop-stream command from outside, this overrun handling mode is interrupted
 *   and there will be no further attempts to restart the radio.
 */
class radio_control : public noc_block_base,
                      public rf_control::core_iface,
                      public rf_control::power_reference_iface,
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

    //! Return the samples per clock (SPC) value of this radio
    //
    // Some radios may operate on multiple samples per clock cycle, usually in
    // order to handle large bandwidths without requiring very fast FPGA clock
    // rates.
    //
    // When the SPC value is greater than one, certain API calls may behave
    // slightly differently. This is most relevant for issue_stream_cmd(). Other
    // commands may round their execution time to the next integer multiple of
    // SPC as well.
    //
    // Ultimately, the exact impact of SPC is device-dependent.
    virtual size_t get_spc() const = 0;

    /**************************************************************************
     * Time-Related API Calls
     *************************************************************************/
    /*! Get tick count
     * \returns tick count
     * \throws uhd::not_implemented_error if not implemented
     */
    virtual uint64_t get_ticks_now() = 0;
    /*! Get the time
     * \returns time now
     * \throws uhd::not_implemented_error if not implemented
     */
    virtual uhd::time_spec_t get_time_now() = 0;

    /**************************************************************************
     * RF-Related API Calls
     *************************************************************************/
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
     * When the radio is running at multiple samples per clock cycle, there are
     * some restrictions in place:
     * - When requesting a burst of length N, N must be an integer multiple of
     *   SPC. If it's not, the radio will round up to the next integer multiple.
     * - When requesting a start time, the start time may be rounded down such
     *   that the first sample has a tick count value that is an integer multiple
     *   of SPC. That means the sample at the requested time will always be
     *   produced, but it might not be the first sample to be returned.
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
