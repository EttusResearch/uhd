//
// Copyright 2010-2012,2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

// define API capabilities for compile time detection of new features
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
#define UHD_USRP_MULTI_USRP_TX_LO_CONFIG_API
#define UHD_USRP_MULTI_USRP_POWER_LEVEL

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/extension/extension.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/types/filters.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <complex>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp {

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
class UHD_API multi_usrp : uhd::noncopyable
{
public:
    typedef std::shared_ptr<multi_usrp> sptr;

    virtual ~multi_usrp(void) = 0;

    //! A wildcard motherboard index
    static const size_t ALL_MBOARDS;

    //! A wildcard channel index
    static const size_t ALL_CHANS;

    //! A wildcard gain element name
    static const std::string ALL_GAINS;

    //! A wildcard LO stage name
    static const std::string ALL_LOS;

    /*!
     * Make a new multi usrp from the device address.
     * \param dev_addr the device address
     * \return a new single usrp object
     * \throws uhd::key_error no device found
     * \throws uhd::index_error fewer devices found than expected
     */
    static sptr make(const device_addr_t& dev_addr);

    /*! Get the underlying device object
     *
     * Note that it is not recommended to use this method. The property tree can
     * be accessed by calling get_tree() on this object, and the streamers own
     * all the streaming-related functionality. get_tx_stream() and
     * get_rx_stream() can also be called on this object.
     *
     * For RFNoC devices, this won't return a true uhd::device anyway, because
     * direct device access is locked for those. The returned pointer will
     * still point to a valid device object, however, it has reduced
     * functionality.
     *
     * \return the device object within this USRP
     */
    virtual device::sptr get_device(void) = 0;

    /*! Return a reference to the property tree
     */
    virtual uhd::property_tree::sptr get_tree(void) const = 0;

    //! Convenience method to get a RX streamer. See also uhd::device::get_rx_stream().
    virtual rx_streamer::sptr get_rx_stream(const stream_args_t& args) = 0;

    //! Convenience method to get a TX streamer. See also uhd::device::get_tx_stream().
    virtual tx_streamer::sptr get_tx_stream(const stream_args_t& args) = 0;

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
     *
     * What exactly this changes is device-dependent, but it will always
     * affect the rate at which the ADC/DAC is running.
     *
     * Like tuning receive or transmit frequencies, this call will do a best
     * effort to change the master clock rate. The device will coerce to the
     * closest clock rate available, and on many devices won't actually change
     * anything at all. Call get_master_clock_rate() to see which rate was
     * actually applied.
     *
     * Note that changing this value during streaming is not recommended and
     * can have random side effects.
     *
     * If the device has an 'auto clock rate' setting (e.g. B200, see also
     * \ref b200_auto_mcr), calling this function will disable the automatic
     * clock rate selection, and the clock rate will be fixed to \p rate.
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

    /*! Return the range within which the master clock rate can be set for this
     *  session
     *
     * Note that many USRPs do not actually support setting the master clock
     * rate during a running session. In this case, the range will consist of
     * a single value, which is the current master clock rate.
     * Values from this range are valid/sensible inputs to
     * set_master_clock_rate(), although keep in mind that the latter coerces.
     *
     * Examples:
     * - The B200 series' master clock rate can be changed at runtime and
     *   will report the true range of supported values
     * - The X300 series has a valid range for the clock rate, but will
     *   always return the clock rate which the USRP was initialized to because
     *   it cannot be changed at runtime
     * - The N200 series does not have a configurable clock rate, and will
     *   always return the same single value as a range
     */
    virtual meta_range_t get_master_clock_rate_range(const size_t mboard = 0) = 0;

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
     *
     * For RFNoC devices with multiple timekeepers, this returns the time of the first
     * timekeeper. To access specific timekeepers, use the corresponding RFNoC APIs
     * (e.g., mb_controller::get_timekeeper()).
     *
     * \param mboard which motherboard to query
     * \return a timespec representing current usrp time
     */
    virtual time_spec_t get_time_now(size_t mboard = 0) = 0;

    /*!
     * Get the time when the last pps pulse occurred.
     *
     * For RFNoC devices with multiple timekeepers, this returns the time of the first
     * timekeeper. To access specific timekeepers, use the corresponding RFNoC APIs
     * (e.g., mb_controller::get_timekeeper()).
     *
     * \param mboard which motherboard to query
     * \return a timespec representing the last pps
     */
    virtual time_spec_t get_time_last_pps(size_t mboard = 0) = 0;

    /*! Sets the time registers on the USRP immediately.
     *
     * This will set the tick count on the timekeepers of all devices as soon
     * as possible.  It is done serially for multiple timekeepers, so times
     * across multiple timekeepers will not be synchronized.
     *
     * \param time_spec the time to latch into the usrp device
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_time_now(
        const time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) = 0;

    /*! Set the time registers on the USRP at the next PPS rising edge.
     *
     * This will set the tick count on the timekeepers of all devices on
     * the next rising edge of the PPS trigger signal.  It is important
     * to note that this means the time may not be set for up to 1 second
     * after this call is made, so it is recommended to wait for 1 second
     * after this call before making any calls that depend on the time to
     * ensure that the time registers will be in a known state prior to use.
     *
     * \b Note: Because this call sets the time on the next PPS edge, the time
     * spec supplied should correspond to the next pulse (i.e. current
     * time + 1 second).
     *
     * \b Note: Make sure to not call this shortly before the next PPS edge. This
     * should be called with plenty of time before the next PPS edge to ensure
     * that all timekeepers on all devices will execute this command on the
     * same PPS edge. If not, timekeepers could be unsynchronized in time by
     * exactly one second. If in doubt, use set_time_unknown_pps() which will
     * take care of this issue (but will also take longer to execute).
     *
     * \b Note: When changing clock sources, a previously set time will most
     * likely be lost. It is recommended to set the time after changing the
     * clock source. Otherwise, an unexpected time may line up with future PPS
     * edges.
     *
     * \param time_spec the time to latch into the usrp device
     * \param mboard the motherboard index 0 to M-1
     */
    virtual void set_time_next_pps(
        const time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) = 0;

    /*! Synchronize the times across all motherboards in this configuration.
     *
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
     * \b Note: When changing clock sources, a previously set time will most
     * likely be lost. It is recommended to set the time after changing the
     * clock source. Otherwise, an unexpected time may line up with future PPS
     * edges.
     *
     * \param time_spec the time to latch at the next pps after catching the edge
     */
    virtual void set_time_unknown_pps(const time_spec_t& time_spec) = 0;

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
    virtual void set_command_time(
        const uhd::time_spec_t& time_spec, size_t mboard = ALL_MBOARDS) = 0;

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
    virtual void issue_stream_cmd(
        const stream_cmd_t& stream_cmd, size_t chan = ALL_CHANS) = 0;

    /*!  Set the time source for the USRP device
     *
     * This sets the method of time synchronization, typically a pulse per
     * second signal. In order to time-align multiple USRPs, it is necessary to
     * connect all of them to a common reference and provide them with the same
     * time source.
     * Typical values for \p source are 'internal', 'external'. Refer to the
     * specific device manual for a full list of options.
     *
     * If the value for for \p source is not available for this device, it will
     * throw an exception. Calling get_time_sources() will return a valid list
     * of options for this method.
     *
     * Side effects: Some devices only support certain combinations of time and
     * clock source. It is possible that the underlying device implementation
     * will change the clock source when the time source changes and vice versa.
     * Reading back the current values of clock and time source using
     * get_clock_source() and get_time_source() is the only certain way of
     * knowing which clock and time source are currently selected.
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. Consider the following snippet:
     * ~~~{.cpp}
     * auto usrp = uhd::usrp::multi_usrp::make(device_args);
     * // This may or may not cause the hardware to reconfigure, depending on
     * // the default state of the device
     * usrp->set_time_source("internal");
     * // Now, the time source is definitely set to "internal"!
     * // The next call probably won't do anything but will return immediately,
     * // because the time source was already set to "internal"
     * usrp->set_time_source("internal");
     * // The time source is still guaranteed to be "internal" at this point
     * ~~~
     *
     * See also:
     * - set_clock_source()
     * - set_sync_source()
     *
     * \param source a string representing the time source
     * \param mboard which motherboard to set the config
     * \throws if \p source is an invalid option
     */
    virtual void set_time_source(
        const std::string& source, const size_t mboard = ALL_MBOARDS) = 0;

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

    /*!  Set the clock source for the USRP device
     *
     * This sets the source of the frequency reference, typically a 10 MHz
     * signal. In order to frequency-align multiple USRPs, it is necessary to
     * connect all of them to a common reference and provide them with the same
     * clock source.
     * Typical values for \p source are 'internal', 'external'. Refer to the
     * specific device manual for a full list of options.
     *
     * If the value for for \p source is not available for this device, it will
     * throw an exception. Calling get_clock_sources() will return a valid list
     * of options for this method.
     *
     * Side effects: Some devices only support certain combinations of time and
     * clock source. It is possible that the underlying device implementation
     * will change the time source when the clock source changes and vice versa.
     * Reading back the current values of clock and time source using
     * get_clock_source() and get_time_source() is the only certain way of
     * knowing which clock and time source are currently selected.
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. Consider the following snippet:
     * ~~~{.cpp}
     * auto usrp = uhd::usrp::multi_usrp::make(device_args);
     * // This may or may not cause the hardware to reconfigure, depending on
     * // the default state of the device
     * usrp->set_clock_source("internal");
     * // Now, the clock source is definitely set to "internal"!
     * // The next call probably won't do anything but will return immediately,
     * // because the clock source was already set to "internal"
     * usrp->set_clock_source("internal");
     * // The clock source is still guaranteed to be "internal" at this point
     * ~~~
     *
     * \b Note: Reconfiguring the clock source will affect the clocking
     * within the FPGAs of USRPs, and affect timekeeping as well as proper
     * functioning of blocks that depend on these clocks. It is therefore
     * strongly recommended to configure clock and time source before doing
     * anything else. In particular, setting the device time should be done
     * after calling this, and there should be no ongoing streaming operation
     * while reconfiguring the clock/time source.
     *
     * See also:
     * - set_time_source()
     * - set_sync_source()
     *
     * \param source a string representing the time source
     * \param mboard which motherboard to set the config
     * \throws if \p source is an invalid option
     */
    virtual void set_clock_source(
        const std::string& source, const size_t mboard = ALL_MBOARDS) = 0;

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

    /*! Set the reference/synchronization sources for the USRP device
     *
     * This is a shorthand for calling
     * `set_sync_source(device_addr_t("clock_source=$CLOCK_SOURCE,time_source=$TIME_SOURCE"))`
     *
     * \param clock_source A string representing the clock source
     * \param time_source A string representing the time source
     * \param mboard which motherboard to set the config
     * \throws uhd::value_error if the sources don't actually exist
     */
    virtual void set_sync_source(const std::string& clock_source,
        const std::string& time_source,
        const size_t mboard = ALL_MBOARDS) = 0;

    /*! Set the reference/synchronization sources for the USRP device
     *
     * Typically, this will set both clock and time source in a single call. For
     * some USRPs, this may be significantly faster than calling
     * set_time_source() and set_clock_source() individually.
     *
     * Example:
     * ~~~{.cpp}
     * auto usrp = uhd::usrp::multi_usrp::make("");
     * usrp->set_sync_source(
     *     device_addr_t("clock_source=external,time_source=external"));
     * ~~~
     *
     * This function does not force a re-initialization of the underlying
     * hardware when the value does not change. See also set_time_source() and
     * set_clock_source() for more details.
     *
     * \b Note: Reconfiguring the sync source may affect the clocking
     * within the FPGAs of USRPs, and affect timekeeping as well as proper
     * functioning of blocks that depend on these clocks. It is therefore
     * strongly recommended to configure clock and time source before doing
     * anything else. In particular, setting the device time should be done
     * after calling this, and there should be no ongoing streaming operation
     * while reconfiguring the sync source.
     *
     * \param sync_source A dictionary representing the various source settings.
     * \param mboard which motherboard to set the config
     * \throws uhd::value_error if the sources don't actually exist or if the
     *         combination of clock and time source is invalid.
     */
    virtual void set_sync_source(
        const device_addr_t& sync_source, const size_t mboard = ALL_MBOARDS) = 0;

    /*! Get the currently set sync source.
     *
     * \param mboard which motherboard to get the config
     * \return the dictionary representing the sync source settings
     */
    virtual device_addr_t get_sync_source(const size_t mboard) = 0;

    /*! Get a list of available sync sources
     *
     * \param mboard which motherboard to get the config
     * \return the dictionary representing the sync source settings
     */
    virtual std::vector<device_addr_t> get_sync_sources(const size_t mboard) = 0;

    /*! Send the clock signal to an output connector.
     *
     * This call is only applicable on devices with reference outputs.
     * By default, the reference output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     *
     * If the device does not support this operation, calling this method will
     * throw a uhd::runtime_error.
     *
     * \param enb true to output the clock source.
     * \param mboard which motherboard to set
     * \throws if the device is incapable of exporting the
     *         clock signal.
     */
    virtual void set_clock_source_out(
        const bool enb, const size_t mboard = ALL_MBOARDS) = 0;

    /*! Send the time signal (PPS) to an output connector.
     *
     * This call is only applicable on devices with PPS outputs.
     * By default, the PPS output will be enabled for ease of use.
     * This call may be used to enable or disable the output.
     *
     * If the device does not support this operation, calling this method will
     * throw a uhd::runtime_error.
     *
     * \param enb true to output the time source.
     * \param mboard which motherboard to set
     * \throws if the device is incapable of exporting the
     *         clock signal.
     */
    virtual void set_time_source_out(
        const bool enb, const size_t mboard = ALL_MBOARDS) = 0;

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
    virtual sensor_value_t get_mboard_sensor(
        const std::string& name, size_t mboard = 0) = 0;

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
     * \throws uhd::not_implemented_error on RFNoC devices, uhd::lookup_error on
     *         other devices if this API is not implemented.
     */
    virtual void set_user_register(
        const uint8_t addr, const uint32_t data, size_t mboard = ALL_MBOARDS) = 0;

    /*! Return a user settings interface object
     *
     * This is only supported by the B2xx series. It will return
     * an object that will allow to peek and poke user settings, which typically
     * are implemented by custom FPGA images.
     * If the device does not support such an interface, it will return a null
     * pointer. This allows to probe this functionality, but can lead to
     * dereferencing errors if no checks are performed.
     *
     * A typical way to use this is as follows:
     * ~~~~{.cpp}
     * auto usrp = multi_usrp::make(device_args);
     * const size_t chan = 0;
     * auto user_settings = usrp->get_user_settings_iface(chan);
     * if (!user_settings) {
     *     std::cout << "No user settings!" << std::endl;
     * } else {
     *     user_settings->poke32(0, 23); // Write value 23 to register 0
     * }
     * ~~~~
     *
     * \returns Either a uhd::wb_iface object to poke the user settings, or a
     *          nullptr if the device doesn't support this interface.
     */
    virtual uhd::wb_iface::sptr get_user_settings_iface(const size_t chan = 0) = 0;

    /*! Get direct access to the underlying RFNoC radio object.
     *
     * Note: This is an advanced API, created for corner cases where the
     * application is using multi_usrp, but some special features from
     * radio_control need to be used that are not exposed by multi_usrp. Note
     * that it is possible to put the radio and multi_usrp into a broken state
     * by directly accessing the radio. For typical radio operations (such as
     * tuning, setting gain or antenna, etc.) it is therefore highly recommended
     * to not use this API call, but use the native multi_usrp API calls.
     *
     * The lifetime of the radio is linked to the lifetime of the device object,
     * so storing a reference from this function is not allowed.
     *
     * \param chan The channel index
     * \returns A reference to the radio block matching the given channel
     * \throws uhd::not_implemented_error if not on an RFNoC device.
     */
    virtual uhd::rfnoc::radio_control& get_radio_control(const size_t chan = 0) = 0;

    /*! Get a handle to any RF extension objects which may exist.
     *
     * \param trx The TX/RX direction
     * \param chan The channel index
     * \returns A pointer to the extension matching the given trx/channel or a nullptr if
     * the extension is not found.
     */
    virtual uhd::extension::extension::sptr get_extension(
        const direction_t trx, const size_t chan) = 0;

    /*! Get a handle to a RF extension object
     *
     * This function retrieves a handle to a RF extension object and casts it to
     * the given type, which must be a derived class of uhd::extension::extension.
     *
     * \param trx The TX/RX direction
     * \param chan The channel index
     * \returns A pointer to the extension matching the given trx/channel
     */
    template <typename T>
    typename T::sptr get_extension(const direction_t trx, const size_t chan)
    {
        return std::dynamic_pointer_cast<T>(get_extension(trx, chan));
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    /*! Set the RX frontend specification
     *
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     *
     * \param spec the new frontend specification
     * \param mboard the motherboard index 0 to M-1
     * \throws if an invalid spec is provided.
     */
    virtual void set_rx_subdev_spec(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard = ALL_MBOARDS) = 0;

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

    /*! Set the RX sample rate
     *
     * This function will coerce the requested rate to a rate that the device
     * can handle. A warning may be logged during coercion. Call get_rx_rate()
     * to identify the actual rate.
     *
     * \param rate the rate in Sps
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_rate(double rate, size_t chan = ALL_CHANS) = 0;

    /*! Set the number of samples sent per packet (spp) for RX streaming
     *
     * On RFNoC devices, this will set the spp value on the radio itself. For
     * older devices, it will inject the spp value into a later get_rx_stream()
     * call, but it won't change anything in existing streamers.
     *
     * \param spp the new spp value
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_spp(const size_t spp, const size_t chan = ALL_CHANS) = 0;

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

    /*! Set the RX center frequency.
     *
     * If the requested frequency is outside of the valid frequency range, it
     * will be coerced to the nearest valid frequency. Check the return value or
     * call get_rx_freq() to get the actual center frequency.
     *
     * \param tune_request tune request instructions
     * \param chan the channel index 0 to N-1
     * \return a tune result object
     */
    virtual tune_result_t set_rx_freq(
        const tune_request_t& tune_request, size_t chan = 0) = 0;

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

    /**************************************************************************
     * LO controls
     *************************************************************************/
    /*! Get a list of possible LO stage names
     *
     * Example: On the TwinRX, this will return "LO1", "LO2". These names can
     * are used in other LO-related API calls, so this function can be used for
     * automatically enumerating LO stages.
     * An empty return value doesn't mean there are no LOs, it means that this
     * radio does not have an LO API implemented, and typically means the LOs
     * have no direct way of being controlled other than setting the frequency.
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names, or an empty list of
     *         this doesn't apply (i.e. there are no controllable LO stages)
     */
    virtual std::vector<std::string> get_rx_lo_names(size_t chan = 0) = 0;

    /*! Set the LO source for the USRP device.
     *
     * For USRPs that support selectable LO sources, this function allows
     * switching between them. Typical options for source: internal, external.
     * Call get_rx_lo_sources() to enumerate the list of valid options. Calling
     * this function with an invalid argument will cause an exception to be
     * thrown.
     *
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update. If the wildcard value
     *             ALL_LOS is used, the setting will be applied to all LOs on
     *             this channel. Call get_tx_lo_names() for a list of valid
     *             argument values.
     * \param chan the channel index 0 to N-1
     * \throws uhd::not_implemented_error if the device cannot set the LO source
     * \throws uhd::value_error if the device can set the LO source, but the LO
     *                          name is invalid.
     */
    virtual void set_rx_lo_source(
        const std::string& src, const std::string& name = ALL_LOS, size_t chan = 0) = 0;

    /*! Get the currently selected LO source.
     *
     * Channels without controllable LO sources will always return "internal".
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual const std::string get_rx_lo_source(
        const std::string& name = ALL_LOS, size_t chan = 0) = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources will return
     * "internal". Typical values are "internal" and "external", although the
     * TwinRX, for example, has more options, such as "companion". These options
     * are device-specific, so consult the individual device manual pages for
     * details.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_rx_lo_sources(
        const std::string& name = ALL_LOS, size_t chan = 0) = 0;

    /*! Set whether the LO used by the device is exported
     *
     * For USRPs that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     *
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     * \throws uhd::runtime_error if LO exporting is not enabled
     */
    virtual void set_rx_lo_export_enabled(
        bool enabled, const std::string& name = ALL_LOS, size_t chan = 0) = 0;

    /*!  Returns true if the currently selected LO is being exported.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_rx_lo_export_enabled(
        const std::string& name = ALL_LOS, size_t chan = 0) = 0;

    /*! Set the RX LO frequency (Advanced).
     *
     * The actual behaviour is device-specific. However, as a rule of thumb,
     * this will coerce the underlying driver into some state. Typical
     * situations include:
     * - LOs are internal, and this function is called to pin an LO to a
     *   certain value. This can force the driver to pick different IFs for
     *   different stages, and there may be situations where this behaviour
     *   can be used to reduce spurs in specific bands.
     * - LOs are external. In this case, this function is used to notify UHD
     *   what the actual value of an externally provided LO is. The only time
     *   when calling this function is necessary is when the LO source is set
     *   to external, but the external LO can't be tuned to the exact value
     *   required by UHD to achieve a certain center frequency. In this case,
     *   calling set_rx_lo_freq() will let UHD know that the LO is not the
     *   expected value, and it's possible that UHD will find other ways to
     *   compensate for the LO offset.
     *
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     * \throws if the LO name is not valid.
     */
    virtual double set_rx_lo_freq(
        double freq, const std::string& name, size_t chan = 0) = 0;

    /*!  Get the current RX LO frequency (Advanced).
     *
     * If the channel does not have independently configurable LOs
     * the current rf frequency will be returned. See also set_rx_lo_freq() for
     * more information.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_rx_lo_freq(const std::string& name, size_t chan = 0) = 0;

    /*!  Get the LO frequency range of the RX LO.
     *
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_rx_lo_freq_range(
        const std::string& name, size_t chan = 0) = 0;

    /*! Get a list of possible TX LO stage names
     *
     * See also get_rx_lo_names().
     *
     * An empty return value doesn't mean there are no LOs, it means that this
     * radio does not have an LO API implemented, and typically means the LOs
     * have no direct way of being controlled other than setting the frequency.
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible LO names, or an empty list of
     *         this doesn't apply (i.e. there are no controllable LO stages)
     */
    virtual std::vector<std::string> get_tx_lo_names(size_t chan = 0) = 0;

    /*! Set the TX LO source for the USRP device.
     *
     * For USRPs that support selectable LO sources, this function allows
     * switching between them. Typical options for source: internal, external.
     * Call get_tx_lo_sources() to enumerate the list of valid options. Calling
     * this function with an invalid argument will cause an exception to be
     * thrown.
     *
     * \param src a string representing the LO source
     * \param name the name of the LO stage to update. If the wildcard value
     *             ALL_LOS is used, the setting will be applied to all LOs on
     *             this channel. Call get_tx_lo_names() for a list of valid
     *             argument values.
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_lo_source(const std::string& src,
        const std::string& name = ALL_LOS,
        const size_t chan       = 0) = 0;

    /*! Get the currently selected TX LO source.
     *
     * Channels without controllable LO sources will always return "internal".
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO source
     */
    virtual const std::string get_tx_lo_source(
        const std::string& name = ALL_LOS, const size_t chan = 0) = 0;

    /*! Get a list of possible LO sources.
     *
     * Channels which do not have controllable LO sources will return
     * "internal". Typical values are "internal" and "external".
     * These options are device-specific.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible settings
     */
    virtual std::vector<std::string> get_tx_lo_sources(
        const std::string& name = ALL_LOS, const size_t chan = 0) = 0;

    /*! Set whether the TX LO used by the device is exported
     *
     * For USRPs that support exportable LOs, this function
     * configures if the LO used by chan is exported or not.
     *
     * \param enabled if true then export the LO
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1 for the source channel
     * \throws uhd::runtime_error if LO exporting is not enabled
     */
    virtual void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name = ALL_LOS, const size_t chan = 0) = 0;

    /*!  Returns true if the currently selected LO is being exported.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     */
    virtual bool get_tx_lo_export_enabled(
        const std::string& name = ALL_LOS, const size_t chan = 0) = 0;

    /*! Set the TX LO frequency (Advanced).
     *
     * The actual behaviour is device-specific. However, as a rule of thumb,
     * this will coerce the underlying driver into some state. Typical
     * situations include:
     * - LOs are internal, and this function is called to pin an LO to a
     *   certain value. This can force the driver to pick different IFs for
     *   different stages, and there may be situations where this behaviour
     *   can be used to reduce spurs in specific bands.
     * - LOs are external. In this case, this function is used to notify UHD
     *   what the actual value of an externally provided LO is. The only time
     *   when calling this function is necessary is when the LO source is set
     *   to external, but the external LO can't be tuned to the exact value
     *   required by UHD to achieve a certain center frequency. In this case,
     *   calling set_tx_lo_freq() will let UHD know that the LO is not the
     *   expected value, and it's possible that UHD will find other ways to
     *   compensate for the LO offset.
     *
     * \param freq the frequency to set the LO to
     * \param name the name of the LO stage to update
     * \param chan the channel index 0 to N-1
     * \return a coerced LO frequency
     * \throws if the LO name is not valid.
     */
    virtual double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan = 0) = 0;

    /*!  Get the current TX LO frequency (Advanced).
     *
     * If the channel does not have independently configurable LOs
     * the current rf frequency will be returned. See also set_tx_lo_freq() for
     * more information.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return the configured LO frequency
     */
    virtual double get_tx_lo_freq(const std::string& name, const size_t chan = 0) = 0;

    /*!  Get the LO frequency range of the TX LO.
     *
     * If the channel does not have independently configurable LOs
     * the rf frequency range will be returned.
     *
     * \param name the name of the LO stage to query
     * \param chan the channel index 0 to N-1
     * \return a frequency range object
     */
    virtual freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan = 0) = 0;

    /**************************************************************************
     * Gain controls
     *************************************************************************/
    /*! Set the RX gain value for the specified gain element.
     *
     * If the requested gain value is outside the valid range, it will be
     * coerced to a valid gain value. Call get_rx_gain_range() to return the
     * currently valid gain range, and call get_rx_gain() after calling this
     * function to return the actual current gain value after coercion.
     *
     * For an empty name, distribute across all gain elements.
     *
     * \param gain the gain in dB
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_gain(double gain, const std::string& name, size_t chan = 0) = 0;

    /*! Get a list of possible RX gain profile options
     *
     * Example: On the TwinRX, this will return "low-noise", "low-distortion" or
     * "default". These names can be used in gain-profile related API called. An empty
     * return value doesn't mean there are no profile options, it means that this radio
     * does not have any gain profiles implemented, and typically means there is only one
     * default profile of set gain
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible gain profile options, or an empty list of
     *         this doesn't apply.
     */
    virtual std::vector<std::string> get_rx_gain_profile_names(const size_t chan = 0) = 0;

    /*! Set the RX gain profile.
     *
     * Call get_rx_gain_profile_names() for valid names.
     *
     * \param profile the profile string option
     * \param chan the channel index 0 to N-1
     * \throws if the requested gain profile name is not valid.
     */
    virtual void set_rx_gain_profile(
        const std::string& profile, const size_t chan = 0) = 0;

    /*!
     * Get the RX gain profile.
     * \param chan the channel index 0 to N-1
     * \return a string of current RX gain profile of corresponding channel.
     */
    virtual std::string get_rx_gain_profile(const size_t chan = 0) = 0;

    //! A convenience wrapper for setting overall RX gain
    void set_rx_gain(double gain, size_t chan = 0)
    {
        return this->set_rx_gain(gain, ALL_GAINS, chan);
    }

    /*! Set the normalized RX gain value.
     *
     * The normalized gain is a value in [0, 1], where 0 is the
     * smallest gain value available, and 1 is the largest, independent
     * of the device. In between, gains are linearly interpolated.
     * If the requested normalized gain is outside of this range, an exception
     * is thrown.
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

    /*! Enable or disable the RX AGC module.
     *
     * Only some devices implement an AGC, including all USRPs from the B200
     * series, the E310, and the E320.
     * When called on a device that does not implement an AGC, an exception will
     * be thrown.
     *
     * Once this module is enabled manual gain settings will be ignored.
     * The AGC will start in a default configuration which should be good for
     * most use cases. Device specific configuration parameters can be found in
     * the property tree.
     *
     * \param enable Enable or Disable the AGC
     * \param chan the channel index 0 to N-1
     * \throws if the underlying device does not
     *         implement an AGC.
     */
    virtual void set_rx_agc(bool enable, size_t chan = 0) = 0;

    /*!
     * Get the RX gain value for the specified gain element.
     * For an empty name, sum across all gain elements.
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     * \return the gain in dB
     */
    virtual double get_rx_gain(const std::string& name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall RX gain
    double get_rx_gain(size_t chan = 0)
    {
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
    virtual gain_range_t get_rx_gain_range(const std::string& name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall RX gain range
    gain_range_t get_rx_gain_range(size_t chan = 0)
    {
        return this->get_rx_gain_range(ALL_GAINS, chan);
    }

    /*!
     * Get the names of the gain elements in the RX chain.
     * Gain elements are ordered from antenna to FPGA.
     * \param chan the channel index 0 to N-1
     * \return a vector of gain element names
     */
    virtual std::vector<std::string> get_rx_gain_names(size_t chan = 0) = 0;

    /*! Select the RX antenna on the frontend.
     *
     * \param ant the antenna name. If an invalid name is provided, an exception
     *            is thrown. Call get_rx_antennas() to return a valid list of
     *            antenna names.
     * \param chan the channel index 0 to N-1
     * \throws if an invalid antenna name is provided
     */
    virtual void set_rx_antenna(const std::string& ant, size_t chan = 0) = 0;

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

    /*! Set the RX bandwidth on the frontend.
     *
     * If a bandwidth is provided that is outside the valid range, it is coerced
     * to the nearest valid value. Call get_rx_bandwidth_range() to identify the
     * valid range of bandwidth values.
     *
     * \param bandwidth the bandwidth in Hz
     * \param chan the channel index 0 to N-1
     */
    virtual void set_rx_bandwidth(double bandwidth, size_t chan = 0) = 0;

    /*! Get the RX bandwidth on the frontend
     *
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
    virtual sensor_value_t get_rx_sensor(const std::string& name, size_t chan = 0) = 0;

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
    virtual void set_rx_dc_offset(
        const std::complex<double>& offset, size_t chan = ALL_CHANS) = 0;

    /*!
     * Get the valid range for RX DC offset values.
     * \param chan the channel index 0 to N-1
     */
    virtual meta_range_t get_rx_dc_offset_range(size_t chan = 0) = 0;

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
    virtual void set_rx_iq_balance(
        const std::complex<double>& correction, size_t chan = ALL_CHANS) = 0;


    /**************************************************************************
     * Power level controls
     *************************************************************************/
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
    virtual bool has_rx_power_reference(const size_t chan = 0) = 0;

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
        const double power_dbm, const size_t chan = 0) = 0;

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
    virtual double get_rx_power_reference(const size_t chan = 0) = 0;

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

    /*******************************************************************
     * TX methods
     ******************************************************************/
    /*! Set the TX frontend specification:
     *
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     * \param spec the new frontend specification
     * \param mboard the motherboard index 0 to M-1
     * \throws if an invalid spec is provided.
     */
    virtual void set_tx_subdev_spec(
        const uhd::usrp::subdev_spec_t& spec, size_t mboard = ALL_MBOARDS) = 0;

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

    /*! Set the TX sample rate.
     *
     * This function will coerce the requested rate to a rate that the device
     * can handle. A warning may be logged during coercion. Call get_rx_rate()
     * to identify the actual rate.
     *
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

    /*! Set the TX center frequency.
     *
     * If the requested frequency is outside of the valid frequency range, it
     * will be coerced to the nearest valid frequency. Check the return value or
     * call get_tx_freq() to get the actual center frequency.
     *
     * \param tune_request tune request instructions
     * \param chan the channel index 0 to N-1
     * \return a tune result object
     */
    virtual tune_result_t set_tx_freq(
        const tune_request_t& tune_request, size_t chan = 0) = 0;

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

    /*! Set the TX gain value for the specified gain element.
     *
     * If the requested gain value is outside the valid range, it will be
     * coerced to a valid gain value. Call get_rx_gain_range() to return the
     * currently valid gain range, and call get_rx_gain() after calling this
     * function to return the actual current gain value after coercion.
     *
     * For an empty name, distribute across all gain elements.
     *
     * \param gain the gain in dB
     * \param name the name of the gain element
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_gain(double gain, const std::string& name, size_t chan = 0) = 0;

    /*! Get a list of possible TX gain profile options
     *
     * Example: On the N310, this will return "manual" or "default".
     * These names can be used in gain related API called.
     * An empty return value doesn't mean there are no profile options, it means that
     * this radio does not have any gain profiles implemented, and typically means
     * there is only one default profile of set gain
     *
     * \param chan the channel index 0 to N-1
     * \return a vector of strings for possible gain profile options, or an empty list of
     *         this doesn't apply.
     */
    virtual std::vector<std::string> get_tx_gain_profile_names(const size_t chan = 0) = 0;

    /*! Set the TX gain profile.
     *
     * Call get_tx_gain_profile_names() for valid names.
     *
     * \param profile the profile string option.
     * \param chan the channel index 0 to N-1
     * \throws if the requested gain profile name is not valid.
     */
    virtual void set_tx_gain_profile(
        const std::string& profile, const size_t chan = 0) = 0;

    /*!
     * Get the TX gain profile.
     * \param chan the channel index 0 to N-1
     * \return a string of current TX gain profile of corresponding channel.
     */
    virtual std::string get_tx_gain_profile(const size_t chan = 0) = 0;

    //! A convenience wrapper for setting overall TX gain
    void set_tx_gain(double gain, size_t chan = 0)
    {
        return this->set_tx_gain(gain, ALL_GAINS, chan);
    }

    /*!
     * Set the normalized TX gain value.
     *
     * See set_normalized_rx_gain() for a discussion on normalized
     * gains.
     * If the requested normalized gain is outside of this range, an exception
     * is thrown.
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
    virtual double get_tx_gain(const std::string& name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall TX gain
    double get_tx_gain(size_t chan = 0)
    {
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
    virtual gain_range_t get_tx_gain_range(const std::string& name, size_t chan = 0) = 0;

    //! A convenience wrapper for getting overall TX gain range
    gain_range_t get_tx_gain_range(size_t chan = 0)
    {
        return this->get_tx_gain_range(ALL_GAINS, chan);
    }

    /*!
     * Get the names of the gain elements in the TX chain.
     * Gain elements are ordered from antenna to FPGA.
     * \param chan the channel index 0 to N-1
     * \return a vector of gain element names
     */
    virtual std::vector<std::string> get_tx_gain_names(size_t chan = 0) = 0;

    /**************************************************************************
     * Power level controls
     *************************************************************************/
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
    virtual bool has_tx_power_reference(const size_t chan = 0) = 0;

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
        const double power_dbm, const size_t chan = 0) = 0;

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
    virtual double get_tx_power_reference(const size_t chan = 0) = 0;

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

    /*!
     * Select the TX antenna on the frontend.
     * \param ant the antenna name. If an invalid name is provided, an exception
     *            is thrown. Call get_tx_antennas() to return a valid list of
     *            antenna names.
     * \param chan the channel index 0 to N-1
     * \throws if an invalid antenna name is provided
     */
    virtual void set_tx_antenna(const std::string& ant, size_t chan = 0) = 0;

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

    /*! Set the TX bandwidth on the frontend.
     *
     * If a bandwidth is provided that is outside the valid range, it is coerced
     * to the nearest valid value. Call get_tx_bandwidth_range() to identify the
     * valid range of bandwidth values.
     *
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
    virtual sensor_value_t get_tx_sensor(const std::string& name, size_t chan = 0) = 0;

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
    virtual void set_tx_dc_offset(
        const std::complex<double>& offset, size_t chan = ALL_CHANS) = 0;

    /*!
     * Get the valid range for TX DC offset values.
     * \param chan the channel index 0 to N-1
     */
    virtual meta_range_t get_tx_dc_offset_range(size_t chan = 0) = 0;

    /*!
     * Set the TX frontend IQ imbalance correction.
     * Use this to adjust the magnitude and phase of I and Q.
     *
     * \param correction the complex correction (1.0 is full-scale)
     * \param chan the channel index 0 to N-1
     */
    virtual void set_tx_iq_balance(
        const std::complex<double>& correction, size_t chan = ALL_CHANS) = 0;

    /*******************************************************************
     * GPIO methods
     ******************************************************************/

    /*! Enumerate GPIO banks on the specified device.
     *
     * \param mboard the motherboard index 0 to M-1
     * \return a list of string for each bank name
     */
    virtual std::vector<std::string> get_gpio_banks(const size_t mboard) = 0;

    /*! Set a GPIO attribute on a particular GPIO bank.
     *
     * Possible attribute names:
     *  - CTRL - 1 for ATR mode, 0 for GPIO mode
     *  - DDR - 1 for output, 0 for input
     *  - OUT - GPIO output level (not ATR mode)
     *  - ATR_0X - ATR idle state
     *  - ATR_RX - ATR receive only state
     *  - ATR_TX - ATR transmit only state
     *  - ATR_XX - ATR full duplex state
     *
     * A note on bank names: Query get_gpio_banks() for a valid list of arguments
     * for bank names. Note that RFNoC devices (E3xx, N3xx, X3x0, X410) behave
     * slightly differently when using this API vs. using the
     * radio_control::set_gpio_attr() API. For backward-compatibility reasons,
     * this API does not have a dedicated argument to address a specific radio,
     * although the aforementioned devices have separate GPIO banks for each
     * radio. This API thus allows appending the slot name (typically "A" or "B")
     * to the GPIO bank to differentiate between radios. The following example
     * shows the difference between the RFNoC and multi_usrp APIs on a USRP N310:
     * ~~~{.py}
     * my_usrp = uhd.usrp.MultiUSRP("type=n3xx")
     * print(my_usrp.get_gpio_banks()) # Will print: FP0A, FP0B
     * # Now set all pins to GPIO for Radio 1 (note the 'B' in 'FP0B'):
     * my_usrp.set_gpio_attr("FP0B", "CTRL", 0x000)
     * # For backwards compatibility, you can omit the 'A', but that will default
     * # to radio 0. The following lines thus do the same:
     * my_usrp.set_gpio_attr("FP0", "CTRL", 0x000)
     * my_usrp.set_gpio_attr("FP0A", "CTRL", 0x000)
     * ### This is how you do the same thing with RFNoC API:
     * print(my_usrp.get_radio_control(0).get_gpio_banks()) # Will print: FP0
     * print(my_usrp.get_radio_control(1).get_gpio_banks()) # Will print: FP0
     * # Note how the radio controller only has a single bank!
     * # When accessing the radio directly, we thus can't specify any other bank
     * # than FP0:
     * my_usrp.get_radio_control(1).set_gpio_attr("FP0", "CTRL", 0x000)
     * ~~~
     *
     * The \p mask argument can be used to apply \p value only to select pins,
     * and retain the existing value on the rest. Because of this feature, this
     * API call will incur two register transactions (one read, one write).
     *
     * Note that this API call alone may not be sufficient to configure the
     * physical GPIO pins. See set_gpio_src() for more details.
     *
     * \param bank the name of a GPIO bank
     * \param attr the name of a GPIO attribute (see list above)
     * \param value the new value for this GPIO bank
     * \param mask the bit mask to effect which pins are changed
     * \param mboard the motherboard index 0 to M-1
     * \throws an exception if either bank or attr are invalid values.
     */
    virtual void set_gpio_attr(const std::string& bank,
        const std::string& attr,
        const uint32_t value,
        const uint32_t mask = 0xffffffff,
        const size_t mboard = 0) = 0;

    /*! Get a GPIO attribute on a particular GPIO bank.
     *
     * Possible attribute names:
     *  - CTRL - 1 for ATR mode, 0 for GPIO mode
     *  - DDR - 1 for output, 0 for input
     *  - OUT - GPIO output level (not ATR mode)
     *  - ATR_0X - ATR idle state
     *  - ATR_RX - ATR receive only state
     *  - ATR_TX - ATR transmit only state
     *  - ATR_XX - ATR full duplex state
     *  - READBACK - readback input GPIOs
     *
     * For bank names, refer to set_gpio_attr().
     *
     * \param bank the name of a GPIO bank
     * \param attr the name of a GPIO attribute (see list above)
     * \param mboard the motherboard index 0 to M-1
     * \return the value set for this attribute
     */
    virtual uint32_t get_gpio_attr(
        const std::string& bank, const std::string& attr, const size_t mboard = 0) = 0;

    /*! Return a list of GPIO banks that can be source-controlled on this motherboard
     *
     * This is a different set of banks than those returned from get_gpio_banks().
     * Here, we return a list of banks that can be used as arguments for
     * get_gpio_src(), get_gpio_srcs(), and set_gpio_src(). These return values
     * correspond to the physical connectors of the USRP, e.g., for X410, it
     * will return "GPIO0" and "GPIO1" (see also \ref page_x400_gpio_api). On
     * X310, it will return a single value, "FP0" (see also \ref xgpio_fpanel_gpio).
     *
     * Some motherboards have GPIO banks that can be driven from different
     * sources, e.g., the N310 can have any radio channel drive the FP-GPIOs,
     * or the PS.
     *
     * \param mboard the motherboard index 0 to M-1
     * \return a list of valid bank names
     */
    virtual std::vector<std::string> get_gpio_src_banks(const size_t mboard = 0) = 0;

    /*! Enumerate sources for a gpio bank on the specified device.
     *
     * Each of the pins in the chosen bank can be driven from one of the
     * returned sources.
     *
     * \param bank the name of a GPIO bank (connector). Valid values can be
     *             obtained by calling get_gpio_src_banks().
     * \param mboard the motherboard index 0 to M-1
     * \return a list of strings with each valid source for the chosen bank
     */
    virtual std::vector<std::string> get_gpio_srcs(
        const std::string& bank, const size_t mboard = 0) = 0;

    /*! Get the current source for each pin in a GPIO bank.
     *
     * \param bank the name of a GPIO bank (connector). Valid values can be
     *             obtained by calling get_gpio_src_banks().
     * \param mboard the motherboard index 0 to M-1
     * \return a list of strings for current source of each GPIO pin in the
     *         chosen bank. The length of the return value matches the number of
     *         programmable GPIO pins.
     */
    virtual std::vector<std::string> get_gpio_src(
        const std::string& bank, const size_t mboard = 0) = 0;

    /*! Set the current source for each pin in a GPIO bank.
     *
     * Note: The length of the vector must be identical to the number of
     * programmable GPIO pins.
     *
     * \param bank the name of a GPIO bank (connector). Valid values can be
     *             obtained by calling get_gpio_src_banks().
     * \param src a list of strings specifying the source of each pin in a GPIO bank
     * \param mboard the motherboard index 0 to M-1
     * \throws uhd::key_error if the bank does not exist
     * \throws uhd::value_error if the source does not exist
     * \throws uhd::not_implemented_error if the current motherboard does not
     *         support this feature
     */
    virtual void set_gpio_src(const std::string& bank,
        const std::vector<std::string>& src,
        const size_t mboard = 0) = 0;

    /*******************************************************************
     * Filter API methods
     ******************************************************************/
    // TODO: This should be a const function, but I don't want to wrestle with the
    // compiler right now
    /*!
     * Enumerate the available filters in the RX signal path.
     * \param chan RX channel index 0 to N-1
     * \return a vector of strings representing the selected filter names.
     * \return Filter names will follow the pattern BLOCK_ID:FILTER_NAME. For example,
     * "0/Radio#0:HB_0"
     */
    virtual std::vector<std::string> get_rx_filter_names(const size_t chan) = 0;

    /*!
     * Return the filter object for the given RX filter name.
     * \param name the name of the filter as returned from get_rx_filter_names().
     * \param chan RX channel index 0 to N-1
     * \return a filter_info_base::sptr.
     */
    virtual uhd::filter_info_base::sptr get_rx_filter(
        const std::string& name, const size_t chan) = 0;

    /*!
     * Write back a filter obtained by get_rx_filter() to the signal path.
     * This filter can be a modified version of the originally returned one.
     * \param name the name of the filter as returned from get_rx_filter_names().
     * \param filter the filter_info_base::sptr of the filter object to be written
     * \param chan RX channel index 0 to N-1
     */
    virtual void set_rx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) = 0;

    // TODO: This should be a const function, but I don't want to wrestle with the
    // compiler right now
    /*!
     * Enumerate the available filters in the TX signal path.
     * \param chan TX channel index 0 to N-1
     * \return a vector of strings representing the selected filter names.
     * \return Filter names will follow the pattern BLOCK_ID:FILTER_NAME. For example,
     * "0/Radio#0:HB_0"
     */
    virtual std::vector<std::string> get_tx_filter_names(const size_t chan) = 0;

    /*!
     * Return the filter object for the given TX filter name.
     * \param name the name of the filter as returned from get_tx_filter_names().
     * \param chan TX channel index 0 to N-1
     * \return a filter_info_base::sptr.
     */
    virtual uhd::filter_info_base::sptr get_tx_filter(
        const std::string& name, const size_t chan) = 0;

    /*!
     * Write back a filter obtained by get_tx_filter() to the signal path.
     * This filter can be a modified version of the originally returned one.
     * \param name the name of the filter as returned from get_tx_filter_names().
     * \param filter the filter_info_base::sptr of the filter object to be written
     * \param chan TX channel index 0 to N-1
     */
    virtual void set_tx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) = 0;

    /*! Get direct access to the underlying mb_controller object.
     *
     * Note: This is an advanced API, created for corner cases where the
     * application is using multi_usrp, but some special features from
     * mb_controller need to be used that are not exposed by multi_usrp.
     * Note that it is possible to put the mb_controller and multi_usrp into a
     * broken state by directly accessing the mb_controller. For typical
     * mb_controller operations it is therefore highly recommended
     * to not use this API call, but use the native multi_usrp API calls.
     *
     * The lifetime of the mb_controller is linked to the lifetime of the
     * device object, so storing a reference from this function is not allowed.
     *
     * \param mboard The motherboard index
     * \returns A reference to the mb_controller for the corresponding mboard
     * \throws uhd::not_implemented_error if not on an RFNoC device.
     */
    virtual uhd::rfnoc::mb_controller& get_mb_controller(const size_t mboard = 0) = 0;
};

}} // namespace uhd::usrp
