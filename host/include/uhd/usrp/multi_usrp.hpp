//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The multi-USRP device class:
 * A multi-USRP facilitates ease-of-use for multiple USRP scenarios.
 * The wrapper provides convenience functions to control the group
 * of underlying devices as if they consisted of a single device.
 *
 * A few notes about a multi-USRP configuration:
 *  - All boards share a common RX sample rate
 *  - All boards share a common TX sample rate
 *  - All boards share a common RX subdevice specification size
 *  - All boards share a common TX subdevice specification size
 *  - All boards must have synchronized times (see the set_time_*() calls)
 *
 * Example to setup channel mapping:
 * <pre>
 *
 * //create a multi_usrp with two boards in the configuration
 * device_addr_t dev_addr;
 * dev_addr["addr"] = "192.168.10.2 192.168.10.3";
 * multi_usrp::sptr dev = multi_usrp::make(dev_addr);
 *
 * //set the board on 10.2 to use the A RX subdevice (RX channel 0)
 * dev->set_rx_subdev_spec(":A", 0);
 *
 * //set the board on 10.3 to use the B RX subdevice (RX channel 1)
 * dev->set_rx_subdev_spec(":B", 1);
 *
 * //set both boards to use the AB TX subdevice (TX channels 0 and 1)
 * dev->set_tx_subdev_spec(":AB", multi_usrp::ALL_MBOARDS);
 *
 * //now that all the channels are mapped, continue with configuration...
 *
 * </pre>
 */
class UHD_API multi_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<multi_usrp> sptr;

    //! A wildcard motherboard index
    static const size_t ALL_MBOARDS = size_t(~0);

    /*!
     * Make a new multi usrp from the device address.
     * \param dev_addr the device address
     * \return a new single usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this single usrp
     */
    virtual device::sptr get_device(void) = 0;

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
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
    virtual std::string get_mboard_name(size_t mboard) = 0;

    /*!
     * Gets the current time in the usrp time registers.
     * \return a timespec representing current usrp time
     */
    virtual time_spec_t get_time_now(void) = 0;

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
     */
    virtual void set_time_next_pps(const time_spec_t &time_spec) = 0;

    /*!
     * Synchronize the times across all motherboards in this configuration.
     * Use this method to sync the times when the edge of the PPS is unknown.
     *
     * Ex: Host machine is not attached to serial port of GPSDO
     * and can therefore not query the GPSDO for the PPS edge.
     *
     * This is a 3-step process, and will take at most 3 seconds to complete.
     * Upon completion, the times will be synchronized to the time provided.
     *
     * - Step1: set the time at the next pps (potential race condition)
     * - Step2: wait for the seconds to rollover to catch the pps edge
     * - Step3: set the time at the next pps (synchronous for all boards)
     *
     * \param time_spec the time to latch into the usrp device
     */
    virtual void set_time_unknown_pps(const time_spec_t &time_spec) = 0;

    /*!
     * Issue a stream command to the usrp device.
     * This tells the usrp to send samples into the host.
     * See the documentation for stream_cmd_t for more info.
     * \param stream_cmd the stream command to issue
     */
    virtual void issue_stream_cmd(const stream_cmd_t &stream_cmd) = 0;

    /*!
     * Set the clock configuration for the usrp device.
     * This tells the usrp how to get a 10Mhz reference and PPS clock.
     * See the documentation for clock_config_t for more info.
     * \param clock_config the clock configuration to set
     * \param mboard which motherboard to set the config
     */
    virtual void set_clock_config(const clock_config_t &clock_config, size_t mboard) = 0;

    /*!
     * Get the number of USRP motherboards in this configuration.
     */
    virtual size_t get_num_mboards(void) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/
    /*!
     * Set the RX subdevice specification:
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     */
    virtual void set_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec, size_t mboard) = 0;
    virtual uhd::usrp::subdev_spec_t get_rx_subdev_spec(size_t mboard) = 0;

    /*!
     * Get the number of RX channels in this configuration.
     * This is the number of USRPs times the number of RX channels per board,
     * where the number of RX channels per board is homogeneous among all USRPs.
     */
    virtual size_t get_rx_num_channels(void) = 0;

    virtual std::string get_rx_subdev_name(size_t chan) = 0;

    virtual void set_rx_rate(double rate) = 0;
    virtual double get_rx_rate(void) = 0;

    virtual tune_result_t set_rx_freq(double freq, size_t chan) = 0;
    virtual tune_result_t set_rx_freq(double freq, double lo_off, size_t chan) = 0;
    virtual double get_rx_freq(size_t chan) = 0;
    virtual freq_range_t get_rx_freq_range(size_t chan) = 0;

    virtual void set_rx_gain(float gain, size_t chan) = 0;
    virtual float get_rx_gain(size_t chan) = 0;
    virtual gain_range_t get_rx_gain_range(size_t chan) = 0;

    virtual void set_rx_antenna(const std::string &ant, size_t chan) = 0;
    virtual std::string get_rx_antenna(size_t chan) = 0;
    virtual std::vector<std::string> get_rx_antennas(size_t chan) = 0;

    virtual bool get_rx_lo_locked(size_t chan) = 0;

    virtual void set_rx_bandwidth(double bandwidth, size_t chan) = 0;
    virtual double get_rx_bandwidth(size_t chan) = 0;

    /*!
     * Read the RSSI value from a usrp device.
     * Or throw if the dboard does not support an RSSI readback.
     * \return the rssi in dB
     */
    virtual float read_rssi(size_t chan) = 0;

    virtual dboard_iface::sptr get_rx_dboard_iface(size_t chan) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    /*!
     * Set the TX subdevice specification:
     * The subdev spec maps a physical part of a daughter-board to a channel number.
     * Set the subdev spec before calling into any methods with a channel number.
     * The subdev spec must be the same size across all motherboards.
     */
    virtual void set_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec, size_t mboard) = 0;
    virtual uhd::usrp::subdev_spec_t get_tx_subdev_spec(size_t mboard) = 0;

    /*!
     * Get the number of TX channels in this configuration.
     * This is the number of USRPs times the number of TX channels per board,
     * where the number of TX channels per board is homogeneous among all USRPs.
     */
    virtual size_t get_tx_num_channels(void) = 0;

    virtual std::string get_tx_subdev_name(size_t chan) = 0;

    virtual void set_tx_rate(double rate) = 0;
    virtual double get_tx_rate(void) = 0;

    virtual tune_result_t set_tx_freq(double freq, size_t chan) = 0;
    virtual tune_result_t set_tx_freq(double freq, double lo_off, size_t chan) = 0;
    virtual double get_tx_freq(size_t chan) = 0;
    virtual freq_range_t get_tx_freq_range(size_t chan) = 0;

    virtual void set_tx_gain(float gain, size_t chan) = 0;
    virtual float get_tx_gain(size_t chan) = 0;
    virtual gain_range_t get_tx_gain_range(size_t chan) = 0;

    virtual void set_tx_antenna(const std::string &ant, size_t chan) = 0;
    virtual std::string get_tx_antenna(size_t chan) = 0;
    virtual std::vector<std::string> get_tx_antennas(size_t chan) = 0;

    virtual bool get_tx_lo_locked(size_t chan) = 0;

    virtual void set_tx_bandwidth(double bandwidth, size_t chan) = 0;
    virtual double get_tx_bandwidth(size_t chan) = 0;

    virtual dboard_iface::sptr get_tx_dboard_iface(size_t chan) = 0;
};

}}

#endif /* INCLUDED_UHD_USRP_MULTI_USRP_HPP */
