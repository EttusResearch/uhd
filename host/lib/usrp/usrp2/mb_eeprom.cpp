//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "usrp2_impl.hpp"
#include <uhdlib/utils/eeprom_utils.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/byte_vector.hpp>
#include <uhd/types/mac_addr.hpp>
#include <boost/asio/ip/address_v4.hpp>

namespace {
    const uint8_t N200_EEPROM_ADDR = 0x50;

    struct n200_eeprom_map{
        uint16_t hardware;
        uint8_t mac_addr[6];
        uint32_t subnet;
        uint32_t ip_addr;
        uint16_t _pad0;
        uint16_t revision;
        uint16_t product;
        unsigned char _pad1;
        unsigned char gpsdo;
        unsigned char serial[SERIAL_LEN];
        unsigned char name[NAME_MAX_LEN];
        uint32_t gateway;
    };

    enum n200_gpsdo_type{
        N200_GPSDO_NONE = 0,
        N200_GPSDO_INTERNAL = 1,
        N200_GPSDO_ONBOARD = 2
    };
}

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

mboard_eeprom_t usrp2_impl::get_mb_eeprom(usrp2_iface &iface)
{
    uhd::usrp::mboard_eeprom_t mb_eeprom;

    //extract the hardware number
    mb_eeprom["hardware"] = uint16_bytes_to_string(
        iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, hardware), 2)
    );

    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
        iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, revision), 2)
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
        iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, product), 2)
    );

    //extract the addresses
    mb_eeprom["mac-addr"] = mac_addr_t::from_bytes(iface.read_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, mac_addr), 6
    )).to_string();

    boost::asio::ip::address_v4::bytes_type ip_addr_bytes;
    byte_copy(iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, ip_addr), 4), ip_addr_bytes);
    mb_eeprom["ip-addr"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    byte_copy(iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, subnet), 4), ip_addr_bytes);
    mb_eeprom["subnet"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    byte_copy(iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, gateway), 4), ip_addr_bytes);
    mb_eeprom["gateway"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

    //gpsdo capabilities
    uint8_t gpsdo_byte = iface.read_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, gpsdo), 1).at(0);
    switch(n200_gpsdo_type(gpsdo_byte)){
    case N200_GPSDO_INTERNAL: mb_eeprom["gpsdo"] = "internal"; break;
    case N200_GPSDO_ONBOARD: mb_eeprom["gpsdo"] = "onboard"; break;
    default: mb_eeprom["gpsdo"] = "none";
    }

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(iface.read_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, serial), SERIAL_LEN
    ));

    //extract the name
    mb_eeprom["name"] = bytes_to_string(iface.read_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, name), NAME_MAX_LEN
    ));

    //Empty serial correction: use the mac address to determine serial.
    //Older usrp2 models don't have a serial burned into EEPROM.
    //The lower mac address bits will function as the serial number.
    if (mb_eeprom["serial"].empty()){
        byte_vector_t mac_addr_bytes = mac_addr_t::from_string(mb_eeprom["mac-addr"]).to_bytes();
        unsigned serial = mac_addr_bytes.at(5) | (unsigned(mac_addr_bytes.at(4) & 0x0f) << 8);
        mb_eeprom["serial"] = std::to_string(serial);
    }

    return mb_eeprom;
}


void usrp2_impl::set_mb_eeprom(
    const std::string &mb,
    const mboard_eeprom_t &mb_eeprom
) {
    auto &iface = _mbc[mb].iface;

    //parse the revision number
    if (mb_eeprom.has_key("hardware")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, hardware),
        string_to_uint16_bytes(mb_eeprom["hardware"])
    );

    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the addresses
    if (mb_eeprom.has_key("mac-addr")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, mac_addr),
        mac_addr_t::from_string(mb_eeprom["mac-addr"]).to_bytes()
    );

    if (mb_eeprom.has_key("ip-addr")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["ip-addr"]).to_bytes(), ip_addr_bytes);
        iface->write_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, ip_addr), ip_addr_bytes);
    }

    if (mb_eeprom.has_key("subnet")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["subnet"]).to_bytes(), ip_addr_bytes);
        iface->write_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, subnet), ip_addr_bytes);
    }

    if (mb_eeprom.has_key("gateway")){
        byte_vector_t ip_addr_bytes(4);
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["gateway"]).to_bytes(), ip_addr_bytes);
        iface->write_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, gateway), ip_addr_bytes);
    }

    //gpsdo capabilities
    if (mb_eeprom.has_key("gpsdo")){
        uint8_t gpsdo_byte = N200_GPSDO_NONE;
        if (mb_eeprom["gpsdo"] == "internal") gpsdo_byte = N200_GPSDO_INTERNAL;
        if (mb_eeprom["gpsdo"] == "onboard") gpsdo_byte = N200_GPSDO_ONBOARD;
        iface->write_eeprom(N200_EEPROM_ADDR, offsetof(n200_eeprom_map, gpsdo), byte_vector_t(1, gpsdo_byte));
    }

    //store the serial
    if (mb_eeprom.has_key("serial")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface->write_eeprom(
        N200_EEPROM_ADDR, offsetof(n200_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

