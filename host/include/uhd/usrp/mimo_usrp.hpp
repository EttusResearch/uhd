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

#ifndef INCLUDED_UHD_USRP_MIMO_USRP_HPP
#define INCLUDED_UHD_USRP_MIMO_USRP_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/tune_result.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The MIMO USRP device class:
 * A mimo usrp facilitates ease-of-use for multi-usrp scenarios.
 * The wrapper provides convenience functions to control the group
 * of underlying devices as if they consisted of a single device.
 */
class UHD_API mimo_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<mimo_usrp> sptr;

    /*!
     * Make a new mimo usrp from the device address.
     * \param dev_addr the device address
     * \return a new mimo usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this simple usrp
     */
    virtual device::sptr get_device(void) = 0;

    /*!
     * Get a printable name for this mimo usrp.
     * \return a printable string
     */
    virtual std::string get_pp_string(void) = 0;

    /*!
     * Get the number of channels in this mimo configuration.
     * The number of rx channels == the number of tx channels.
     * \return the number of channels
     */
    virtual size_t get_num_channels(void) = 0;

    /*******************************************************************
     * Misc
     ******************************************************************/
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
     * This call works across all mboards in the mimo configuration.
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

    /*******************************************************************
     * RX methods
     ******************************************************************/
    virtual void set_rx_rate_all(double rate) = 0;
    virtual double get_rx_rate_all(void) = 0;

    virtual tune_result_t set_rx_freq(size_t chan, double freq) = 0;
    virtual tune_result_t set_rx_freq(size_t chan, double freq, double lo_off) = 0;
    virtual double get_rx_freq(size_t chan) = 0;
    virtual freq_range_t get_rx_freq_range(size_t chan) = 0;

    virtual void set_rx_gain(size_t chan, float gain) = 0;
    virtual float get_rx_gain(size_t chan) = 0;
    virtual gain_range_t get_rx_gain_range(size_t chan) = 0;

    virtual void set_rx_antenna(size_t chan, const std::string &ant) = 0;
    virtual std::string get_rx_antenna(size_t chan) = 0;
    virtual std::vector<std::string> get_rx_antennas(size_t chan) = 0;

    virtual bool get_rx_lo_locked(size_t chan) = 0;

    /*!
     * Read the RSSI value from a usrp device.
     * Or throw if the dboard does not support an RSSI readback.
     * \param chan which mimo channel 0 to N-1
     * \return the rssi in dB
     */
    virtual float read_rssi(size_t chan) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    virtual void set_tx_rate_all(double rate) = 0;
    virtual double get_tx_rate_all(void) = 0;

    virtual tune_result_t set_tx_freq(size_t chan, double freq) = 0;
    virtual tune_result_t set_tx_freq(size_t chan, double freq, double lo_off) = 0;
    virtual double get_tx_freq(size_t chan) = 0;
    virtual freq_range_t get_tx_freq_range(size_t chan) = 0;

    virtual void set_tx_gain(size_t chan, float gain) = 0;
    virtual float get_tx_gain(size_t chan) = 0;
    virtual gain_range_t get_tx_gain_range(size_t chan) = 0;

    virtual void set_tx_antenna(size_t chan, const std::string &ant) = 0;
    virtual std::string get_tx_antenna(size_t chan) = 0;
    virtual std::vector<std::string> get_tx_antennas(size_t chan) = 0;

    virtual bool get_tx_lo_locked(size_t chan) = 0;

};

}}

#endif /* INCLUDED_UHD_USRP_MIMO_USRP_HPP */
