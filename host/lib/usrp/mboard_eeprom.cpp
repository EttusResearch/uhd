//
// Copyright 2010-2013,2015 Ettus Research LLC
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
#include <uhd/types/byte_vector.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include <iostream>
#include <cstddef>

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

//! convert a string to a byte vector to write to eeprom
static byte_vector_t string_to_uint16_bytes(const std::string &num_str){
    const boost::uint16_t num = boost::lexical_cast<boost::uint16_t>(num_str);
    const byte_vector_t lsb_msb = boost::assign::list_of
        (boost::uint8_t(num >> 0))(boost::uint8_t(num >> 8));
    return lsb_msb;
}

//! convert a byte vector read from eeprom to a string
static std::string uint16_bytes_to_string(const byte_vector_t &bytes){
    const boost::uint16_t num = (boost::uint16_t(bytes.at(0)) << 0) | (boost::uint16_t(bytes.at(1)) << 8);
    return (num == 0 or num == 0xffff)? "" : boost::lexical_cast<std::string>(num);
}

/***********************************************************************
 * Implementation of N100 load/store
 **********************************************************************/
static const boost::uint8_t N100_EEPROM_ADDR = 0x50;

struct n100_eeprom_map{
    boost::uint16_t hardware;
    boost::uint8_t mac_addr[6];
    boost::uint32_t subnet;
    boost::uint32_t ip_addr;
    boost::uint16_t _pad0;
    boost::uint16_t revision;
    boost::uint16_t product;
    unsigned char _pad1;
    unsigned char gpsdo;
    unsigned char serial[SERIAL_LEN];
    unsigned char name[NAME_MAX_LEN];
    boost::uint32_t gateway;
};

enum n200_gpsdo_type{
    N200_GPSDO_NONE = 0,
    N200_GPSDO_INTERNAL = 1,
    N200_GPSDO_ONBOARD = 2
};

static void load_n100(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the hardware number
    mb_eeprom["hardware"] = uint16_bytes_to_string(
        iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, hardware), 2)
    );

    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, revision), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, product), 2)
    );

    //extract the addresses
    mb_eeprom["mac-addr"] = mac_addr_t::from_bytes(iface.read_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, mac_addr), 6
    )).to_string();

    boost::asio::ip::address_v4::bytes_type ip_addr_bytes;
    byte_copy(iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, ip_addr), 4), ip_addr_bytes);
    mb_eeprom["ip-addr"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    byte_copy(iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, subnet), 4), ip_addr_bytes);
    mb_eeprom["subnet"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    byte_copy(iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, gateway), 4), ip_addr_bytes);
    mb_eeprom["gateway"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    //gpsdo capabilities
    boost::uint8_t gpsdo_byte = iface.read_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, gpsdo), 1).at(0);
    switch(n200_gpsdo_type(gpsdo_byte)){
    case N200_GPSDO_INTERNAL: mb_eeprom["gpsdo"] = "internal"; break;
    case N200_GPSDO_ONBOARD: mb_eeprom["gpsdo"] = "onboard"; break;
    default: mb_eeprom["gpsdo"] = "none";
    }

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, name), NAME_MAX_LEN
    ));

    //Empty serial correction: use the mac address to determine serial.
    //Older usrp2 models don't have a serial burned into EEPROM.
    //The lower mac address bits will function as the serial number.
    if (mb_eeprom["serial"].empty()){
        byte_vector_t mac_addr_bytes = mac_addr_t::from_string(mb_eeprom["mac-addr"]).to_bytes();
        unsigned serial = mac_addr_bytes.at(5) | (unsigned(mac_addr_bytes.at(4) & 0x0f) << 8);
        mb_eeprom["serial"] = boost::lexical_cast<std::string>(serial);
    }
}

static void store_n100(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //parse the revision number
    if (mb_eeprom.has_key("hardware")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, hardware),
        string_to_uint16_bytes(mb_eeprom["hardware"])
    );

    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the addresses
    if (mb_eeprom.has_key("mac-addr")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, mac_addr),
        mac_addr_t::from_string(mb_eeprom["mac-addr"]).to_bytes()
    );

    if (mb_eeprom.has_key("ip-addr")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["ip-addr"]).to_bytes(), ip_addr_bytes);
        iface.write_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, ip_addr), ip_addr_bytes);
    }

    if (mb_eeprom.has_key("subnet")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["subnet"]).to_bytes(), ip_addr_bytes);
        iface.write_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, subnet), ip_addr_bytes);
    }

    if (mb_eeprom.has_key("gateway")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["gateway"]).to_bytes(), ip_addr_bytes);
        iface.write_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, gateway), ip_addr_bytes);
    }

    //gpsdo capabilities
    if (mb_eeprom.has_key("gpsdo")){
        boost::uint8_t gpsdo_byte = N200_GPSDO_NONE;
        if (mb_eeprom["gpsdo"] == "internal") gpsdo_byte = N200_GPSDO_INTERNAL;
        if (mb_eeprom["gpsdo"] == "onboard") gpsdo_byte = N200_GPSDO_ONBOARD;
        iface.write_eeprom(N100_EEPROM_ADDR, offsetof(n100_eeprom_map, gpsdo), byte_vector_t(1, gpsdo_byte));
    }

    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        N100_EEPROM_ADDR, offsetof(n100_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

/***********************************************************************
 * Implementation of X300 load/store
 **********************************************************************/
static const boost::uint8_t X300_EEPROM_ADDR = 0x50;

struct x300_eeprom_map
{
    //indentifying numbers
    unsigned char revision[2];
    unsigned char product[2];
    unsigned char revision_compat[2];
    boost::uint8_t _pad0[2];

    //all the mac addrs
    boost::uint8_t mac_addr0[6];
    boost::uint8_t _pad1[2];
    boost::uint8_t mac_addr1[6];
    boost::uint8_t _pad2[2];

    //all the IP addrs
    boost::uint32_t gateway;
    boost::uint32_t subnet[4];
    boost::uint32_t ip_addr[4];
    boost::uint8_t _pad3[16];

    //names and serials
    unsigned char name[NAME_MAX_LEN];
    unsigned char serial[SERIAL_LEN];
};

static void load_x300(mboard_eeprom_t &mb_eeprom, i2c_iface &iface)
{
    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision), 2)
    );

    //extract the revision compat number
    mb_eeprom["revision_compat"] = uint16_bytes_to_string(
        iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision_compat), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, product), 2)
    );

    //extract the mac addresses
    mb_eeprom["mac-addr0"] = mac_addr_t::from_bytes(iface.read_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr0), 6
    )).to_string();
    mb_eeprom["mac-addr1"] = mac_addr_t::from_bytes(iface.read_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr1), 6
    )).to_string();

    //extract the ip addresses
    boost::asio::ip::address_v4::bytes_type ip_addr_bytes;
    byte_copy(iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, gateway), 4), ip_addr_bytes);
    mb_eeprom["gateway"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();
    for (size_t i = 0; i < 4; i++)
    {
        const std::string n(1, i+'0');
        byte_copy(iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, ip_addr)+(i*4), 4), ip_addr_bytes);
        mb_eeprom["ip-addr"+n] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

        byte_copy(iface.read_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, subnet)+(i*4), 4), ip_addr_bytes);
        mb_eeprom["subnet"+n] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();
    }

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, name), NAME_MAX_LEN
    ));
}

static void store_x300(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface)
{
    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the revision compat number
    if (mb_eeprom.has_key("revision_compat")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision_compat),
        string_to_uint16_bytes(mb_eeprom["revision_compat"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the mac addresses
    if (mb_eeprom.has_key("mac-addr0")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr0),
        mac_addr_t::from_string(mb_eeprom["mac-addr0"]).to_bytes()
    );
    if (mb_eeprom.has_key("mac-addr1")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr1),
        mac_addr_t::from_string(mb_eeprom["mac-addr1"]).to_bytes()
    );

    //store the ip addresses
    byte_vector_t ip_addr_bytes(4);
    if (mb_eeprom.has_key("gateway")){
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["gateway"]).to_bytes(), ip_addr_bytes);
        iface.write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, gateway), ip_addr_bytes);
    }
    for (size_t i = 0; i < 4; i++)
    {
        const std::string n(1, i+'0');
        if (mb_eeprom.has_key("ip-addr"+n)){
            byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["ip-addr"+n]).to_bytes(), ip_addr_bytes);
            iface.write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, ip_addr)+(i*4), ip_addr_bytes);
        }

        if (mb_eeprom.has_key("subnet"+n)){
            byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["subnet"+n]).to_bytes(), ip_addr_bytes);
            iface.write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, subnet)+(i*4), ip_addr_bytes);
        }
    }

    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

/***********************************************************************
 * Implementation of B000 load/store
 **********************************************************************/
static const boost::uint8_t B000_EEPROM_ADDR = 0x50;
static const size_t B000_SERIAL_LEN = 8;

//use char array so we dont need to attribute packed
struct b000_eeprom_map{
    unsigned char _r[221];
    unsigned char mcr[4];
    unsigned char name[NAME_MAX_LEN];
    unsigned char serial[B000_SERIAL_LEN];
};

static void load_b000(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        B000_EEPROM_ADDR, offsetof(b000_eeprom_map, serial), B000_SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        B000_EEPROM_ADDR, offsetof(b000_eeprom_map, name), NAME_MAX_LEN
    ));

    //extract master clock rate as a 32-bit uint in Hz
    boost::uint32_t master_clock_rate;
    const byte_vector_t rate_bytes = iface.read_eeprom(
        B000_EEPROM_ADDR, offsetof(b000_eeprom_map, mcr), sizeof(master_clock_rate)
    );
    std::copy(
        rate_bytes.begin(), rate_bytes.end(), //input
        reinterpret_cast<boost::uint8_t *>(&master_clock_rate) //output
    );
    master_clock_rate = ntohl(master_clock_rate);
    if (master_clock_rate > 1e6 and master_clock_rate < 1e9){
        mb_eeprom["mcr"] = boost::lexical_cast<std::string>(master_clock_rate);
    }
    else mb_eeprom["mcr"] = "";
}

static void store_b000(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        B000_EEPROM_ADDR, offsetof(b000_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], B000_SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        B000_EEPROM_ADDR, offsetof(b000_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );

    //store the master clock rate as a 32-bit uint in Hz
    if (mb_eeprom.has_key("mcr")){
        boost::uint32_t master_clock_rate = boost::uint32_t(boost::lexical_cast<double>(mb_eeprom["mcr"]));
        master_clock_rate = htonl(master_clock_rate);
        const byte_vector_t rate_bytes(
            reinterpret_cast<const boost::uint8_t *>(&master_clock_rate),
            reinterpret_cast<const boost::uint8_t *>(&master_clock_rate) + sizeof(master_clock_rate)
        );
        iface.write_eeprom(
            B000_EEPROM_ADDR, offsetof(b000_eeprom_map, mcr), rate_bytes
        );
    }
}

/***********************************************************************
 * Implementation of B100 load/store
 **********************************************************************/
static const boost::uint8_t B100_EEPROM_ADDR = 0x50;

//use char array so we dont need to attribute packed
struct b100_eeprom_map{
    unsigned char _r[220];
    unsigned char revision[2];
    unsigned char product[2];
    unsigned char name[NAME_MAX_LEN];
    unsigned char serial[SERIAL_LEN];
};

static void load_b100(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface.read_eeprom(B100_EEPROM_ADDR, offsetof(b100_eeprom_map, revision), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface.read_eeprom(B100_EEPROM_ADDR, offsetof(b100_eeprom_map, product), 2)
    );

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, name), NAME_MAX_LEN
    ));
}

static void store_b100(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface.write_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface.write_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        B100_EEPROM_ADDR, offsetof(b100_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

/***********************************************************************
 * Implementation of B200 load/store
 **********************************************************************/
/* On the B200, this field indicates the slave address. From the FX3, this
 * address is always 0. */
static const boost::uint8_t B200_EEPROM_SLAVE_ADDR = 0x04;

//use char array so we dont need to attribute packed
struct b200_eeprom_map{
    unsigned char _r[220];
    unsigned char revision[2];
    unsigned char product[2];
    unsigned char name[NAME_MAX_LEN];
    unsigned char serial[SERIAL_LEN];
};

static void load_b200(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface.read_eeprom(B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, revision), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface.read_eeprom(B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, product), 2)
    );

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, name), NAME_MAX_LEN
    ));
}

static void store_b200(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface.write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface.write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the serial
    if (mb_eeprom.has_key("serial")) iface.write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface.write_eeprom(
        B200_EEPROM_SLAVE_ADDR, offsetof(b200_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}
/***********************************************************************
 * Implementation of E100 load/store
 **********************************************************************/
static const boost::uint8_t E100_EEPROM_ADDR = 0x51;

struct e100_eeprom_map{
    boost::uint16_t vendor;
    boost::uint16_t device;
    unsigned char revision;
    unsigned char content;
    unsigned char model[8];
    unsigned char env_var[16];
    unsigned char env_setting[64];
    unsigned char serial[10];
    unsigned char name[NAME_MAX_LEN];
};

template <typename T> static const byte_vector_t to_bytes(const T &item){
    return byte_vector_t(
        reinterpret_cast<const byte_vector_t::value_type *>(&item),
        reinterpret_cast<const byte_vector_t::value_type *>(&item)+sizeof(item)
    );
}

#define sizeof_member(struct_name, member_name) \
    sizeof(reinterpret_cast<struct_name*>(NULL)->member_name)

static void load_e100(mboard_eeprom_t &mb_eeprom, i2c_iface &iface){
    const size_t num_bytes = offsetof(e100_eeprom_map, model);
    byte_vector_t map_bytes = iface.read_eeprom(E100_EEPROM_ADDR, 0, num_bytes);
    e100_eeprom_map map; std::memcpy(&map, &map_bytes[0], map_bytes.size());

    mb_eeprom["vendor"] = boost::lexical_cast<std::string>(uhd::ntohx(map.vendor));
    mb_eeprom["device"] = boost::lexical_cast<std::string>(uhd::ntohx(map.device));
    mb_eeprom["revision"] = boost::lexical_cast<std::string>(unsigned(map.revision));
    mb_eeprom["content"] = boost::lexical_cast<std::string>(unsigned(map.content));

    #define load_e100_string_xx(key) mb_eeprom[#key] = bytes_to_string(iface.read_eeprom( \
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, key), sizeof_member(e100_eeprom_map, key) \
    ));

    load_e100_string_xx(model);
    load_e100_string_xx(env_var);
    load_e100_string_xx(env_setting);
    load_e100_string_xx(serial);
    load_e100_string_xx(name);
}

static void store_e100(const mboard_eeprom_t &mb_eeprom, i2c_iface &iface){

    if (mb_eeprom.has_key("vendor")) iface.write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, vendor),
        to_bytes(uhd::htonx(boost::lexical_cast<boost::uint16_t>(mb_eeprom["vendor"])))
    );

    if (mb_eeprom.has_key("device")) iface.write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, device),
        to_bytes(uhd::htonx(boost::lexical_cast<boost::uint16_t>(mb_eeprom["device"])))
    );

    if (mb_eeprom.has_key("revision")) iface.write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, revision),
        byte_vector_t(1, boost::lexical_cast<unsigned>(mb_eeprom["revision"]))
    );

    if (mb_eeprom.has_key("content")) iface.write_eeprom(
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, content),
        byte_vector_t(1, boost::lexical_cast<unsigned>(mb_eeprom["content"]))
    );

    #define store_e100_string_xx(key) if (mb_eeprom.has_key(#key)) iface.write_eeprom( \
        E100_EEPROM_ADDR, offsetof(e100_eeprom_map, key), \
        string_to_bytes(mb_eeprom[#key], sizeof_member(e100_eeprom_map, key)) \
    );

    store_e100_string_xx(model);
    store_e100_string_xx(env_var);
    store_e100_string_xx(env_setting);
    store_e100_string_xx(serial);
    store_e100_string_xx(name);
}

/***********************************************************************
 * Implementation of mboard eeprom
 **********************************************************************/
mboard_eeprom_t::mboard_eeprom_t(void){
    /* NOP */
}

mboard_eeprom_t::mboard_eeprom_t(i2c_iface &iface, const std::string &which){
    if (which == "N100") load_n100(*this, iface);
    if (which == "X300") load_x300(*this, iface);
    if (which == "B000") load_b000(*this, iface);
    if (which == "B100") load_b100(*this, iface);
    if (which == "B200") load_b200(*this, iface);
    if (which == "E100") load_e100(*this, iface);
}

void mboard_eeprom_t::commit(i2c_iface &iface, const std::string &which) const{
    if (which == "N100") store_n100(*this, iface);
    if (which == "X300") store_x300(*this, iface);
    if (which == "B000") store_b000(*this, iface);
    if (which == "B100") store_b100(*this, iface);
    if (which == "B200") store_b200(*this, iface);
    if (which == "E100") store_e100(*this, iface);
}
