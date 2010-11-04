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
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The simple USRP device class (DEPRECATED):
 * This interface has been deprecated in favor of the single USRP interface.
 * A simple usrp facilitates ease-of-use for most use-case scenarios.
 * The wrapper provides convenience functions to tune the devices
 * as well as to set the dboard gains, antennas, and other properties.
 */
class UHD_API UHD_DEPRECATED simple_usrp : boost::noncopyable{
public:
    typedef boost::shared_ptr<simple_usrp> sptr;

    /*!
     * Make a new simple usrp from the device address.
     * \param dev_addr the device address
     * \return a new simple usrp object
     */
    static sptr make(const device_addr_t &dev_addr);

    /*!
     * Get the underlying device object.
     * This is needed to get access to the streaming API and properties.
     * \return the device object within this simple usrp
     */
    virtual device::sptr get_device(void) = 0;

    /*!
     * Get a printable name for this simple usrp.
     * \return a printable string
     */
    virtual std::string get_pp_string(void) = 0;

    /*******************************************************************
     * Misc
     ******************************************************************/
    /*!
     * Gets the current time in the usrp time registers.
     * \return a timespec representing current usrp time
     */
    virtual time_spec_t get_time_now(void) = 0;

    /*!
     * Sets the time registers on the usrp immediately.
     * \param time_spec the time to latch into the usrp device
     */
    virtual void set_time_now(const time_spec_t &time_spec) = 0;

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
     */
    virtual void set_clock_config(const clock_config_t &clock_config) = 0;

    /*******************************************************************
     * RX methods
     ******************************************************************/
    virtual void set_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec) = 0;
    virtual uhd::usrp::subdev_spec_t get_rx_subdev_spec(void) = 0;

    virtual void set_rx_rate(double rate) = 0;
    virtual double get_rx_rate(void) = 0;

    virtual tune_result_t set_rx_freq(double freq) = 0;
    //virtual tune_result_t set_rx_freq(double freq, double lo_off) = 0;
    virtual double get_rx_freq(void) = 0;
    virtual freq_range_t get_rx_freq_range(void) = 0;

    virtual void set_rx_gain(float gain) = 0;
    virtual float get_rx_gain(void) = 0;
    virtual gain_range_t get_rx_gain_range(void) = 0;

    virtual void set_rx_antenna(const std::string &ant) = 0;
    virtual std::string get_rx_antenna(void) = 0;
    virtual std::vector<std::string> get_rx_antennas(void) = 0;

    virtual bool get_rx_lo_locked(void) = 0;

    /*!
     * Read the RSSI value from a usrp device.
     * Or throw if the dboard does not support an RSSI readback.
     * \return the rssi in dB
     */
    virtual float read_rssi(void) = 0;

    virtual dboard_iface::sptr get_rx_dboard_iface(void) = 0;
    
    virtual void set_rx_bandwidth(float) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    virtual void set_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec) = 0;
    virtual uhd::usrp::subdev_spec_t get_tx_subdev_spec(void) = 0;

    virtual void set_tx_rate(double rate) = 0;
    virtual double get_tx_rate(void) = 0;

    virtual tune_result_t set_tx_freq(double freq) = 0;
    //virtual tune_result_t set_tx_freq(double freq, double lo_off) = 0;
    virtual double get_tx_freq(void) = 0;
    virtual freq_range_t get_tx_freq_range(void) = 0;

    virtual void set_tx_gain(float gain) = 0;
    virtual float get_tx_gain(void) = 0;
    virtual gain_range_t get_tx_gain_range(void) = 0;

    virtual void set_tx_antenna(const std::string &ant) = 0;
    virtual std::string get_tx_antenna(void) = 0;
    virtual std::vector<std::string> get_tx_antennas(void) = 0;

    virtual bool get_tx_lo_locked(void) = 0;

    virtual dboard_iface::sptr get_tx_dboard_iface(void) = 0;
};

}}

#include <uhd/usrp/single_usrp.hpp>
#include <uhd/utils/warning.hpp>

namespace uhd{ namespace usrp{ namespace /*anon*/{

/***********************************************************************
 * Simple USRP Implementation
 **********************************************************************/
class simple_usrp_impl : public simple_usrp{
public:
    simple_usrp_impl(const device_addr_t &addr){
        _sdev = single_usrp::make(addr);
    }

    ~simple_usrp_impl(void){
        /* NOP */
    }

    device::sptr get_device(void){
        return _sdev->get_device();
    }

    std::string get_pp_string(void){
        return _sdev->get_pp_string();
    }

    /*******************************************************************
     * Misc
     ******************************************************************/
    time_spec_t get_time_now(void){
        return _sdev->get_time_now();
    }

    void set_time_now(const time_spec_t &time_spec){
        return _sdev->set_time_now(time_spec);
    }

    void set_time_next_pps(const time_spec_t &time_spec){
        return _sdev->set_time_next_pps(time_spec);
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd){
        return _sdev->issue_stream_cmd(stream_cmd);
    }

    void set_clock_config(const clock_config_t &clock_config){
        return _sdev->set_clock_config(clock_config);
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_subdev_spec(const subdev_spec_t &spec){
        return _sdev->set_rx_subdev_spec(spec);
    }

    subdev_spec_t get_rx_subdev_spec(void){
        return _sdev->get_rx_subdev_spec();
    }

    void set_rx_rate(double rate){
        return _sdev->set_rx_rate(rate);
    }

    double get_rx_rate(void){
        return _sdev->get_rx_rate();
    }

    tune_result_t set_rx_freq(double target_freq){
        return _sdev->set_rx_freq(target_freq);
    }

    //tune_result_t set_rx_freq(double target_freq, double lo_off){
    //    return _sdev->set_rx_freq(target_freq, lo_off);
    //}

    double get_rx_freq(void){
        return _sdev->get_rx_freq();
    }

    freq_range_t get_rx_freq_range(void){
        return _sdev->get_rx_freq_range();
    }

    void set_rx_gain(float gain){
        return _sdev->set_rx_gain(gain);
    }

    float get_rx_gain(void){
        return _sdev->get_rx_gain();
    }

    gain_range_t get_rx_gain_range(void){
        return _sdev->get_rx_gain_range();
    }

    void set_rx_antenna(const std::string &ant){
        return _sdev->set_rx_antenna(ant);
    }

    std::string get_rx_antenna(void){
        return _sdev->get_rx_antenna();
    }

    std::vector<std::string> get_rx_antennas(void){
        return _sdev->get_rx_antennas();
    }

    bool get_rx_lo_locked(void){
        return _sdev->get_rx_lo_locked();
    }

    float read_rssi(void){
        return _sdev->read_rssi();
    }

    dboard_iface::sptr get_rx_dboard_iface(void){
        return _sdev->get_rx_dboard_iface();
    }
    
    void set_rx_bandwidth(float bandwidth) {
        return _sdev->set_rx_bandwidth(bandwidth);
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_subdev_spec(const subdev_spec_t &spec){
        return _sdev->set_tx_subdev_spec(spec);
    }

    subdev_spec_t get_tx_subdev_spec(void){
        return _sdev->get_tx_subdev_spec();
    }

    void set_tx_rate(double rate){
        return _sdev->set_tx_rate(rate);
    }

    double get_tx_rate(void){
        return _sdev->get_tx_rate();
    }

    tune_result_t set_tx_freq(double target_freq){
        return _sdev->set_tx_freq(target_freq);
    }

    //tune_result_t set_tx_freq(double target_freq, double lo_off){
    //    return _sdev->set_tx_freq(target_freq, lo_off);
    //}

    double get_tx_freq(void){
        return _sdev->get_tx_freq();
    }

    freq_range_t get_tx_freq_range(void){
        return _sdev->get_tx_freq_range();
    }

    void set_tx_gain(float gain){
        return _sdev->set_tx_gain(gain);
    }

    float get_tx_gain(void){
        return _sdev->get_tx_gain();
    }

    gain_range_t get_tx_gain_range(void){
        return _sdev->get_tx_gain_range();
    }

    void set_tx_antenna(const std::string &ant){
        return _sdev->set_tx_antenna(ant);
    }

    std::string get_tx_antenna(void){
        return _sdev->get_tx_antenna();
    }

    std::vector<std::string> get_tx_antennas(void){
        return _sdev->get_tx_antennas();
    }

    bool get_tx_lo_locked(void){
        return _sdev->get_tx_lo_locked();
    }

    dboard_iface::sptr get_tx_dboard_iface(void){
        return _sdev->get_tx_dboard_iface();
    }

private:
    single_usrp::sptr _sdev;
};

}}}

namespace uhd{ namespace usrp{

/***********************************************************************
 * The Make Function
 **********************************************************************/
inline simple_usrp::sptr simple_usrp::make(const device_addr_t &dev_addr){
    uhd::warning::post(
        "The simple USRP interface has been deprecated.\n"
        "Please switch to the single USRP interface.\n"
        "#include <uhd/usrp/single_usrp.hpp>\n"
        "single_usrp::sptr sdev = single_usrp::make(args);\n"
    );
    return sptr(new simple_usrp_impl(dev_addr));
}

}}

#endif /* INCLUDED_UHD_USRP_SIMPLE_USRP_HPP */
