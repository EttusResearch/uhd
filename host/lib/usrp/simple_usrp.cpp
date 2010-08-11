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

#include <uhd/usrp/simple_usrp.hpp>
#include <uhd/usrp/tune_helper.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <stdexcept>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

static inline freq_range_t add_dsp_shift(const freq_range_t &range, wax::obj dsp){
    double codec_rate = dsp[DSP_PROP_CODEC_RATE].as<double>();
    return freq_range_t(range.min - codec_rate/2.0, range.max + codec_rate/2.0);
}

/***********************************************************************
 * Simple USRP Implementation
 **********************************************************************/
class simple_usrp_impl : public simple_usrp{
public:
    simple_usrp_impl(const device_addr_t &addr){
        _dev = device::make(addr);
    }

    ~simple_usrp_impl(void){
        /* NOP */
    }

    device::sptr get_device(void){
        return _dev;
    }

    std::string get_pp_string(void){
        return str(boost::format(
            "Simple USRP:\n"
            "  Device: %s\n"
            "  Mboard: %s\n"
            "  RX DSP: %s\n"
            "  RX Dboard: %s\n"
            "  RX Subdev: %s\n"
            "  TX DSP: %s\n"
            "  TX Dboard: %s\n"
            "  TX Subdev: %s\n"
        )
            % (*_dev)[DEVICE_PROP_NAME].as<std::string>()
            % _mboard()[MBOARD_PROP_NAME].as<std::string>()
            % _rx_dsp()[DSP_PROP_NAME].as<std::string>()
            % _rx_dboard()[DBOARD_PROP_NAME].as<std::string>()
            % _rx_subdev()[SUBDEV_PROP_NAME].as<std::string>()
            % _tx_dsp()[DSP_PROP_NAME].as<std::string>()
            % _tx_dboard()[DBOARD_PROP_NAME].as<std::string>()
            % _tx_subdev()[SUBDEV_PROP_NAME].as<std::string>()
        );
    }

    /*******************************************************************
     * Misc
     ******************************************************************/
    time_spec_t get_time_now(void){
        return _mboard()[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
    }

    void set_time_now(const time_spec_t &time_spec){
        _mboard()[MBOARD_PROP_TIME_NOW] = time_spec;
    }

    void set_time_next_pps(const time_spec_t &time_spec){
        _mboard()[MBOARD_PROP_TIME_NEXT_PPS] = time_spec;
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd){
        _mboard()[MBOARD_PROP_STREAM_CMD] = stream_cmd;
    }

    void set_clock_config(const clock_config_t &clock_config){
        _mboard()[MBOARD_PROP_CLOCK_CONFIG] = clock_config;
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_subdev_spec(const subdev_spec_t &spec){
        _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC] = spec;
        std::cout << "RX " << _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().to_pp_string() << std::endl;
    }

    subdev_spec_t get_rx_subdev_spec(void){
        return _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>();
    }

    void set_rx_rate(double rate){
        _rx_dsp()[DSP_PROP_HOST_RATE] = rate;
    }

    double get_rx_rate(void){
        return _rx_dsp()[DSP_PROP_HOST_RATE].as<double>();
    }

    tune_result_t set_rx_freq(double target_freq){
        return tune_rx_subdev_and_dsp(_rx_subdev(), _rx_dsp(), target_freq);
    }

    tune_result_t set_rx_freq(double target_freq, double lo_off){
        return tune_rx_subdev_and_dsp(_rx_subdev(), _rx_dsp(), target_freq, lo_off);
    }

    double get_rx_freq(void){
        return derive_freq_from_rx_subdev_and_dsp(_rx_subdev(), _rx_dsp());
    }

    freq_range_t get_rx_freq_range(void){
        return add_dsp_shift(_rx_subdev()[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _rx_dsp());
    }

    void set_rx_gain(float gain){
        return _rx_gain_group()->set_value(gain);
    }

    float get_rx_gain(void){
        return _rx_gain_group()->get_value();
    }

    gain_range_t get_rx_gain_range(void){
        return _rx_gain_group()->get_range();
    }

    void set_rx_antenna(const std::string &ant){
        _rx_subdev()[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_rx_antenna(void){
        return _rx_subdev()[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_rx_antennas(void){
        return _rx_subdev()[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_rx_lo_locked(void){
        return _rx_subdev()[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

    float read_rssi(void){
        return _rx_subdev()[SUBDEV_PROP_RSSI].as<float>();
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_subdev_spec(const subdev_spec_t &spec){
        _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC] = spec;
        std::cout << "TX " << _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().to_pp_string() << std::endl;
    }

    subdev_spec_t get_tx_subdev_spec(void){
        return _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>();
    }

    void set_tx_rate(double rate){
        _tx_dsp()[DSP_PROP_HOST_RATE] = rate;
    }

    double get_tx_rate(void){
        return _tx_dsp()[DSP_PROP_HOST_RATE].as<double>();
    }

    tune_result_t set_tx_freq(double target_freq){
        return tune_tx_subdev_and_dsp(_tx_subdev(), _tx_dsp(), target_freq);
    }

    tune_result_t set_tx_freq(double target_freq, double lo_off){
        return tune_tx_subdev_and_dsp(_tx_subdev(), _tx_dsp(), target_freq, lo_off);
    }

    double get_tx_freq(void){
        return derive_freq_from_tx_subdev_and_dsp(_tx_subdev(), _tx_dsp());
    }

    freq_range_t get_tx_freq_range(void){
        return add_dsp_shift(_tx_subdev()[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _tx_dsp());
    }

    void set_tx_gain(float gain){
        return _tx_gain_group()->set_value(gain);
    }

    float get_tx_gain(void){
        return _tx_gain_group()->get_value();
    }

    gain_range_t get_tx_gain_range(void){
        return _tx_gain_group()->get_range();
    }

    void set_tx_antenna(const std::string &ant){
        _tx_subdev()[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_tx_antenna(void){
        return _tx_subdev()[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_tx_antennas(void){
        return _tx_subdev()[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_tx_lo_locked(void){
        return _tx_subdev()[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

    /*******************************************************************
     * Interface access methods
     ******************************************************************/

    wax::obj get_rx_dboard_iface(void) {
        return _rx_dboard();
    }

    wax::obj get_tx_dboard_iface(void) {
        return _tx_dboard();
    }

private:
    device::sptr _dev;
    wax::obj _mboard(void){
        return (*_dev)[DEVICE_PROP_MBOARD];
    }
    wax::obj _rx_dsp(void){
        return _mboard()[MBOARD_PROP_RX_DSP];
    }
    wax::obj _tx_dsp(void){
        return _mboard()[MBOARD_PROP_TX_DSP];
    }
    wax::obj _rx_dboard(void){
        std::string db_name = _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().db_name;
        return _mboard()[named_prop_t(MBOARD_PROP_RX_DBOARD, db_name)];
    }
    wax::obj _tx_dboard(void){
        std::string db_name = _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().db_name;
        return _mboard()[named_prop_t(MBOARD_PROP_TX_DBOARD, db_name)];
    }
    wax::obj _rx_subdev(void){
        std::string sd_name = _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _rx_dboard()[named_prop_t(DBOARD_PROP_SUBDEV, sd_name)];
    }
    wax::obj _tx_subdev(void){
        std::string sd_name = _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _tx_dboard()[named_prop_t(DBOARD_PROP_SUBDEV, sd_name)];
    }
    gain_group::sptr _rx_gain_group(void){
        std::string sd_name = _mboard()[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _rx_dboard()[named_prop_t(DBOARD_PROP_GAIN_GROUP, sd_name)].as<gain_group::sptr>();
    }
    gain_group::sptr _tx_gain_group(void){
        std::string sd_name = _mboard()[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _tx_dboard()[named_prop_t(DBOARD_PROP_GAIN_GROUP, sd_name)].as<gain_group::sptr>();
    }
};

/***********************************************************************
 * The Make Function
 **********************************************************************/
simple_usrp::sptr simple_usrp::make(const device_addr_t &dev_addr){
    return sptr(new simple_usrp_impl(dev_addr));
}
