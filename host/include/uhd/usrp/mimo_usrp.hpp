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
#include <uhd/usrp/subdev_spec.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The MIMO USRP device class (DEPRECATED):
 * A mimo usrp facilitates ease-of-use for multi-usrp scenarios.
 * The wrapper provides convenience functions to control the group
 * of underlying devices as if they consisted of a single device.
 */
class UHD_API UHD_DEPRECATED mimo_usrp : boost::noncopyable{
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
    virtual void set_rx_subdev_spec(size_t chan, const uhd::usrp::subdev_spec_t &spec) = 0;
    virtual uhd::usrp::subdev_spec_t get_rx_subdev_spec(size_t chan) = 0;

    virtual void set_rx_rate_all(double rate) = 0;
    virtual double get_rx_rate_all(void) = 0;

    virtual tune_result_t set_rx_freq(size_t chan, double freq) = 0;
    //virtual tune_result_t set_rx_freq(size_t chan, double freq, double lo_off) = 0;
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
    
    virtual void set_rx_bandwidth(size_t chan, float bandwidth) = 0;

    /*******************************************************************
     * TX methods
     ******************************************************************/
    virtual void set_tx_subdev_spec(size_t chan, const uhd::usrp::subdev_spec_t &spec) = 0;
    virtual uhd::usrp::subdev_spec_t get_tx_subdev_spec(size_t chan) = 0;

    virtual void set_tx_rate_all(double rate) = 0;
    virtual double get_tx_rate_all(void) = 0;

    virtual tune_result_t set_tx_freq(size_t chan, double freq) = 0;
    //virtual tune_result_t set_tx_freq(size_t chan, double freq, double lo_off) = 0;
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

#include <uhd/utils/warning.hpp>
#include <uhd/usrp/tune_helper.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/warning.hpp>
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

namespace uhd{ namespace usrp{ namespace /*anon*/{

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

        //set the clock config across all mboards (TODO set through api)
        clock_config_t clock_config;
        clock_config.ref_source = clock_config_t::REF_SMA;
        clock_config.pps_source = clock_config_t::PPS_SMA;
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            _mboard(chan)[MBOARD_PROP_CLOCK_CONFIG] = clock_config;
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
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            buff += str(boost::format(
                "  Channel: %u\n"
                "    Mboard: %s\n"
                "    RX DSP: %s\n"
                "    RX Dboard: %s\n"
                "    RX Subdev: %s\n"
                "    TX DSP: %s\n"
                "    TX Dboard: %s\n"
                "    TX Subdev: %s\n"
            ) % chan
                % _mboard(chan)[MBOARD_PROP_NAME].as<std::string>()
                % _rx_dsp(chan)[DSP_PROP_NAME].as<std::string>()
                % _rx_dboard(chan)[DBOARD_PROP_NAME].as<std::string>()
                % _rx_subdev(chan)[SUBDEV_PROP_NAME].as<std::string>()
                % _tx_dsp(chan)[DSP_PROP_NAME].as<std::string>()
                % _tx_dboard(chan)[DBOARD_PROP_NAME].as<std::string>()
                % _tx_subdev(chan)[SUBDEV_PROP_NAME].as<std::string>()
            );
        }
        return buff;
    }

    size_t get_num_channels(void){
        return (*_dev)[DEVICE_PROP_MBOARD_NAMES].as<prop_names_t>().size();
    }

    /*******************************************************************
     * Misc
     ******************************************************************/
    time_spec_t get_time_now(void){
        //the time on the first mboard better be the same on all
        return _mboard(0)[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
    }

    void set_time_next_pps(const time_spec_t &time_spec){
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            _mboard(chan)[MBOARD_PROP_TIME_NEXT_PPS] = time_spec;
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
        for (size_t chan = 1; chan < get_num_channels(); chan++){
            time_spec_t time_0 = _mboard(0)[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
            time_spec_t time_i = _mboard(chan)[MBOARD_PROP_TIME_NOW].as<time_spec_t>();
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)){ //10 ms: greater than RTT but not too big
                uhd::print_warning(str(boost::format(
                    "Detected time deviation between board %d and board 0.\n"
                    "Board 0 time is %f seconds.\n"
                    "Board %d time is %f seconds.\n"
                ) % chan % time_0.get_real_secs() % chan % time_i.get_real_secs()));
            }
        }
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd){
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            _mboard(chan)[MBOARD_PROP_STREAM_CMD] = stream_cmd;
        }
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_subdev_spec(size_t chan, const subdev_spec_t &spec){
        UHD_ASSERT_THROW(spec.size() <= 1);
        _mboard(chan)[MBOARD_PROP_RX_SUBDEV_SPEC] = spec;
    }

    subdev_spec_t get_rx_subdev_spec(size_t chan){
        return _mboard(chan)[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>();
    }

    void set_rx_rate_all(double rate){
        std::vector<double> _actual_rates;
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            _rx_dsp(chan)[DSP_PROP_HOST_RATE] = rate;
            _actual_rates.push_back(_rx_dsp(chan)[DSP_PROP_HOST_RATE].as<double>());
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
        return tune_rx_subdev_and_dsp(_rx_subdev(chan), _rx_dsp(chan), 0, target_freq);
    }

    //tune_result_t set_rx_freq(size_t chan, double target_freq, double lo_off){
    //    return tune_rx_subdev_and_dsp(_rx_subdev(chan), _rx_dsp(chan), 0, target_freq, lo_off);
    //}

    double get_rx_freq(size_t chan){
        return derive_freq_from_rx_subdev_and_dsp(_rx_subdev(chan), _rx_dsp(chan), 0);
    }

    freq_range_t get_rx_freq_range(size_t chan){
        return add_dsp_shift(_rx_subdev(chan)[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _rx_dsp(chan));
    }

    void set_rx_gain(size_t chan, float gain){
        return _rx_gain_group(chan)->set_value(gain);
    }

    float get_rx_gain(size_t chan){
        return _rx_gain_group(chan)->get_value();
    }

    gain_range_t get_rx_gain_range(size_t chan){
        return _rx_gain_group(chan)->get_range();
    }

    void set_rx_antenna(size_t chan, const std::string &ant){
        _rx_subdev(chan)[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_rx_antenna(size_t chan){
        return _rx_subdev(chan)[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_rx_antennas(size_t chan){
        return _rx_subdev(chan)[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_rx_lo_locked(size_t chan){
        return _rx_subdev(chan)[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

    float read_rssi(size_t chan){
        return _rx_subdev(chan)[SUBDEV_PROP_RSSI].as<float>();
    }
    
    void set_rx_bandwidth(size_t chan, float bandwidth){
        _rx_subdev(chan)[SUBDEV_PROP_BANDWIDTH] = bandwidth;
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_subdev_spec(size_t chan, const subdev_spec_t &spec){
        UHD_ASSERT_THROW(spec.size() <= 1);
        _mboard(chan)[MBOARD_PROP_TX_SUBDEV_SPEC] = spec;
    }

    subdev_spec_t get_tx_subdev_spec(size_t chan){
        return _mboard(chan)[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>();
    }

    void set_tx_rate_all(double rate){
        std::vector<double> _actual_rates;
        for (size_t chan = 0; chan < get_num_channels(); chan++){
            _tx_dsp(chan)[DSP_PROP_HOST_RATE] = rate;
            _actual_rates.push_back(_tx_dsp(chan)[DSP_PROP_HOST_RATE].as<double>());
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
        return tune_tx_subdev_and_dsp(_tx_subdev(chan), _tx_dsp(chan), 0, target_freq);
    }

    //tune_result_t set_tx_freq(size_t chan, double target_freq, double lo_off){
    //    return tune_tx_subdev_and_dsp(_tx_subdev(chan), _tx_dsp(chan), 0, target_freq, lo_off);
    //}

    double get_tx_freq(size_t chan){
        return derive_freq_from_tx_subdev_and_dsp(_tx_subdev(chan), _tx_dsp(chan), 0);
    }

    freq_range_t get_tx_freq_range(size_t chan){
        return add_dsp_shift(_tx_subdev(chan)[SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>(), _tx_dsp(chan));
    }

    void set_tx_gain(size_t chan, float gain){
        return _tx_gain_group(chan)->set_value(gain);
    }

    float get_tx_gain(size_t chan){
        return _tx_gain_group(chan)->get_value();
    }

    gain_range_t get_tx_gain_range(size_t chan){
        return _tx_gain_group(chan)->get_range();
    }

    void set_tx_antenna(size_t chan, const std::string &ant){
        _tx_subdev(chan)[SUBDEV_PROP_ANTENNA] = ant;
    }

    std::string get_tx_antenna(size_t chan){
        return _tx_subdev(chan)[SUBDEV_PROP_ANTENNA].as<std::string>();
    }

    std::vector<std::string> get_tx_antennas(size_t chan){
        return _tx_subdev(chan)[SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>();
    }

    bool get_tx_lo_locked(size_t chan){
        return _tx_subdev(chan)[SUBDEV_PROP_LO_LOCKED].as<bool>();
    }

private:
    device::sptr _dev;
    wax::obj _mboard(size_t chan){
        prop_names_t names = (*_dev)[DEVICE_PROP_MBOARD_NAMES].as<prop_names_t>();
        return (*_dev)[named_prop_t(DEVICE_PROP_MBOARD, names.at(chan))];
    }
    wax::obj _rx_dsp(size_t chan){
        return _mboard(chan)[MBOARD_PROP_RX_DSP];
    }
    wax::obj _tx_dsp(size_t chan){
        return _mboard(chan)[MBOARD_PROP_TX_DSP];
    }
    wax::obj _rx_dboard(size_t chan){
        std::string db_name = _mboard(chan)[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().db_name;
        return _mboard(chan)[named_prop_t(MBOARD_PROP_RX_DBOARD, db_name)];
    }
    wax::obj _tx_dboard(size_t chan){
        std::string db_name = _mboard(chan)[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().db_name;
        return _mboard(chan)[named_prop_t(MBOARD_PROP_TX_DBOARD, db_name)];
    }
    wax::obj _rx_subdev(size_t chan){
        std::string sd_name = _mboard(chan)[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _rx_dboard(chan)[named_prop_t(DBOARD_PROP_SUBDEV, sd_name)];
    }
    wax::obj _tx_subdev(size_t chan){
        std::string sd_name = _mboard(chan)[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _tx_dboard(chan)[named_prop_t(DBOARD_PROP_SUBDEV, sd_name)];
    }
    gain_group::sptr _rx_gain_group(size_t chan){
        std::string sd_name = _mboard(chan)[MBOARD_PROP_RX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _rx_dboard(chan)[named_prop_t(DBOARD_PROP_GAIN_GROUP, sd_name)].as<gain_group::sptr>();
    }
    gain_group::sptr _tx_gain_group(size_t chan){
        std::string sd_name = _mboard(chan)[MBOARD_PROP_TX_SUBDEV_SPEC].as<subdev_spec_t>().front().sd_name;
        return _tx_dboard(chan)[named_prop_t(DBOARD_PROP_GAIN_GROUP, sd_name)].as<gain_group::sptr>();
    }

    //shadows
    double _rx_rate, _tx_rate;
};
}}}

namespace uhd{ namespace usrp{
/***********************************************************************
 * The Make Function
 **********************************************************************/
inline mimo_usrp::sptr mimo_usrp::make(const device_addr_t &dev_addr){
    uhd::print_warning(
        "The mimo USRP interface has been deprecated.\n"
        "Please switch to the multi USRP interface.\n"
        "#include <uhd/usrp/multi_usrp.hpp>\n"
        "multi_usrp::sptr sdev = multi_usrp::make(args);\n"
    );
    return sptr(new mimo_usrp_impl(dev_addr));
}
}}

#endif /* INCLUDED_UHD_USRP_MIMO_USRP_HPP */
