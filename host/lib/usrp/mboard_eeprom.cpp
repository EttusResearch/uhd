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

#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Constants
 **********************************************************************/
static const size_t SERIAL_LEN = 9;
static const size_t NAME_MAX_LEN = 32 - SERIAL_LEN;

/***********************************************************************
 * Utility functions
 **********************************************************************/

//! create a string from a byte vector, return empty if invalid ascii
static const std::string bytes_to_string(const byte_vector_t &bytes){
    std::string out;
    BOOST_FOREACH(boost::uint8_t byte, bytes){
        if (byte < 32 or byte > 127) return out;
        out += byte;
    }
    return out;
}

//! create a byte vector from a string, null terminate unless max length
static const byte_vector_t string_to_bytes(const std::string &string, size_t max_length){
    byte_vector_t bytes;
    for (size_t i = 0; i < std::min(string.size(), max_length); i++){
        bytes.push_back(string[i]);
    }
    if (bytes.size() < max_length - 1) bytes.push_back('\0');
    return bytes;
}

/***********************************************************************
 * Implementation of N100 load/store
 **********************************************************************/
static const boost::uint8_t N100_EEPROM_ADDR = 0x50;

static const uhd::dict<std::string, boost::uint8_t> USRP_N100_OFFSETS = boost::assign::map_list_of
    ("rev-lsb-msb", 0x00)
    ("mac-addr", 0x02)
    ("ip-addr", 0x0C)
    //leave space here for other addresses (perhaps)
    ("serial", 0x18)
    ("name", 0x18 + SERIAL_LEN)
;

static void load_n100(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the revision number
    byte_vector_t rev_lsb_msb = iface.read_eeprom(N100_EEPROM_ADDR, USRP_N100_OFFSETS["rev-lsb-msb"], 2);
    boost::uint16_t rev = (boost::uint16_t(rev_lsb_msb.at(0)) << 0) | (boost::uint16_t(rev_lsb_msb.at(1)) << 8);
    mb_eeprom["rev"] = boost::lexical_cast<std::string>(rev);

    //extract the addresses
    mb_eeprom["mac-addr"] = mac_addr_t::from_bytes(iface.read_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["mac-addr"], 6
    )).to_string();

    boost::asio::ip::address_v4::bytes_type ip_addr_bytes;
    std::copy(iface.read_eeprom(N100_EEPROM_ADDR, USRP_N100_OFFSETS["ip-addr"], 4), ip_addr_bytes);
    mb_eeprom["ip-addr"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["serial"], SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["name"], NAME_MAX_LEN
    ));

    //empty serial correction: use the mac address
    if (mb_eeprom["serial"].empty()) mb_eeprom["serial"] = mb_eeprom["mac-addr"];
}

static void store_n100(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //parse the revision number
    if (mb_eeprom.has_key("rev")){
        boost::uint16_t rev = boost::lexical_cast<boost::uint16_t>(mb_eeprom["rev"]);
        byte_vector_t rev_lsb_msb = boost::assign::list_of
            (boost::uint8_t(rev >> 0))
            (boost::uint8_t(rev >> 8))
        ;
        iface.write_eeprom(N100_EEPROM_ADDR, USRP_N100_OFFSETS["rev-lsb-msb"], rev_lsb_msb);
    }

    //store the addresses
    if (mb_eeprom.has_key("mac-addr")) iface.write_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["mac-addr"],
        mac_addr_t::from_string(mb_eeprom["mac-addr"]).to_bytes()
    );

    if (mb_eeprom.has_key("ip-addr")){
        byte_vector_t ip_addr_bytes(4);
        std::copy(boost::asio::ip::address_v4::from_string(mb_eeprom["ip-addr"]).to_bytes(), ip_addr_bytes);
        iface.write_eeprom(N100_EEPROM_ADDR, USRP_N100_OFFSETS["ip-addr"], ip_addr_bytes);
    }

    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["serial"],
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        N100_EEPROM_ADDR, USRP_N100_OFFSETS["name"],
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

/***********************************************************************
 * Implementation of B000 load/store
 **********************************************************************/
static const boost::uint8_t B000_EEPROM_ADDR = 0x50;
static const size_t B000_SERIAL_LEN = 8;

static const uhd::dict<std::string, boost::uint8_t> USRP_B000_OFFSETS = boost::assign::map_list_of
    ("serial", 0xf8)
    ("name", 0xf8 - NAME_MAX_LEN)
;

static void load_b000(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        B000_EEPROM_ADDR, USRP_B000_OFFSETS["serial"], B000_SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        B000_EEPROM_ADDR, USRP_B000_OFFSETS["name"], NAME_MAX_LEN
    ));
}

static void store_b000(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        B000_EEPROM_ADDR, USRP_B000_OFFSETS["serial"],
        string_to_bytes(mb_eeprom["serial"], B000_SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        B000_EEPROM_ADDR, USRP_B000_OFFSETS["name"],
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

/***********************************************************************
 * Implementation of mboard eeprom
 **********************************************************************/
mboard_eeprom_t::mboard_eeprom_t(void){
    /* NOP */
}

mboard_eeprom_t::mboard_eeprom_t(i2c_iface &iface, map_type map){
    switch(map){
    case MAP_N100: load_n100(*this, iface); break;
    case MAP_B000: load_b000(*this, iface); break;
    }
}

void mboard_eeprom_t::commit(i2c_iface &iface, map_type map){
    switch(map){
    case MAP_N100: store_n100(*this, iface); break;
    case MAP_B000: store_b000(*this, iface); break;
    }
}
