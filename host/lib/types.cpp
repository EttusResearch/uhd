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

#include <uhd/types/ranges.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/io_type.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/cstdint.hpp>
#include <stdexcept>
#include <complex>

using namespace uhd;

/***********************************************************************
 * ranges
 **********************************************************************/
gain_range_t::gain_range_t(float min_, float max_, float step_){
    min = min_;
    max = max_;
    step = step_;
}

freq_range_t::freq_range_t(double min_, double max_){
    min = min_;
    max = max_;
}

/***********************************************************************
 * tune result
 **********************************************************************/
tune_result_t::tune_result_t(void){
    target_inter_freq = 0.0;
    actual_inter_freq = 0.0;
    target_dxc_freq = 0.0;
    actual_dxc_freq = 0.0;
    spectrum_inverted = false;
}

/***********************************************************************
 * clock config
 **********************************************************************/
clock_config_t::clock_config_t(void){
    ref_source = REF_INT,
    pps_source = PPS_INT,
    pps_polarity = PPS_NEG;
}

/***********************************************************************
 * stream command
 **********************************************************************/
stream_cmd_t::stream_cmd_t(const stream_mode_t &stream_mode_){
    stream_mode = stream_mode_;
    stream_now = true;
    num_samps = 0;
}

/***********************************************************************
 * metadata
 **********************************************************************/
rx_metadata_t::rx_metadata_t(void){
    stream_id = 0;
    has_stream_id = false;
    time_spec = time_spec_t();
    has_time_spec = false;
    more_fragments = false;
    fragment_offset = 0;
}

tx_metadata_t::tx_metadata_t(void){
    stream_id = 0;
    has_stream_id = false;
    time_spec = time_spec_t();
    has_time_spec = false;
    start_of_burst = false;
    end_of_burst = false;
}

/***********************************************************************
 * time spec
 **********************************************************************/
time_spec_t::time_spec_t(boost::uint32_t new_secs, boost::uint32_t new_ticks){
    secs = new_secs;
    ticks = new_ticks;
}

static const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
static double time_tick_rate = double(boost::posix_time::time_duration::ticks_per_second());

time_spec_t::time_spec_t(boost::posix_time::ptime time, double tick_rate){
    boost::posix_time::time_duration td = time - epoch;
    secs = boost::uint32_t(td.total_seconds());
    double time_ticks_per_device_ticks = time_tick_rate/tick_rate;
    ticks = boost::uint32_t(td.fractional_seconds()/time_ticks_per_device_ticks);
}

/***********************************************************************
 * device addr
 **********************************************************************/
std::string device_addr_t::to_string(void) const{
    const device_addr_t &device_addr = *this;
    std::stringstream ss;
    BOOST_FOREACH(std::string key, device_addr.get_keys()){
        ss << boost::format("%s: %s") % key % device_addr[key] << std::endl;
    }
    return ss.str();
}

static const std::string arg_delim = ";";
static const std::string pair_delim = "=";

static std::string trim(const std::string &in){
    return boost::algorithm::trim_copy(in);
}

std::string device_addr_t::to_args_str(void) const{
    std::string args_str;
    const device_addr_t &device_addr = *this;
    BOOST_FOREACH(const std::string &key, device_addr.get_keys()){
        args_str += key + pair_delim + device_addr[key] + arg_delim;
    }
    return args_str;
}

device_addr_t device_addr_t::from_args_str(const std::string &args_str){
    device_addr_t addr;

    //split the args at the semi-colons
    std::vector<std::string> pairs;
    boost::split(pairs, args_str, boost::is_any_of(arg_delim));
    BOOST_FOREACH(const std::string &pair, pairs){
        if (trim(pair) == "") continue;

        //split the key value pairs at the equals
        std::vector<std::string> key_val;
        boost::split(key_val, pair, boost::is_any_of(pair_delim));
        if (key_val.size() != 2) throw std::runtime_error("invalid args string: "+args_str);
        addr[trim(key_val[0])] = trim(key_val[1]);
    }

    return addr;
}

/***********************************************************************
 * mac addr
 **********************************************************************/
mac_addr_t::mac_addr_t(const boost::uint8_t *bytes){
    std::copy(bytes, bytes+hlen, _bytes);
}

mac_addr_t mac_addr_t::from_bytes(const boost::uint8_t *bytes){
    return mac_addr_t(bytes);
}

mac_addr_t mac_addr_t::from_string(const std::string &mac_addr_str){

    boost::uint8_t p[hlen] = {0x00, 0x50, 0xC2, 0x85, 0x30, 0x00}; // Matt's IAB

    try{
        //only allow patterns of xx:xx or xx:xx:xx:xx:xx:xx
        //the IAB above will fill in for the shorter pattern
        if (mac_addr_str.size() != 5 and mac_addr_str.size() != 17)
            throw std::runtime_error("expected exactly 5 or 17 characters");

        //split the mac addr hex string at the colons
        std::vector<std::string> hex_strs;
        boost::split(hex_strs, mac_addr_str, boost::is_any_of(":"));
        for (size_t i = 0; i < hex_strs.size(); i++){
            int hex_num;
            std::istringstream iss(hex_strs[i]);
            iss >> std::hex >> hex_num;
            p[i] = boost::uint8_t(hex_num);
        }

    }
    catch(std::exception const& e){
        throw std::runtime_error(str(
            boost::format("Invalid mac address: %s\n\t%s") % mac_addr_str % e.what()
        ));
    }

    return from_bytes(p);
}

const boost::uint8_t *mac_addr_t::to_bytes(void) const{
    return _bytes;
}

std::string mac_addr_t::to_string(void) const{
    return str(
        boost::format("%02x:%02x:%02x:%02x:%02x:%02x")
        % int(to_bytes()[0]) % int(to_bytes()[1]) % int(to_bytes()[2])
        % int(to_bytes()[3]) % int(to_bytes()[4]) % int(to_bytes()[5])
    );
}

/***********************************************************************
 * otw type
 **********************************************************************/
otw_type_t::otw_type_t(void){
    width = 0;
    shift = 0;
    byteorder = BO_NATIVE;
}

/***********************************************************************
 * io type
 **********************************************************************/
static size_t tid_to_size(io_type_t::tid_t tid){
    switch(tid){
    case io_type_t::COMPLEX_FLOAT32: return sizeof(std::complex<float>);
    case io_type_t::COMPLEX_INT16:   return sizeof(std::complex<boost::int16_t>);
    case io_type_t::COMPLEX_INT8:    return sizeof(std::complex<boost::int8_t>);
    default: throw std::runtime_error("unknown io type tid");
    }
}

io_type_t::io_type_t(tid_t tid)
: size(tid_to_size(tid)), tid(tid){
    /* NOP */
}

io_type_t::io_type_t(size_t size)
: size(size), tid(CUSTOM_TYPE){
    /* NOP */
}
