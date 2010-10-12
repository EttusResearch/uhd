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

#include <uhd/usrp/single_usrp.hpp>
#include <uhd/usrp/simple_usrp.hpp>
#include <uhd/utils/warning.hpp>

using namespace uhd;
using namespace uhd::usrp;

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

    tune_result_t set_rx_freq(double target_freq, double lo_off){
        return _sdev->set_rx_freq(target_freq, lo_off);
    }

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

    tune_result_t set_tx_freq(double target_freq, double lo_off){
        return _sdev->set_tx_freq(target_freq, lo_off);
    }

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

/***********************************************************************
 * The Make Function
 **********************************************************************/
simple_usrp::sptr simple_usrp::make(const device_addr_t &dev_addr){
    uhd::print_warning(
        "The simple USRP interface has been deprecated.\n"
        "Please switch to the single USRP interface.\n"
        "#include <uhd/usrp/single_usrp.hpp>\n"
        "single_usrp::sptr sdev = single_usrp::make(args);\n"
    );
    return sptr(new simple_usrp_impl(dev_addr));
}
