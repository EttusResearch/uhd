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

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <uhd/device.hpp>
#include <vector>

#ifndef INCLUDED_UHD_SIMPLE_DEVICE_HPP
#define INCLUDED_UHD_SIMPLE_DEVICE_HPP

namespace uhd{

/*!
 * The tune result struct holds result of a 2-phase tuning:
 * The struct hold the result of tuning the dboard as
 * the target and actual intermediate frequency.
 * The struct hold the result of tuning the DDC/DUC as
 * the target and actual digital converter frequency.
 * It also tell us weather or not the spectrum is inverted.
 */
struct tune_result_t{
    double target_inter_freq;
    double actual_inter_freq;
    double target_dxc_freq;
    double actual_dxc_freq;
    bool spectrum_inverted;
    tune_result_t(void);
};

/*!
 * The simple UHD device class:
 * A simple device facilitates ease-of-use for most use-case scenarios.
 * The wrapper provides convenience functions to tune the devices
 * as well as to set the dboard gains, antennas, and other properties.
 */
class simple_device : boost::noncopyable{
public:
    typedef boost::shared_ptr<simple_device> sptr;
    static sptr make(const std::string &args);

    virtual device::sptr get_device(void) = 0;

    virtual std::string get_name(void) = 0;

    /*******************************************************************
     * Streaming
     ******************************************************************/
    virtual void set_streaming(bool enb) = 0;
    virtual bool get_streaming(void) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/
    virtual void set_rx_rate(double rate) = 0;
    virtual double get_rx_rate(void) = 0;
    virtual std::vector<double> get_rx_rates(void) = 0;

    virtual tune_result_t set_rx_freq(double freq) = 0;
    virtual double get_rx_freq_min(void) = 0;
    virtual double get_rx_freq_max(void) = 0;

    virtual void set_rx_gain(float gain) = 0;
    virtual float get_rx_gain(void) = 0;
    virtual float get_rx_gain_min(void) = 0;
    virtual float get_rx_gain_max(void) = 0;
    virtual float get_rx_gain_step(void) = 0;

    virtual void set_rx_antenna(const std::string &ant) = 0;
    virtual std::string get_rx_antenna(void) = 0;
    virtual std::vector<std::string> get_rx_antennas(void) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    virtual void set_tx_rate(double rate) = 0;
    virtual double get_tx_rate(void) = 0;
    virtual std::vector<double> get_tx_rates(void) = 0;

    virtual tune_result_t set_tx_freq(double freq) = 0;
    virtual double get_tx_freq_min(void) = 0;
    virtual double get_tx_freq_max(void) = 0;

    virtual void set_tx_gain(float gain) = 0;
    virtual float get_tx_gain(void) = 0;
    virtual float get_tx_gain_min(void) = 0;
    virtual float get_tx_gain_max(void) = 0;
    virtual float get_tx_gain_step(void) = 0;

    virtual void set_tx_antenna(const std::string &ant) = 0;
    virtual std::string get_tx_antenna(void) = 0;
    virtual std::vector<std::string> get_tx_antennas(void) = 0;
};

} //namespace uhd

#endif /* INCLUDED_UHD_SIMPLE_DEVICE_HPP */
