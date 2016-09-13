//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_USRP_MULTI_USRP_HPP
#define INCLUDED_UHD_USRP_MULTI_USRP_HPP

//define API capabilities for compile time detection of new features
#define UHD_USRP_MULTI_USRP_REF_SOURCES_API
#define UHD_USRP_MULTI_USRP_GET_RATES_API
#define UHD_USRP_MULTI_USRP_FRONTEND_CAL_API
#define UHD_USRP_MULTI_USRP_FRONTEND_IQ_AUTO_API
#define UHD_USRP_MULTI_USRP_COMMAND_TIME_API
#define UHD_USRP_MULTI_USRP_BW_RANGE_API
#define UHD_USRP_MULTI_USRP_USER_REGS_API
#define UHD_USRP_MULTI_USRP_GET_USRP_INFO_API
#define UHD_USRP_MULTI_USRP_NORMALIZED_GAIN
#define UHD_USRP_MULTI_USRP_GPIO_API
#define UHD_USRP_MULTI_USRP_REGISTER_API
#define UHD_USRP_MULTI_USRP_FILTER_API
#define UHD_USRP_MULTI_USRP_LO_CONFIG_API

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/deprecated.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/filters.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <complex>
#include <string>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The Multi-USRP device class:
 *
 * This class facilitates ease-of-use for most use-case scenarios.
 * The wrapper provides convenience functions to tune the devices,
 * set the dboard gains, antennas, filters, and other properties.
 * This class can be used to interface with a single USRP with
 * one or more channels, or multiple USRPs in a homogeneous setup.
 * All members take an optional parameter for board number or channel number.
 * In the single device, single channel case, these parameters can be unspecified.
 *
 * When using a single device with multiple channels:
 *  - Channel mapping is determined by the frontend specifications
 *  - All channels share a common RX sample rate
 *  - All channels share a common TX sample rate
 *
 * When using multiple devices in a configuration:
 *  - Channel mapping is determined by the device address arguments
 *  - All boards share a common RX sample rate
 *  - All boards share a common TX sample rate
 *  - All boards share a common RX frontend specification size
 *  - All boards share a common TX frontend specification size
 *  - All boards must have synchronized times (see the set_time_*() calls)
 *
 * Example to setup channel mapping for multiple devices:
 * <pre>
 *
 * //create a multi_usrp with two boards in the configuration
 * device_addr_t dev_addr;
 * dev_addr["addr0"] = "192.168.10.2"
 * dev_addr["addr1"] = "192.168.10.3";
 * multi_usrp::sptr dev = multi_usrp::make(dev_addr);
 *
 * //set the board on 10.2 to use the A RX frontend (RX channel 0)
 * dev->set_rx_subdev_spec("A:A", 0);
 *
 * //set the board on 10.3 to use the B RX frontend (RX channel 1)
 * dev->set_rx_subdev_spec("A:B", 1);
 *
 * //set both boards to use the AB TX frontend (TX channels 0 and 1)
 * dev->set_tx_subdev_spec("A:AB", multi_usrp::ALL_MBOARDS);
 *
 * //now that all the channels are mapped, continue with configuration...
 *
 * </pre>
 */
class UHD_API multi_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<multi_usrp> sptr;

    virtual ~multi_usrp(void) = 0;

    //! A wildcard motherboard index
    static const size_t ALL_MBOARDS = size_t(~0);

    //! A wildcard channel index
    static const size_t ALL_CHANS = size_t(~0);

    //! A wildcard gain element name
    static const std::string ALL_GAINS;

    //! A wildcard gain element name
    static const std::string ALL_LOS;

    /*!
     * Make a new multi usrp from the device address.
     * \param dev_addr the device address
     * \return a new single usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this USRP
     */
    virtual device::sptr get_device(void) = 0;

    //! Convenience method to get a RX streamer. See also uhd::device::get_rx_stream().
    virtual rx_streamer::sptr get_rx_stream(const stream_args_t &args) = 0;

    //! Convenience method to get a TX streamer. See also uhd::device::get_rx_stream().
    virtual tx_streamer::sptr get_tx_stream(const stream_args_t &args) = 0;

    /*!
     * Returns identifying information about this USRP's configuration.
     * Returns motherboard ID, name, and serial.
     * Returns daughterboard RX ID, subdev name and spec, serial, and antenna.
     * \param chan channel index 0 to N-1
     * \return RX info
     */
    virtual dict<std::string, std::string> get_usrp_rx_info(size_t chan = 0) = 0;

    /*!
     * Returns identifying information about this USRP's configuration.
     * Returns motherboard ID, name, and serial.
     * Returns daughterboard TX ID, subdev name and spec, serial, and antenna.
     * \param chan channel index 0 to N-1
     * \return TX info
     */
     virtual dict<std::string, std::string> get_usrp_tx_info(size_t chan = 0) = 0;

    /*******************************************************************
     * Mboard methods
     ******************************************************************/

    /*!
     * Set the master clock rate.
     * This controls the rate of the clock that feeds the FPGA DSP.
     * On some devices, this re-tunes the clock to the specified rate.
     * If the specified rate is not available, this method will throw.
     * On other devices, this method notifies the software of the rate,
     * but requires the the user has made the necessary hardware change.
     *
     * If the device has an 'auto clock rate' setting (e.g. B200, see also
     * \ref b200_auto_mcr), this will get disabled and the clock rate will be
     * fixed to \p rate.
     *
     * \param rate the new master clock rate in Hz
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_master_clock_rate(double rate, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the master clock rate.
     * \param mboard the motherboard index 0 to M-1
     * \return the master clock rate in Hz.
     */
    virtual double get_master_clock_rate(size_t mboard = 0) = 0;

    /*!
     * Get a printable summary for this USRP configuration.
     * \return a printable string
     */
    virtual std::string get_pp_string(void) = 0;

    /*!
     * Get canonical name for this USRP motherboard.
     * \param mboard which motherboard to query
     * \return a string representing the name
     */
    virtual std::string get_mboard_name(size_t mboard = 0) = 0;

    /*!
     * Get the current time in the usrp time registers.
     * \param mboard which motherboard to query
     * \return a timespec representing current usrp time
     */
    virtual time_spec_t get_time_now(size_t mboard = 0) = 0;

    /*!
     * Get the time when the last pps pulse occurred.
     * \param mboard which motherboard to query
     * \return a timespec representing the last pps
     */
    virtual time_spec_t get_time_last_pps(size_t mboard = 0) = 0;

    /*!
     * Sets the time registers on the usrp immediately.
     *
     * If only one MIMO master is present in your configuration, set_time_now is
     * safe to use because the slave's time automatically follows the master's time.
     * Otherwise, this call cannot set the time synchronously across multiple devices.
     * Please use the set_time_next_pps or set_time_unknown_pps calls with a PPS signal.
     *
     * \param time_spec the time to latch into the usrp device
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_time_now(const time_spec_t &time_spec, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Set the time registers on the usrp at the next pps tick.
     * The values will not be latched in until the pulse occurs.
     * It is recommended that the user sleep(1) after calling to ensure
     * that the time registers will be in a known state prior to use.
     *
     * Note: Because this call sets the time on the "next" pps,
     * the seconds in the time spec should be current seconds + 1.
     *
     * \param time_spec the time to latch into the usrp device
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_time_next_pps(const time_spec_t &time_spec, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Synchronize the times across all motherboards in this configuration.
     * Use this method to sync the times when the edge of the PPS is unknown.
     *
     * Ex: Host machine is not attached to serial port of GPSDO
     * and can therefore not query the GPSDO for the PPS edge.
     *
     * This is a 2-step process, and will take at most 2 seconds to complete.
     * Upon completion, the times will be synchronized to the time provided.
     *
     * - Step1: wait for the last pps time to transition to catch the edge
     * - Step2: set the time at the next pps (synchronous for all boards)
     *
     * \param time_spec the time to latch at the next pps after catching the edge
     */
    virtual void set_time_unknown_pps(const time_spec_t &time_spec) = 0;

    /*!
     * Are the times across all motherboards in this configuration synchronized?
     * Checks that all time registers are approximately close but not exact,
     * given that the RTT may varying for a control packet transaction.
     * \return true when all motherboards time registers are in sync
     */
    virtual bool get_time_synchronized(void) = 0;

    /*!
     * Set the time at which the control commands will take effect.
     *
     * A timed command will back-pressure all subsequent timed commands,
     * assuming that the subsequent commands occur within the time-window.
     * If the time spec is late, the command will be activated upon arrival.
     *
     * \param time_spec the time at which the next command will activate
     * \param mboard which motherboard to set the config
     */
    virtual void set_command_time(const uhd::time_spec_t &time_spec, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Clear the command time so future commands are sent ASAP.
     *
     * \param mboard which motherboard to set the config
     */
    virtual void clear_command_time(size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Issue a stream command to the usrp device.
     * This tells the usrp to send samples into the host.
     * See the documentation for stream_cmd_t for more info.
     *
     * With multiple devices, the first stream command in a chain of commands
     * should have a time spec in the near future and stream_now = false;
     * to ensure that the packets can be aligned by their time specs.
     *
     * \param stream_cmd the stream command to issue
     * \param chan the channel index 0 to N-1
     */
    virtual void issue_stream_cmd(const stream_cmd_t &stream_cmd, size_t chan = ALL_CHANS) = 0;

    /*!
     * Set the clock configuration for the usrp device.
     * DEPRECATED in favor of set time and clock source calls.
     * This tells the usrp how to get a 10MHz reference and PPS clock.
     * See the documentation for clock_config_t for more info.
     * \param clock_config the clock configuration to set
     * \param mboard which motherboard to set the config
     */
    virtual void set_clock_config(const clock_config_t &clock_config, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Set the time source for the usrp device.
     * This sets the method of time synchronization,
     * typically a pulse per second or an encoded time.
     * Typical options for source: external, MIMO.
     * \param source a string representing the time source
     * \param mboard which motherboard to set the config
     */
    virtual void set_time_source(const std::string &source, const size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the currently set time source.
     * \param mboard which motherboard to get the config
     * \return the string representing the time source
     */
    virtual std::string get_time_source(const size_t mboard) = 0;

    /*!
     * Get a list of possible time sources.
     * \param mboard which motherboard to get the list
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_time_sources(const size_t mboard) = 0;

    /*!
     * Set the clock source for the usrp device.
     * This sets the source for a 10 MHz reference clock.
     * Typical options for source: internal, external, MIMO.
     * \param source a string representing the clock source
     * \param mboard which motherboard to set the config
     */
    virtual void set_clock_source(const std::string &source, const size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the currently set clock source.
     * \param mboard which motherboard to get the config
     * \return the string representing the clock source
     */
    virtual std::string get_clock_source(const size_t mboard) = 0;

    /*!
     * Get a list of possible clock sources.
     * \param mboard which motherboard to get the list
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_clock_sources(const size_t mboard) = 0;

    /*!
     * Send the clock source to an output connector.
     * This call is only applicable on devices with reference outputs.
     * By default, the reference output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     * \param enb true to output the clock source.
     * \param mboard which motherboard to set
     */
    virtual void set_clock_source_out(const bool enb, const size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Send the time source to an output connector.
     * This call is only applicable on devices with PPS outputs.
     * By default, the PPS output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     * \param enb true to output the time source.
     * \param mboard which motherboard to set
     */
    virtual void set_time_source_out(const bool enb, const size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the number of USRP motherboards in this configuration.
     */
    virtual size_t get_num_mboards(void) = 0;

    /*!
     * Get a motherboard sensor value.
     * \param name the name of the sensor
     * \param mboard the motherboard index 0 to M-1
     * \return a sensor value object
     */
    virtual sensor_value_t get_mboard_sensor(const std::string &name, size_t mboard = 0) = 0;

    /*!
     * Get a list of possible motherboard sensor names.
     * \param mboard the motherboard index 0 to M-1
     * \return a vector of sensor names
     */
    virtual std::vector<std::string> get_mboard_sensor_names(size_t mboard = 0) = 0;

    /*!
     * Perform write on the user configuration register bus. These only exist if
     * the user has implemented custom setting registers in the device FPGA.
     * \param addr 8-bit register address
     * \param data 32-bit register value
     * \param mboard which motherboard to set the user register
     */
    virtual void set_user_register(const boost::uint8_t addr, const boost::uint32_t data, size_t mboard = ALL_MBOARDS) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/
    /*!
     * Set the RX frontend specification:
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     * \param spec the new frontend specification
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the RX frontend specification.
     * \param mboard the motherboard index 0 to M-1
     * \return the frontend specification in use
     */
    virtual uhd::usrp::subdev_spec_t get_rx_subdev_spec(size_t mboard = 0) = 0;

    /*!
     * Get the number of RX channels in this configuration.
     * This is the number of USRPs times the number of RX channels per board,
     * where the number of RX channels per board is homogeneous among all USRPs.
     */
    virtual size_t get_rx_num_channels(void) = 0;

    /*!
     * Get the name of the RX frontend.
     * \param chan the channel index 0 to N-1
     * \return the frontend name
     */
    virtual std::string get_rx_subdev_name(size_t chan = 0) = 0;

    /*!
     * Set the RX sample rate.
     * \param rate the rate in Sps
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_rate(double rate, size_t chan = ALL_CHANS) = 0;

    /*!
     * Gets the RX sample rate.
     * \param chan the channel index 0 to N-1
     * \return the rate in Sps
     */
    virtual double get_rx_rate(size_t chan = 0) = 0;

    /*!
     * Get a range of possible RX rates.
     * \param chan the channel index 0 to N-1
     * \return the meta range of rates
     */
    virtual meta_range_t get_rx_rates(size_t chan = 0) = 0;

    /*!
     * Set the RX center frequency.
     * \param tune_request tune request instructions
     * \param chan the channel index 0 to N-1
     * \return a tune result object
     */
    virtual tune_result_t set_rx_freq(
        const tune_request_t &tune_request, size_t chan = 0
    ) = 0;

    /*!
     * Get the RX center frequency.
     * \param chan the channel index 0 to N-1
     * \return the frequency in Hz
     */
    virtual double get_rx_freq(size_t chan = 0) = 0;

    /*!
     * Get the RX center frequency range.
     * This range includes the overall tunable range of the RX chain,
     * including frontend chain and digital down conversion chain.
     * This tunable limit does not include the baseband bandwidth;
     * users should assume that the actual range is +/- samp_rate/2.
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_rx_freq_range(size_t chan = 0) = 0;

    /*!
     * Get the center frequency range of the RF frontend.
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_fe_rx_freq_range(size_t chan = 0) = 0;

    /*!
     * Get a list of possible LO stage names
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names
     */
    virtual std::vector<std::string> get_rx_lo_names(size_t chan = 0) = 0;

    /*!
     * Set the LO source for the usrp device.
     * For usrps that support selectable LOs, this function
     * allows switching between them.
     * Typical options for source: internal, external.
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_lo_source(const std::string &src, const std::string &name = ALL_LOS, size_t chan = 0) = 0;

    /*!
     * Get the currently set LO source.
     * Channels without controllable LO sources will return
     * "internal"
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual const std::string get_rx_lo_source(const std::string &name = ALL_LOS, size_t chan = 0) = 0;

    /*!
     * Get a list of possible LO sources.
     * Channels which do not have controllable LO sources
     * will return "internal".
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_rx_lo_sources(const std::string &name = ALL_LOS, size_t chan = 0) = 0;

    /*!
     * Set whether the LO used by the usrp device is exported
     * For usrps that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     */
    virtual void set_rx_lo_export_enabled(bool enabled, const std::string &name = ALL_LOS, size_t chan = 0) = 0;

    /*!
     * Returns true if the currently selected LO is being exported.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_rx_lo_export_enabled(const std::string &name = ALL_LOS, size_t chan = 0) = 0;

    /*!
     * Set the RX LO frequency (Advanced).
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     */
    virtual double set_rx_lo_freq(double freq, const std::string &name, size_t chan = 0) = 0;

    /*!
     * Get the current RX LO frequency (Advanced).
     * If the channel does not have independently configurable LOs
     * the current rf frequency will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_rx_lo_freq(const std::string &name, size_t chan = 0) = 0;

    /*!
     * Get the LO frequency range of the RX LO.
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_rx_lo_freq_range(const std::string &name, size_t chan = 0) = 0;

    /*!
     * Set the RX gain value for the specified gain element.
     * For an empty name, distribute across all gain elements.
     * \param gain the gain in dB
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_gain(double gain, const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for setting overall RX gain
    void set_rx_gain(double gain, size_t chan = 0){
        return this->set_rx_gain(gain, ALL_GAINS, chan);
    }

    /*!
     * Set the normalized RX gain value.
     *
     * The normalized gain is a value in [0, 1], where 0 is the
     * smallest gain value available, and 1 is the largest, independent
     * of the device. In between, gains are linearly interpolated.
     *
     * Check the individual device manual for notes on the gain range.
     *
     * Note that it is not possible to specify a gain name for
     * this function, it will always set the overall gain.
     *
     * \param gain the normalized gain value
     * \param chan the channel index 0 to N-1
     * \throws A uhd::runtime_error if the gain value is outside [0, 1].
     */
    virtual void set_normalized_rx_gain(double gain, size_t chan = 0) = 0;

    /*!
     * Enable or disable the RX AGC module.
     * Once this module is enabled manual gain settings will be ignored.
     * The AGC will start in a default configuration which should be good for most use cases.
     * Device specific configuration parameters can be found in the property tree.
     * \param enable Enable or Disable the AGC
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_agc(bool enable, size_t chan = 0) = 0;

    /*!
     * Get the RX gain value for the specified gain element.
     * For an empty name, sum across all gain elements.
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     * \return the gain in dB
     */
    virtual double get_rx_gain(const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall RX gain
    double get_rx_gain(size_t chan = 0){
        return this->get_rx_gain(ALL_GAINS, chan);
    }

    /*!
     * Return the normalized RX gain value.
     *
     * See set_normalized_rx_gain() for a discussion of normalized
     * gains.
     *
     * \param chan the channel index 0 to N-1
     * \returns The normalized gain (in [0, 1])
     * \throws A uhd::runtime_error if the gain value is outside [0, 1].
     */
    virtual double get_normalized_rx_gain(size_t chan = 0) = 0;

    /*!
     * Get the RX gain range for the specified gain element.
     * For an empty name, calculate the overall gain range.
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     * \return a gain range object
     */
    virtual gain_range_t get_rx_gain_range(const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall RX gain range
    gain_range_t get_rx_gain_range(size_t chan = 0){
        return this->get_rx_gain_range(ALL_GAINS, chan);
    }

    /*!
     * Get the names of the gain elements in the RX chain.
     * Gain elements are ordered from antenna to FPGA.
     * \param chan the channel index 0 to N-1
     * \return a vector of gain element names
     */
    virtual std::vector<std::string> get_rx_gain_names(size_t chan = 0) = 0;

    /*!
     * Select the RX antenna on the frontend.
     * \param ant the antenna name
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_antenna(const std::string &ant, size_t chan = 0) = 0;

    /*!
     * Get the selected RX antenna on the frontend.
     * \param chan the channel index 0 to N-1
     * \return the antenna name
     */
    virtual std::string get_rx_antenna(size_t chan = 0) = 0;

    /*!
     * Get a list of possible RX antennas on the frontend.
     * \param chan the channel index 0 to N-1
     * \return a vector of antenna names
     */
    virtual std::vector<std::string> get_rx_antennas(size_t chan = 0) = 0;

    /*!
     * Set the RX bandwidth on the frontend.
     * \param bandwidth the bandwidth in Hz
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_bandwidth(double bandwidth, size_t chan = 0) = 0;

    /*!
     * Get the RX bandwidth on the frontend.
     * \param chan the channel index 0 to N-1
     * \return the bandwidth in Hz
     */
    virtual double get_rx_bandwidth(size_t chan = 0) = 0;

    /*!
     * Get the range of the possible RX bandwidth settings.
     * \param chan the channel index 0 to N-1
     * \return a range of bandwidths in Hz
     */
    virtual meta_range_t get_rx_bandwidth_range(size_t chan = 0) = 0;

    /*!
     * Get the dboard interface object for the RX frontend.
     * The dboard interface gives access to GPIOs, SPI, I2C, low-speed ADC and DAC.
     * Use at your own risk!
     * \param chan the channel index 0 to N-1
     * \return the dboard interface sptr
     */
    virtual dboard_iface::sptr get_rx_dboard_iface(size_t chan = 0) = 0;

    /*!
     * Get an RX frontend sensor value.
     * \param name the name of the sensor
     * \param chan the channel index 0 to N-1
     * \return a sensor value object
     */
    virtual sensor_value_t get_rx_sensor(const std::string &name, size_t chan = 0) = 0;

    /*!
     * Get a list of possible RX frontend sensor names.
     * \param chan the channel index 0 to N-1
     * \return a vector of sensor names
     */
    virtual std::vector<std::string> get_rx_sensor_names(size_t chan = 0) = 0;

    /*!
     * Enable/disable the automatic RX DC offset correction.
     * The automatic correction subtracts out the long-run average.
     *
     * When disabled, the averaging option operation is halted.
     * Once halted, the average value will be held constant
     * until the user re-enables the automatic correction
     * or overrides the value by manually setting the offset.
     *
     * \param enb true to enable automatic DC offset correction
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) = 0;

    /*!
     * Set a constant RX DC offset value.
     * The value is complex to control both I and Q.
     * Only set this when automatic correction is disabled.
     * \param offset the dc offset (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_dc_offset(const std::complex<double> &offset, size_t chan = ALL_CHANS) = 0;

    /*!
     * Enable/disable the automatic IQ imbalance correction.
     *
     * \param enb true to enable automatic IQ balance correction
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_iq_balance(const bool enb, size_t chan) = 0;

    /*!
     * Set the RX frontend IQ imbalance correction.
     * Use this to adjust the magnitude and phase of I and Q.
     *
     * \param correction the complex correction (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_iq_balance(const std::complex<double> &correction, size_t chan = ALL_CHANS) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    /*!
     * Set the TX frontend specification:
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     * \param spec the new frontend specification
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec, size_t mboard = ALL_MBOARDS) = 0;

    /*!
     * Get the TX frontend specification.
     * \param mboard the motherboard index 0 to M-1
     * \return the frontend specification in use
     */
    virtual uhd::usrp::subdev_spec_t get_tx_subdev_spec(size_t mboard = 0) = 0;

    /*!
     * Get the number of TX channels in this configuration.
     * This is the number of USRPs times the number of TX channels per board,
     * where the number of TX channels per board is homogeneous among all USRPs.
     */
    virtual size_t get_tx_num_channels(void) = 0;

    /*!
     * Get the name of the TX frontend.
     * \param chan the channel index 0 to N-1
     * \return the frontend name
     */
    virtual std::string get_tx_subdev_name(size_t chan = 0) = 0;

    /*!
     * Set the TX sample rate.
     * \param rate the rate in Sps
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_rate(double rate, size_t chan = ALL_CHANS) = 0;

    /*!
     * Gets the TX sample rate.
     * \param chan the channel index 0 to N-1
     * \return the rate in Sps
     */
    virtual double get_tx_rate(size_t chan = 0) = 0;

    /*!
     * Get a range of possible TX rates.
     * \param chan the channel index 0 to N-1
     * \return the meta range of rates
     */
    virtual meta_range_t get_tx_rates(size_t chan = 0) = 0;

    /*!
     * Set the TX center frequency.
     * \param tune_request tune request instructions
     * \param chan the channel index 0 to N-1
     * \return a tune result object
     */
    virtual tune_result_t set_tx_freq(
        const tune_request_t &tune_request, size_t chan = 0
    ) = 0;

    /*!
     * Get the TX center frequency.
     * \param chan the channel index 0 to N-1
     * \return the frequency in Hz
     */
    virtual double get_tx_freq(size_t chan = 0) = 0;

    /*!
     * Get the TX center frequency range.
     * This range includes the overall tunable range of the TX chain,
     * including frontend chain and digital up conversion chain.
     * This tunable limit does not include the baseband bandwidth;
     * users should assume that the actual range is +/- samp_rate/2.
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_tx_freq_range(size_t chan = 0) = 0;

    /*!
     * Get the center frequency range of the TX frontend.
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_fe_tx_freq_range(size_t chan = 0) = 0;

    /*!
     * Set the TX gain value for the specified gain element.
     * For an empty name, distribute across all gain elements.
     * \param gain the gain in dB
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_gain(double gain, const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for setting overall TX gain
    void set_tx_gain(double gain, size_t chan = 0){
        return this->set_tx_gain(gain, ALL_GAINS, chan);
    }

    /*!
     * Set the normalized TX gain value.
     *
     * See set_normalized_rx_gain() for a discussion on normalized
     * gains.
     *
     * \param gain the normalized gain value
     * \param chan the channel index 0 to N-1
     * \throws A uhd::runtime_error if the gain value is outside [0, 1].
     */
    virtual void set_normalized_tx_gain(double gain, size_t chan = 0) = 0;

    /*!
     * Get the TX gain value for the specified gain element.
     * For an empty name, sum across all gain elements.
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     * \return the gain in dB
     */
    virtual double get_tx_gain(const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall TX gain
    double get_tx_gain(size_t chan = 0){
        return this->get_tx_gain(ALL_GAINS, chan);
    }

    /*!
     * Return the normalized TX gain value.
     *
     * See set_normalized_rx_gain() for a discussion of normalized
     * gains.
     *
     * \param chan the channel index 0 to N-1
     * \returns The normalized gain (in [0, 1])
     * \throws A uhd::runtime_error if the gain value is outside [0, 1].
     */
    virtual double get_normalized_tx_gain(size_t chan = 0) = 0;

    /*!
     * Get the TX gain range for the specified gain element.
     * For an empty name, calculate the overall gain range.
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     * \return a gain range object
     */
    virtual gain_range_t get_tx_gain_range(const std::string &name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall TX gain range
    gain_range_t get_tx_gain_range(size_t chan = 0){
        return this->get_tx_gain_range(ALL_GAINS, chan);
    }

    /*!
     * Get the names of the gain elements in the TX chain.
     * Gain elements are ordered from antenna to FPGA.
     * \param chan the channel index 0 to N-1
     * \return a vector of gain element names
     */
    virtual std::vector<std::string> get_tx_gain_names(size_t chan = 0) = 0;

    /*!
     * Select the TX antenna on the frontend.
     * \param ant the antenna name
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_antenna(const std::string &ant, size_t chan = 0) = 0;

    /*!
     * Get the selected TX antenna on the frontend.
     * \param chan the channel index 0 to N-1
     * \return the antenna name
     */
    virtual std::string get_tx_antenna(size_t chan = 0) = 0;

    /*!
     * Get a list of possible TX antennas on the frontend.
     * \param chan the channel index 0 to N-1
     * \return a vector of antenna names
     */
    virtual std::vector<std::string> get_tx_antennas(size_t chan = 0) = 0;

    /*!
     * Set the TX bandwidth on the frontend.
     * \param bandwidth the bandwidth in Hz
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_bandwidth(double bandwidth, size_t chan = 0) = 0;

    /*!
     * Get the TX bandwidth on the frontend.
     * \param chan the channel index 0 to N-1
     * \return the bandwidth in Hz
     */
    virtual double get_tx_bandwidth(size_t chan = 0) = 0;

    /*!
     * Get the range of the possible TX bandwidth settings.
     * \param chan the channel index 0 to N-1
     * \return a range of bandwidths in Hz
     */
    virtual meta_range_t get_tx_bandwidth_range(size_t chan = 0) = 0;

    /*!
     * Get the dboard interface object for the TX frontend.
     * The dboard interface gives access to GPIOs, SPI, I2C, low-speed ADC and DAC.
     * Use at your own risk!
     * \param chan the channel index 0 to N-1
     * \return the dboard interface sptr
     */
    virtual dboard_iface::sptr get_tx_dboard_iface(size_t chan = 0) = 0;

    /*!
     * Get an TX frontend sensor value.
     * \param name the name of the sensor
     * \param chan the channel index 0 to N-1
     * \return a sensor value object
     */
    virtual sensor_value_t get_tx_sensor(const std::string &name, size_t chan = 0) = 0;

    /*!
     * Get a list of possible TX frontend sensor names.
     * \param chan the channel index 0 to N-1
     * \return a vector of sensor names
     */
    virtual std::vector<std::string> get_tx_sensor_names(size_t chan = 0) = 0;

    /*!
     * Set a constant TX DC offset value.
     * The value is complex to control both I and Q.
     * \param offset the dc offset (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_dc_offset(const std::complex<double> &offset, size_t chan = ALL_CHANS) = 0;

    /*!
     * Set the TX frontend IQ imbalance correction.
     * Use this to adjust the magnitude and phase of I and Q.
     *
     * \param correction the complex correction (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_iq_balance(const std::complex<double> &correction, size_t chan = ALL_CHANS) = 0;

    /*******************************************************************
     * GPIO methods
     ******************************************************************/

    /*!
     * Enumerate gpio banks on the specified device.
     * \param mboard the motherboard index 0 to M-1
     * \return a list of string for each bank name
     */
    virtual std::vector<std::string> get_gpio_banks(const size_t mboard) = 0;

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
     * \param bank the name of a GPIO bank
     * \param attr the name of a GPIO attribute
     * \param value the new value for this GPIO bank
     * \param mask the bit mask to effect which pins are changed
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_gpio_attr(const std::string &bank, const std::string &attr, const boost::uint32_t value, const boost::uint32_t mask = 0xffffffff, const size_t mboard = 0) = 0;

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
     * \param mboard the motherboard index 0 to M-1
     * \return the value set for this attribute
     */
    virtual boost::uint32_t get_gpio_attr(const std::string &bank, const std::string &attr, const size_t mboard = 0) = 0;

    /*******************************************************************
     * Register IO methods
     ******************************************************************/
    struct register_info_t {
        size_t bitwidth;
        bool readable;
        bool writable;
    };

    /*!
     * Enumerate the full paths of all low-level USRP registers accessible to read/write
     * \param mboard the motherboard index 0 to M-1
     * \return a vector of register paths
     */
    virtual std::vector<std::string> enumerate_registers(const size_t mboard = 0) = 0;

    /*!
     * Get more information about a low-level device register
     * \param path the full path to the register
     * \param mboard the motherboard index 0 to M-1
     * \return the info struct which contains the bitwidth and read-write access information
     */
    virtual register_info_t get_register_info(const std::string &path, const size_t mboard = 0) = 0;

    /*!
     * Write a low-level register field for a register in the USRP hardware
     * \param path the full path to the register
     * \param field the identifier of bitfield to be written (all other bits remain unchanged)
     * \param value the value to write to the register field
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void write_register(const std::string &path, const boost::uint32_t field, const boost::uint64_t value, const size_t mboard = 0) = 0;

    /*!
     * Read a low-level register field from a register in the USRP hardware
     * \param path the full path to the register
     * \param field the identifier of bitfield to be read
     * \param mboard the motherboard index 0 to M-1
     * \return the value of the register field
     */
    virtual boost::uint64_t read_register(const std::string &path, const boost::uint32_t field, const size_t mboard = 0) = 0;

    /*******************************************************************
     * Filter API methods
     ******************************************************************/

    /*!
     * Enumerate the available filters in the signal path.
     * \param search_mask
     * \parblock
     * Select only certain filter names by specifying this search mask.
     *
     * E.g. if search mask is set to "rx_frontends/A" only filter names including that string will be returned.
     * \endparblock
     * \return a vector of strings representing the selected filter names.
     */
    virtual std::vector<std::string> get_filter_names(const std::string &search_mask = "") = 0;

    /*!
     * Return the filter object for the given name.
     * \param path the name of the filter as returned from get_filter_names().
     * \return a filter_info_base::sptr.
     */
    virtual filter_info_base::sptr get_filter(const std::string &path) = 0;

    /*!
     * Write back a filter obtained by get_filter() to the signal path.
     * This filter can be a modified version of the originally returned one.
     * The information about Rx or Tx is contained in the path parameter.
     * \param path the name of the filter as returned from get_filter_names().
     * \param filter the filter_info_base::sptr of the filter object to be written
     */
    virtual void set_filter(const std::string &path, filter_info_base::sptr filter) = 0;

};

}}

#endif /* INCLUDED_UHD_USRP_MULTI_USRP_HPP */
