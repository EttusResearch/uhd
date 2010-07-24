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

#include <uhd/usrp/mimo_usrp.hpp>
#include <uhd/usrp/tune_helper.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <stdexcept>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

static inline freq_range_t add_dsp_shift(const freq_range_t &range, wax::obj dsp){
    double codec_rate = dsp[DSP_PROP_CODEC_RATE].as<double>();
    return freq_range_t(range.min - codec_rate/2.0, range.max + codec_rate/2.0);
}

/***********************************************************************
 * MIMO USRP Implementation
 **********************************************************************/
class mimo_usrp_impl : public mimo_usrp{
public:
    mimo_usrp_impl(const device_addr_t &addr){
        _dev = device::make(addr);

        //extract each mboard and its sub-devices
        BOOST_FOREACH(const std::string &name, (*_dev)[DEVICE_PROP_MBOARD_NAMES].as<prop_names_t>()){
            _mboards.push_back((*_dev)[named_prop_t(DEVICE_PROP_MBOARD, name)]);
            _rx_dsps.push_back(_mboards.back()[MBOARD_PROP_RX_DSP]);
            _tx_dsps.push_back(_mboards.back()[MBOARD_PROP_TX_DSP]);

            //extract rx subdevice
            _rx_dboards.push_back(_mboards.back()[MBOARD_PROP_RX_DBOARD]);
            std::string rx_subdev_in_use = _rx_dboards.back()[DBOARD_PROP_USED_SUBDEVS].as<prop_names_t>().at(0);
            _rx_subdevs.push_back(_rx_dboards.back()[named_prop_t(DBOARD_PROP_SUBDEV, rx_subdev_in_use)]);
            _rx_gain_groups.push_back(_rx_dboards.back()[named_prop_t(DBOARD_PROP_GAIN_GROUP, rx_subdev_in_use)].as<gain_group::sptr>());

            //extract tx subdevice
            _tx_dboards.push_back(_mboards.back()[MBOARD_PROP_TX_DBOARD]);
            std::string tx_subdev_in_use = _tx_dboards.back()[DBOARD_PROP_USED_SUBDEVS].as<prop_names_t>().at(0);
            _tx_subdevs.push_back(_tx_dboards.back()[named_prop_t(DBOARD_PROP_SUBDEV, tx_subdev_in_use)]);
            _tx_gain_groups.push_back(_tx_dboards.back()[named_prop_t(DBOARD_PROP_GAIN_GROUP, tx_subdev_in_use)].as<gain_group::sptr>());
        }

        //set the clock config across all mboards (TODO set through api)
        clock_config_t clock_config;
        clock_config.ref_source = clock_config_t::REF_SMA;
        clock_config.pps_source = clock_config_t::PPS_SMA;
        BOOST_FOREACH(wax::obj mboard, _mboards){
            mboard[MBOARD_PROP_CLOCK_CONFIG] = clock_config;
        }
    }

    ~mimo_usrp_impl(void){
        /* NOP */
    }

    device::sptr get_device(void){
        return _dev;
    }

    std::string get_pp_string(void){
        std::string buff = str(boost::format(
            "MIMO USRP:\n"
            "  Device: %s\n"
        )
            % (*_dev)[DEVICE_PROP_NAME].as<std::string>()
        );
        for (size_t i = 0; i < get_num_channels(); i++){
            buff += str(boost::format(
                "  Channel: %u\n"
                "    Mboard: %s\n"
                "    RX DSP: %s\n"
                "    RX Dboard: %s\n"
                "    RX Subdev: %s\n"
                "    TX DSP: %s\n"
                "    TX Dboard: %s\n"
                "    TX Subdev: %s\n"
            ) % i
                % _mboards.at(i)[MBOARD_PROP_NAME].as<std::string>()
                % _rx_dsps.at(i)[DSP_PROP_NAME].as<std::string>()
                % _rx_dboards.at(i)[DBOARD_PROP_NAME].as<std::string>()
                % _rx_subdevs.at(i)[SUBDEV_PROP_NAME].as<std::string>()
                % _tx_dsps.at(i)[DSP_PROP_NAME].as<std::string>()
                % _tx_dboards.at(i)[DBOARD_PROP_NAME].as<std::string>()
                % _tx_subdevs.at(i)[SUBDEV_PROP_NAME].as<std::string>()
            );
        }
        return buff;
    }

    size_t get_num_channels(void){
        return _mboards.size();
    }

    /*******************************************************************
     * Misc
     ******************************************************************/
    time_spec_t get_time_now(void){
        //the time on the first mboard better be the same on all
        return _mboards.front()[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
    }

    void set_time_next_pps(const time_spec_t &time_spec){
        BOOST_FOREACH(wax::obj mboard, _mboards){
            mboard[MBOARD_PROP_TIME_NEXT_PPS] = time_spec;
        }
    }

    void set_time_unknown_pps(const time_spec_t &time_spec){
        std::cout << "Set time with unknown pps edge:" << std::endl;
        std::cout << "    1) set times next pps (race condition)" << std::endl;
        set_time_next_pps(time_spec);
        boost::this_thread::sleep(boost::posix_time::seconds(1));

        std::cout << "    2) catch seconds rollover at pps edge" << std::endl;
        time_t last_secs = 0, curr_secs = 0;
        while(curr_secs == last_secs){
            last_secs = curr_secs;
            curr_secs = get_time_now().get_full_secs();
        }

        std::cout << "    3) set times next pps (synchronously)" << std::endl;
        set_time_next_pps(time_spec);
        boost::this_thread::sleep(boost::posix_time::seconds(1));

        //verify that the time registers are read to be within a few RTT
        for (size_t i = 1; i < get_num_channels(); i++){
            time_spec_t time_0 = _mboards.front()[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
            time_spec_t time_i = _mboards.at(i)[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)){ //10 ms: greater than RTT but not too big
                std::cerr << boost::format(
                    "Error: time deviation between board %d and board 0.\n"
                    "    Board 0 time is %f seconds.\n"
                    "    Board %d time is %f seconds.\n"
                ) % i % time_0.get_real_secs() % i % time_i.get_real_secs() << std::endl;
            }
        }
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd){
        BOOST_FOREACH(wax::obj mboard, _mboards){
            mboard[MBOARD_PROP_STREAM_CMD] = stream_cmd;
        }
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_rate_all(double rate){
        std::vector<double> _actual_rates;
        BOOST_FOREACH(wax::obj rx_dsp, _rx_dsps){
            rx_dsp[DSP_PROP_HOST_RATE] = rate;
            _actual_rates.push_back(rx_dsp[DSP_PROP_HOST_RATE].as<double>());
        }
        _rx_rate = _actual_rates.front();
        if (std::count(_actual_rates, _rx_rate) != _actual_rates.size()) throw std::runtime_error(
            "MIMO configuratio error: rx rate inconsistent across mboards"
        );
    }

    double get_rx_rate_all(void){
        return _rx_rate;
    }

    tune_result_t set_rx_freq(size_t chan, double target_freq){
        return tune_rx_subdev_and_dsp(_rx_subdevs.at(chan), _rx_dsps.at(chan), target_freq);
    }

    tune_result_t set_rx_freq(size_t chan, double target_freq, double lo_off){
        return tune_rx_subdev_and_dsp(_rx_subdevs.at(chan), _rx_dsps.at(chan), target_freq, lo_off);
    }

    double get_rx_freq(size_t chan){
        return derive_freq_from_rx_subdev_and_dsp(_rx_subdevs.at(chan), _rx_dsps.at(chan));
    }

    freq_range_t get_rx_freq_range(size_t chan){
        return add_dsp_shift(_rx_subdevs.at(chan)[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _rx_dsps.at(chan));
    }

    void set_rx_gain(size_t chan, float gain){
        _rx_gain_groups.at(chan)->set_value(gain);
    }

    float get_rx_gain(size_t chan){
        return _rx_gain_groups.at(chan)->get_value();
    }

    gain_range_t get_rx_gain_range(size_t chan){
        return _rx_gain_groups.at(chan)->get_range();
    }

    void set_rx_antenna(size_t chan, const std::string &ant){
        _rx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_rx_antenna(size_t chan){
        return _rx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_rx_antennas(size_t chan){
        return _rx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_rx_lo_locked(size_t chan){
        return _rx_subdevs.at(chan)[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

    float read_rssi(size_t chan){
        return _rx_subdevs.at(chan)[SUBDEV_PROP_RSSI].as<float>();
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_rate_all(double rate){
        std::vector<double> _actual_rates;
        BOOST_FOREACH(wax::obj tx_dsp, _tx_dsps){
            tx_dsp[DSP_PROP_HOST_RATE] = rate;
            _actual_rates.push_back(tx_dsp[DSP_PROP_HOST_RATE].as<double>());
        }
        _tx_rate = _actual_rates.front();
        if (std::count(_actual_rates, _tx_rate) != _actual_rates.size()) throw std::runtime_error(
            "MIMO configuratio error: tx rate inconsistent across mboards"
        );
    }

    double get_tx_rate_all(void){
        return _tx_rate;
    }

    tune_result_t set_tx_freq(size_t chan, double target_freq){
        return tune_tx_subdev_and_dsp(_tx_subdevs.at(chan), _tx_dsps.at(chan), target_freq);
    }

    tune_result_t set_tx_freq(size_t chan, double target_freq, double lo_off){
        return tune_tx_subdev_and_dsp(_tx_subdevs.at(chan), _tx_dsps.at(chan), target_freq, lo_off);
    }

    double get_tx_freq(size_t chan){
        return derive_freq_from_tx_subdev_and_dsp(_tx_subdevs.at(chan), _tx_dsps.at(chan));
    }

    freq_range_t get_tx_freq_range(size_t chan){
        return add_dsp_shift(_tx_subdevs.at(chan)[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _tx_dsps.at(chan));
    }

    void set_tx_gain(size_t chan, float gain){
        _tx_gain_groups.at(chan)->set_value(gain);
    }

    float get_tx_gain(size_t chan){
        return _tx_gain_groups.at(chan)->get_value();
    }

    gain_range_t get_tx_gain_range(size_t chan){
        return _tx_gain_groups.at(chan)->get_range();
    }

    void set_tx_antenna(size_t chan, const std::string &ant){
        _tx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_tx_antenna(size_t chan){
        return _tx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_tx_antennas(size_t chan){
        return _tx_subdevs.at(chan)[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_tx_lo_locked(size_t chan){
        return _tx_subdevs.at(chan)[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

private:
    device::sptr _dev;
    std::vector<wax::obj> _mboards;
    std::vector<wax::obj> _rx_dsps;
    std::vector<wax::obj> _tx_dsps;
    std::vector<wax::obj> _rx_dboards;
    std::vector<wax::obj> _tx_dboards;
    std::vector<wax::obj> _rx_subdevs;
    std::vector<wax::obj> _tx_subdevs;
    std::vector<gain_group::sptr> _rx_gain_groups;
    std::vector<gain_group::sptr> _tx_gain_groups;

    //shadows
    double _rx_rate, _tx_rate;
};

/***********************************************************************
 * The Make Function
 **********************************************************************/
mimo_usrp::sptr mimo_usrp::make(const device_addr_t &dev_addr){
    return sptr(new mimo_usrp_impl(dev_addr));
}
