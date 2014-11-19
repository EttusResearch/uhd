//
// Copyright 2014 Per Vices Corporation
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

#ifndef INCLUDED_UHD_USRP_MULTI_CRIMSON_HPP
#define INCLUDED_UHD_USRP_MULTI_CRIMSON_HPP

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/deprecated.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <complex>
#include <string>
#include <vector>
//#include "crimson/crimson_fw_common.hpp"
#include "crimson/crimson_impl.hpp"
#include <uhd/stream.hpp>

namespace uhd{ namespace usrp{

/*!
 * The Crimson device class:
 *
 * This is the Crimson API interface that is used for Per Vices' Crimson products.
 * It contains all the base class functionality that the Ettus' USRP has, including
 * additional features. The standard Crimson features 4 independent RX chains and
 * 4 independent TX chains. This interface will also allow the use of multiple Crimson
 * units.
 *
 * Remember to map multiple Crimson units accordingly as per notes in multi_usrp.hpp.
 *
 * The notes for each function derived from the base class multi_usrp is defined in
 * the file multi_usrp.hpp. Notes for Crimson specific functions are located here.
 *
 *  A lot of the features that the multi_usrp API offers is not supported in Crimson
 *  hardware because USRP allows the user to purchase different daughter boards and have
 *  multiple mboards as well. However, Crimson is fixed to its single 1 mboard and 2 dboard,
 *  where a dboard has 4 TX chains and the other has 4 RX chains.
 *
 *  Crimson is thus represented through the UHD driver as 1 fixed mboard, and 8 dboard,
 *  where each dboard represents 1 RF chain.
 */

class multi_crimson_impl : public multi_usrp{
public:
    multi_crimson_impl(const device_addr_t &addr);
    device::sptr get_device(void);
    dict<std::string, std::string> get_usrp_rx_info(size_t chan);
    dict<std::string, std::string> get_usrp_tx_info(size_t chan);

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
    void set_master_clock_rate(double rate, size_t mboard);
    double get_master_clock_rate(size_t mboard);
    std::string get_pp_string(void);
    std::string get_mboard_name(size_t mboard);
    time_spec_t get_time_now(size_t mboard = 0);
    time_spec_t get_time_last_pps(size_t mboard = 0);
    void set_time_now(const time_spec_t &time_spec, size_t mboard);
    void set_time_next_pps(const time_spec_t &time_spec, size_t mboard);
    void set_time_unknown_pps(const time_spec_t &time_spec);
    bool get_time_synchronized(void);
    void set_command_time(const time_spec_t &time_spec, size_t mboard);
    void clear_command_time(size_t mboard);
    void issue_stream_cmd(const stream_cmd_t &stream_cmd, size_t chan);
    void set_clock_config(const clock_config_t &clock_config, size_t mboard);
    void set_time_source(const std::string &source, const size_t mboard);
    std::string get_time_source(const size_t mboard);
    std::vector<std::string> get_time_sources(const size_t mboard);
    void set_clock_source(const std::string &source, const size_t mboard);
    std::string get_clock_source(const size_t mboard);
    std::vector<std::string> get_clock_sources(const size_t mboard);
    void set_clock_source_out(const bool enb, const size_t mboard);
    void set_time_source_out(const bool enb, const size_t mboard);
    size_t get_num_mboards(void);
    sensor_value_t get_mboard_sensor(const std::string &name, size_t mboard);
    std::vector<std::string> get_mboard_sensor_names(size_t mboard);
    void set_user_register(const boost::uint8_t addr, const boost::uint32_t data, size_t mboard);

    /*******************************************************************
     * RX methods
     ******************************************************************/
    rx_streamer::sptr get_rx_stream(const stream_args_t &args) ;
    void set_rx_subdev_spec(const subdev_spec_t &spec, size_t mboard);
    subdev_spec_t get_rx_subdev_spec(size_t mboard);
    size_t get_rx_num_channels(void);
    std::string get_rx_subdev_name(size_t chan);
    void set_rx_rate(double rate, size_t chan);
    double get_rx_rate(size_t chan);
    meta_range_t get_rx_rates(size_t chan);
    tune_result_t set_rx_freq(const tune_request_t &tune_request, size_t chan);
    double get_rx_freq(size_t chan);
    freq_range_t get_rx_freq_range(size_t chan);
    freq_range_t get_fe_rx_freq_range(size_t chan);
    void set_rx_gain(double gain, const std::string &name, size_t chan);
    double get_rx_gain(const std::string &name, size_t chan);
    gain_range_t get_rx_gain_range(const std::string &name, size_t chan);
    std::vector<std::string> get_rx_gain_names(size_t chan);
    void set_rx_antenna(const std::string &ant, size_t chan);
    std::string get_rx_antenna(size_t chan);
    std::vector<std::string> get_rx_antennas(size_t chan);
    void set_rx_bandwidth(double bandwidth, size_t chan);
    double get_rx_bandwidth(size_t chan);
    meta_range_t get_rx_bandwidth_range(size_t chan);
    dboard_iface::sptr get_rx_dboard_iface(size_t chan);
    sensor_value_t get_rx_sensor(const std::string &name, size_t chan);
    std::vector<std::string> get_rx_sensor_names(size_t chan);
    void set_rx_dc_offset(const bool enb, size_t chan);
    void set_rx_dc_offset(const std::complex<double> &offset, size_t chan);
    void set_rx_iq_balance(const std::complex<double> &offset, size_t chan);

    /*******************************************************************
     * TX methods
     ******************************************************************/
    tx_streamer::sptr get_tx_stream(const stream_args_t &args) ;
    void set_tx_subdev_spec(const subdev_spec_t &spec, size_t mboard);
    subdev_spec_t get_tx_subdev_spec(size_t mboard);
    size_t get_tx_num_channels(void);
    std::string get_tx_subdev_name(size_t chan);
    void set_tx_rate(double rate, size_t chan);
    double get_tx_rate(size_t chan);
    meta_range_t get_tx_rates(size_t chan);
    tune_result_t set_tx_freq(const tune_request_t &tune_request, size_t chan);
    double get_tx_freq(size_t chan);
    freq_range_t get_tx_freq_range(size_t chan);
    freq_range_t get_fe_tx_freq_range(size_t chan);
    void set_tx_gain(double gain, const std::string &name, size_t chan);
    double get_tx_gain(const std::string &name, size_t chan);
    gain_range_t get_tx_gain_range(const std::string &name, size_t chan);
    std::vector<std::string> get_tx_gain_names(size_t chan);
    void set_tx_antenna(const std::string &ant, size_t chan);
    std::string get_tx_antenna(size_t chan);
    std::vector<std::string> get_tx_antennas(size_t chan);
    void set_tx_bandwidth(double bandwidth, size_t chan);
    double get_tx_bandwidth(size_t chan);
    meta_range_t get_tx_bandwidth_range(size_t chan);
    dboard_iface::sptr get_tx_dboard_iface(size_t chan);
    sensor_value_t get_tx_sensor(const std::string &name, size_t chan);
    std::vector<std::string> get_tx_sensor_names(size_t chan);
    void set_tx_dc_offset(const std::complex<double> &offset, size_t chan);
    void set_tx_iq_balance(const std::complex<double> &offset, size_t chan);

    /*******************************************************************
     * GPIO methods
     ******************************************************************/
    // not supported on Crimson
    std::vector<std::string> get_gpio_banks(const size_t mboard);
    // not supported on Crimson
    void set_gpio_attr(const std::string &bank, const std::string &attr, const boost::uint32_t value, const boost::uint32_t mask, const size_t mboard);
    // no supported on Crimson
    boost::uint32_t get_gpio_attr(const std::string &bank, const std::string &attr, const size_t mboard);

    /*******************************************************************
     * Crimson methods
     ******************************************************************/

private:
    // Pointer to the Crimson device
    device::sptr _dev;

    // Crimson does not support a tree/file-system structure on the SoC for properties
    property_tree::sptr _tree;

    // pointer to the streamer that is being used to send commands
    tx_streamer::sptr _stream;

    // get the string representation of the channel: 1 -> "chan1"
    std::string chan_to_string(size_t chan);

    // get the root path
    fs_path mb_root(const size_t mboard);
    fs_path rx_rf_fe_root(const size_t chan);
    fs_path rx_dsp_root(const size_t chan);
    fs_path tx_rf_fe_root(const size_t chan);
    fs_path tx_dsp_root(const size_t chan);
};

}}
#endif
