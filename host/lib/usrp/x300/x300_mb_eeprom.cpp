//
// Copyright 2017 Ettus Research (National Instruments Corp.)
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include <uhdlib/utils/eeprom_utils.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/types/serial.hpp>

namespace {
    const uint8_t X300_EEPROM_ADDR = 0x50;

    struct x300_eeprom_map
    {
        //identifying numbers
        unsigned char revision[2];
        unsigned char product[2];
        unsigned char revision_compat[2];
        uint8_t _pad0[2];

        //all the mac addrs
        uint8_t mac_addr0[6];
        uint8_t _pad1[2];
        uint8_t mac_addr1[6];
        uint8_t _pad2[2];

        //all the IP addrs
        uint32_t gateway;
        uint32_t subnet[4];
        uint32_t ip_addr[4];
        uint8_t _pad3[16];

        //names and serials
        unsigned char name[NAME_MAX_LEN];
        unsigned char serial[SERIAL_LEN];
    };
}

using namespace uhd;
using uhd::usrp::mboard_eeprom_t;

mboard_eeprom_t x300_impl::get_mb_eeprom(uhd::i2c_iface::sptr iface)
{
    byte_vector_t bytes =
        iface->read_eeprom(X300_EEPROM_ADDR, 0, sizeof(struct x300_eeprom_map));

    mboard_eeprom_t mb_eeprom;
    if (bytes.empty()) {
        return mb_eeprom;
    }

    //extract the revision number
    mb_eeprom["revision"] = uint16_bytes_to_string(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, revision),
            bytes.begin() + (offsetof(x300_eeprom_map, revision)+2))
    );

    //extract the revision compat number
    mb_eeprom["revision_compat"] = uint16_bytes_to_string(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, revision_compat),
            bytes.begin() + (offsetof(x300_eeprom_map, revision_compat)+2))
    );

    //extract the product code
    mb_eeprom["product"] = uint16_bytes_to_string(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, product),
            bytes.begin() + (offsetof(x300_eeprom_map, product)+2))
    );

    //extract the mac addresses
    mb_eeprom["mac-addr0"] = mac_addr_t::from_bytes(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, mac_addr0),
            bytes.begin() + (offsetof(x300_eeprom_map, mac_addr0)+6))
    ).to_string();
    mb_eeprom["mac-addr1"] = mac_addr_t::from_bytes(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, mac_addr1),
            bytes.begin() + (offsetof(x300_eeprom_map, mac_addr1)+6))
    ).to_string();

    //extract the ip addresses
    boost::asio::ip::address_v4::bytes_type ip_addr_bytes;
    byte_copy(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, gateway),
            bytes.begin() + (offsetof(x300_eeprom_map, gateway)+4)),
            ip_addr_bytes
    );
    mb_eeprom["gateway"] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();
    for (size_t i = 0; i < 4; i++)
    {
        const std::string n(1, i+'0');
        byte_copy(
                byte_vector_t(
                bytes.begin() + (offsetof(x300_eeprom_map, ip_addr)+(i*4)),
                bytes.begin() + (offsetof(x300_eeprom_map, ip_addr)+(i*4)+4)),
                ip_addr_bytes
        );
        mb_eeprom["ip-addr"+n] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();

        byte_copy(
                byte_vector_t(
                bytes.begin() + (offsetof(x300_eeprom_map, subnet)+(i*4)),
                bytes.begin() + (offsetof(x300_eeprom_map, subnet)+(i*4)+4)),
                ip_addr_bytes
        );
        mb_eeprom["subnet"+n] = boost::asio::ip::address_v4(ip_addr_bytes).to_string();
    }

    //extract the serial
    mb_eeprom["serial"] = bytes_to_string(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, serial),
            bytes.begin() + (offsetof(x300_eeprom_map, serial)+SERIAL_LEN))
    );

    //extract the name
    mb_eeprom["name"] = bytes_to_string(
            byte_vector_t(
            bytes.begin() + offsetof(x300_eeprom_map, name),
            bytes.begin() + (offsetof(x300_eeprom_map, name)+NAME_MAX_LEN))
    );

    return mb_eeprom;
}


void x300_impl::set_mb_eeprom(
        i2c_iface::sptr iface,
        const mboard_eeprom_t &mb_eeprom
) {
    //parse the revision number
    if (mb_eeprom.has_key("revision")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision),
        string_to_uint16_bytes(mb_eeprom["revision"])
    );

    //parse the revision compat number
    if (mb_eeprom.has_key("revision_compat")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, revision_compat),
        string_to_uint16_bytes(mb_eeprom["revision_compat"])
    );

    //parse the product code
    if (mb_eeprom.has_key("product")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, product),
        string_to_uint16_bytes(mb_eeprom["product"])
    );

    //store the mac addresses
    if (mb_eeprom.has_key("mac-addr0")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr0),
        mac_addr_t::from_string(mb_eeprom["mac-addr0"]).to_bytes()
    );
    if (mb_eeprom.has_key("mac-addr1")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, mac_addr1),
        mac_addr_t::from_string(mb_eeprom["mac-addr1"]).to_bytes()
    );

    //store the ip addresses
    byte_vector_t ip_addr_bytes(4);
    if (mb_eeprom.has_key("gateway")){
        byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["gateway"]).to_bytes(), ip_addr_bytes);
        iface->write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, gateway), ip_addr_bytes);
    }
    for (size_t i = 0; i < 4; i++)
    {
        const std::string n(1, i+'0');
        if (mb_eeprom.has_key("ip-addr"+n)){
            byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["ip-addr"+n]).to_bytes(), ip_addr_bytes);
            iface->write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, ip_addr)+(i*4), ip_addr_bytes);
        }

        if (mb_eeprom.has_key("subnet"+n)){
            byte_copy(boost::asio::ip::address_v4::from_string(mb_eeprom["subnet"+n]).to_bytes(), ip_addr_bytes);
            iface->write_eeprom(X300_EEPROM_ADDR, offsetof(x300_eeprom_map, subnet)+(i*4), ip_addr_bytes);
        }
    }

    //store the serial
    if (mb_eeprom.has_key("serial")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, serial),
        string_to_bytes(mb_eeprom["serial"], SERIAL_LEN)
    );

    //store the name
    if (mb_eeprom.has_key("name")) iface->write_eeprom(
        X300_EEPROM_ADDR, offsetof(x300_eeprom_map, name),
        string_to_bytes(mb_eeprom["name"], NAME_MAX_LEN)
    );
}

