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

#include <uhd/utils/assert.hpp>
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
#include <uhd/types/serial.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>
#include <stdexcept>
#include <complex>

using namespace uhd;

/***********************************************************************
 * ranges
 **********************************************************************/
gain_range_t::gain_range_t(float min, float max, float step):
    min(min),
    max(max),
    step(step)
{
    /* NOP */
}

freq_range_t::freq_range_t(double min, double max):
    min(min),
    max(max)
{
    /* NOP */
}

/***********************************************************************
 * tune result
 **********************************************************************/
tune_result_t::tune_result_t(void):
    target_inter_freq(0.0),
    actual_inter_freq(0.0),
    target_dsp_freq(0.0),
    actual_dsp_freq(0.0),
    spectrum_inverted(false)
{
    /* NOP */
}

/***********************************************************************
 * clock config
 **********************************************************************/
clock_config_t::clock_config_t(void):
    ref_source(REF_INT),
    pps_source(PPS_INT),
    pps_polarity(PPS_NEG)
{
    /* NOP */
}

/***********************************************************************
 * stream command
 **********************************************************************/
stream_cmd_t::stream_cmd_t(const stream_mode_t &stream_mode):
    stream_mode(stream_mode),
    num_samps(0),
    stream_now(true)
{
    /* NOP */
}

/***********************************************************************
 * metadata
 **********************************************************************/
rx_metadata_t::rx_metadata_t(void):
    has_time_spec(false),
    time_spec(time_spec_t()),
    more_fragments(false),
    fragment_offset(0),
    start_of_burst(false),
    end_of_burst(false)
{
    /* NOP */
}

tx_metadata_t::tx_metadata_t(void):
    has_time_spec(false),
    time_spec(time_spec_t()),
    start_of_burst(false),
    end_of_burst(false)
{
    /* NOP */
}

/***********************************************************************
 * time spec
 **********************************************************************/
time_spec_t::time_spec_t(double secs):
    _full_secs(0),
    _frac_secs(secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, double frac_secs):
    _full_secs(full_secs),
    _frac_secs(frac_secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, size_t tick_count, double tick_rate):
    _full_secs(full_secs),
    _frac_secs(double(tick_count)/tick_rate)
{
    /* NOP */
}

size_t time_spec_t::get_tick_count(double tick_rate) const{
    return boost::math::iround(this->get_frac_secs()*tick_rate);
}

double time_spec_t::get_real_secs(void) const{
    return this->_full_secs + this->_frac_secs;
}

time_t time_spec_t::get_full_secs(void) const{
    return this->_full_secs + time_t(std::floor(this->_frac_secs));
}

double time_spec_t::get_frac_secs(void) const{
    return std::fmod(this->_frac_secs, 1.0);
}

time_spec_t &time_spec_t::operator+=(const time_spec_t &rhs){
    this->_full_secs += rhs.get_full_secs();
    this->_frac_secs += rhs.get_frac_secs();
    return *this;
}

time_spec_t &time_spec_t::operator-=(const time_spec_t &rhs){
    this->_full_secs -= rhs.get_full_secs();
    this->_frac_secs -= rhs.get_frac_secs();
    return *this;
}

bool uhd::operator==(const time_spec_t &lhs, const time_spec_t &rhs){
    return lhs.get_full_secs() == rhs.get_full_secs() and lhs.get_frac_secs() == rhs.get_frac_secs();
}

bool uhd::operator<(const time_spec_t &lhs, const time_spec_t &rhs){
    if (lhs.get_full_secs() < rhs.get_full_secs()) return true;
    if (lhs.get_full_secs() > rhs.get_full_secs()) return false;
    return lhs.get_frac_secs() < rhs.get_frac_secs();
}

/***********************************************************************
 * device addr
 **********************************************************************/
static const std::string arg_delim = ",";
static const std::string pair_delim = "=";

static std::string trim(const std::string &in){
    return boost::algorithm::trim_copy(in);
}

device_addr_t::device_addr_t(const std::string &args){
    //split the args at the semi-colons
    std::vector<std::string> pairs;
    boost::split(pairs, args, boost::is_any_of(arg_delim));
    BOOST_FOREACH(const std::string &pair, pairs){
        if (trim(pair) == "") continue;

        //split the key value pairs at the equals
        std::vector<std::string> key_val;
        boost::split(key_val, pair, boost::is_any_of(pair_delim));
        if (key_val.size() != 2) throw std::runtime_error("invalid args string: "+args);
        (*this)[trim(key_val[0])] = trim(key_val[1]);
    }
}

std::string device_addr_t::to_pp_string(void) const{
    if (this->size() == 0) return "Empty Device Address";

    std::stringstream ss;
    BOOST_FOREACH(std::string key, this->keys()){
        ss << boost::format("%s: %s") % key % (*this)[key] << std::endl;
    }
    return ss.str();
}

std::string device_addr_t::to_string(void) const{
    std::string args_str;
    BOOST_FOREACH(const std::string &key, this->keys()){
        args_str += key + pair_delim + (*this)[key] + arg_delim;
    }
    return args_str;
}

/***********************************************************************
 * mac addr
 **********************************************************************/
mac_addr_t::mac_addr_t(const byte_vector_t &bytes) : _bytes(bytes){
    UHD_ASSERT_THROW(_bytes.size() == 6);
}

mac_addr_t mac_addr_t::from_bytes(const byte_vector_t &bytes){
    return mac_addr_t(bytes);
}

mac_addr_t mac_addr_t::from_string(const std::string &mac_addr_str){

    byte_vector_t bytes = boost::assign::list_of
        (0x00)(0x50)(0xC2)(0x85)(0x30)(0x00); // Matt's IAB

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
            bytes[i] = boost::uint8_t(hex_num);
        }

    }
    catch(std::exception const& e){
        throw std::runtime_error(str(
            boost::format("Invalid mac address: %s\n\t%s") % mac_addr_str % e.what()
        ));
    }

    return mac_addr_t::from_bytes(bytes);
}

byte_vector_t mac_addr_t::to_bytes(void) const{
    return _bytes;
}

std::string mac_addr_t::to_string(void) const{
    std::string addr = "";
    BOOST_FOREACH(boost::uint8_t byte, this->to_bytes()){
        addr += str(boost::format("%s%02x") % ((addr == "")?"":":") % int(byte));
    }
    return addr;
}

/***********************************************************************
 * otw type
 **********************************************************************/
size_t otw_type_t::get_sample_size(void) const{
    return (this->width * 2) / 8;
}

otw_type_t::otw_type_t(void):
    width(0),
    shift(0),
    byteorder(BO_NATIVE)
{
    /* NOP */
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

/***********************************************************************
 * serial
 **********************************************************************/
spi_config_t::spi_config_t(edge_t edge):
    mosi_edge(edge),
    miso_edge(edge)
{
    /* NOP */
}

void i2c_iface::write_eeprom(
    boost::uint8_t addr,
    boost::uint8_t offset,
    const byte_vector_t &bytes
){
    for (size_t i = 0; i < bytes.size(); i++){
        //write a byte at a time, its easy that way
        byte_vector_t cmd = boost::assign::list_of(offset+i)(bytes[i]);
        this->write_i2c(addr, cmd);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10)); //worst case write
    }
}

byte_vector_t i2c_iface::read_eeprom(
    boost::uint8_t addr,
    boost::uint8_t offset,
    size_t num_bytes
){
    byte_vector_t bytes;
    for (size_t i = 0; i < num_bytes; i++){
        //do a zero byte write to start read cycle
        this->write_i2c(addr, byte_vector_t(1, offset+i));
        bytes.push_back(this->read_i2c(addr, 1).at(0));
    }
    return bytes;
}
