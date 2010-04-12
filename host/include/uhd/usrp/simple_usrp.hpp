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

#ifndef INCLUDED_UHD_USRP_SIMPLE_USRP_HPP
#define INCLUDED_UHD_USRP_SIMPLE_USRP_HPP

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
 * The simple USRP device class:
 * A simple usrp facilitates ease-of-use for most use-case scenarios.
 * The wrapper provides convenience functions to tune the devices
 * as well as to set the dboard gains, antennas, and other properties.
 */
class UHD_API simple_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<simple_usrp> sptr;
    static sptr make(const std::string &args);

    virtual device::sptr get_device(void) = 0;

    virtual std::string get_name(void) = 0;

    /*******************************************************************
     * Misc
     ******************************************************************/
    virtual void set_time_now(const time_spec_t &time_spec) = 0;
    virtual void set_time_next_pps(const time_spec_t &time_spec) = 0;
    virtual void issue_stream_cmd(const stream_cmd_t &stream_cmd) = 0;
    virtual void set_clock_config(const clock_config_t &clock_config) = 0;
    virtual double get_clock_rate(void) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/
    virtual void set_rx_rate(double rate) = 0;
    virtual double get_rx_rate(void) = 0;

    virtual tune_result_t set_rx_freq(double freq) = 0;
    virtual freq_range_t get_rx_freq_range(void) = 0;

    virtual void set_rx_gain(float gain) = 0;
    virtual float get_rx_gain(void) = 0;
    virtual gain_range_t get_rx_gain_range(void) = 0;

    virtual void set_rx_antenna(const std::string &ant) = 0;
    virtual std::string get_rx_antenna(void) = 0;
    virtual std::vector<std::string> get_rx_antennas(void) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    virtual void set_tx_rate(double rate) = 0;
    virtual double get_tx_rate(void) = 0;

    virtual tune_result_t set_tx_freq(double freq) = 0;
    virtual freq_range_t get_tx_freq_range(void) = 0;

    virtual void set_tx_gain(float gain) = 0;
    virtual float get_tx_gain(void) = 0;
    virtual gain_range_t get_tx_gain_range(void) = 0;

    virtual void set_tx_antenna(const std::string &ant) = 0;
    virtual std::string get_tx_antenna(void) = 0;
    virtual std::vector<std::string> get_tx_antennas(void) = 0;
};

}}

#endif /* INCLUDED_UHD_USRP_SIMPLE_USRP_HPP */
