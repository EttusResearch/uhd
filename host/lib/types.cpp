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
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <stdexcept>

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
stream_cmd_t::stream_cmd_t(void){
    stream_now = true;
    continuous = false;
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
    is_fragment = false;
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

/***********************************************************************
 * mac addr
 **********************************************************************/
uhd::mac_addr_t::mac_addr_t(const std::string &mac_addr_str_){
    std::string mac_addr_str = (mac_addr_str_ == "")? "ff:ff:ff:ff:ff:ff" : mac_addr_str_;

    //ether_aton_r(str.c_str(), &mac_addr);
    boost::uint8_t p[6] = {0x00, 0x50, 0xC2, 0x85, 0x30, 0x00}; // Matt's IAB

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

    memcpy(&mac_addr, p, sizeof(mac_addr));
}

std::string uhd::mac_addr_t::to_string(void) const{
    //ether_ntoa_r(&mac_addr, addr_buf);
    const boost::uint8_t *p = reinterpret_cast<const boost::uint8_t *>(&mac_addr);
    return str(
        boost::format("%02x:%02x:%02x:%02x:%02x:%02x")
        % int(p[0]) % int(p[1]) % int(p[2])
        % int(p[3]) % int(p[4]) % int(p[5])
    );
}
