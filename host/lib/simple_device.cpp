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

#include <uhd/simple_device.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/props.hpp>
#include <uhd/types.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace uhd;

/***********************************************************************
 * Tune Helper Function
 **********************************************************************/
static tune_result_t tune(
    double target_freq,
    double lo_offset,
    wax::obj subdev,
    wax::obj dxc,
    bool is_tx
){
    wax::obj subdev_freq_proxy = subdev[SUBDEV_PROP_FREQ];
    bool subdev_quadrature = subdev[SUBDEV_PROP_QUADRATURE].as<bool>();
    bool subdev_spectrum_inverted = subdev[SUBDEV_PROP_SPECTRUM_INVERTED].as<bool>();
    wax::obj dxc_freq_proxy = dxc[std::string("freq")];
    double dxc_sample_rate = dxc[std::string("rate")].as<double>();

    // Ask the d'board to tune as closely as it can to target_freq+lo_offset
    double target_inter_freq = target_freq + lo_offset;
    subdev_freq_proxy = target_inter_freq;
    double actual_inter_freq = subdev_freq_proxy.as<double>();

    // Calculate the DDC setting that will downconvert the baseband from the
    // daughterboard to our target frequency.
    double delta_freq = target_freq - actual_inter_freq;
    double delta_sign = std::signum(delta_freq);
    delta_freq *= delta_sign;
    delta_freq = fmod(delta_freq, dxc_sample_rate);
    bool inverted = delta_freq > dxc_sample_rate/2.0;
    double target_dxc_freq = inverted? (delta_freq - dxc_sample_rate) : (-delta_freq);
    target_dxc_freq *= delta_sign;

    // If the spectrum is inverted, and the daughterboard doesn't do
    // quadrature downconversion, we can fix the inversion by flipping the
    // sign of the dxc_freq...  (This only happens using the basic_rx board)
    if (subdev_spectrum_inverted){
        inverted = not inverted;
    }
    if (inverted and not subdev_quadrature){
        target_dxc_freq *= -1.0;
        inverted = not inverted;
    }
    // down conversion versus up conversion, fight!
    // your mother is ugly and your going down...
    target_dxc_freq *= (is_tx)? -1.0 : +1.0;

    dxc_freq_proxy = target_dxc_freq;
    double actual_dxc_freq = dxc_freq_proxy.as<double>();

    //return some kind of tune result tuple/struct
    tune_result_t tune_result;
    tune_result.target_inter_freq = target_inter_freq;
    tune_result.actual_inter_freq = actual_inter_freq;
    tune_result.target_dxc_freq = target_dxc_freq;
    tune_result.actual_dxc_freq = actual_dxc_freq;
    tune_result.spectrum_inverted = inverted;
    return tune_result;
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static std::string trim(const std::string &in){
    return boost::algorithm::trim_copy(in);
}

device_addr_t args_to_device_addr(const std::string &args){
    device_addr_t addr;

    //split the args at the semi-colons
    std::vector<std::string> pairs;
    boost::split(pairs, args, boost::is_any_of(";"));
    BOOST_FOREACH(std::string pair, pairs){
        if (trim(pair) == "") continue;

        //split the key value pairs at the equals
        std::vector<std::string> key_val;
        boost::split(key_val, pair, boost::is_any_of("="));
        if (key_val.size() != 2) throw std::runtime_error("invalid args string: "+args);
        addr[trim(key_val[0])] = trim(key_val[1]);
    }

    return addr;
}

static std::vector<double> get_xx_rates(wax::obj decerps, wax::obj rate){
    std::vector<double> rates;
    BOOST_FOREACH(size_t decerp, decerps.as<std::vector<size_t> >()){
        rates.push_back(rate.as<double>()/decerp);
    }
    return rates;
}

/***********************************************************************
 * Simple Device Implementation
 **********************************************************************/
class simple_device_impl : public simple_device{
public:
    simple_device_impl(const device_addr_t &addr){
        _dev = device::make(addr);
        _mboard = (*_dev)[DEVICE_PROP_MBOARD];
        _rx_ddc = _mboard[named_prop_t(MBOARD_PROP_RX_DSP, "ddc0")];
        _tx_duc = _mboard[named_prop_t(MBOARD_PROP_TX_DSP, "duc0")];

        //extract rx subdevice
        wax::obj rx_dboard = _mboard[MBOARD_PROP_RX_DBOARD];
        std::string rx_subdev_in_use = rx_dboard[DBOARD_PROP_USED_SUBDEVS].as<prop_names_t>().at(0);
        _rx_subdev = rx_dboard[named_prop_t(DBOARD_PROP_SUBDEV, rx_subdev_in_use)];

        //extract tx subdevice
        wax::obj tx_dboard = _mboard[MBOARD_PROP_TX_DBOARD];
        std::string tx_subdev_in_use = tx_dboard[DBOARD_PROP_USED_SUBDEVS].as<prop_names_t>().at(0);
        _tx_subdev = tx_dboard[named_prop_t(DBOARD_PROP_SUBDEV, tx_subdev_in_use)];
    }

    ~simple_device_impl(void){
        /* NOP */
    }

    device::sptr get_device(void){
        return _dev;
    }

    std::string get_name(void){
        return _mboard[MBOARD_PROP_NAME].as<std::string>();
    }

    /*******************************************************************
     * Timing
     ******************************************************************/
    void set_time_now(const time_spec_t &time_spec){
        _mboard[MBOARD_PROP_TIME_NOW] = time_spec;
    }

    void set_time_next_pps(const time_spec_t &time_spec){
        _mboard[MBOARD_PROP_TIME_NEXT_PPS] = time_spec;
    }

    /*******************************************************************
     * Streaming
     ******************************************************************/
    void issue_stream_cmd(const stream_cmd_t &stream_cmd){
        _rx_ddc[std::string("stream_cmd")] = stream_cmd;
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_rate(double rate){
        double samp_rate = _rx_ddc[std::string("rate")].as<double>();
        assert_has(get_rx_rates(), rate, "simple device rx rate");
        _rx_ddc[std::string("decim")] = size_t(samp_rate/rate);
    }

    double get_rx_rate(void){
        double samp_rate = _rx_ddc[std::string("rate")].as<double>();
        size_t decim = _rx_ddc[std::string("decim")].as<size_t>();
        return samp_rate/decim;
    }

    std::vector<double> get_rx_rates(void){
        return get_xx_rates(_rx_ddc[std::string("decims")], _rx_ddc[std::string("rate")]);
    }

    tune_result_t set_rx_freq(double target_freq){
        double lo_offset = 0.0;
        //if the local oscillator will be in the passband, use an offset
        if (_rx_subdev[SUBDEV_PROP_LO_INTERFERES].as<bool>()){
            lo_offset = get_rx_rate()*2.0;
        }
        return tune(target_freq, lo_offset, _rx_subdev, _rx_ddc, false/* not tx */);
    }

    freq_range_t get_rx_freq_range(void){
        return _rx_subdev[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>();
    }

    void set_rx_gain(float gain){
        _rx_subdev[SUBDEV_PROP_GAIN] = gain;
    }

    float get_rx_gain(void){
        return _rx_subdev[SUBDEV_PROP_GAIN].as<gain_t>();
    }

    gain_range_t get_rx_gain_range(void){
        return _rx_subdev[SUBDEV_PROP_GAIN_RANGE].as<gain_range_t>();
    }

    void set_rx_antenna(const std::string &ant){
        _rx_subdev[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_rx_antenna(void){
        return _rx_subdev[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_rx_antennas(void){
        return _rx_subdev[SUBDEV_PROP_ANTENNA_NAMES].as<std::vector<std::string> >();
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_rate(double rate){
        double samp_rate = _tx_duc[std::string("rate")].as<double>();
        assert_has(get_tx_rates(), rate, "simple device tx rate");
        _tx_duc[std::string("interp")] = size_t(samp_rate/rate);
    }

    double get_tx_rate(void){
        double samp_rate = _tx_duc[std::string("rate")].as<double>();
        size_t interp = _tx_duc[std::string("interp")].as<size_t>();
        return samp_rate/interp;
    }

    std::vector<double> get_tx_rates(void){
        return get_xx_rates(_tx_duc[std::string("interps")], _tx_duc[std::string("rate")]);
    }

    tune_result_t set_tx_freq(double target_freq){
        double lo_offset = 0.0;
        //if the local oscillator will be in the passband, use an offset
        if (_tx_subdev[SUBDEV_PROP_LO_INTERFERES].as<bool>()){
            lo_offset = get_tx_rate()*2.0;
        }
        return tune(target_freq, lo_offset, _tx_subdev, _tx_duc, true/* is tx */);
    }

    freq_range_t get_tx_freq_range(void){
        return _tx_subdev[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>();
    }

    void set_tx_gain(float gain){
        _tx_subdev[SUBDEV_PROP_GAIN] = gain;
    }

    float get_tx_gain(void){
        return _tx_subdev[SUBDEV_PROP_GAIN].as<gain_t>();
    }

    gain_range_t get_tx_gain_range(void){
        return _tx_subdev[SUBDEV_PROP_GAIN_RANGE].as<gain_range_t>();
    }

    void set_tx_antenna(const std::string &ant){
        _tx_subdev[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_tx_antenna(void){
        return _tx_subdev[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_tx_antennas(void){
        return _tx_subdev[SUBDEV_PROP_ANTENNA_NAMES].as<std::vector<std::string> >();
    }

private:
    device::sptr _dev;
    wax::obj _mboard;
    wax::obj _rx_ddc;
    wax::obj _tx_duc;
    wax::obj _rx_subdev;
    wax::obj _tx_subdev;
};

/***********************************************************************
 * The Make Function
 **********************************************************************/
simple_device::sptr simple_device::make(const std::string &args){
    return sptr(new simple_device_impl(args_to_device_addr(args)));
}
